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

alias(
    name = "api",
    actual = ":diag_api",
    visibility = [
        "//score:__subpackages__",
    ],
)

rust_library(
    name = "diag_api",
    srcs = ["diag_api.rs"],
    deps = [
        ":common",
        ":data_resource",
        ":operation",
        ":simple_operation",
        ":uds",
        ":uds_adapters",
    ],
)

rust_library(
    name = "common",
    srcs = ["common.rs"],
    deps = [
        "@crate_index//:indexmap",
        "@crate_index//:serde_json",
    ],
)

rust_test(
    name = "common_test",
    crate = ":common",
)

rust_library(
    name = "data_resource",
    srcs = ["data_resource.rs"],
    deps = [
        ":common",
        "@crate_index//:serde_json",
    ],
)

rust_test(
    name = "data_resource_test",
    crate = ":data_resource",
)

rust_library(
    name = "operation",
    srcs = ["operation.rs"],
    deps = [
        ":common",
        "@crate_index//:futures",
        "@crate_index//:indexmap",
    ],
)

rust_test(
    name = "operation_test",
    crate = ":operation",
    deps = [
        "@crate_index//:tokio",
    ],
)

rust_library(
    name = "simple_operation",
    srcs = ["simple_operation.rs"],
    deps = [
        ":common",
        ":operation",
        "@crate_index//:futures",
        "@crate_index//:tokio",
    ],
)

rust_test(
    name = "simple_operation_test",
    crate = ":simple_operation",
    deps = [
        "@crate_index//:tokio",
    ],
)

rust_library(
    name = "uds",
    srcs = ["uds.rs"],
    deps = [
        ":common",
    ],
)

rust_library(
    name = "uds_adapters",
    srcs = ["uds_adapters.rs"],
    deps = [
        ":common",
        ":data_resource",
        ":operation",
        ":simple_operation",
        ":uds",
        "@crate_index//:tokio",
    ],
)

rust_test(
    name = "uds_adapters_test",
    crate = ":uds_adapters",
)
