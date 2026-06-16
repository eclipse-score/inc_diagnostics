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

/// @file generic_service_test.cpp
/// @brief Unit tests for score/mw/diag/generic_service.h
///        Covers: GenericService default implementation and GenericServiceMock.

#include "score/mw/diag/uds/generic_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::An;
using ::testing::Return;

namespace score::mw::diag::uds
{

TEST(GenericServiceTest, DefaultHandleMessageReturnsSubFunctionNotSupported)
{
    // Concrete subclass that does not override HandleMessage —
    // relies on the default implementation.
    struct DefaultGenericService final : public GenericService
    {
    };

    DefaultGenericService svc{};
    const ByteVector request{std::byte{0xB2}, std::byte{0x01}};
    const auto result = svc.HandleMessage(ByteView{request});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SubFunctionNotSupported);
}

TEST(GenericServiceTest, MockHandleMessageReturnsBytes)
{
    GenericServiceMock mock{};
    const ByteVector request{std::byte{0xB2}, std::byte{0x01}};
    const ByteVector response{std::byte{0xF2}, std::byte{0x00}};
    EXPECT_CALL(mock, HandleMessage(An<ByteView>())).WillOnce(Return(Result<ByteVector>{response}));

    const auto result = mock.HandleMessage(ByteView{request});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2U);
    EXPECT_EQ((*result)[0], std::byte{0xF2});
}

TEST(GenericServiceTest, MockHandleMessageReturnsError)
{
    GenericServiceMock mock{};
    const ByteVector request{std::byte{0xB2}};
    EXPECT_CALL(mock, HandleMessage(An<ByteView>()))
        .WillOnce(Return(Result<ByteVector>{score::unexpect, NegativeResponseCode::SecurityAccessDenied}));

    const auto result = mock.HandleMessage(ByteView{request});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::SecurityAccessDenied);
}

}  // namespace score::mw::diag::uds
