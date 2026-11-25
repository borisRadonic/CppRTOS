#include <gmock/gmock.h>
#include "KernelFactory.hpp"
#include "Kernel.hpp"
#include "Task.hpp"
#include "Port.hpp"
#include "Mutex.hpp"


using namespace CppRtos;
using ::testing::Return;
using ::testing::_;


class MockTask : public Task
{
public:
    MOCK_METHOD(void, run, (), (override));
};

// Test Fixture
class MutexTest : public ::testing::Test
{


public:
    std::aligned_storage_t<sizeof(CppRtos::Kernel), alignof(CppRtos::Kernel)> _prealoc_kernel_mem;
    Kernel* kernel;
    MockTask mockTask1;
    MockTask mockTask2;
    Mutex mutex1;

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


// Test Fixture
class MutexTest2 : public ::testing::Test
{


public:
    std::aligned_storage_t<sizeof(CppRtos::Kernel), alignof(CppRtos::Kernel)> _prealoc_kernel_mem;
    Kernel* kernel;
    MockTask mockTaskLow;
    MockTask mockTaskMedium;
    MockTask mockTaskHigh;
    Mutex mutex1;
    Mutex mutex2;

    void SetUp() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();

        kernel = kernelFactory.create(&_prealoc_kernel_mem);

        mockTaskLow.setPriority(TaskPriority::PRIORITY_LOW);
        mockTaskMedium.setPriority(TaskPriority::PRIORITY_MEDIUM_HIGH);
        mockTaskHigh.setPriority(TaskPriority::PRIORITY_HIGH);

        kernel->addTask(mockTaskLow);
        kernel->addTask(mockTaskMedium);
        kernel->addTask(mockTaskHigh);

    }

    void TearDown() override
    {
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernelFactory.destroy(&_prealoc_kernel_mem);
    }

};




/*
Mutex ownership and handoff (strict priority scheduling):
- Task 1 (high) owns the mutex, task 2 (lower) blocks on it.
- Task 1 releases: ownership is handed over to task 2, but task 1 keeps running (higher priority).
*/
TEST_F(MutexTest, TestMutex1)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.selectHighestPriorityTask(); // task1 is running
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    /*Task 1 acquires mutex1*/
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::Success);

    /*Try to acquire again*/
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::AlreadyOwned);

    // Task 1 sleeps, task 2 runs and blocks on the mutex
    EXPECT_THROW(mockTask1.sleep(1u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);
    EXPECT_EQ(mutex1.acquire(0u), MutexResult::Timeout);
    EXPECT_THROW(mutex1.acquire(WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eBlocked);

    // No priority inheritance: the waiter has a lower priority than the owner
    EXPECT_EQ(ptrTaskData1->getPriority(), TaskPriority::PRIORITY_HIGH);

    // Task 1 wakes up and runs
    port.incrementTickCount();
    port.tick();
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    // Task 1 releases: handoff to task 2, no context switch since task 1 has the higher priority
    EXPECT_EQ(mutex1.release(), MutexResult::Success);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);
    EXPECT_EQ(ptrTaskData2->getWakeReason(), WakeReason::eSignaled);
    EXPECT_EQ(mutex1.release(), MutexResult::NotOwner); // task 2 owns it now

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);

    // Task 1 sleeps, task 2 (the new owner) runs and releases
    EXPECT_THROW(mockTask1.sleep(1u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);
    EXPECT_EQ(mutex1.release(), MutexResult::Success);
}

/*
Priority Inversion Scenario with priority inheritance

    Task Setup:
        Low, Medium, High priority tasks.
    Low acquires the mutex while High and Medium sleep.
    High wakes up, preempts Low and blocks on the mutex.
    Low inherits High's priority, so Medium (ready) cannot preempt Low.
    Low releases the mutex: its priority is restored, ownership is handed over to High, which runs next.
    Medium runs only when High blocks again.
*/
TEST_F(MutexTest2, PriorityInversion)
{
    TaskData* ptrTaskDataLow    = mockTaskLow.getTaskData();
    TaskData* ptrTaskDataMedium = mockTaskMedium.getTaskData();
    TaskData* ptrTaskDataHigh   = mockTaskHigh.getTaskData();

    Port::Port& port = kernel->getPort();

    // High and Medium sleep, Low runs and acquires the mutex
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataHigh);
    EXPECT_THROW(mockTaskHigh.sleep(2u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataMedium);
    EXPECT_THROW(mockTaskMedium.sleep(2u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataLow);
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::Success);

    // High and Medium wake up, High preempts Low
    port.incrementTickCount();
    port.tick();
    port.incrementTickCount();
    port.tick();
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataHigh);
    EXPECT_EQ(ptrTaskDataLow->getState(), TaskStateType::eReady);

    // High blocks on the mutex, Low inherits High's priority
    EXPECT_THROW(mutex1.acquire(WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskDataHigh->getState(), TaskStateType::eBlocked);
    EXPECT_EQ(ptrTaskDataLow->getPriority(), TaskPriority::PRIORITY_HIGH);
    EXPECT_EQ(ptrTaskDataLow->getBasePriority(), TaskPriority::PRIORITY_LOW);

    // Low runs, Medium cannot preempt it
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataLow);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataLow);
    EXPECT_EQ(ptrTaskDataMedium->getState(), TaskStateType::eReady);

    // Low releases: priority restored, handoff to High, context switch requested
    EXPECT_THROW(mutex1.release(), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskDataLow->getPriority(), TaskPriority::PRIORITY_LOW);
    EXPECT_EQ(ptrTaskDataHigh->getWakeReason(), WakeReason::eSignaled);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataHigh);
    EXPECT_EQ(ptrTaskDataHigh->getState(), TaskStateType::eRunning);

    // High owns the mutex now
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::AlreadyOwned);
    EXPECT_EQ(mutex1.release(), MutexResult::Success);

    // High sleeps: Medium runs before Low
    EXPECT_THROW(mockTaskHigh.sleep(5u), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataMedium);
    EXPECT_EQ(ptrTaskDataLow->getState(), TaskStateType::eReady);
}


/*
Deadlock Scenario

    Objective: The test illustrates the deadlock scenario when multiple tasks are waiting on multiple resources.

Test Steps:

    Task Setup:
        Task 1: High priority.
        Task 2: Medium priority.
    Task 1 Acquires Mutex 1: Task 1 acquires mutex1.
    Task 2 Acquires Mutex 2: Task 2 acquires mutex2.
    Task 1 Attempts to Acquire Mutex 2: Task 1 tries to acquire mutex2 but is blocked since Task 2 holds it.
    Task 2 Attempts to Acquire Mutex 1: Task 2 tries to acquire mutex1 but is blocked since Task 1 holds it.

Expected Results:
   
    If the system doesn’t handle deadlocks, the test should illustrate the deadlock scenario clearly.
*/
TEST_F(MutexTest2, DeadlockScenario)
{
    TaskData* ptrTaskData1 = mockTaskLow.getTaskData();
    TaskData* ptrTaskData2 = mockTaskMedium.getTaskData();
    TaskData* ptrTaskData3 = mockTaskHigh.getTaskData();

    Port::Port& port = kernel->getPort();

    port.setTaskReady(ptrTaskData1);
    port.setTaskReady(ptrTaskData2);
    port.setTaskReady(ptrTaskData3);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData3); //high priority task3 is running
    
    EXPECT_THROW(mockTaskHigh.sleep(4); , CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2); //medium priority task2 is running

    EXPECT_THROW(mockTaskMedium.sleep(2);, CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); //low  priority task1 is running

    // Task 1 acquires mutex1   
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::Success);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); //low  priority task1 is still running

    port.incrementTickCount();
    port.tick();
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); //low  priority task1 is still running

    port.incrementTickCount();
    port.tick();
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2); //medium priority task2 is running
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eRunning);

    // Task 2 acquires mutex2
    EXPECT_EQ(mutex2.acquire(WAIT_FOREVER), MutexResult::Success);
          
    // Task 1 attempts to acquire mutex2 and gets blocked
    port.setCurrentTask(ptrTaskData1);
    ptrTaskData1->setState(TaskStateType::eRunning);
    EXPECT_THROW(mutex2.acquire(WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eBlocked);

    // Task 2 attempts to acquire mutex1 and should also get blocked or trigger deadlock handling
    port.setCurrentTask(ptrTaskData2);
    ptrTaskData2->setState(TaskStateType::eRunning);
    EXPECT_THROW(mutex1.acquire(WAIT_FOREVER), CppRtos::UnitTestException);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eBlocked);

    //Deadlock
}

/*
Task Switching While Holding a Mutex

    Objective: Test task switching and priority handling when a task holding a mutex is preempted by a higher-priority task.

Test Steps:

    Task Setup:
        Task 1: High priority.
        Task 2: Medium priority.
        Task 3: Low priority.
    Task 3 Acquires Mutex: Task 3 acquires the mutex.
    Task 1 Preempts Task 3: Task 1 preempts Task 3 and attempts to acquire the mutex, getting blocked.
    Task 2 Runs: Task 2 starts running and completes its execution.
    Task 3 Resumes and Releases Mutex: Task 3 resumes, releases the mutex, and Task 1 is unblocked and resumes execution.

Expected Results:

    The test should verify that Task 1, with the highest priority, is correctly unblocked and resumes execution as soon as the mutex is available.
    Task 2 should complete its execution normally while Task 1 is blocked.
*/
TEST_F(MutexTest2, TaskSwitchingWhileHoldingMutex)
{
    TaskData* ptrTaskDataLow = mockTaskLow.getTaskData();
    TaskData* ptrTaskDataMedium = mockTaskMedium.getTaskData();
    TaskData* ptrTaskDataHigh = mockTaskHigh.getTaskData();

    Port::Port& port = kernel->getPort();

    //// Task 3 (Low Priority) is ready and acquires the mutex
    //kernel->setTaskReady(ptrTaskDataLow);
    //kernel->resetTaskReady(ptrTaskDataLow, TaskStateType::eRunning);
    //kernel->setCurrentTask(ptrTaskDataLow);
    //EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::Success);

    //// Task 1 (High Priority) should now run and attempt to acquire the mutex, but get blocked
    //kernel->setTaskReady(ptrTaskDataHigh);
    //kernel->selectHighestPriorityTask();
    //EXPECT_EQ(kernel->getCurrentTask(), ptrTaskDataHigh);
    //EXPECT_THROW(mutex1.acquire(WAIT_FOREVER), CppRtos::UnitTestException);
    //EXPECT_EQ(ptrTaskDataHigh->getState(), TaskStateType::eBlocked);

    //// Task 2 (Medium Priority) should run next
    //kernel->setTaskReady(ptrTaskDataMedium);
    //kernel->selectHighestPriorityTask();
    //EXPECT_EQ(kernel->getCurrentTask(), ptrTaskDataMedium);
    //EXPECT_EQ(ptrTaskDataMedium->getState(), TaskStateType::eRunning);

    //// Task 3 (Low Priority) releases the mutex
    //kernel->setCurrentTask(ptrTaskDataLow);
    //EXPECT_EQ(mutex1.release(), MutexResult::Success);

    //// Task 1 (High Priority) should now be unblocked and run
    //kernel->selectHighestPriorityTask();
    //EXPECT_EQ(kernel->getCurrentTask(), ptrTaskDataHigh);
    //EXPECT_EQ(ptrTaskDataHigh->getState(), TaskStateType::eRunning);
}