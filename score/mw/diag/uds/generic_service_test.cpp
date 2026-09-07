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

#include "score/mw/diag/uds/generic_service.h"
#include "score/mw/diag/uds/negative_response_code.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds::test
{

namespace
{

class SimpleGenericServiceForTest : public SimpleGenericService
{
  public:
    Result<ByteVector> result{ByteVector{std::byte{0xAB}}};

    Result<ByteVector> HandleMessage(ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        return result;
    }
};

}  // namespace

TEST(GenericServiceTest, SimpleGenericServiceBridgesResultToFuture)
{
    SimpleGenericServiceForTest service;
    MetaData meta_data{};
    score::cpp::stop_token stop_token{};
    ByteVector payload{std::byte{0x31}};

    GenericService& full_interface = service;
    auto future = full_interface.HandleMessage(payload, meta_data, stop_token);

    auto outer_res = future.Get(stop_token);
    ASSERT_TRUE(outer_res.has_value());
    auto result = outer_res.value();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value()[0], std::byte{0xAB});
}

TEST(GenericServiceTest, SimpleGenericServicePropagatesError)
{
    SimpleGenericServiceForTest service;
    service.result = score::MakeUnexpected(NegativeResponseCode::ServiceNotSupported);

    MetaData meta_data{};
    score::cpp::stop_token stop_token{};
    ByteVector payload{std::byte{0x31}};

    GenericService& full_interface = service;
    auto future = full_interface.HandleMessage(payload, meta_data, stop_token);

    auto outer_res = future.Get(stop_token);
    ASSERT_TRUE(outer_res.has_value());
    auto result = outer_res.value();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ToNegativeResponseCode(*result.error()), NegativeResponseCode::ServiceNotSupported);
}

}  // namespace score::mw::diag::uds::test
