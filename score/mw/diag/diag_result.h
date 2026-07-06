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
/// The error type is uds::NegativeResponseCode.
/// A richer error variant (e.g. SOVD codes) can be introduced later by
/// defining an Error struct here and updating the aliases.

#ifndef SCORE_MW_DIAG_DIAG_RESULT_H
#define SCORE_MW_DIAG_DIAG_RESULT_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/uds/negative_response_code.h"
#include <score/expected.hpp>  // score::cpp::expected — public futurecpp API

namespace score::mw::diag
{

/// Result type: either a value T or a UDS NegativeResponseCode.
template <typename T>
using Result = score::cpp::expected<T, uds::NegativeResponseCode>;

}  // namespace score::mw::diag

#endif  // SCORE_MW_DIAG_DIAG_RESULT_H
