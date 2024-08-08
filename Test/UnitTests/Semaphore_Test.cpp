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
    std::aligned_storage_t<sizeof(CppRtos::Kernel), 4> _prealoc_kernel_mem;
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
        CppRtos::KernelFactory& kernelFactory = CppRtos::KernelFactory::getInstance();
        kernelFactory.destroy(&_prealoc_kernel_mem);
    }


};

/*
Test Scenario:

- **Task 1**: Highest priority.
- **Task 2**: Lower priority.

### Sequence of Events:

1. **Task 1 Running**:
   - Task 1 is executing and signals the semaphore.
   - Since no tasks are currently waiting on the semaphore, no task is unblocked, and the semaphore's internal counter is incremented.

2. **Kernel Yields**:
   - The kernel's scheduler continues to run Task 1 because it has the highest priority.

3. **Task 1 Voluntarily Yields**:
   - Task 1 explicitly yields the CPU by calling the `task->yield()` function.
   - This action allows lower-priority tasks, such as Task 2, to be scheduled.

4. **Task 2 Scheduling**:
   - The scheduler, finding that Task 1 has yielded, schedules Task 2, as it is the next highest-priority task that is ready to run.

5. **Task 2 Running**:
   - Task 2 starts running and attempts to acquire the semaphore.
   - Since Task 1 previously signaled the semaphore, Task 2 successfully acquires it.

### Key Points:
- Task 2 will only be scheduled after Task 1 yields, blocks, or completes.
- The semaphore signaled by Task 1 allows Task 2 to proceed when it is eventually scheduled.
*/

TEST_F(SemaphoreTest, TestSemaphore1)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();
    Port::Port& port = kernel->getPort();

    port.setTaskReady(ptrTaskData1);
    port.setTaskReady(ptrTaskData2);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // task1 is running
    /*Task 1 signals semaphore1*/
    EXPECT_THROW(
    {
        ptrSem1->signal();
    }, CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // task1 is still running

    //Task 1 gives CPU time to others (Task2)

    EXPECT_THROW(
    {
       mockTask1.yield();
    }, CppRtos::UnitTestException);
    
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2); // task2 is running


    /*Task 2 waits on semaphore1*/
    EXPECT_EQ(ptrSem1->wait(WAIT_FOREVER), SemResult::Success);
}

/*
Test Scenario:

- **Task 1**: Highest priority.
- **Task 2**: Lower priority.

### Sequence of Events:

1. **Task 2 Running and Blocking**:
   - Task 2 is currently running and attempts to acquire a semaphore.
   - The semaphore is not available (e.g., its count is zero), so Task 2 is blocked and moved to the waiting state.
   - Task 2 will remain blocked until the semaphore is signaled by another task or an ISR.

2. **Task 1 Running**:
   - After Task 2 is blocked, the scheduler runs Task 1, which has the highest priority.
   - Task 1 executes and eventually signals the semaphore that Task 2 is waiting on.
   - Signaling the semaphore increments its count, unblocking Task 2 and moving it to the ready state.

3. **Task 1 Yields**:
   - Task 1, after signaling the semaphore, calls `task->yield()` to voluntarily give up the CPU.
   - This allows the scheduler to consider other tasks that are ready to run.

4. **Task 2 Scheduling**:
   - The scheduler, upon Task 1 yielding, checks the ready queue and finds Task 2, which is now ready because it was unblocked when Task 1 signaled the semaphore.
   - Task 2 is scheduled to run next since Task 1 has yielded.

5. **Task 2 Running**:
   - Task 2, now unblocked and scheduled, resumes execution and successfully acquires the semaphore.
   - Task 2 continues its execution based on the newly acquired semaphore.

### Key Points:
- Task 2 becomes blocked when it tries to acquire a semaphore that is not available.
- Task 1, while running, signals the semaphore, which unblocks Task 2.
- Task 2 is scheduled to run after Task 1 voluntarily yields the CPU, and it then successfully acquires the semaphore.
*/


TEST_F(SemaphoreTest, TestSemaphore2)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();
    
    port.setTaskReady(ptrTaskData1);
    port.setTaskReady(ptrTaskData2);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // task1 is running

    //Task 1 gives CPU time to others (Task2)

    EXPECT_THROW(
    {
        mockTask1.yield();
    }, CppRtos::UnitTestException);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2); // task2 is running

    //Task 2 is currently running and attempts to acquire a semaphore.
    EXPECT_THROW(
    {
        ptrSem1->wait( WAIT_FOREVER );
    }, CppRtos::UnitTestException);
    
    //Task 2 is blocked and moved to the waiting state.
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); //task 1 is running again

    /*Task 1 signals semaphore1*/
    EXPECT_THROW(
    {
        ptrSem1->signal();
    }, CppRtos::UnitTestException);
    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1); // task1 is still running

    //Task 1 gives CPU time to others (Task2)

    EXPECT_THROW(
    {
        mockTask1.yield();
    }, CppRtos::UnitTestException);

    port.selectHighestPriorityTask();
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2); // task2 is running
}