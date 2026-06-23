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

/// @file dtc_mock.h
/// @brief GMock test double for DtcMonitor.

#ifndef SCORE_MW_DIAG_DTC_MOCK_H
#define SCORE_MW_DIAG_DTC_MOCK_H

#include "score/mw/diag/dtc.h"

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace diag
{

/// Mock for score::mw::diag::DtcMonitor.
class DtcMonitorMock : public DtcMonitor
{
  public:
    using ReportResult = ResultBlank;

    MOCK_METHOD(ResultBlank, report, (DtcStatus status), (override));
    MOCK_METHOD(void, make_clearable, (), (noexcept, override));
    MOCK_METHOD(void, make_not_clearable, (), (noexcept, override));
    MOCK_METHOD(std::uint32_t, dtc_number, (), (const, noexcept, override));
    MOCK_METHOD(void, on_init, (std::function<void(DtcInitReason)> callback), (override));
};

/// Mock for score::mw::diag::DtcStorageGuard.
class DtcStorageGuardMock : public DtcStorageGuard
{
  public:
    MOCK_METHOD(void, inhibit_storage, (), (noexcept, override));
    MOCK_METHOD(void, allow_storage,   (), (noexcept, override));
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_DTC_MOCK_H
