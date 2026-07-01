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

/// @file read_data_by_identifier_test.cpp
/// @brief Unit tests for score/mw/diag/read_data_by_identifier.h
///        Covers: ReadDataByIdentifier via ReadDataByIdentifierMock.

#include "score/mw/diag/read_data_by_identifier_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Return;

namespace score::mw::diag::uds
{

TEST(ReadDataByIdentifierTest, ReadReturnsSuccessBytes)
{
    ReadDataByIdentifierMock mock{};
    EXPECT_CALL(mock, Read()).WillOnce(Return(ResultWithData{ByteVector{std::byte{0xDE}, std::byte{0xAD}}}));

    const auto result = mock.Read();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2U);
    EXPECT_EQ((*result)[0], std::byte{0xDE});
}

TEST(ReadDataByIdentifierTest, ReadReturnsError)
{
    ReadDataByIdentifierMock mock{};
    EXPECT_CALL(mock, Read())
        .WillOnce(Return(ResultWithData{score::unexpect, NegativeResponseCode::SecurityAccessDenied}));

    const auto result = mock.Read();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SecurityAccessDenied);
}

TEST(ReadDataByIdentifierTest, ReadCalledMultipleTimes)
{
    ReadDataByIdentifierMock mock{};
    EXPECT_CALL(mock, Read()).Times(3).WillRepeatedly(Return(ResultWithData{ByteVector{std::byte{0x01}}}));

    for (int i = 0; i < 3; ++i)
    {
        EXPECT_TRUE(mock.Read().has_value());
    }
}

}  // namespace score::mw::diag::uds
