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
# Common type aliases
# ---------------------------------------------------------------------------

cc_library(
    name = "byte_types",
    srcs = ["byte_types.cpp"],
    hdrs = ["byte_types.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "diag_result",
    srcs = ["diag_result.cpp"],
    hdrs = ["diag_result.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "future",
    srcs = ["future.cpp"],
    hdrs = ["future.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/concurrency/future",
    ],
)

# ---------------------------------------------------------------------------
# Test aggregation
# ---------------------------------------------------------------------------

cc_test(
    name = "future_test",
    srcs = ["future_test.cpp"],
    tags = ["unit"],
    deps = [
        ":diag_result",
        ":future",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "unit_tests",
    tests = [
        ":future_test",
        "//score/mw/diag/uds:unit_tests",
    ],
    visibility = ["//score/mw:__pkg__"],
)
