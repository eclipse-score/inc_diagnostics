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

// NOTE: write_data_by_identifier_mock.h and generic_data_identifier_mock.h are excluded:
// their MOCK_METHOD(Result<void>, ...) forces instantiation of expected<void, E>, which
// the current futurecpp version does not support in a standalone TU.
// TODO: include them here once futurecpp fixes the expected<void, E> specialization.
#include "score/mw/diag/uds/generic_service_mock.h"
#include "score/mw/diag/uds/read_data_by_identifier_mock.h"
#include "score/mw/diag/uds/routine_control_mock.h"
