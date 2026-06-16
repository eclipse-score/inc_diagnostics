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

load("@rules_cc//cc:defs.bzl", "cc_library")

# ---------------------------------------------------------------------------
# Common type aliases
# ---------------------------------------------------------------------------

cc_library(
    name = "byte_types",
    hdrs = ["byte_types.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "diag_result",
    hdrs = ["diag_result.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":byte_types",
        "//score/mw/diag/uds:negative_response_code",
        "@score_baselibs//score/result",
    ],
)

# ---------------------------------------------------------------------------
# Aggregation target
# ---------------------------------------------------------------------------

cc_library(
    name = "diag",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":byte_types",
        ":diag_result",
        "//score/mw/diag/uds",
    ],
)
