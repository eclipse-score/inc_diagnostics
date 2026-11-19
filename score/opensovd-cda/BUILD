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
load("@rules_python//python:py_binary.bzl", "py_binary")
load("@rules_rust//rust:defs.bzl", "rust_test")
load("//tools/rust_junit:defs.bzl", "rust_junit_test")
load(":needs.bzl", "cda_needs_json")

alias(
    name = "opensovd-cda",
    actual = "@classic-diagnostic-adapter//:opensovd-cda",
)

# Packaging target: the consumer chooses the install location.
pkg_files(
    name = "cda_files",
    srcs = [":opensovd-cda"],
    attributes = pkg_attributes(mode = "0755"),
    strip_prefix = strip_prefix.files_only(),
    visibility = ["//visibility:public"],
)

_INTEGRATION_DATA = [
    "mdd/functional_groups.mdd",
    "mdd/functional_groups.mdd.license",
    "opensovd-cda.sample.toml",
    ":opensovd-cda",
    "//score/examples/sovd-client",
    "//score/opensovd-core:opensovd-gateway",
]

rust_test(
    name = "integration_test_binary",
    srcs = ["tests/integration.rs"],
    data = _INTEGRATION_DATA,
    tags = ["manual"],
    deps = [
        "@opensovd_core//:opensovd-client",
        "@opensovd_core//:tokio",
    ],
)

rust_junit_test(
    name = "integration_test",
    test = ":integration_test_binary",
    sources = ["tests/integration.rs"],
    data = _INTEGRATION_DATA,
)

py_binary(
    name = "cda_needs",
    srcs = ["cda_needs.py"],
    deps = [
        "@module_template_docs_hub_env//sphinx",
        "@module_template_docs_hub_env//sphinx_needs",
    ],
)

# CDA requirements at the pinned commit, for docs() in //:BUILD.
cda_needs_json(
    name = "needs_json",
    srcs = ["@classic-diagnostic-adapter//:requirements_docs"],
    project_url = "https://eclipse-opensovd.github.io/classic-diagnostic-adapter",
    visibility = ["//:__pkg__"],
)
