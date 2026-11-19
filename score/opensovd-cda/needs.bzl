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
"""Builds a needs.json in the layout docs-as-code expects of a `needs_json` target."""

def _needs_json_impl(ctx):
    out = ctx.actions.declare_directory(ctx.label.name)
    args = ctx.actions.args()
    args.add("--out", out.path + "/_build/needs")
    args.add("--project-url", ctx.attr.project_url)
    args.add_all(ctx.files.srcs)
    ctx.actions.run(
        executable = ctx.executable._tool,
        arguments = [args],
        inputs = ctx.files.srcs,
        outputs = [out],
        mnemonic = "CdaNeedsJson",
        progress_message = "Building CDA requirements %{label}",
    )
    return [DefaultInfo(files = depset([out]), runfiles = ctx.runfiles([out]))]

cda_needs_json = rule(
    implementation = _needs_json_impl,
    attrs = {
        "project_url": attr.string(mandatory = True),
        "srcs": attr.label_list(allow_files = [".py", ".rst"], mandatory = True),
        "_tool": attr.label(default = ":cda_needs", executable = True, cfg = "exec"),
    },
)
