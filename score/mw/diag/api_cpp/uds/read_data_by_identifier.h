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

#ifndef SCORE_MW_DIAG_API_CPP_UDS_READ_DATA_BY_IDENTIFIER_H
#define SCORE_MW_DIAG_API_CPP_UDS_READ_DATA_BY_IDENTIFIER_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"

namespace score::mw::diag::uds
{

/// UDS ReadDataByIdentifier service (See ISO 14229-1:2020, Service 0x22).
class ReadDataByIdentifier
{
  public:
    /// Read raw bytes for the data identifier.
    /// Returns Ok(ByteVector) on success, Err(Error) on failure.
    [[nodiscard]] virtual Result<ByteVector> Read() = 0;

    ReadDataByIdentifier() = default;
    ReadDataByIdentifier(const ReadDataByIdentifier&) = delete;
    ReadDataByIdentifier(ReadDataByIdentifier&&) noexcept = delete;
    ReadDataByIdentifier& operator=(const ReadDataByIdentifier&) & = delete;
    ReadDataByIdentifier& operator=(ReadDataByIdentifier&&) & noexcept = delete;
    virtual ~ReadDataByIdentifier() noexcept;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_API_CPP_UDS_READ_DATA_BY_IDENTIFIER_H
