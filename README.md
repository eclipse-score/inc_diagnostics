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

This repository uses a **hybrid toolchain strategy** for builds:

### Target Platform (Production Code)

| Component | Toolchain | Version |
|-----------|-----------|---------|
| **C/C++** | GCC (hermetic) | 12.2.0 |
| **Rust** | Ferrocene | 1.2.0 |

The target platform uses **strict C11/C++17** standards. The Ferrocene Rust toolchain is the Eclipse S-CORE qualified toolchain for safety-critical systems.

### Host Platform (Build Tools)

| Component | Toolchain | Version |
|-----------|-----------|---------|
| **C/C++** | System GCC (auto-detected) | Host system default |
| **Rust** | Ferrocene | 1.2.0 |

The host platform uses Bazel's auto-detected system toolchain for C/C++ builds. This is necessary because `rules_foreign_cc BootstrapPkgConfig` builds glib from source, which uses GNU `asm` extensions that are incompatible with the hermetic toolchain's strict `-std=c11`.

### Why This Matters

1. **Target hermeticity**: Production code uses fully hermetic, qualified toolchains.
2. **Safety compliance**: Ferrocene provides qualified Rust tooling for ASIL/QM development.
3. **Build compatibility**: System toolchain for host allows building native dependencies (pkg-config, glib) required by Rust crates like `aws-lc-sys`.

### Configuration

Toolchains are configured in:
- **MODULE.bazel**: GCC and Ferrocene toolchain registration
- **.bazelrc**: Platform-specific flags and toolchain selection

### Patches

This repository maintains local patches for upstream dependencies:

| Patch | Purpose | Status |
|-------|---------|--------|
| `classic-diagnostic-adapter-linkme.patch` | Preserves linkme sections in CDA binary | Required — upstream does not include `--no-gc-sections` |
| `opensovd-core-expose-tokio.patch` | Exposes tokio and other crate aliases | Required — upstream only exposes gateway target |
| `opensovd-core-expose-client.patch` | Adds BUILD for opensovd-client | Required — upstream has no Bazel support for client crate |
