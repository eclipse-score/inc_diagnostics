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

/// @file generic_data_identifier_test.cpp
/// @brief Unit tests for score/mw/diag/generic_data_identifier.h
///        Covers: GenericDataIdentifier read and write via GenericDataIdentifierMock.

#include "score/mw/diag/generic_data_identifier_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::An;
using ::testing::Return;

namespace score::mw::diag::uds
{

TEST(GenericDataIdentifierTest, ReadReturnsSuccessBytes)
{
    GenericDataIdentifierMock mock{};
    EXPECT_CALL(mock, Read()).WillOnce(Return(Result<ByteVector>{ByteVector{std::byte{0xDE}, std::byte{0xAD}}}));

    const auto result = mock.Read();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2U);
    EXPECT_EQ((*result)[0], std::byte{0xDE});
}

TEST(GenericDataIdentifierTest, ReadReturnsError)
{
    GenericDataIdentifierMock mock{};
    EXPECT_CALL(mock, Read())
        .WillOnce(Return(Result<ByteVector>{score::unexpect, NegativeResponseCode::RequestOutOfRange}));

    const auto result = mock.Read();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::RequestOutOfRange);
}

TEST(GenericDataIdentifierTest, WriteReturnsOk)
{
    GenericDataIdentifierMock mock{};
    const ByteVector data{std::byte{0x01}, std::byte{0x02}};
    EXPECT_CALL(mock, Write(An<ByteView>())).WillOnce(Return(ResultBlank{}));

    const auto result = mock.Write(ByteView{data});
    EXPECT_TRUE(result.has_value());
}

TEST(GenericDataIdentifierTest, WriteReturnsError)
{
    GenericDataIdentifierMock mock{};
    const ByteVector data{std::byte{0xFF}};
    EXPECT_CALL(mock, Write(An<ByteView>()))
        .WillOnce(Return(ResultBlank{score::unexpect, NegativeResponseCode::ConditionsNotCorrect}));

    const auto result = mock.Write(ByteView{data});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

}  // namespace score::mw::diag::uds
