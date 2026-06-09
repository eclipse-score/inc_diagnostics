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

/// @file uds_mock.h
/// @brief GMock implementations of score::mw::diag UDS interfaces.
///
/// Provides: ReadDataByIdentifierMock, WriteDataByIdentifierMock, RoutineControlMock

#ifndef SCORE_MW_DIAG_UDS_MOCK_H
#define SCORE_MW_DIAG_UDS_MOCK_H

#include "score/mw/diag/uds.h"

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace diag
{

/// Mock for score::mw::diag::ReadDataByIdentifier (Service 0x22).
class ReadDataByIdentifierMock : public ReadDataByIdentifier
{
  public:
    // Alias to avoid preprocessor confusion with the comma inside std::variant.
    using ReadResult = Result<ByteVector>;

    MOCK_METHOD(ReadResult, read, (), (override));
};

/// Mock for score::mw::diag::WriteDataByIdentifier (Service 0x2E).
class WriteDataByIdentifierMock : public WriteDataByIdentifier
{
  public:
    MOCK_METHOD(ResultBlank, write, (ByteView input), (override));
};

/// Mock for score::mw::diag::GenericDataIdentifier (Services 0x22 + 0x2E combined).
class GenericDataIdentifierMock : public GenericDataIdentifier
{
  public:
    using ReadResult = Result<ByteVector>;

    MOCK_METHOD(ReadResult, read, (), (override));
    MOCK_METHOD(ResultBlank, write, (ByteView input), (override));
};

/// Mock for score::mw::diag::RoutineControl (Service 0x31).
class RoutineControlMock : public RoutineControl
{
  public:
    using StartResult = Result<StartRoutine>;
    using StopResult  = Result<std::optional<ByteVector>>;

    MOCK_METHOD(StartResult, start, (std::optional<ByteView> input), (override));
    MOCK_METHOD(StopResult,  stop,  (std::optional<ByteView> input), (override));
    MOCK_METHOD(std::optional<std::uint8_t>, completion_percentage, (), (const, noexcept, override));
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_UDS_MOCK_H
