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

load("@rules_rust//rust:defs.bzl", "rust_test")

genrule(
    name = "cda_binary",
    srcs = ["@classic-diagnostic-adapter//:opensovd-cda"],
    outs = ["opensovd-cda"],
    cmd = "cp $(location @classic-diagnostic-adapter//:opensovd-cda) $@ && chmod +x $@",
    visibility = ["//visibility:public"],
)

rust_test(
    name = "integration_test",
    srcs = ["tests/integration.rs"],
    data = [
        "mdd/functional_groups.mdd",
        "mdd/functional_groups.mdd.license",
        "opensovd-cda.sample.toml",
        ":cda_binary",
    ],
    deps = [
        "@opensovd_core//:opensovd-client",
        "@opensovd_core//:tokio",
    ],
)
