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

/// @file routine_control_mock.h
/// @brief GMock implementation of score::mw::diag::RoutineControl.

#ifndef SCORE_MW_DIAG_API_CPP_UDS_ROUTINE_CONTROL_MOCK_H
#define SCORE_MW_DIAG_API_CPP_UDS_ROUTINE_CONTROL_MOCK_H

#include "score/mw/diag/routine_control.h"

#include <gmock/gmock.h>

namespace score::mw::diag::uds
{

/// Mock for score::mw::diag::uds::RoutineControl (Service 0x31).
class RoutineControlMock : public RoutineControl
{
  public:
    using StartResult = Result<StartRoutine>;
    using StopResult = Result<std::optional<ByteVector>>;

    MOCK_METHOD(StartResult, Start, (std::optional<ByteView> input), (override));
    MOCK_METHOD(StopResult, Stop, (std::optional<ByteView> input), (override));
    MOCK_METHOD(std::optional<std::uint8_t>, CompletionPercentage, (), (const, noexcept, override));
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_API_CPP_UDS_ROUTINE_CONTROL_MOCK_H
