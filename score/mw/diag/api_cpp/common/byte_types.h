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

/// @file byte_types.h
/// @brief Byte container type aliases: ByteVector (owning) and ByteView (non-owning).

#ifndef SCORE_MW_DIAG_API_CPP_COMMON_BYTE_TYPES_H
#define SCORE_MW_DIAG_API_CPP_COMMON_BYTE_TYPES_H

#include "score/span.hpp"

#include <cstddef>
#include <vector>

namespace score::mw::diag
{

/// Owning byte buffer.
using ByteVector = std::vector<std::byte>;

/// Non-owning read-only view over a byte buffer (C++17 polyfill via score_baselibs).
/// Equivalent to std::span<const std::byte> (C++20).
using ByteView = score::cpp::span<const std::byte>;

}  // namespace score::mw::diag

#endif  // SCORE_MW_DIAG_API_CPP_COMMON_BYTE_TYPES_H
