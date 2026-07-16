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

/// @file write_data_by_identifier.h
/// @brief UDS WriteDataByIdentifier service interface (See ISO 14229-1:2020, Service 0x2E).
///
/// Provides two levels of abstraction:
///   - `WriteDataByIdentifier`       — full interface with `MetaData` and cancellation support.
///   - `SimpleWriteDataByIdentifier` — simplified adapter for non-blocking, context-free writes.

#ifndef SCORE_MW_DIAG_UDS_WRITE_DATA_BY_IDENTIFIER_H
#define SCORE_MW_DIAG_UDS_WRITE_DATA_BY_IDENTIFIER_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/meta_data.h"

#include <score/stop_token.hpp>

namespace score::mw::diag::uds
{

/// UDS WriteDataByIdentifier service (See ISO 14229-1:2020, Service 0x2E).
///
/// Full interface — receives request `MetaData` (session, security level, etc.) and
/// a `stop_token` for cooperative cancellation of long-running writes (e.g., NVM writing).
class WriteDataByIdentifier
{
  public:
    /// @param input       Non-owning view of the raw bytes to write.
    /// @param meta_data   Context provided by the diagnostic runtime for this request.
    /// @param stop_token  Token that becomes stopped if the runtime cancels the request.
    /// @return Result<score::cpp::blank> on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<score::cpp::blank> Write(ByteView input,
                                                          const MetaData& meta_data,
                                                          score::cpp::stop_token stop_token) = 0;

    virtual ~WriteDataByIdentifier() noexcept = default;
};

/// Simplified adapter for `WriteDataByIdentifier` (must be non-blocking!)
///
/// Implement the simple `Write()` — the adapter bridges it to the full
/// `WriteDataByIdentifier` interface by ignoring `meta_data` and `stop_token`.
class SimpleWriteDataByIdentifier : public WriteDataByIdentifier
{
  public:
    /// Write raw bytes for the data identifier in a fast and non-blocking manner.
    /// @param input  Non-owning view of the raw bytes to write.
    /// @return Result<score::cpp::blank> on success, NegativeResponseCode on failure.
    [[nodiscard]] virtual Result<score::cpp::blank> Write(ByteView input) = 0;

    virtual ~SimpleWriteDataByIdentifier() noexcept = default;

  private:
    Result<score::cpp::blank> Write(ByteView input,
                                    const MetaData& /*meta_data*/,
                                    score::cpp::stop_token /*stop_token*/) final
    {
        return Write(input);
    }
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_WRITE_DATA_BY_IDENTIFIER_H
