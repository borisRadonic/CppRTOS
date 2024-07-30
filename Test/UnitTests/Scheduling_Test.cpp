#include <gmock/gmock.h>
#include "KernelFactory.hpp"
#include "Kernel.hpp"
#include "Task.hpp"
#include "Port.hpp"


using namespace CppRtos;
using ::testing::Return;
using ::testing::_;


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

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}