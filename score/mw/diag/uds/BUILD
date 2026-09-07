# *******************************************************************************
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
# *******************************************************************************

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------

# Kept as a standalone target: diag_result depends on this, so merging
# it into :uds would create a circular dependency (uds → diag_result → uds).
cc_library(
    name = "negative_response_code",
    srcs = ["negative_response_code.cpp"],
    hdrs = ["negative_response_code.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "meta_data",
    srcs = ["meta_data.cpp"],
    hdrs = ["meta_data.h"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "generic_service",
    srcs = ["generic_service.cpp"],
    hdrs = ["generic_service.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":meta_data",
        "//score/mw/diag:byte_types",
        "//score/mw/diag:diag_result",
        "//score/mw/diag:future",
    ],
)

cc_library(
    name = "read_data_by_identifier",
    srcs = ["read_data_by_identifier.cpp"],
    hdrs = ["read_data_by_identifier.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":meta_data",
        "//score/mw/diag:byte_types",
        "//score/mw/diag:diag_result",
        "//score/mw/diag:future",
    ],
)

cc_library(
    name = "write_data_by_identifier",
    srcs = ["write_data_by_identifier.cpp"],
    hdrs = ["write_data_by_identifier.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":meta_data",
        "//score/mw/diag:byte_types",
        "//score/mw/diag:diag_result",
        "//score/mw/diag:future",
    ],
)

cc_library(
    name = "generic_data_identifier",
    srcs = ["generic_data_identifier.cpp"],
    hdrs = ["generic_data_identifier.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":meta_data",
        ":read_data_by_identifier",
        ":write_data_by_identifier",
    ],
)

cc_library(
    name = "routine_control",
    srcs = ["routine_control.cpp"],
    hdrs = ["routine_control.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":meta_data",
        "//score/mw/diag:byte_types",
        "//score/mw/diag:diag_result",
        "//score/mw/diag:future",
    ],
)

# Umbrella: depends on all individual targets above. Use this when you need
# the full UDS interface set, or as a migration path from the old monolithic target.
cc_library(
    name = "uds",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":generic_data_identifier",
        ":generic_service",
        ":read_data_by_identifier",
        ":routine_control",
        ":write_data_by_identifier",
    ],
)

# ---------------------------------------------------------------------------
# Mock libraries (testonly)
# ---------------------------------------------------------------------------

cc_library(
    name = "mocks",
    testonly = True,
    srcs = ["mocks.cpp"],
    hdrs = [
        "generic_data_identifier_mock.h",
        "generic_service_mock.h",
        "read_data_by_identifier_mock.h",
        "routine_control_mock.h",
        "write_data_by_identifier_mock.h",
    ],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":generic_data_identifier",
        ":generic_service",
        ":read_data_by_identifier",
        ":routine_control",
        ":write_data_by_identifier",
        "@googletest//:gtest",
    ],
)

# ---------------------------------------------------------------------------
# Unit tests
# ---------------------------------------------------------------------------

cc_test(
    name = "unit_test",
    srcs = [
        "generic_data_identifier_test.cpp",
        "generic_service_test.cpp",
        "meta_data_test.cpp",
        "negative_response_code_test.cpp",
        "read_data_by_identifier_test.cpp",
        "routine_control_test.cpp",
        "write_data_by_identifier_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":generic_data_identifier",
        ":generic_service",
        ":meta_data",
        ":negative_response_code",
        ":read_data_by_identifier",
        ":routine_control",
        ":write_data_by_identifier",
        "//score/mw/diag:byte_types",
        "//score/mw/diag:diag_result",
        "//score/mw/diag:future",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "unit_tests",
    tests = [
        ":unit_test",
    ],
    visibility = ["//score/mw/diag:__pkg__"],
)
