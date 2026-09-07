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

#include "score/mw/diag/uds/generic_data_identifier.h"
#include "score/mw/diag/uds/negative_response_code.h"

#include <gtest/gtest.h>

namespace score::mw::diag::uds::test
{

namespace
{

class SimpleGenericDataIdentifierForTest : public SimpleGenericDataIdentifier
{
  public:
    Result<ByteVector> read_result{ByteVector{std::byte{0xDE}, std::byte{0xAD}}};
    Result<void> write_result{};

    Result<ByteVector> Read(const MetaData& /*meta_data*/) override
    {
        return read_result;
    }

    Result<void> Write(ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        return write_result;
    }
};

}  // namespace

TEST(GenericDataIdentifierTest, SimpleGenericDataIdentifierReadAndWrite)
{
    SimpleGenericDataIdentifierForTest gdid;
    MetaData meta_data{};
    score::cpp::stop_token stop_token{};
    ByteVector payload{std::byte{0x01}};

    ReadDataByIdentifier& read_interface = gdid;
    auto read_future = read_interface.Read(meta_data, stop_token);
    auto read_outer = read_future.Get(stop_token);
    ASSERT_TRUE(read_outer.has_value());
    auto read_res = read_outer.value();
    ASSERT_TRUE(read_res.has_value());
    EXPECT_EQ(read_res.value().size(), 2U);

    WriteDataByIdentifier& write_interface = gdid;
    auto write_future = write_interface.Write(payload, meta_data, stop_token);
    auto write_outer = write_future.Get(stop_token);
    ASSERT_TRUE(write_outer.has_value());
    auto write_res = write_outer.value();
    EXPECT_TRUE(write_res.has_value());
}

}  // namespace score::mw::diag::uds::test
