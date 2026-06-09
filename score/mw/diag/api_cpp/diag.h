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

/// @file diag.h
/// @brief Convenience umbrella header — includes the full score::mw::diag C++ API.
///
/// Users may include this single header instead of individual headers.

#ifndef SCORE_MW_DIAG_DIAG_H
#define SCORE_MW_DIAG_DIAG_H

#include "score/mw/diag/result.h"       // ByteVector, ByteView, Result<T>, Error, NRC, payloads
#include "score/mw/diag/sovd_error.h"   // sovd::ErrorCode, GenericError, DataError
#include "score/mw/diag/uds.h"          // ReadDataByIdentifier, WriteDataByIdentifier, RoutineControl
#include "score/mw/diag/data_resource.h"// sovd::DataCategory, DataResourceMetadata, DataResource
#include "score/mw/diag/operation.h"    // Operation, ExecutionHandle, OperationMetadata, ...

#endif  // SCORE_MW_DIAG_DIAG_H
