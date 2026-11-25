#include <gmock/gmock.h>
#include "KernelFactory.hpp"
#include "Kernel.hpp"
#include "Task.hpp"
#include "Port.hpp"
#include "EventGroup.h"

using namespace CppRtos;

namespace
{
    class EventMockTask : public Task
    {
    public:
        MOCK_METHOD(void, run, (), (override));
    };
}

// Test Fixture
class EventGroupTest : public ::testing::Test
{
public:
    std::aligned_storage_t<sizeof(CppRtos::Kernel), alignof(CppRtos::Kernel)> _prealoc_kernel_mem;
    Kernel* kernel;
    EventMockTask mockTask1;
    EventMockTask mockTask2;
    EventGroup* ptrEvents = nullptr;

    void SetUp() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernel = kernelFactory.create(&_prealoc_kernel_mem);
        ptrEvents = new EventGroup();

        mockTask1.setPriority(TaskPriority::PRIORITY_HIGH);
        mockTask2.setPriority(TaskPriority::PRIORITY_MEDIUM_HIGH);

        kernel->addTask(mockTask1);
        kernel->addTask(mockTask2);
    }

    void TearDown() override
    {
        delete ptrEvents;
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernelFactory.destroy(&_prealoc_kernel_mem);
    }
};

TEST_F(EventGroupTest, WaiterIsWokenOnlyByMatchingFlags)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    EXPECT_FALSE(ptrEvents->waitForEvents(0x1u, 0u));
    EXPECT_THROW(ptrEvents->waitForEvents(0x1u, WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eBlocked);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    // Non-matching flag: task 1 stays blocked, no context switch
    EXPECT_NO_THROW(ptrEvents->setEvents(0x2u));
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eBlocked);

    // Matching flag: task 1 is woken and preempts task 2
    EXPECT_THROW(ptrEvents->setEvents(0x1u), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eSignaled);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    EXPECT_EQ(ptrEvents->getEvents(), 0x3u);
    EXPECT_TRUE(ptrEvents->waitForEvents(0x1u, 0u));
    ptrEvents->clearEvents(0x1u);
    EXPECT_EQ(ptrEvents->getEvents(), 0x2u);
}

TEST_F(EventGroupTest, WaitTimesOut)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask();
    EXPECT_THROW(ptrEvents->waitForEvents(0x4u, 1u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);

    port.incrementTickCount();
    port.tick();
    EXPECT_EQ(ptrTaskData1->getWakeReason(), WakeReason::eTimeout);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    // Setting the flag later does not affect the timed-out task
    EXPECT_NO_THROW(ptrEvents->setEvents(0x4u));
}
