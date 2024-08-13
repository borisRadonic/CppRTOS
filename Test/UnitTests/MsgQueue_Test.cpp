
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
    std::aligned_storage_t<sizeof(CppRtos::Kernel), 4> _prealoc_kernel_mem;
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
}
