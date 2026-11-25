
#include <gmock/gmock.h>
#include "KernelFactory.hpp"
#include "Kernel.hpp"
#include "Task.hpp"
#include "Port.hpp"
#include "MessageQueue.hpp"


using namespace CppRtos;
using ::testing::Return;
using ::testing::_;


struct TestMsg
{
    int id;
    char text[99];
};

class MockTask : public Task
{
public:
    MOCK_METHOD(void, run, (), (override));
};

// Test Fixture
class TestMsgQueue : public ::testing::Test
{


public:
    std::aligned_storage_t<sizeof(CppRtos::Kernel), alignof(CppRtos::Kernel)> _prealoc_kernel_mem;
    Kernel* kernel;
    MockTask mockTask1;
    MockTask mockTask2;
    MessageQueue< TestMsg, 32> queue;

    void SetUp() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();

        kernel = kernelFactory.create(&_prealoc_kernel_mem);

        mockTask1.setPriority(TaskPriority::PRIORITY_HIGH);
        mockTask2.setPriority(TaskPriority::PRIORITY_MEDIUM_HIGH);

        kernel->addTask(mockTask1);
        kernel->addTask(mockTask2);

    }

    void TearDown() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernelFactory.destroy(&_prealoc_kernel_mem);
    }
};


TEST_F(TestMsgQueue, TestMsgQueue1)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    TestMsg msgOut = { 7, "hello" };
    TestMsg msgIn = {};

    EXPECT_EQ(queue.send(msgOut, 0u), MsgQueueResult::Success);
    EXPECT_EQ(queue.getNumMsg(), 1u);
    EXPECT_EQ(queue.receive(msgIn, 0u), MsgQueueResult::Success);
    EXPECT_EQ(msgIn.id, 7);
    EXPECT_STREQ(msgIn.text, "hello");
    EXPECT_EQ(queue.receive(msgIn, 0u), MsgQueueResult::ErrorQueueEmpty);
}

TEST_F(TestMsgQueue, BlockedReceiverIsWokenBySend)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    // Task 1 (high) waits for a message and blocks
    TestMsg msgIn = {};
    EXPECT_THROW(queue.receive(msgIn, WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eBlocked);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    // Task 2 sends: task 1 is woken and preempts task 2
    TestMsg msgOut = { 42, "msg" };
    EXPECT_THROW(queue.send(msgOut, WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(queue.getNumMsg(), 1u);
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eSignaled);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    // Task 1 re-checks the queue after resuming and gets the message
    EXPECT_EQ(queue.receive(msgIn, 0u), MsgQueueResult::Success);
    EXPECT_EQ(msgIn.id, 42);
}

TEST_F(TestMsgQueue, FullQueueSenderTimesOut)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    TestMsg msg = {};
    for (int i = 0; i < 32; ++i)
    {
        msg.id = i;
        EXPECT_EQ(queue.send(msg, 0u), MsgQueueResult::Success);
    }
    EXPECT_EQ(queue.send(msg, 0u), MsgQueueResult::ErrorQueueFull);

    // From an ISR the send never blocks
    port.unitTestSetisInsideInterrupt(true);
    EXPECT_EQ(queue.send(msg, WAIT_FOREVER), MsgQueueResult::ErrorQueueFull);
    EXPECT_EQ(queue.receive(msg, 0u), MsgQueueResult::ErrorCalledFromISR);
    port.unitTestSetisInsideInterrupt(false);

    // Task 1 blocks with timeout 2
    EXPECT_THROW(queue.send(msg, 2u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    port.incrementTickCount();
    port.tick();
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eBlocked);
    port.incrementTickCount();
    port.tick();
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eTimeout);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    EXPECT_EQ(queue.getNumMsg(), 32u);
}

TEST_F(TestMsgQueue, BlockedSenderIsWokenByReceive)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    TestMsg msg = {};
    for (int i = 0; i < 32; ++i)
    {
        EXPECT_EQ(queue.send(msg, 0u), MsgQueueResult::Success);
    }

    // Task 1 (high) blocks on the full queue
    EXPECT_THROW(queue.send(msg, WAIT_FOREVER), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    // Task 2 receives: a slot is free, task 1 is woken and preempts
    EXPECT_THROW(queue.receive(msg, 0u), CppRtos::UnitTestException);
    EXPECT_EQ(queue.getNumMsg(), 31u);
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eSignaled);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
}
