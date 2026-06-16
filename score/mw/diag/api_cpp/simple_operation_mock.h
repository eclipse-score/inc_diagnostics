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

/// @file simple_operation_mock.h
/// @brief GMock implementation of score::mw::diag::SimpleOperation.

#ifndef SCORE_MW_DIAG_SIMPLE_OPERATION_MOCK_H
#define SCORE_MW_DIAG_SIMPLE_OPERATION_MOCK_H

#include "score/mw/diag/simple_operation.h"

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace diag
{

/// Mock for score::mw::diag::SimpleOperation.
class SimpleOperationMock : public SimpleOperation
{
  public:
    using StartResult = Result<ExecutionHandle>;
    using StopResult  = Result<std::optional<DiagnosticReply>>;

    MOCK_METHOD(StartResult, start, (ExecuteArguments input), (override));
    MOCK_METHOD(StopResult,  stop,  (std::optional<ExecuteArguments> input), (override));
    MOCK_METHOD(std::optional<std::uint8_t>, completion_percentage, (), (const, noexcept, override));
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_SIMPLE_OPERATION_MOCK_H
