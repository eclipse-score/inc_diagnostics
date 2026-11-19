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

"""Runs a Rust test binary with a JUnit report that carries verification metadata."""

load("@rules_python//python:py_test.bzl", "py_test")

def rust_junit_test(name, test, sources, data = [], **kwargs):
    """Runs the `rust_test` binary `test` test by test and writes its JUnit report.

    Args:
      name: name of the test target, which `bazel test` runs.
      test: label of the `rust_test` binary. Tag it `manual`, so that only this target runs it.
      sources: source files that define the tests. Each test declares its requirements in a
        `// # verifies:` line above its function.
      data: runtime data of the test binary, as for its `rust_test`.
      **kwargs: forwarded to `py_test`.
    """
    py_test(
        name = name,
        srcs = ["//tools/rust_junit:rust_junit.py"],
        main = "rust_junit.py",
        args = ["--binary=$(rlocationpath %s)" % test] +
               ["--source=$(rlocationpath %s)" % source for source in sources],
        data = [test] + sources + data,
        **kwargs
    )
