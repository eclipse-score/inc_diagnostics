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
load("@rules_rust//rust:defs.bzl", "rust_binary", "rust_test")

cc_library(
    name = "read_did_bridge",
    srcs = ["bridge/read_did.cpp"],
    hdrs = ["bridge/read_did.h"],
    deps = [
        "//score/mw/diag/uds",
        "//score/mw/diag/uds:negative_response_code",
    ],
)

cc_test(
    name = "read_did_bridge_test",
    srcs = ["bridge/read_did_test.cpp"],
    deps = [
        ":read_did_bridge",
        "@googletest//:gtest_main",
    ],
)

rust_binary(
    name = "opensovd-gateway",
    srcs = [
        "bridge/read_did.rs",
        "src/main.rs",
    ],
    edition = "2021",
    features = ["link_std_cpp_lib"],
    proc_macro_deps = ["@opensovd_core//:async-trait"],
    visibility = ["//visibility:public"],
    deps = [
        ":read_did_bridge",
        "@opensovd_core//:opensovd-core",
        "@opensovd_core//:opensovd-models",
        "@opensovd_core//:opensovd-providers",
        "@opensovd_core//:opensovd-server",
        "@opensovd_core//:tokio",
    ],
)

rust_test(
    name = "application_registration_test",
    crate = ":opensovd-gateway",
    deps = ["@opensovd_core//:opensovd-client"],
)
