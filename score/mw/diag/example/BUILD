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

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "lib_runtime",
    srcs = ["lib_runtime.rs"],
    deps = [
        "//score/mw/diag/api",
        "@score_crate_index//:futures",
        "@score_crate_index//:indexmap",
        "@score_crate_index//:tokio",
    ],
)

rust_library(
    name = "diag_runtime",
    srcs = ["diag_runtime.rs"],
    deps = [
        ":lib_runtime",
    ],
)

rust_test(
    name = "example",
    srcs = ["example.rs"],
    features = ["link_std_cpp_lib"],
    deps = [
        ":diag_runtime",
        "//score/mw/diag/api",
        "@score_crate_index//:futures",
        "@score_crate_index//:tokio",
    ],
)
