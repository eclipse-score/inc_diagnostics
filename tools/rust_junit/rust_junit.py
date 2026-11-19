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
"""Runs a Rust test binary test by test and writes a JUnit report with verification metadata.

Rust's test harness writes no JUnit XML, so this runner runs every test with ``--exact`` and
records its result. A test declares the requirements it verifies on a line above its function:

    // # verifies: comp_req__uds_data_bridge__read_timeout, comp_req__uds_data_bridge__read_handlers
    #[test]
    fn read_times_out() { ... }

The report carries the same testcase properties as the C++ tests (``RecordProperty``), so the
documentation build reads Rust and C++ verification alike. Every test is requirements-based.
"""

import argparse
import dataclasses
import os
import re
import subprocess
import sys
import time
from pathlib import Path
from xml.etree import ElementTree as ET

TEST_TYPE = "requirements-based"
DERIVATION_TECHNIQUE = "requirements-analysis"
VERIFIES = re.compile(r"^\s*//\s*#\s*verifies:\s*(.+)$")
FUNCTION = re.compile(r"^\s*(?:pub\s+)?(?:async\s+)?fn\s+(\w+)\b")
SUMMARY = re.compile(r"(\d+) passed; (\d+) failed; (\d+) ignored")
MAX_MESSAGE = 4000


@dataclasses.dataclass
class Options:
    binary: str = ""
    source: list[str] = dataclasses.field(default_factory=list)
    output: str = ""


@dataclasses.dataclass
class Result:
    name: str
    file: str
    line: int
    verifies: list[str]
    status: str
    message: str
    seconds: float


def resolve(path: str) -> Path:
    """Finds a path given relative to the runfiles root, as Bazel passes it."""
    candidate = Path(path)
    if candidate.exists():
        return candidate
    srcdir = os.environ.get("TEST_SRCDIR", "")
    workspace = os.environ.get("TEST_WORKSPACE", "")
    for base in (Path(srcdir), Path(srcdir) / workspace):
        if srcdir and (base / path).exists():
            return base / path
    raise FileNotFoundError(f"{path} is not in the runfiles")


def display_path(path: str) -> str:
    """The path as the workspace knows it, without the runfiles workspace prefix."""
    prefix = os.environ.get("TEST_WORKSPACE", "") + "/"
    return path[len(prefix) :] if path.startswith(prefix) else path


def list_tests(binary: Path) -> list[str]:
    listing = subprocess.run(
        [str(binary), "--list"], capture_output=True, text=True, check=True
    ).stdout
    return [
        line[: -len(": test")]
        for line in listing.splitlines()
        if line.endswith(": test")
    ]


def find_test(name: str, sources: list[tuple[str, Path]]) -> tuple[str, int, list[str]]:
    """Returns the file, line and verified requirements of the `#[test]` function of `name`."""
    function = name.rsplit("::", 1)[-1]
    for display, path in sources:
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            match = FUNCTION.match(line)
            if not match or match.group(1) != function:
                continue
            attributes: list[str] = []
            verifies: list[str] = []
            for above in reversed(lines[:index]):
                stripped = above.strip()
                tag = VERIFIES.match(above)
                if tag:
                    verifies = [
                        r.strip() for r in tag.group(1).split(",") if r.strip()
                    ] + verifies
                elif stripped.startswith("#["):
                    attributes.append(stripped)
                elif stripped and not stripped.startswith("//"):
                    break
            if any("test" in attribute for attribute in attributes):
                return display, index + 1, verifies
    raise LookupError(f"no #[test] function `{function}` in the given sources")


def run_test(binary: Path, name: str) -> tuple[str, str, float]:
    started = time.monotonic()
    process = subprocess.run(
        [str(binary), "--exact", name], capture_output=True, text=True
    )
    seconds = time.monotonic() - started
    output: str = process.stdout + process.stderr
    summary = SUMMARY.search(process.stdout)
    if process.returncode != 0:
        return "failed", output[-MAX_MESSAGE:], seconds
    if summary is None or summary.group(1) + summary.group(3) == "00":
        return "failed", "the test did not run:\n" + output[-MAX_MESSAGE:], seconds
    if summary.group(1) == "0":
        return "skipped", "ignored", seconds
    return "passed", "", seconds


def write_report(output: Path, results: list[Result]) -> None:
    failures = sum(r.status == "failed" for r in results)
    skipped = sum(r.status == "skipped" for r in results)
    # The documentation build reads `testsuite` elements below a `testsuites` root.
    root = ET.Element("testsuites")
    suite = ET.SubElement(
        root,
        "testsuite",
        name=os.environ.get("TEST_TARGET", "rust"),
        tests=str(len(results)),
        failures=str(failures),
        errors="0",
        skipped=str(skipped),
        time=f"{sum(r.seconds for r in results):.3f}",
    )
    for result in results:
        case = ET.SubElement(
            suite,
            "testcase",
            name=result.name,
            file=result.file,
            line=str(result.line),
            time=f"{result.seconds:.3f}",
        )
        properties = ET.SubElement(case, "properties")
        _ = ET.SubElement(properties, "property", name="TestType", value=TEST_TYPE)
        _ = ET.SubElement(
            properties,
            "property",
            name="DerivationTechnique",
            value=DERIVATION_TECHNIQUE,
        )
        if result.verifies:
            _ = ET.SubElement(
                properties,
                "property",
                name="PartiallyVerifies",
                value=",".join(result.verifies),
            )
        if result.status == "failed":
            failure = ET.SubElement(case, "failure", message="test failed")
            failure.text = result.message
        elif result.status == "skipped":
            _ = ET.SubElement(case, "skipped", message=result.message)
    ET.indent(root)
    ET.ElementTree(root).write(output, encoding="utf-8", xml_declaration=True)


def parse_arguments(argv: list[str] | None) -> Options:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    _ = parser.add_argument("--binary", required=True, help="the rust_test binary")
    _ = parser.add_argument(
        "--source",
        action="append",
        required=True,
        help="a source file that defines tests",
    )
    _ = parser.add_argument(
        "--output", help="JUnit report to write, default $XML_OUTPUT_FILE"
    )
    options = parser.parse_args(
        argv, namespace=Options(output=os.environ.get("XML_OUTPUT_FILE", ""))
    )
    if not options.output:
        parser.error("no report path: pass --output or run under `bazel test`")
    return options


def main(argv: list[str] | None = None) -> int:
    options = parse_arguments(argv)
    binary = resolve(options.binary)
    sources = [(display_path(source), resolve(source)) for source in options.source]
    results: list[Result] = []
    for name in list_tests(binary):
        file, line, verifies = find_test(name, sources)
        status, message, seconds = run_test(binary, name)
        print(f"{status:8} {name}", file=sys.stderr)
        if status == "failed":
            print(message, file=sys.stderr)
        results.append(Result(name, file, line, verifies, status, message, seconds))

    write_report(Path(options.output), results)
    return 1 if any(r.status == "failed" for r in results) else 0


if __name__ == "__main__":
    sys.exit(main())
