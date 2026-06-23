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

/// @file dtc.h
/// @brief DTC (Diagnostic Trouble Code) reporter API.
///
/// Types: `DtcStatus` (Passed/Failed), `DtcInitReason` (clear/restart/re-enable reasons),
/// `CounterBasedDebouncingConfig`, `TimeBasedDebouncingConfig`, `DebouncingConfig` (variant),
/// `DtcMonitor` (abstract fault-reporting interface).
///
/// Application code receives a `DtcMonitor` reference from the middleware and calls
/// `report()` each monitoring cycle.

#ifndef SCORE_MW_DIAG_DTC_H
#define SCORE_MW_DIAG_DTC_H

#include "score/mw/diag/sovd_error.h"

#include <cstdint>
#include <functional>
#include <variant>

namespace score
{
namespace mw
{
namespace diag
{

/************************************/
/* DtcStatus                        */
/************************************/

/// Fault-monitor outcome to report for a single monitoring cycle.
enum class DtcStatus : std::uint8_t
{
    Passed,  ///< The monitored signal is within its healthy range.
    Failed,  ///< The monitored signal has exceeded its healthy range (fault detected).
};

/************************************/
/* DtcInitReason                    */
/************************************/

/// Reason code delivered to the `DtcMonitor::on_init()` callback.
/// Applications should reset internal debouncing state whenever the callback is invoked.
enum class DtcInitReason : std::uint8_t
{
    /// DTC storage was cleared (e.g. ClearDiagnosticInformation request from a tester).
    ClearDtc,

    /// The ECU was restarted (power-on or warm reset).
    Restart,

    /// DTC monitoring was re-enabled after being disabled by an enable-condition change.
    ReEnabled,

    /// DTC storage was re-enabled after being disabled by a storage-condition change.
    StorageReEnabled,
};

/************************************/
/* Debouncing configuration         */
/************************************/

/// Counter-based debouncing: counter increments by `failed_stepsize` per Failed report
/// and decrements by `passed_stepsize` per Passed report; qualifies at the respective threshold.
struct CounterBasedDebouncingConfig
{
    /// Counter value that qualifies the fault as Failed. Must be > 0.
    std::int16_t failed_threshold{10};

    /// Counter value that qualifies the fault as Passed. Must be < 0.
    std::int16_t passed_threshold{-10};

    /// Amount added to the counter per Failed report. Must be > 0.
    std::int16_t failed_stepsize{1};

    /// Amount subtracted from the counter per Passed report. Must be > 0.
    std::int16_t passed_stepsize{1};
};

/// Time-based debouncing: qualifies as Failed after `failed_ms` ms of continuous failure,
/// and as Passed after `passed_ms` ms of continuous pass.
struct TimeBasedDebouncingConfig
{
    /// Continuous fail duration in milliseconds required to qualify as Failed.
    std::uint32_t failed_ms{100U};

    /// Continuous pass duration in milliseconds required to qualify as Passed.
    std::uint32_t passed_ms{100U};
};

/// Debouncing configuration for a DTC monitor.
/// Exactly one of counter-based or time-based debouncing; no other states are possible.
using DebouncingConfig = std::variant<CounterBasedDebouncingConfig, TimeBasedDebouncingConfig>;

/************************************/
/* DtcMonitor                       */
/************************************/

/// Abstract interface for reporting fault status on a single DTC.
/// Instances are created by the diagnostic middleware; application code receives a DtcMonitor instance at runtime startup 
/// and calls `report()` for each monitored signal evaluation.
class DtcMonitor
{
  public:
    /// Report the fault status for one monitoring cycle.
    /// @return Ok on success; Err if reporting failed (e.g. monitor not yet initialised).
    [[nodiscard]] virtual ResultBlank report(DtcStatus status) = 0;

    /// Allow the DTC to be cleared by a tester ClearDiagnosticInformation request (default state).
    virtual void make_clearable() noexcept = 0;

    /// Prevent the DTC from being cleared by a tester ClearDiagnosticInformation request.
    virtual void make_not_clearable() noexcept = 0;

    /// Returns the 3-byte UDS DTC number; assigned by middleware configuration, stable across restarts.
    virtual std::uint32_t dtc_number() const noexcept = 0;

    /// Register a callback invoked with a `DtcInitReason` on startup, tester clear, or
    /// condition re-enable. Applications SHOULD reset internal debouncing state in the callback.
    virtual void on_init(std::function<void(DtcInitReason)> callback) = 0;

    DtcMonitor()                             = default;
    DtcMonitor(const DtcMonitor&)            = delete;
    DtcMonitor(DtcMonitor&&) noexcept        = delete;
    DtcMonitor& operator=(const DtcMonitor&)  & = delete;
    DtcMonitor& operator=(DtcMonitor&&) &    noexcept = delete;
    virtual ~DtcMonitor() noexcept = default;
};

/************************************/
/* DtcStorageGuard                  */
/************************************/

/// Abstract interface for temporarily inhibiting or re-enabling DTC storage.
///
/// Allows application code to suppress DTC storage during conditions where fault
/// qualification is unreliable (e.g. ECU initialisation, known transient states).
/// Instances are provided by the middleware; obtain one via the factory/DI layer.
///
/// @note Inhibiting storage does not clear already-stored DTCs. Faults detected while
///       storage is inhibited are still debounced but not written to the DTC memory.
class DtcStorageGuard
{
  public:
    /// Inhibit DTC storage. Faults detected while inhibited are debounced but not stored.
    virtual void inhibit_storage() noexcept = 0;

    /// Re-enable DTC storage after a prior call to inhibit_storage().
    virtual void allow_storage() noexcept = 0;

    DtcStorageGuard()                                    = default;
    DtcStorageGuard(const DtcStorageGuard&)              = delete;
    DtcStorageGuard(DtcStorageGuard&&) noexcept          = delete;
    DtcStorageGuard& operator=(const DtcStorageGuard&)  & = delete;
    DtcStorageGuard& operator=(DtcStorageGuard&&) &      noexcept = delete;
    virtual ~DtcStorageGuard() noexcept = default;
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_DTC_H
