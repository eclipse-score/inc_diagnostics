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
"""Builds the needs.json of the CDA requirements with Sphinx and sphinx-needs.

Takes the CDA's docs/conf.py and docs/02_requirements/*.rst. Only the sphinx-needs
settings are read from conf.py, without executing it, as it loads extensions that
are not needed for the requirements.
"""

import argparse
import ast
import json
import shutil
import sys
import tempfile
from pathlib import Path

from sphinx.cmd.build import build_main

NEEDS_SETTINGS = ("needs_types", "needs_id_regex", "needs_id_required", "needs_fields")
REQUIREMENTS_DIR = "02_requirements"
# CDA requirements are component requirements of the CDA in the S-CORE metamodel,
# which has no `req` type.
NEED_TYPES = {"req": "comp_req"}


def _literal(node: ast.AST):
    """Value of a literal, also allowing string concatenation with `+`."""
    if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
        left, right = _literal(node.left), _literal(node.right)
        if isinstance(left, str) and isinstance(right, str):
            return left + right
        raise ValueError(f"unsupported expression at line {node.lineno}")
    if isinstance(node, ast.Dict):
        return {_literal(k): _literal(v) for k, v in zip(node.keys, node.values)}
    if isinstance(node, ast.List):
        return [_literal(e) for e in node.elts]
    return ast.literal_eval(node)


def needs_settings(conf: Path) -> dict:
    settings = {}
    for node in ast.parse(conf.read_text(encoding="utf-8")).body:
        if (
            isinstance(node, ast.Assign)
            and len(node.targets) == 1
            and isinstance(node.targets[0], ast.Name)
            and node.targets[0].id in NEEDS_SETTINGS
        ):
            settings[node.targets[0].id] = _literal(node.value)
    if "needs_types" not in settings:
        raise SystemExit(f"{conf}: no needs_types")
    return settings


# Directives of CDA extensions that are not loaded, e.g. `uml`, render as nothing.
SKIPPED_DIRECTIVES = """
import collections
from docutils.parsers.rst import Directive, directives


class _Skipped(Directive):
    has_content = True
    optional_arguments = 1
    final_argument_whitespace = True
    option_spec = collections.defaultdict(lambda: directives.unchanged)

    def run(self):
        return []


def setup(app):
    app.add_directive("uml", _Skipped)
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--project-url", required=True)
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()

    confs = [f for f in args.files if f.name == "conf.py"]
    sources = [f for f in args.files if f.suffix == ".rst"]
    if len(confs) != 1 or not sources:
        raise SystemExit("expected one conf.py and the requirement .rst files")

    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "src"
        (src / REQUIREMENTS_DIR).mkdir(parents=True)
        for source in sources:
            shutil.copy(source, src / REQUIREMENTS_DIR / source.name)
        (src / "index.rst").write_text(
            f"CDA\n###\n\n.. toctree::\n\n   {REQUIREMENTS_DIR}/index\n",
            encoding="utf-8",
        )
        (src / "conf.py").write_text(
            "project = 'CDA'\n"
            "version = 'pinned'\n"
            "extensions = ['sphinx_needs', 'sphinx.ext.todo']\n"
            "needs_build_json = True\n"
            + "".join(f"{k} = {v!r}\n" for k, v in needs_settings(confs[0]).items())
            + SKIPPED_DIRECTIVES,
            encoding="utf-8",
        )

        out = Path(tmp) / "out"
        status = build_main(["-b", "needs", "-Q", str(src), str(out)])
        if status != 0:
            return status
        needs = json.loads((out / "needs.json").read_text(encoding="utf-8"))

    for version in needs["versions"].values():
        exported = set(version["needs"])
        for need in version["needs"].values():
            need["type"] = NEED_TYPES.get(need["type"], need["type"])
            # Links to CDA architecture and design, which are not exported.
            need["links"] = [link for link in need.get("links", []) if link in exported]
    needs["project_url"] = args.project_url
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "needs.json").write_text(json.dumps(needs, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
