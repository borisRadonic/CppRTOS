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
class KernelTest : public ::testing::Test
{
   

public:
    std::aligned_storage_t<sizeof(CppRtos::Kernel), 4> _prealoc_kernel_mem;
    Kernel* kernel;
    MockTask mockTask1;
    MockTask mockTask2;


    void SetUp() override
    {       
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        
        kernel = kernelFactory.create(&_prealoc_kernel_mem);
   
        mockTask1.setPriority( TaskPriority::PRIORITY_HIGH );
        mockTask2.setPriority( TaskPriority::PRIORITY_MEDIUM_HIGH );

        kernel->addTask(mockTask1);
        kernel->addTask(mockTask2);
       
    }

    void TearDown() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernelFactory.destroy(&_prealoc_kernel_mem);
    }


};

TEST_F(KernelTest, SelectHighestPriorityTask_SelectsCorrectTask)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.setTaskReady(ptrTaskData1);
    port.setTaskReady(ptrTaskData2);

    port.selectHighestPriorityTask();

    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);
}

TEST_F(KernelTest, SelectHighestPriorityTask_LowerPriorityDoesNotPreempt)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.setTaskReady(ptrTaskData1);
    port.setTaskReady(ptrTaskData2);
    port.selectHighestPriorityTask(); // taskData1 is running
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    // Next scheduling point (e.g. tick): the higher-priority task keeps running
    port.selectHighestPriorityTask();

    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);
}

TEST_F(KernelTest, SelectHighestPriorityTask_HigherPriorityWakeUpPreempts)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    // Task 1 (higher priority) blocks, Task 2 runs
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    port.resetTaskReady(ptrTaskData1, TaskStateType::eBlocked);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    // Task 1 becomes ready again and preempts Task 2
    port.setTaskReady(ptrTaskData1);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);
}

TEST_F(KernelTest, SelectHighestPriorityTask_IdleRunsWhenNothingElseIsReady)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    port.resetTaskReady(ptrTaskData1, TaskStateType::eSleeping);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);
    port.resetTaskReady(ptrTaskData2, TaskStateType::eBlocked);
    port.selectHighestPriorityTask();

    TaskData* ptrIdle = port.getCurrentTask();
    ASSERT_NE(ptrIdle, nullptr);
    EXPECT_NE(ptrIdle, ptrTaskData1);
    EXPECT_NE(ptrIdle, ptrTaskData2);
    EXPECT_EQ(ptrIdle->getPriority(), TaskPriority::PRIORITY_IDLE);
    EXPECT_EQ(ptrIdle->getState(), TaskStateType::eRunning);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eSleeping);
}

TEST_F(KernelTest, SelectHighestPriorityTask_SleepingTaskWakesUpOnTick)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    EXPECT_THROW(mockTask1.sleep(2), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    port.incrementTickCount();
    port.tick();
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2); // still sleeping after 1 tick

    port.incrementTickCount();
    port.tick();
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // woken up and preempts task 2
}

// Fixture with three tasks of equal priority for round-robin tests
class RoundRobinTest : public ::testing::Test
{
public:
    std::aligned_storage_t<sizeof(CppRtos::Kernel), 8> _prealoc_kernel_mem;
    Kernel* kernel;
    MockTask mockTaskA;
    MockTask mockTaskB;
    MockTask mockTaskC;
    MockTask mockTaskLow;

    void SetUp() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernel = kernelFactory.create(&_prealoc_kernel_mem);

        mockTaskA.setPriority(TaskPriority::PRIORITY_AVERAGE);
        mockTaskB.setPriority(TaskPriority::PRIORITY_AVERAGE);
        mockTaskC.setPriority(TaskPriority::PRIORITY_AVERAGE);
        mockTaskLow.setPriority(TaskPriority::PRIORITY_LOW);

        kernel->addTask(mockTaskA);
        kernel->addTask(mockTaskB);
        kernel->addTask(mockTaskC);
        kernel->addTask(mockTaskLow);
    }

    void TearDown() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernelFactory.destroy(&_prealoc_kernel_mem);
    }
};

TEST_F(RoundRobinTest, EqualPriorityTasksRotate_LowerPriorityNeverRuns)
{
    TaskData* a = mockTaskA.getTaskData();
    TaskData* b = mockTaskB.getTaskData();
    TaskData* c = mockTaskC.getTaskData();
    TaskData* low = mockTaskLow.getTaskData();

    Port::Port& port = kernel->getPort();

    TaskData* expected[] = { a, b, c, a, b, c, a };
    for (TaskData* e : expected)
    {
        port.selectHighestPriorityTask();
        EXPECT_EQ(port.getCurrentTask(), e);
        EXPECT_EQ(e->getState(), TaskStateType::eRunning);
        EXPECT_EQ(low->getState(), TaskStateType::eReady);
    }
}

TEST_F(RoundRobinTest, BlockedTaskIsSkippedInRotation)
{
    TaskData* a = mockTaskA.getTaskData();
    TaskData* b = mockTaskB.getTaskData();
    TaskData* c = mockTaskC.getTaskData();

    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), a);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), b);

    // b blocks
    port.resetTaskReady(b, TaskStateType::eBlocked);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), c);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), a);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), c);
    EXPECT_EQ(b->getState(), TaskStateType::eBlocked);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}