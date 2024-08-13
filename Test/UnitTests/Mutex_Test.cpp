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
    std::aligned_storage_t<sizeof(CppRtos::Kernel), 4> _prealoc_kernel_mem;
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
    std::aligned_storage_t<sizeof(CppRtos::Kernel), 4> _prealoc_kernel_mem;
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
Test Scenario: Mutex Acquisition and Ownership Test

### Setup:
- **Task 1**: Highest priority.
- **Task 2**: Lower priority.
- **Mutex**: A mutual exclusion object (mutex1) used for synchronizing access to a shared resource.

### Sequence of Events:

1. **Tasks Ready**:
   - The kernel marks both Task 1 and Task 2 as ready to run using `setTaskReady()`.
   - The kernel then selects the highest-priority task to run, which is Task 1.

2. **Task 1 Acquires Mutex**:
   - Task 1, now running, attempts to acquire `mutex1` using `mutex1.acquire(WAIT_FOREVER)`.
   - Since the mutex is available, Task 1 successfully acquires it, and the mutex becomes owned by Task 1.
   - Task 1 then attempts to acquire `mutex1` again while already owning it, which should return `MutexResult::AlreadyOwned`, indicating that the task already holds the mutex.

3. **Task 2 Attempts to Acquire Mutex**:
   - The kernel changes the current task to Task 2, simulating Task 2 attempting to run.
   - Task 2 tries to acquire `mutex1`, but since it is already owned by Task 1, this should fail. The test expects an exception (`CppRtos::UnitTestException`) to be thrown because Task 2 cannot acquire the mutex that is currently owned by Task 1.

4. **Task State Check**:
   - The kernel switches back to Task 1.
   - The test checks that Task 2 is in the blocked state (`TaskStateType::eBlocked`), waiting for the mutex, and Task 1 is still in the running state (`TaskStateType::eRunning`).

5. **Task 1 Continues Running**:
   - The kernel confirms that Task 1 continues to run, as it is still the highest-priority task.
   - Task 2 remains blocked, waiting for the mutex.

6. **Task 1 Releases the Mutex**:
   - Task 1 attempts to release the mutex it owns.
   - The test expects an exception (`CppRtos::UnitTestException`) because the code appears to simulate an unexpected condition during the mutex release (likely part of the test's validation logic).

7. **Task 2 Becomes Ready and Runs**:
   - After the exception, the kernel schedules the next highest-priority task.
   - Since Task 1 released the mutex, Task 2 is unblocked and is now ready to run.
   - The kernel switches to Task 2, which successfully transitions to the running state (`TaskStateType::eRunning`).
   - Task 1 is moved to the ready state (`TaskStateType::eReady`), awaiting its next opportunity to run.

### Key Points:
- Task 1 successfully acquires the mutex and holds it, preventing Task 2 from acquiring it.
- Task 2 is blocked when it attempts to acquire the mutex owned by Task 1.
- An exception is expected during specific mutex operations as part of the test scenario using implementation of UnitTest Port.
- Task 2 is scheduled to run after Task 1 releases the mutex, demonstrating the correct behavior of task prioritization and mutex ownership.
*/
TEST_F(MutexTest, TestMutex1)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    
    Port::Port& port = kernel->getPort();

    port.setTaskReady(ptrTaskData1);
    port.setTaskReady(ptrTaskData2);
    port.selectHighestPriorityTask(); // task1 is running
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    /*Task 1 acquires mutex1*/
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::Success);

    /*Try to acquire again*/
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::AlreadyOwned);

    //change current task to Task2
    port.setCurrentTask(ptrTaskData2);
    port.resetTaskReady(ptrTaskData2, TaskStateType::eRunning);

    EXPECT_THROW(
    {
        MutexResult res = mutex1.acquire(WAIT_FOREVER);
    }, CppRtos::UnitTestException);

    //change current task to Task1
    port.setCurrentTask(ptrTaskData1);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eBlocked);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);

    port.selectHighestPriorityTask();

    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eBlocked);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);

    /*free mutex from task1*/
    //change current task to Task1
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);


    EXPECT_THROW(
        {
            MutexResult res = mutex1.release();
        }, CppRtos::UnitTestException);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eRunning);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eReady);
}

/*
Priority Inversion Scenario

    Objective: Test the system’s handling of priority inversion, where a lower-priority task holds a mutex that a higher-priority task needs.

Test Steps:

    Task Setup:
        Task 1: Low priority.
        Task 2: Medium priority.
        Task 3: High priority.
    Task 1 Acquires Mutex: Task 1 acquires the mutex and begins execution.
    Task 3 Attempts to Acquire Mutex: Task 3, the highest priority task, tries to acquire the mutex but is blocked because Task 1 holds it.
    Task 2 Begins Execution: Task 2, with medium priority, begins running since Task 3 is blocked.
    Priority Inversion Handling:
        If the RTOS supports priority inheritance, Task 1's priority is temporarily raised to the level of Task 3 to avoid priority inversion.
        Task 1 finishes its work and releases the mutex.
    Task 3 Resumes: Task 3 acquires the mutex and continues its work. Task 2 remains in its normal execution flow.

Expected Results:

    Task 3 should be able to run as soon as Task 1 releases the mutex, demonstrating proper handling of priority inversion.
    If priority inheritance is supported, Task 1 should inherit Task 3’s priority when Task 3 tries to acquire the mutex.
       
*/
TEST_F(MutexTest2, PriorityInversion)
{
    TaskData* ptrTaskDataLow    = mockTaskLow.getTaskData();
    TaskData* ptrTaskDataMedium = mockTaskMedium.getTaskData();
    TaskData* ptrTaskDataHigh   = mockTaskHigh.getTaskData();

    Port::Port& port = kernel->getPort();

    // Task 1 (Low Priority) acquires the mutex
    port.setTaskReady(ptrTaskDataLow);
    port.setTaskReady(ptrTaskDataMedium);
    port.setTaskReady(ptrTaskDataHigh);

    // Simulate Task 1 (Low Priority) running and acquiring the mutex
    port.resetTaskReady(ptrTaskDataLow, TaskStateType::eRunning);
    port.setCurrentTask(ptrTaskDataLow); // Simulates Task 1 is running
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::Success);

    // Task 3 (High Priority) should now attempt to run
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataHigh);

    // Task 3 (High Priority) attempts to acquire the mutex but gets blocked
    EXPECT_THROW( mutex1.acquire(WAIT_FOREVER), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(ptrTaskDataHigh->getState(), TaskStateType::eBlocked);

    // Task 2 (Medium Priority) should run next
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataMedium);
    EXPECT_EQ(ptrTaskDataMedium->getState(), TaskStateType::eRunning);
       
    // Simulate Task 2 yielding
    EXPECT_THROW( mockTaskMedium.yield(), CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataLow);

    // Task 1 (Low Priority) releases the mutex
    EXPECT_THROW( mutex1.release(), CppRtos::UnitTestException);
    
    // Task 3 (High Priority) should be unblocked and run now
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskDataHigh);
    EXPECT_EQ(ptrTaskDataHigh->getState(), TaskStateType::eRunning);
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