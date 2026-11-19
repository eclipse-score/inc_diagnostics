<!--
*******************************************************************************
Copyright (c) 2026 Contributors to the Eclipse Foundation

See the NOTICE file(s) distributed with this work for additional
information regarding copyright ownership.

This program and the accompanying materials are made available under the
terms of the Apache License Version 2.0 which is available at
https://www.apache.org/licenses/LICENSE-2.0

SPDX-License-Identifier: Apache-2.0
*******************************************************************************
-->

# Agent Instructions

Guidance for AI coding agents working in this repository. Follow it unless the user explicitly asks otherwise.

## Error handling (C++ and Rust)

S-CORE treats availability as the top priority. Errors are reported, not hidden and not turned into aborts.

### Principles

- **Return errors; do not throw.** Exceptions are not used for error reporting. They normally allocate and end in `std::terminate` when uncaught.
- **Use `score::Result` for C++.** Use the Baselibs error handling (`score/result/result.h`) rather than ad-hoc return codes. Give each module its own error domain, following `score/mw/diag/uds/negative_response_code.cpp`:
  - subclass `score::result::ErrorDomain` and implement `MessageFor`,
  - keep the domain in an anonymous namespace behind a `GetXxxErrorDomain()` accessor,
  - build errors with `score::result::Error{code, domain, message}` and return them with `score::MakeUnexpected<T>(error)` (the `T` must be given explicitly for this overload).
- **Never encode errors in a return value.** No `nullptr`, `-1`, `0` or magic strings as "failed". Express the failure in the type: `score::Result<T>`, or a status enum with an out-parameter across a C ABI.
- **Handle errors internally where feasible.** Bubble an error up only when the caller needs to know about it.
- **Prefer degraded operation to abort.** Report the error so the caller can continue without the component or decide to abort itself. Aborts reduce platform availability and are a last resort.

### Catching

`catch` is allowed only at a boundary, and each boundary must convert the exception into a `score::Result` or status value and explain why in a comment. Current boundaries:

- `extern "C"` entry points: an exception cannot cross the C ABI. Catch it and return a status such as `SCORE_DIAG_FAILED`.
- Detached worker threads that run user or handler code: nothing above can catch the exception. Catch it and report the operation as failed.

Do not add `catch` inside internal logic to hide errors. A `catch (...)` that discards the exception without reporting it is not acceptable.

### C ABI pattern

Used by `score/opensovd-core/bridge/read_did.h`:

- Return a status enum (`ScoreDiagStatus`: `OK`, `INVALID_ARGUMENT`, `BUSY`, `FAILED`).
- Pass the handle back through an out-parameter, set to `NULL` before the call and left `NULL` on any non-`OK` status.
- For asynchronous calls, a non-`OK` status means the completion callback is not called. The caller reclaims any context it allocated.
- Null inputs return `INVALID_ARGUMENT` rather than dereferencing.

On the Rust side, map the status to a typed error (`ReadError::Busy`, `ReadError::Failed`) and never panic across the FFI boundary.

### Known gaps

- Allocation and thread-creation failures inside C++ wrappers are reported as `FAILED`. The status enum cannot say more. Making them specific needs nothrow allocation or pre-created workers, which is a design decision still open.
- A few values are no longer produced after the status change (`SCORE_DIAG_READ_BUSY` and `ReadStatus::Busy`). They can be removed in a follow-up.

### Before you finish a change

- Add or update a test for the error path you changed. A test should fail if the error check is removed.
- Run the CI test set: `bazel test --config=score_diag_x86_64_linux -- //examples/... //tests/... //score/...`.
- If you cannot run it, say so in your summary.

## Requirements traceability

The documentation links requirements to code and to tests. Keep the two apart:

- **Implementation links** come from `req-Id:` comments in the files listed in a `requirement_sources` filegroup, which `scan_code` in the root `BUILD` scans. Put them directly above the function that implements the requirement. Never put them in test code, because a test would then count as implementation coverage.
- **Verification** comes from the test reports. Each test names the requirements it verifies:
  - C++ tests call `RecordProperty("PartiallyVerifies", "<id>")`, with `TestType` set to `requirements-based` and `DerivationTechnique` set to `requirements-analysis`.
  - Rust tests put `// # verifies: <id>[, <id>...]` directly above the `#[test]` function, and run through `rust_junit_test` from `//tools/rust_junit:defs.bzl`. The `rust_test` binary it wraps is tagged `manual`, so only the wrapper runs.
- CI uploads the reports as the `test-reports` artifact, and the documentation build downloads it. To check locally, run the tests (so `bazel-testlogs` exists), then `bazel run //:docs`, and read `_build/metrics.json`.

Requirements that no local code implements (for example, ones the upstream CDA or `opensovd-extra` implements) get a test link and no code link. Do not add a `req-Id:` for them to a local file.

## Pre-commit hooks

The repository uses [pre-commit](https://pre-commit.com) (`.pre-commit-config.yaml`). The hook is installed in `.git/hooks/pre-commit`, so `git commit` runs it. Hooks cover YAML, TOML and JSON checks, whitespace and end-of-file fixes, merge conflicts, private keys, large files, Bazel module and lockfile checks, `actionlint`, `ruff`, `BasedPyright`, copyright headers, and `clang-format` for C and C++.

Run the hooks on the files you changed before you finish:

```sh
pre-commit run --files <changed files>
```

Run `pre-commit run --all-files` only when a change touches many files, because it is slow.

Several hooks rewrite files (for example `clang-format -i` and the whitespace fixers). When a hook reports "files were modified by this hook", the commit is aborted:

1. Review the rewritten files (`git diff`).
2. Stage them again (`git add <files>`). Do not stage them with a stale copy: the index must match the worktree.
3. Run the hooks again until they pass.

Do not bypass the hooks with `--no-verify`. If a hook fails for a reason you cannot fix, report the hook name and output in your summary.
