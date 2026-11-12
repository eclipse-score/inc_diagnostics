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
from collections.abc import Callable
from pathlib import Path
from typing import cast

NEEDS_SETTINGS = ("needs_types", "needs_id_regex", "needs_id_required", "needs_fields")
REQUIREMENTS_DIR = "02_requirements"
# CDA requirements are component requirements of the CDA in the S-CORE metamodel,
# which has no `req` type.
NEED_TYPES = {"req": "comp_req"}

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


def _literal(node: ast.expr) -> object:
    """Value of a literal, also allowing string concatenation with `+`."""
    if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add):
        left, right = _literal(node.left), _literal(node.right)
        if isinstance(left, str) and isinstance(right, str):
            return left + right
        raise ValueError(f"unsupported expression at line {node.lineno}")
    if isinstance(node, ast.Dict):
        result: dict[object, object] = {}
        for key, value in zip(node.keys, node.values):
            if key is None:
                raise ValueError(f"unsupported dict unpacking at line {node.lineno}")
            result[_literal(key)] = _literal(value)
        return result
    if isinstance(node, ast.List):
        return [_literal(element) for element in node.elts]
    return cast(object, ast.literal_eval(node))


def needs_settings(conf: Path) -> dict[str, object]:
    settings: dict[str, object] = {}
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


def _objects(value: object) -> dict[str, object]:
    if not isinstance(value, dict):
        raise SystemExit("unexpected needs.json layout")
    return cast(dict[str, object], value)


def finalize(needs: dict[str, object], project_url: str) -> None:
    """Maps need types, drops links to needs that are not exported, sets the project URL."""
    for version in _objects(needs["versions"]).values():
        exported = _objects(_objects(version)["needs"])
        for need in (_objects(n) for n in exported.values()):
            need_type = str(need["type"])
            need["type"] = NEED_TYPES.get(need_type, need_type)
            links = need.get("links", [])
            if isinstance(links, list):
                need["links"] = [
                    link for link in cast(list[object], links) if link in exported
                ]
    needs["project_url"] = project_url


def build(sources: list[Path], conf: Path) -> dict[str, object]:
    build_main = cast(
        Callable[[list[str]], int],
        __import__("sphinx.cmd.build", fromlist=["build_main"]).build_main,
    )
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "src"
        (src / REQUIREMENTS_DIR).mkdir(parents=True)
        for source in sources:
            _ = shutil.copy(source, src / REQUIREMENTS_DIR / source.name)
        _ = (src / "index.rst").write_text(
            f"CDA\n###\n\n.. toctree::\n\n   {REQUIREMENTS_DIR}/index\n",
            encoding="utf-8",
        )
        settings = "".join(f"{k} = {v!r}\n" for k, v in needs_settings(conf).items())
        _ = (src / "conf.py").write_text(
            "project = 'CDA'\n"
            + "version = 'pinned'\n"
            + "extensions = ['sphinx_needs', 'sphinx.ext.todo']\n"
            + "needs_build_json = True\n"
            + settings
            + SKIPPED_DIRECTIVES,
            encoding="utf-8",
        )
        built = Path(tmp) / "out"
        if build_main(["-b", "needs", "-Q", str(src), str(built)]) != 0:
            raise SystemExit("building the CDA requirements failed")
        return _objects(
            cast(object, json.loads((built / "needs.json").read_text(encoding="utf-8")))
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    _ = parser.add_argument("--out", required=True)
    _ = parser.add_argument("--project-url", required=True)
    _ = parser.add_argument("files", nargs="+")
    args = parser.parse_args()
    out = Path(cast(str, args.out))
    project_url = cast(str, args.project_url)
    files = [Path(f) for f in cast(list[str], args.files)]

    confs = [f for f in files if f.name == "conf.py"]
    sources = [f for f in files if f.suffix == ".rst"]
    if len(confs) != 1 or not sources:
        raise SystemExit("expected one conf.py and the requirement .rst files")

    needs = build(sources, confs[0])
    finalize(needs, project_url)
    out.mkdir(parents=True, exist_ok=True)
    _ = (out / "needs.json").write_text(json.dumps(needs, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
