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
/// @brief GMock implementation of score::mw::diag::uds::RoutineControl.

#ifndef SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_MOCK_H
#define SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_MOCK_H

#include "score/mw/diag/uds/routine_control.h"

#include <gmock/gmock.h>

namespace score::mw::diag::uds::test
{

/// Mock for for the full context-aware score::mw::diag::uds::RoutineControl (Service 0x31).
class RoutineControlMock : public RoutineControl
{
  public:
    MOCK_METHOD(std::future<Result<ByteVector>>,
                Start,
                (ByteView input, const MetaData& meta_data, score::cpp::stop_token stop_token),
                (override));

    MOCK_METHOD(std::future<Result<ByteVector>>,
                Stop,
                (ByteView input, const MetaData& meta_data, score::cpp::stop_token stop_token),
                (override));

    MOCK_METHOD(std::future<Result<ByteVector>>,
                RequestResults,
                (ByteView input, const MetaData& meta_data, score::cpp::stop_token stop_token),
                (override));

    MOCK_METHOD(std::optional<std::uint8_t>, CompletionPercentage, (), (const, noexcept, override));
};

/// Mock for the context-free score::mw::diag::uds::SimpleRoutineControl (Service 0x31).
class SimpleRoutineControlMock : public SimpleRoutineControl
{
  public:
    MOCK_METHOD(Result<ByteVector>, Start, (ByteView input), (override));
    MOCK_METHOD(Result<ByteVector>, Stop, (ByteView input), (override));
    MOCK_METHOD(Result<ByteVector>, RequestResults, (ByteView input), (override));
    MOCK_METHOD(std::optional<std::uint8_t>, CompletionPercentage, (), (const, noexcept, override));
};

}  // namespace score::mw::diag::uds::test

#endif  // SCORE_MW_DIAG_UDS_ROUTINE_CONTROL_MOCK_H
