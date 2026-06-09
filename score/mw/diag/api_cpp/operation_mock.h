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

/// @file operation_mock.h
/// @brief GMock implementations of score::mw::diag::Operation and ExecutionControl.

#ifndef SCORE_MW_DIAG_OPERATION_MOCK_H
#define SCORE_MW_DIAG_OPERATION_MOCK_H

#include "score/mw/diag/operation.h"

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace diag
{

/// Mock for score::mw::diag::ExecutionControl.
class ExecutionControlMock : public ExecutionControl
{
  public:
    MOCK_METHOD(const ExecutionId&, exec_id,          (), (const, noexcept, override));
    MOCK_METHOD(ExecutionEvent,     next_exec_event,  (), (override));
};

/// Mock for score::mw::diag::Operation.
class OperationMock : public Operation
{
  public:
    using ExecResult = Result<ExecutionHandle>;

    MOCK_METHOD(ExecResult, execute, (ExecuteArguments input, ExecutionControl& control), (override));
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_OPERATION_MOCK_H
