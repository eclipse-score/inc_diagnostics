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

# C++ & Rust Bazel Template Repository

This repository serves as a **template** for setting up **C++ and Rust projects** using **Bazel**.
It provides a **standardized project structure**, ensuring best practices for:

- **Build configuration** with Bazel.
- **Testing** (unit and integration tests).
- **Documentation** setup.
- **CI/CD workflows**.
- **Development environment** configuration.

---

## 📂 Project Structure

| File/Folder                         | Description                                       |
| ----------------------------------- | ------------------------------------------------- |
| `README.md`                         | Short description & build instructions            |
| `src/`                              | Source files for the module                       |
| `tests/`                            | Unit tests (UT) and integration tests (IT)        |
| `examples/`                         | Example files used for guidance                   |
| `docs/`                             | Documentation (Doxygen for C++ / mdBook for Rust) |
| `.github/workflows/`                | CI/CD pipelines                                   |
| `.vscode/`                          | Recommended VS Code settings                      |
| `.bazelrc`, `MODULE.bazel`, `BUILD` | Bazel configuration & settings                    |
| `project_config.bzl`                | Project-specific metadata for Bazel macros        |
| `LICENSE.md`                        | Licensing information                             |
| `CONTRIBUTION.md`                   | Contribution guidelines                           |

---

## 🚀 Getting Started

### 1️⃣ Clone the Repository

```sh
git clone https://github.com/eclipse-score/YOUR_PROJECT.git
cd YOUR_PROJECT
```

### 2️⃣ Build the Examples of module

> DISCLAIMER: Depending what module implements, it's possible that different
> configuration flags needs to be set on command line.

To build all targets of the module the following command can be used:

```sh
bazel build //src/...
```

This command will instruct Bazel to build all targets that are under Bazel
package `src/`. The ideal solution is to provide single target that builds
artifacts, for example:

```sh
bazel build //src/<module_name>:release_artifacts
```

where `:release_artifacts` is filegroup target that collects all release
artifacts of the module.

> NOTE: This is just proposal, the final decision is on module maintainer how
> the module code needs to be built.

### 3️⃣ Run Tests

```sh
bazel test //tests/...
```

---

## 🛠 Tools & Linters

The template integrates **tools and linters** from **centralized repositories** to ensure consistency across projects.

- **C++:** `clang-tidy`, `cppcheck`, `Google Test`
- **Rust:** `clippy`, `rustfmt`, `Rust Unit Tests`
- **CI/CD:** GitHub Actions for automated builds and tests

### CodeQL Findings

Assess production alerts separately from tests and mocks. The selected CodeQL
checks provide automated findings for review; this C++ scan does not analyze Rust
or establish safety across the C ABI.

For reusable analysis configuration and tooling, see the communication project's
[static-analysis setup](https://github.com/eclipse-score/communication/tree/main/quality/static_analysis).

The following remaining patterns need review rather than blanket suppression.
These are triage rationales, not approved exceptions:

| Check | Scope and Rationale |
|-------|---------------------|
| Global namespace declarations | The bridge's opaque types and callback typedefs form a C-compatible interface. C++ namespaces cannot replace those C declarations without redesigning the interface. |
| Use of `reinterpret_cast` | The bridge lends the byte buffer to a callback without copying. A compile-time assertion requires `uint8_t` to be `unsigned char`, which may inspect object representations. The callback must not retain the pointer; Rust copies the bytes before returning. |
| Smart-pointer factory functions | The bridge adopts handles using custom deleters that call the C release functions. `make_unique` does not construct these custom-deleter pointer types or adopt existing C handles. |
| Hidden inherited virtual functions | The `Simple*` UDS adapters deliberately expose synchronous overloads and privately finalize the cancellation-aware overloads. The existing tests exercise dispatch through the full base interfaces; renaming the synchronous methods would change the implementer API. |
| Implicit boolean conversions | The validator precondition and future-result check are reported as conversions from `unknown`. Treat these as analysis limitations pending manual type review, not demonstrated non-boolean conditions. |

GoogleTest/GoogleMock expansions also produce alerts at test call sites, including
generated branches, boolean conversions, linkage and mock exception paths. Keep
those findings visible in the test report, but do not modify vendored framework
code or weaken production interfaces merely to reduce their count. Intentional
contract-test temporaries and the bridge's callback-context cast also require
test-specific review. Alert counts are not counts of confirmed defects.

---

## 📖 Documentation

- A **centralized docs structure** is planned.

---

## ⚙️ `project_config.bzl`

This file defines project-specific metadata used by Bazel macros, such as `dash_license_checker`.

### 📌 Purpose

It provides structured configuration that helps determine behavior such as:

- Source language type (used to determine license check file format)
- Safety level or other compliance info (e.g. ASIL level)

### 📄 Example Content

```python
PROJECT_CONFIG = {
    "asil_level": "QM",  # or "ASIL-A", "ASIL-B", etc.
    "source_code": ["cpp", "rust"]  # Languages used in the module
}
```

### 🔧 Use Case

When used with macros like `dash_license_checker`, it allows dynamic selection of file types
 (e.g., `cargo`, `requirements`) based on the languages declared in `source_code`.

---

## 🔧 Toolchain Architecture

The OpenSOVD gateway, CDA and client are root-only development prototypes.
Run them with this repository as the root Bazel module. Their dependencies are
marked `dev_dependency`, and their targets are not public integration APIs.
Consumers of `score_diagnostics` do not inherit its root-only overrides or
patches and must not depend on these prototype targets.

All Linux targets, including the OpenSOVD gateway, CDA and example client, use
the `score_diag_x86_64_linux` configuration. Host build tools and target code
use the same hermetic toolchains:

| Component | Toolchain | Version |
|-----------|-----------|---------|
| **C/C++** | GCC (hermetic) | 12.2.0 |
| **Rust** | Ferrocene | Pinned by `score_toolchains_rust` 0.9.1 |

`rules_foreign_cc` 0.17.0 uses pkgconf for native dependency builds with the
hermetic GCC toolchain. Auto-detected system C/C++ toolchains are disabled.
Using the Ferrocene toolchain does not itself qualify the application code.

QNX dependency triples are not enabled in the shared crate index: the pinned
`rules_rust` crate generator rejects `aarch64-unknown-nto-qnx800` during dependency
generation. QNX support requires compatible dependency-generation tooling and
separate validation; restoring those entries alone breaks Linux builds too.

Strict C/C++ warnings are enabled. Warnings are errors for this repository's
sources in both target and host builds; external dependencies and vendored C
compiled by Rust build scripts retain nonfatal warnings.

### Configuration

Toolchains are registered in [MODULE.bazel](MODULE.bazel), and platform and
warning settings are defined in [.bazelrc](.bazelrc).

```sh
bazel build --config=score_diag_x86_64_linux //examples/... //tests/... //score/...
bazel test --config=score_diag_x86_64_linux //examples/... //tests/... //score/...
```

### Patches

This repository maintains local patches for upstream dependencies:

| Patch | Purpose | Status |
|-------|---------|--------|
| `classic-diagnostic-adapter-linkme.patch` | Preserves linkme sections in CDA binary | Required — upstream does not include `--no-gc-sections` |
| `classic-diagnostic-adapter-resource-ids.patch` | Includes routing IDs in CDA resource collections for typed client discovery | Required with the pinned CDA revision |
| `opensovd-core-expose-deps.patch` | Public aliases for the Core crates and shared crates (tokio, tracing, ...) | Required until eclipse-opensovd/opensovd-core#247; upstream limits them to its own workspace |
| `opensovd-core-expose-client.patch` | Adds BUILD for opensovd-client | Required until eclipse-opensovd/opensovd-core#247; upstream has no Bazel target for the client crate |
