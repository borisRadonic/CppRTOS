#include <gtest/gtest.h>
#include <string_view>
#include "Fifo.hpp"
#include "Task.hpp"
#include "KernelFactory.hpp"
#include "Kernel.hpp"

using namespace CppRtos;

TEST(FifoTest, GetAtUsesLogicalIndexAfterWrapAround)
{
    Fifo<int, 4> fifo;
    fifo.enqueue(1);
    fifo.enqueue(2);
    fifo.enqueue(3);
    EXPECT_EQ(fifo.dequeue(), 1);
    EXPECT_EQ(fifo.dequeue(), 2);
    fifo.enqueue(4);
    fifo.enqueue(5); // wraps around
    fifo.enqueue(6);

    ASSERT_EQ(fifo.getSize(), 4u);
    EXPECT_TRUE(fifo.isFull());
    EXPECT_EQ(fifo.getAt(0), 3);
    EXPECT_EQ(fifo.getAt(1), 4);
    EXPECT_EQ(fifo.getAt(2), 5);
    EXPECT_EQ(fifo.getAt(3), 6);
    EXPECT_EQ(fifo.getAt(4), 0); // out of bounds -> default value
}

TEST(FifoTest, RemoveAtKeepsOrderAfterWrapAround)
{
    Fifo<int, 4> fifo;
    fifo.enqueue(1);
    fifo.enqueue(2);
    fifo.enqueue(3);
    EXPECT_EQ(fifo.dequeue(), 1);
    fifo.enqueue(4);
    fifo.enqueue(5); // wraps around: logical content 2,3,4,5

    fifo.removeAt(1); // removes 3
    ASSERT_EQ(fifo.getSize(), 3u);
    EXPECT_EQ(fifo.getAt(0), 2);
    EXPECT_EQ(fifo.getAt(1), 4);
    EXPECT_EQ(fifo.getAt(2), 5);

    fifo.removeAt(7); // out of bounds: ignored
    EXPECT_EQ(fifo.getSize(), 3u);

    fifo.enqueue(6);
    EXPECT_EQ(fifo.dequeue(), 2);
    EXPECT_EQ(fifo.dequeue(), 4);
    EXPECT_EQ(fifo.dequeue(), 5);
    EXPECT_EQ(fifo.dequeue(), 6);
    EXPECT_TRUE(fifo.isEmpty());
}

TEST(TaskDataTest, SetNameTruncatesAndHandlesEmptyName)
{
    TaskData data;
    int dummyStack[4] = {};
    data.setStartStack(&dummyStack[0]);
    data.setTopStack(&dummyStack[3]);

    // The name buffer is followed by the stack pointers: an overflow would overwrite them
    data.setName(std::string_view("ThisNameIsMuchLongerThanSixteenChars"));
    EXPECT_EQ(data.getStartStackddress(), static_cast<void*>(&dummyStack[0]));
    EXPECT_EQ(data.getEndStackddress(), static_cast<void*>(&dummyStack[3]));

    data.setName(std::string_view(""));
    EXPECT_EQ(data.getStartStackddress(), static_cast<void*>(&dummyStack[0]));
    EXPECT_EQ(data.getEndStackddress(), static_cast<void*>(&dummyStack[3]));
}

TEST(KernelFactoryTest, RejectsMisalignedMemory)
{
    alignas(CppRtos::Kernel) static unsigned char memory[sizeof(CppRtos::Kernel) + alignof(CppRtos::Kernel)];
    KernelFactory& factory = KernelFactory::getInstance();
#ifdef NDEBUG
    EXPECT_EQ(factory.create(memory + 1), nullptr);
#endif
    Kernel* kernel = factory.create(memory);
    ASSERT_NE(kernel, nullptr);
    factory.destroy(memory);
    EXPECT_EQ(factory.getKernel(), nullptr);
}
