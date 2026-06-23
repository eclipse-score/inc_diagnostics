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

/// @file uds.h
/// @brief UDS diagnostic service interfaces: ReadDataByIdentifier, WriteDataByIdentifier,
///        RoutineControl and StartRoutine.

#ifndef SCORE_MW_DIAG_UDS_H
#define SCORE_MW_DIAG_UDS_H

#include "score/mw/diag/result.h"
#include "score/mw/diag/sovd_error.h"

#include <functional>
#include <optional>

namespace score
{
namespace mw
{
namespace diag
{

/************************************/
/* StartRoutine               */
/************************************/

/// Returned by RoutineControl::start().
/// Contains the synchronous reply to the start request (optional) and a
/// callable that will be invoked asynchronously to await the routine result.
///
/// @note The Rust API wraps the async part in a Pin<Box<dyn Future>>. In C++,
///       the async future is modelled as a std::function returning Result<optional<ByteVector>>,
///       which the diagnostic runtime invokes at its discretion.
struct StartRoutine
{
    /// Optional synchronous reply payload to be returned as the response to
    /// the RoutineControl Start request (sub-function 0x01).
    std::optional<ByteVector> reply;

    /// Callable that produces the final routine result.
    /// The runtime invokes this and delivers the result via the execution status mechanism.
    /// Returns: Ok(Some(bytes)) on success with result data,
    ///          Ok(None)        on success without result data,
    ///          Err(Error)      on failure.
    std::function<Result<std::optional<ByteVector>>()> result_provider;
};

/************************************/
/* ReadDataByIdentifier             */
/************************************/

/// UDS ReadDataByIdentifier service (cf. ISO 14229-1:2020, Service 0x22).
class ReadDataByIdentifier
{
  public:
    /// Read raw bytes for the data identifier.
    /// Returns Ok(ByteVector) on success, Err(Error) on failure.
    [[nodiscard]] virtual Result<ByteVector> read() = 0;

    ReadDataByIdentifier() = default;
    ReadDataByIdentifier(const ReadDataByIdentifier&) = delete;
    ReadDataByIdentifier(ReadDataByIdentifier&&) noexcept = delete;
    ReadDataByIdentifier& operator=(const ReadDataByIdentifier&) & = delete;
    ReadDataByIdentifier& operator=(ReadDataByIdentifier&&) & noexcept = delete;
    virtual ~ReadDataByIdentifier() noexcept = default;
};

/************************************/
/* WriteDataByIdentifier            */
/************************************/

/// UDS WriteDataByIdentifier service (cf. ISO 14229-1:2020, Service 0x2E).
class WriteDataByIdentifier
{
  public:
    /// Write raw bytes for the data identifier.
    /// Returns Ok on success, Err(Error) on failure.
    [[nodiscard]] virtual ResultBlank write(ByteView input) = 0;

    WriteDataByIdentifier() = default;
    WriteDataByIdentifier(const WriteDataByIdentifier&) = delete;
    WriteDataByIdentifier(WriteDataByIdentifier&&) noexcept = delete;
    WriteDataByIdentifier& operator=(const WriteDataByIdentifier&) & = delete;
    WriteDataByIdentifier& operator=(WriteDataByIdentifier&&) & noexcept = delete;
    virtual ~WriteDataByIdentifier() noexcept = default;
};

/************************************/
/* GenericDataIdentifier            */
/************************************/

/// Combined UDS DataIdentifier — supports both ReadDataByIdentifier (Service 0x22)
/// and WriteDataByIdentifier (Service 0x2E) through a single implementation class.
///
/// Use this when a DID must handle both read and write requests.
/// For read-only or write-only DIDs, implement ReadDataByIdentifier or
/// WriteDataByIdentifier directly.
// NOLINTBEGIN(fuchsia-multiple-inheritance)
class GenericDataIdentifier : public ReadDataByIdentifier, public WriteDataByIdentifier
{
  public:
    GenericDataIdentifier() = default;
    GenericDataIdentifier(const GenericDataIdentifier&) = delete;
    GenericDataIdentifier(GenericDataIdentifier&&) noexcept = delete;
    GenericDataIdentifier& operator=(const GenericDataIdentifier&) & = delete;
    GenericDataIdentifier& operator=(GenericDataIdentifier&&) & noexcept = delete;
    virtual ~GenericDataIdentifier() noexcept = default;
};
// NOLINTEND(fuchsia-multiple-inheritance)

/************************************/
/* RoutineControl                   */
/************************************/


/// UDS RoutineControl service (cf. ISO 14229-1:2020, Service 0x31).
///
/// NOTE: RequestResults (sub-function 0x03) is handled implicitly by the
///       diagnostic runtime via the execution status reporting mechanism —
///       implementors do not need to override it.
class RoutineControl
{
  public:
    /// Start the routine (sub-function 0x01).
    /// @param input  Optional input data accompanying the start request.
    /// @return Ok(StartRoutine) on success, Err(Error) on failure.
    [[nodiscard]] virtual Result<StartRoutine> start(std::optional<ByteView> input) = 0;

    /// Stop the routine (sub-function 0x02).
    /// @param input  Optional input data accompanying the stop request.
    /// @return Ok(optional<ByteVector>) on success (byte vector is the stop reply data),
    ///         Err(Error) on failure.
    [[nodiscard]] virtual Result<std::optional<ByteVector>> stop(std::optional<ByteView> input) = 0;

    /// Optionally provide the current routine completion percentage (0..100).
    /// Returns std::nullopt if completion percentage is not available.
    virtual std::optional<std::uint8_t> completion_percentage() const noexcept
    {
        return std::nullopt;
    }

    RoutineControl() = default;
    RoutineControl(const RoutineControl&) = delete;
    RoutineControl(RoutineControl&&) noexcept = delete;
    RoutineControl& operator=(const RoutineControl&) & = delete;
    RoutineControl& operator=(RoutineControl&&) & noexcept = delete;
    virtual ~RoutineControl() noexcept = default;
};

/************************************/
/* UdsService                       */
/************************************/

/// Raw UDS service handler for custom or proprietary services not covered by
/// ReadDataByIdentifier, WriteDataByIdentifier or RoutineControl.
/// cf. ISO 14229-1:2020 — vendor-specific service identifiers.
///
/// The default implementation rejects all messages with
/// uds::NegativeResponseCode::SubFunctionNotSupported, indicating that the
/// specific sub-function is not handled. Implementors override handle_message()
/// to process the raw byte payload and return the response bytes.
class UdsService
{
  public:
    /// Handle a raw UDS message.
    /// @param input  Raw request payload bytes (service identifier + data).
    /// @return Ok(ByteVector) containing the raw response bytes on success,
    ///         Err(Error) with a UDS NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<ByteVector> handle_message(ByteView input)
    {
        (void)input;
        return Result<ByteVector>{
            score::unexpect,
            Error::from_nrc(uds::NegativeResponseCode::SubFunctionNotSupported)};
    }

    UdsService() = default;
    UdsService(const UdsService&) = delete;
    UdsService(UdsService&&) noexcept = delete;
    UdsService& operator=(const UdsService&) & = delete;
    UdsService& operator=(UdsService&&) & noexcept = delete;
    virtual ~UdsService() noexcept = default;
};

}  // namespace diag
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_DIAG_UDS_H
