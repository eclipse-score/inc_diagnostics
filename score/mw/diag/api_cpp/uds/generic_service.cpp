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

/// @file generic_service.cpp
/// @brief Default handle_message implementation and destructor for GenericService.

#include "score/mw/diag/generic_service.h"

namespace score::mw::diag::uds
{

// Default implementation rejects with SubFunctionNotSupported.
Result<ByteVector> GenericService::HandleMessage(ByteView /*input*/)
{
    return Result<ByteVector>{score::unexpect, NegativeResponseCode::SubFunctionNotSupported};
}

GenericService::~GenericService() noexcept = default;

}  // namespace score::mw::diag::uds
