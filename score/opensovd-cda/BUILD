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

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files", "strip_prefix")
load("@rules_rust//rust:defs.bzl", "rust_test")

alias(
    name = "opensovd-cda",
    actual = "@classic-diagnostic-adapter//:opensovd-cda",
)

rust_test(
    name = "integration_test",
    srcs = ["tests/integration.rs"],
    data = [
        "mdd/functional_groups.mdd",
        "mdd/functional_groups.mdd.license",
        "opensovd-cda.sample.toml",
        ":opensovd-cda",
        "//score/examples/sovd-client:sovd-client",
    ],
    deps = [
        "@opensovd_core//:opensovd-client",
        "@opensovd_core//:tokio",
    ],
)

# Packaging for deployment into an image. The install location is chosen by the consumer
# (for example with pkg_filegroup(prefix = ...)).
pkg_files(
    name = "cda_files",
    srcs = [":opensovd-cda"],
    attributes = pkg_attributes(mode = "0755"),
    strip_prefix = strip_prefix.files_only(),
    visibility = ["//visibility:public"],
)
