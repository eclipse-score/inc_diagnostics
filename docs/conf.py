# *******************************************************************************
# Copyright (c) 2024 Contributors to the Eclipse Foundation
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

from collections.abc import Callable
from typing import cast

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html


# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "Module Template Project"
project_url = "https://eclipse-score.github.io/module_template/"
project_prefix = "MODULE_TEMPLATE_"
author = "S-CORE"
version = "0.1"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration


extensions = [
    "score_sphinx_bundle",
]

exclude_patterns = [
    # The following entries are not required when building the documentation via 'bazel
    # build //docs:docs', as that command runs in a sandboxed environment. However, when
    # building the documentation via 'bazel run //docs:incremental' or esbonio, these
    # entries are required to prevent the build from failing.
    "bazel-*",
    ".venv_docs",
]

templates_path = ["templates"]

# Enable numref
numfig = True

# The CDA requirements are built from the pinned CDA sources by
# //score/opensovd-cda:needs_json. Link them to the CDA's published documentation
# instead of the docs-as-code default of <project_url>/main.
CDA_DOCS_URL = "https://eclipse-opensovd.github.io/classic-diagnostic-adapter"


def _link_cda_needs_to_cda_docs(_app: object, config: object) -> None:
    sources: object = getattr(config, "needs_external_needs", [])
    if not isinstance(sources, list):
        return
    for source in cast(list[object], sources):
        if isinstance(source, dict):
            entry = cast(dict[str, object], source)
            if "/score/opensovd-cda/needs_json/" in str(entry.get("json_path", "")):
                entry["base_url"] = CDA_DOCS_URL


def setup(app: object) -> None:
    # After score_metamodel registers the external needs (priority 425).
    connect = cast(Callable[..., object], getattr(app, "connect"))
    _ = connect("config-inited", _link_cda_needs_to_cda_docs, priority=500)
