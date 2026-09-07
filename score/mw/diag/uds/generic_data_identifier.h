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

/// @file generic_data_identifier.h
/// @brief Combined UDS DataIdentifier supporting both ReadDataByIdentifier (Service 0x22)
///        and WriteDataByIdentifier (Service 0x2E) through a single implementation class.
///
/// Provides two levels of abstraction:
///   - `GenericDataIdentifier`       — full interface combining RDBI and WDBI with context support.
///   - `SimpleGenericDataIdentifier` — simplified adapter for non-blocking read/write DIDs with MetaData.

#ifndef SCORE_MW_DIAG_UDS_GENERIC_DATA_IDENTIFIER_H
#define SCORE_MW_DIAG_UDS_GENERIC_DATA_IDENTIFIER_H

#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/mw/diag/uds/write_data_by_identifier.h"

namespace score::mw::diag::uds
{

/// Combined UDS DataIdentifier — supports both ReadDataByIdentifier (Service 0x22)
/// and WriteDataByIdentifier (Service 0x2E) through a single implementation class.
///
/// Full interface — receives request `MetaData` and `stop_token` for both operations.
// NOLINTBEGIN(fuchsia-multiple-inheritance): GenericDataIdentifier intentionally inherits from
// both ReadDataByIdentifier and WriteDataByIdentifier. Both bases are stateless abstract interfaces,
// making multiple inheritance safe.
class GenericDataIdentifier : public ReadDataByIdentifier, public WriteDataByIdentifier
{
  public:
    ~GenericDataIdentifier() noexcept override = default;

  protected:
    constexpr GenericDataIdentifier() noexcept = default;

    constexpr GenericDataIdentifier(GenericDataIdentifier&&) noexcept = default;
    constexpr GenericDataIdentifier(const GenericDataIdentifier&) noexcept = default;
    constexpr GenericDataIdentifier& operator=(GenericDataIdentifier&&) & noexcept = default;
    constexpr GenericDataIdentifier& operator=(const GenericDataIdentifier&) & noexcept = default;
};
// NOLINTEND(fuchsia-multiple-inheritance)

/// Simplified adapter for `GenericDataIdentifier` (must be non-blocking!)
///
/// Use this when a DID must handle both read and write requests in a fast manner.
/// Implementors override `Read(const MetaData& meta_data)` and `Write(ByteView input, const MetaData& meta_data)`.
// NOLINTBEGIN(fuchsia-multiple-inheritance): SimpleGenericDataIdentifier inherits from
// GenericDataIdentifier and the simple variants to properly route interface adapters.
class SimpleGenericDataIdentifier : public SimpleReadDataByIdentifier, public SimpleWriteDataByIdentifier
{
  public:
    ~SimpleGenericDataIdentifier() noexcept override = default;

  protected:
    constexpr SimpleGenericDataIdentifier() noexcept = default;

    constexpr SimpleGenericDataIdentifier(SimpleGenericDataIdentifier&&) noexcept = default;
    constexpr SimpleGenericDataIdentifier(const SimpleGenericDataIdentifier&) noexcept = default;
    constexpr SimpleGenericDataIdentifier& operator=(SimpleGenericDataIdentifier&&) & noexcept = default;
    constexpr SimpleGenericDataIdentifier& operator=(const SimpleGenericDataIdentifier&) & noexcept = default;
};
// NOLINTEND(fuchsia-multiple-inheritance)

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_GENERIC_DATA_IDENTIFIER_H
