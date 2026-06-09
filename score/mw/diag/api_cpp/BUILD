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
    "//platform:__subpackages__",
]

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------

cc_library(
    name = "result",
    hdrs = ["result.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
)

cc_library(
    name = "sovd_error",
    hdrs = ["sovd_error.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [":result"],
)

cc_library(
    name = "uds",
    hdrs = ["uds.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":result",
        ":sovd_error",
    ],
)

cc_library(
    name = "data_resource",
    hdrs = ["data_resource.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":result",
        ":sovd_error",
    ],
)

cc_library(
    name = "operation",
    hdrs = ["operation.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":result",
        ":sovd_error",
    ],
)

cc_library(
    name = "diag",
    hdrs = ["diag.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":data_resource",
        ":operation",
        ":result",
        ":sovd_error",
        ":uds",
    ],
)

# ---------------------------------------------------------------------------
# Mock libraries (testonly)
# ---------------------------------------------------------------------------

cc_library(
    name = "uds_mock",
    testonly = True,
    hdrs = ["uds_mock.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":uds",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "data_resource_mock",
    testonly = True,
    hdrs = ["data_resource_mock.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":data_resource",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "operation_mock",
    testonly = True,
    hdrs = ["operation_mock.h"],
    include_prefix = "score/mw/diag",
    visibility = _VISIBILITY,
    deps = [
        ":operation",
        "@googletest//:gtest",
    ],
)

# ---------------------------------------------------------------------------
# Unit tests
# ---------------------------------------------------------------------------

cc_test(
    name = "result_test",
    srcs = ["result_test.cpp"],
    tags = ["unit"],
    deps = [
        ":result",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "sovd_error_test",
    srcs = ["sovd_error_test.cpp"],
    tags = ["unit"],
    deps = [
        ":sovd_error",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "uds_test",
    srcs = ["uds_test.cpp"],
    tags = ["unit"],
    deps = [
        ":uds_mock",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "data_resource_test",
    srcs = ["data_resource_test.cpp"],
    tags = ["unit"],
    deps = [
        ":data_resource_mock",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "operation_test",
    srcs = ["operation_test.cpp"],
    tags = ["unit"],
    deps = [
        ":operation_mock",
        "@googletest//:gtest_main",
    ],
)
