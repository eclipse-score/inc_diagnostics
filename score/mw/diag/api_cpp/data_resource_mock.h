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

/// @file data_resource_mock.h
/// @brief GMock implementation of score::mw::diag::DataResource.

#ifndef SCORE_MW_DIAG_DATA_RESOURCE_MOCK_H
#define SCORE_MW_DIAG_DATA_RESOURCE_MOCK_H

#include "score/mw/diag/data_resource.h"

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace diag
{

/// Mock for score::mw::diag::DataResource.
class DataResourceMock : public DataResource
{
  public:
    using ReadResult  = Result<ReadValueReply>;
    using WriteResult = std::variant<std::monostate, sovd::DataError>;

    MOCK_METHOD(ReadResult,  read,  (ReadValueArgs  input), (override));
    MOCK_METHOD(WriteResult, write, (WriteValueArgs input), (override));
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_DATA_RESOURCE_MOCK_H
