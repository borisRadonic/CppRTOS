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

TEST_F(KernelTest, SelectHighestPriorityTask_PreemptsCurrentTask)
{
    TaskData* ptrTaskData1 = mockTask1.getTaskData();
    TaskData* ptrTaskData2 = mockTask2.getTaskData();

    Port::Port& port = kernel->getPort();

    port.setTaskReady(ptrTaskData1);
    port.setTaskReady(ptrTaskData2);
    port.selectHighestPriorityTask(); // taskData1 is running
    EXPECT_EQ(port.getCurrentTask(), ptrTaskData1);

    port.selectHighestPriorityTask();

    EXPECT_EQ(port.getCurrentTask(), ptrTaskData2);
    EXPECT_EQ(ptrTaskData1->getState(), TaskStateType::eReady);
    EXPECT_EQ(ptrTaskData2->getState(), TaskStateType::eRunning);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}