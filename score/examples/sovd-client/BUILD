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

load("@rules_rust//rust:defs.bzl", "rust_binary", "rust_test")

rust_binary(
    name = "sovd-client",
    srcs = ["src/main.rs"],
    edition = "2021",
    deps = [
        "@opensovd_core//:opensovd-client",
        "@score_crate_index//:tokio",
    ],
    visibility = ["//visibility:public"],
)

rust_test(
    name = "integration_test",
    srcs = ["tests/integration.rs"],
    edition = "2021",
    deps = [
        "@opensovd_core//:opensovd-core",
        "@opensovd_core//:opensovd-client",
        "@opensovd_core//:opensovd-models",
        "@opensovd_core//:opensovd-providers",
        "@opensovd_core//:opensovd-server",
        "@opensovd_core//:tokio",
    ],
)
