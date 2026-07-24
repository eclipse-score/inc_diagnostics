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

/// @file meta_data.h
/// @brief MetaData — contextual information associated with an incoming UDS diagnostic request.
///
/// Reserved for future use. Currently an empty struct; fields (e.g. active session,
/// security level, tester address) will be added as requirements are defined.

#ifndef SCORE_MW_DIAG_UDS_META_DATA_H
#define SCORE_MW_DIAG_UDS_META_DATA_H

namespace score::mw::diag::uds
{

/// Contextual metadata provided by the diagnostic runtime to each service handler call.
/// Currently empty — extended in future releases.
struct MetaData
{
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_META_DATA_H
