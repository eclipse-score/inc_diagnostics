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
    name = "opensovd-cda",
    srcs = ["src/main.rs"],
    data = [
        "opensovd-cda.sample.toml",
        "mdd/functional_groups.mdd",
        "mdd/functional_groups.mdd.license",
    ],
    edition = "2021",
    deps = [
        "@classic-diagnostic-adapter//cda-main:opensovd_cda_lib",
        "@classic-diagnostic-adapter//:tokio",
    ],
    visibility = ["//visibility:public"],
)

rust_test(
    name = "integration_test",
    srcs = ["tests/integration.rs"],
    data = [
        ":opensovd-cda",
        "opensovd-cda.sample.toml",
        "mdd/functional_groups.mdd",
        "mdd/functional_groups.mdd.license",
    ],
    deps = [
        "@opensovd_core//:opensovd-client",
        "@opensovd_core//:tokio",
    ],
)
