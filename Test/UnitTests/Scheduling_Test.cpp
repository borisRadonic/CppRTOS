// CMakeProject1.cpp : Defines the entry point for the application.
//

#include <gtest/gtest.h>

// Include your kernel header here if needed
// #include "Kernel.h"

// Example function to test
int add(int a, int b)
{
    return a + b;
}

// Test cases
TEST(AdditionTest, HandlesPositiveInput)
{
    EXPECT_EQ(add(1, 2), 3);
    EXPECT_EQ(add(5, 6), 11);
}

TEST(AdditionTest, HandlesNegativeInput)
{
    EXPECT_EQ(add(-1, -2), -3);
    EXPECT_EQ(add(-5, -6), -11);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}