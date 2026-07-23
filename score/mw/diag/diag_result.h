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

/// @file diag_result.h
/// @brief Result<T> type alias for the diagnostic API.
///
/// The error type is NegativeResponseCode.
/// For SOVD support a new common error type can be added to score::mw::diag
/// without touching existing UDS code that uses this alias.

#ifndef SCORE_MW_DIAG_DIAG_RESULT_H
#define SCORE_MW_DIAG_DIAG_RESULT_H

#include "score/result/result.h"

namespace score::mw::diag::uds
{

/// Result type: either a value T or a UDS NegativeResponseCode.
template <typename T>
using Result = score::Result<T>;

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_DIAG_RESULT_H
