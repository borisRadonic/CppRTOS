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
    Port::Port port;
    MockTask mockTask1;
    MockTask mockTask2;
    
    Mutex mutex1;
    Semaphore* ptrSem1;
    MessageQueue<TestMsg, 30> msgQueue1;


    void SetUp() override
    {
        ptrSem1 = new Semaphore(1, 0);
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

    kernel->setTaskReady(ptrTaskData1);
    kernel->setTaskReady(ptrTaskData2);

    kernel->selectHighestPriorityTask();

    EXPECT_EQ(kernel->getCurrentTask(), ptrTaskData1);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eReady);
}

TEST_F(KernelTest, SelectHighestPriorityTask_PreemptsCurrentTask)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    kernel->setTaskReady(ptrTaskData1);
    kernel->setTaskReady(ptrTaskData2);
    kernel->selectHighestPriorityTask(); // taskData1 is running
    EXPECT_EQ(kernel->getCurrentTask(), ptrTaskData1);

    kernel->selectHighestPriorityTask();

    EXPECT_EQ(kernel->getCurrentTask(), ptrTaskData2);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eReady);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eRunning);
}


TEST_F(KernelTest, TestMutex)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    kernel->setTaskReady(ptrTaskData1);
    kernel->setTaskReady(ptrTaskData2);
    kernel->selectHighestPriorityTask(); // task1 is running
    EXPECT_EQ(kernel->getCurrentTask(), ptrTaskData1);
    
    /*Task 1 acquires mutex1*/
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::Success);

    /*Try to acquire again*/
    EXPECT_EQ(mutex1.acquire(WAIT_FOREVER), MutexResult::AlreadyOwned);
    
    //change current task to Task2
    kernel->setCurrentTask(ptrTaskData2);

    EXPECT_THROW(
    {
       MutexResult res = mutex1.acquire(WAIT_FOREVER);
    }, CppRtos::UnitTestException);
    
    //change current task to Task1
    kernel->setCurrentTask(ptrTaskData1);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eBlocked);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);
    
    kernel->selectHighestPriorityTask();

    EXPECT_EQ(kernel->getCurrentTask(), ptrTaskData1);

    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eBlocked);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eRunning);

    /*free mutex from task1*/
    //change current task to Task1
    EXPECT_EQ(kernel->getCurrentTask(), ptrTaskData1);
   

    EXPECT_THROW(
    {
        MutexResult res = mutex1.release();
    }, CppRtos::UnitTestException);
    
    kernel->selectHighestPriorityTask();
    EXPECT_EQ(kernel->getCurrentTask(), ptrTaskData2);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eRunning);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eReady);
}



int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}