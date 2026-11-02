/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include <gtest/gtest.h>

#include <cstdint>

namespace
{
// Function to be tested
std::int32_t add(std::int32_t left, std::int32_t right)
{
    return left + right;
}

// Test case
TEST(AdditionTest, HandlesPositiveNumbers)
{
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(10, 20), 30);
}

TEST(AdditionTest, HandlesNegativeNumbers)
{
    EXPECT_EQ(add(-2, -3), -5);
    EXPECT_EQ(add(-10, 5), -5);
}

TEST(AdditionTest, HandlesZero)
{
    EXPECT_EQ(add(0, 0), 0);
    EXPECT_EQ(add(0, 5), 5);
    EXPECT_EQ(add(5, 0), 5);
}
}  // namespace

// Main function for running tests
int main(int argc, char** argv)
{
    try
    {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
    catch (...)
    {
        return 1;
    }
}
