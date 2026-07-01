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

/// @file write_data_by_identifier_test.cpp
/// @brief Unit tests for score/mw/diag/write_data_by_identifier.h
///        Covers: WriteDataByIdentifier via WriteDataByIdentifierMock.

#include "score/mw/diag/write_data_by_identifier_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::An;
using ::testing::Return;

namespace score::mw::diag::uds
{

TEST(WriteDataByIdentifierTest, WriteReturnsOk)
{
    WriteDataByIdentifierMock mock{};
    const ByteVector data{std::byte{0x01}, std::byte{0x02}};
    EXPECT_CALL(mock, Write(An<ByteView>())).WillOnce(Return(ResultBlank{}));

    const auto result = mock.Write(ByteView{data});
    EXPECT_TRUE(result.has_value());
}

TEST(WriteDataByIdentifierTest, WriteReturnsError)
{
    WriteDataByIdentifierMock mock{};
    const ByteVector data{std::byte{0xFF}};
    EXPECT_CALL(mock, Write(An<ByteView>()))
        .WillOnce(Return(ResultBlank{score::unexpect, NegativeResponseCode::GeneralProgrammingFailure}));

    const auto result = mock.Write(ByteView{data});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::GeneralProgrammingFailure);
}

TEST(WriteDataByIdentifierTest, WriteEmptyInput)
{
    WriteDataByIdentifierMock mock{};
    const ByteView empty_view{};
    EXPECT_CALL(mock, Write(An<ByteView>())).WillOnce(Return(ResultBlank{}));
    EXPECT_TRUE(mock.Write(empty_view).has_value());
}

}  // namespace score::mw::diag::uds
