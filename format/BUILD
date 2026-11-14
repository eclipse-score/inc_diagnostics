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

load("@score_tooling//:defs.bzl", "use_format_targets")

# Formatting targets: bazel run //format:format.fix, bazel test //format:format.check
#
# They are not in the root package on purpose. Newer score_tooling versions no longer export
# use_format_targets from defs.bzl, and a root BUILD that fails to load cannot be mounted by a
# consumer such as reference_integration (its docs bundle is //:docs_bundle).
use_format_targets()
