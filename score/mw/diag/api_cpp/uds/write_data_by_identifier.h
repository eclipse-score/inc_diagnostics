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

#ifndef SCORE_MW_DIAG_API_CPP_UDS_WRITE_DATA_BY_IDENTIFIER_H
#define SCORE_MW_DIAG_API_CPP_UDS_WRITE_DATA_BY_IDENTIFIER_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"

namespace score::mw::diag::uds
{

/// UDS WriteDataByIdentifier service (See ISO 14229-1:2020, Service 0x2E).
class WriteDataByIdentifier
{
  public:
    /// Write raw bytes for the data identifier.
    /// Returns Ok on success, Err(Error) on failure.
    [[nodiscard]] virtual ResultBlank Write(ByteView input) = 0;

    WriteDataByIdentifier() = default;
    WriteDataByIdentifier(const WriteDataByIdentifier&) = delete;
    WriteDataByIdentifier(WriteDataByIdentifier&&) noexcept = delete;
    WriteDataByIdentifier& operator=(const WriteDataByIdentifier&) & = delete;
    WriteDataByIdentifier& operator=(WriteDataByIdentifier&&) & noexcept = delete;
    virtual ~WriteDataByIdentifier() noexcept;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_API_CPP_UDS_WRITE_DATA_BY_IDENTIFIER_H
