# *****************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *****************************************************************************

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

_VISIBILITY = [
    "//score:__subpackages__",
]

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------

# Kept as a standalone target: common:diag_result depends on this, so merging
# it into :uds would create a circular dependency (uds → common:diag_result → uds).
cc_library(
    name = "negative_response_code",
    hdrs = ["negative_response_code.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
)

cc_library(
    name = "uds",
    srcs = [
        "generic_service.cpp",
    ],
    hdrs = [
        "generic_data_identifier.h",
        "generic_service.h",
        "read_data_by_identifier.h",
        "routine_control.h",
        "write_data_by_identifier.h",
    ],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":negative_response_code",
        "//score/mw/diag/api_cpp/common:byte_types",
        "//score/mw/diag/api_cpp/common:diag_result",
    ],
)

# ---------------------------------------------------------------------------
# Mock libraries (testonly)
# ---------------------------------------------------------------------------

cc_library(
    name = "uds_mocks",
    testonly = True,
    hdrs = [
        "generic_data_identifier_mock.h",
        "generic_service_mock.h",
        "read_data_by_identifier_mock.h",
        "routine_control_mock.h",
        "write_data_by_identifier_mock.h",
    ],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":uds",
        "@googletest//:gtest",
    ],
)

# ---------------------------------------------------------------------------
# Unit tests
# ---------------------------------------------------------------------------

cc_test(
    name = "uds_test",
    srcs = [
        "generic_data_identifier_test.cpp",
        "generic_service_test.cpp",
        "negative_response_code_test.cpp",
        "read_data_by_identifier_test.cpp",
        "routine_control_test.cpp",
        "write_data_by_identifier_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":uds_mocks",
        "@googletest//:gtest_main",
    ],
)
