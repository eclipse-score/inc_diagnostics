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

#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/mw/diag/uds/negative_response_code.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds::test
{

namespace
{

class SimpleReadDataByIdentifierForTest : public SimpleReadDataByIdentifier
{
  public:
    Result<ByteVector> result{ByteVector{std::byte{0x12}, std::byte{0x34}}};

    Result<ByteVector> Read(const MetaData& /*meta_data*/) override
    {
        return result;
    }
};

}  // namespace

TEST(ReadDataByIdentifierTest, SimpleReadDataByIdentifierBridgesResultToFuture)
{
    SimpleReadDataByIdentifierForTest reader;
    MetaData meta_data{};
    score::cpp::stop_token stop_token{};

    ReadDataByIdentifier& full_interface = reader;
    auto future = full_interface.Read(meta_data, stop_token);

    auto outer_res = future.Get(stop_token);
    ASSERT_TRUE(outer_res.has_value());
    auto result = outer_res.value();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 2U);
    EXPECT_EQ(result.value()[0], std::byte{0x12});
}

TEST(ReadDataByIdentifierTest, SimpleReadDataByIdentifierPropagatesError)
{
    SimpleReadDataByIdentifierForTest reader;
    reader.result = score::MakeUnexpected(NegativeResponseCode::SecurityAccessDenied);

    MetaData meta_data{};
    score::cpp::stop_token stop_token{};

    ReadDataByIdentifier& full_interface = reader;
    auto future = full_interface.Read(meta_data, stop_token);

    auto outer_res = future.Get(stop_token);
    ASSERT_TRUE(outer_res.has_value());
    auto result = outer_res.value();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ToNegativeResponseCode(*result.error()), NegativeResponseCode::SecurityAccessDenied);
}

}  // namespace score::mw::diag::uds::test
