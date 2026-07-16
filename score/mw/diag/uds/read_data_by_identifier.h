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

/// @file read_data_by_identifier.h
/// @brief UDS ReadDataByIdentifier service interface (See ISO 14229-1:2020, Service 0x22).
///
/// Provides two levels of abstraction:
///   - `ReadDataByIdentifier`       — full interface with `MetaData` and cancellation support.
///   - `SimpleReadDataByIdentifier` — simplified adapter for non-blocking, context-free reads.

#ifndef SCORE_MW_DIAG_UDS_READ_DATA_BY_IDENTIFIER_H
#define SCORE_MW_DIAG_UDS_READ_DATA_BY_IDENTIFIER_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/meta_data.h"

#include <score/stop_token.hpp>

namespace score::mw::diag::uds
{

/// UDS ReadDataByIdentifier service (See ISO 14229-1:2020, Service 0x22).
///
/// Full interface — receives request `MetaData` (session, security level, etc.) and
/// a `stop_token` for cooperative cancellation of long-running reads.
class ReadDataByIdentifier
{
  public:
    /// @param meta_data   Context provided by the diagnostic runtime for this request.
    /// @param stop_token  Token that becomes stopped if the runtime cancels the request.
    /// @return Result<ByteVector> on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<ByteVector> Read(const MetaData& meta_data, score::cpp::stop_token stop_token) = 0;

    virtual ~ReadDataByIdentifier() noexcept = default;
};

/// Simplified adapter for `ReadDataByIdentifier` (must be non-blocking!)
///
/// Implement the simple `Read()` — the adapter bridges it to the full
/// `ReadDataByIdentifier` interface by ignoring `meta_data` and `stop_token`.
class SimpleReadDataByIdentifier : public ReadDataByIdentifier
{
  public:
    /// Read raw bytes for the data identifier in a fast and non-blocking manner.
    /// @return Result<ByteVector> on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<ByteVector> Read() = 0;

    virtual ~SimpleReadDataByIdentifier() noexcept = default;

  private:
    Result<ByteVector> Read(const MetaData& /*meta_data*/, score::cpp::stop_token /*stop_token*/) final
    {
        return Read();
    }
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_READ_DATA_BY_IDENTIFIER_H
