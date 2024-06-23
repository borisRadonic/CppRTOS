#include "pch.h"

#include "task.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstddef>

#include <iostream>
#include <thread>
#include <chrono>


constexpr std::size_t PRODUCER_STACK_SIZE = 2048U;
constexpr std::size_t CONSUMER_STACK_SIZE = 2048U;
constexpr std::size_t NUMBER_OF_REGISTERS = 16;

class ProducerTask : public CppRtos::Task<NUMBER_OF_REGISTERS, PRODUCER_STACK_SIZE>
{
public:
    ProducerTask(int priority)
        : CppRtos::Task<NUMBER_OF_REGISTERS, PRODUCER_STACK_SIZE>(priority)
    {
    }

    void run() override
    {
        while (true)
        {
            // Simulate work
            std::cout << "Producer Task is running" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // Yield control back to the scheduler (simulated)
            break;
        }
    }
};

class ConsumerTask : public CppRtos::Task<NUMBER_OF_REGISTERS, CONSUMER_STACK_SIZE> 
{
public:
    ConsumerTask(int priority)
        : Task(priority) {}

    void run() override
    {
        while (true)
        {
            // Simulate work
            std::cout << "Consumer Task is running" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            // Yield control back to the scheduler (simulated)
            break;
        }
    }
};


TEST(TestCaseName, TestName)
{
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}