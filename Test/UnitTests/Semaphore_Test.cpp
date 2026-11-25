#include <gmock/gmock.h>
#include "KernelFactory.hpp"
#include "Kernel.hpp"
#include "Task.hpp"
#include "Port.hpp"
#include "Mutex.hpp"
#include "Semaphore.hpp"
#include "MessageQueue.hpp"


using namespace CppRtos;
using ::testing::Return;
using ::testing::_;


struct TestMsg
{
    int id;
    char text[99];
    /* data */
};

class MockTask : public Task
{
public:
    MOCK_METHOD(void, run, (), (override));
};

// Test Fixture
class SemaphoreTest : public ::testing::Test
{
public:
    std::aligned_storage_t<sizeof(CppRtos::Kernel), alignof(CppRtos::Kernel)> _prealoc_kernel_mem;
    Kernel* kernel;
    MockTask mockTask1;
    MockTask mockTask2;
    Semaphore* ptrSem1;

    void SetUp() override
    {       
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernel = kernelFactory.create(&_prealoc_kernel_mem);

        ptrSem1 = new Semaphore(1, 0);

        mockTask1.setPriority(TaskPriority::PRIORITY_HIGH);
        mockTask2.setPriority(TaskPriority::PRIORITY_MEDIUM_HIGH);

        kernel->addTask(mockTask1);
        kernel->addTask(mockTask2);

    }

    void TearDown() override
    {
        delete ptrSem1;
        ptrSem1 = nullptr;
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernelFactory.destroy(&_prealoc_kernel_mem);
    }


};


/*
Semaphore with strict priority scheduling and direct handoff:
- signal() without waiters stores the token (count is incremented up to maxCount).
- yield() only gives the CPU to tasks of the same priority, so a lower-priority task does not run.
*/
TEST_F(SemaphoreTest, TestSemaphore1)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // task1 is running

    // Task 1 signals the semaphore: nobody waits, no context switch required
    EXPECT_NO_THROW(ptrSem1->signal());
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    // Task 1 yields: no other task of the same priority is ready, task 1 keeps running
    EXPECT_THROW(mockTask1.yield(), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);

    // Task 1 takes the stored token without blocking
    EXPECT_EQ(ptrSem1->wait(WAIT_FOREVER), SemResult::Success);

    // No token left: zero timeout returns immediately
    EXPECT_EQ(ptrSem1->wait(0u), SemResult::Timeout);
}

/*
Blocking wait and handoff:
- Task 1 (high) waits on an empty semaphore and blocks, task 2 (lower) runs.
- Task 2 signals: the token is handed over to task 1, which preempts task 2.
*/
TEST_F(SemaphoreTest, TestSemaphore2)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // task1 is running

    // Task 1 waits and blocks
    EXPECT_THROW(ptrSem1->wait(WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eBlocked);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2); // task2 is running

    // Task 2 signals: task 1 is woken and has a higher priority -> context switch requested
    EXPECT_THROW(ptrSem1->signal(), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eReady);
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eSignaled);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // task 1 preempts task 2
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);

    // The token was handed over, not stored
    EXPECT_EQ(ptrSem1->wait(0u), SemResult::Timeout);
}

TEST_F(SemaphoreTest, WaitTimesOut)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    EXPECT_THROW(ptrSem1->wait(3u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    for (int i = 0; i < 2; ++i)
    {
        port.incrementTickCount();
        port.tick();
        port.selectHighestPriorityTask();
        EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eBlocked);
        EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);
    }

    port.incrementTickCount();
    port.tick();
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eTimeout);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    // A later signal does not wake the timed-out task again, the token is stored
    EXPECT_NO_THROW(ptrSem1->signal());
    EXPECT_EQ(ptrSem1->wait(0u), SemResult::Success);
}

TEST_F(SemaphoreTest, SignalFromIsrWakesHighestPriorityWaiterFirst)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    // Task 1 sleeps, task 2 (lower priority) starts waiting first
    port.selectHighestPriorityTask();
    EXPECT_THROW(mockTask1.sleep(1u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);
    EXPECT_THROW(ptrSem1->wait(WAIT_FOREVER), CppRtos::UnitTestException);

    // Task 1 wakes up and also waits
    port.incrementTickCount();
    port.tick();
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    EXPECT_THROW(ptrSem1->wait(WAIT_FOREVER), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask()->getPriority(), TaskPriority::PRIORITY_IDLE);

    // Signal from an ISR: the higher-priority waiter is woken, although task 2 waited longer
    port.unitTestSetisInsideInterrupt(true);
    EXPECT_THROW(ptrSem1->signal(), CppRtos::UnitTestException); // context switch requested
    port.unitTestSetisInsideInterrupt(false);

    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eReady);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eBlocked);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
}

TEST_F(SemaphoreTest, FlushWakesAllWaiters)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_THROW(ptrSem1->wait(WAIT_FOREVER), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_THROW(ptrSem1->wait(WAIT_FOREVER), CppRtos::UnitTestException);
    port.selectHighestPriorityTask(); // idle runs

    EXPECT_THROW(ptrSem1->flush(), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eSignaled);
    EXPECT_EQ(ptrTaskData2->getWakeReason(), WakeReason::eSignaled);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eReady);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);
}
