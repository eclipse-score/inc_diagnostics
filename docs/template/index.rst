..
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

Template Information
====================

This information describes the structure, usage and configuration of the Bazel-based C++/Rust module template according to the `SCORE module folder structure <https://eclipse-score.github.io/score/main/contribute/general/folder.html#module-folder-structure>`_ and the `SCORE building blocks concept <https://eclipse-score.github.io/process_description/main/general_concepts/score_building_blocks_concept.html>`_.


Overview
--------

This repository provides a standardized setup for projects using **C++** or **Rust** and **Bazel** as a build system.
It integrates best practices for build, test, CI/CD and documentation.
It also provides an example for the documentation of an module with all necessary artifacts for safety and security management, verification and release management.
It also provides an example of modeling architecture in Sphinx Needs in :ref:`definition_architectural_design`.
It also provides the component architecture template snippets in :ref:`component_architecture`.
It also provides an example of documenting detailed design in :ref:`detailed_design_example`.

Module Folder Structure (Multiple Features Variant)
---------------------------------------------------

The module template includes the following top-level structure:

.. parsed-literal::

    <module_name>/                      # Root folder of the module, subfolder only if more than one module exists in the repository
    ├── .github/                        # GitHub configuration
    │   └── workflows/                  # CI/CD pipelines
    ├── docs/                           # Global documentation of the module
    │   ├── features/                   # Feature documentation and architecture
    │   │   └── <feature_name>/         # Folder for each feature parts which should be in module documentation
    │   │       ├── architecture/       # Feature architecture [:need:`wp__feature_arch`]
    |   |       |                       #   and of architecture review [:need:`wp__sw_arch_verification`]
    │   │       ├── safety_analysis/    # Feature safety analysis artifacts
    |   |       |                       #   ([:need:`wp__feature_fmea`], [:need:`wp__feature_dfa`], [:need:`wp__requirements_feat_aou`])
    │   │       ├── safety_planning/    # Feature safety planning artifacts
    │   │       ├── security_analysis/  # Feature security analysis artifacts
    |   |       |                       #   ([:need:`wp__feature_security_analysis`], [:need:`wp__requirements_feat_aou`])
    │   │       └── security_planning/  # Feature security planning artifacts
    │   ├── module/                     # Module documentation
    │   |   ├── manuals/                # User manual, integration manual, table of assumptions of use
    │   │   |   |                       #   safety manual [:need:`wp__module_safety_manual`],
    │   │   |   |                       #   needs table of [:need:`wp__requirements_feat_aou`]
    │   │   |   |                       #   security manual [:need:`wp__module_security_manual`]
    │   │   |   │                       # Additional optional user centric documentation (e.g. configuration guide,
    │   │   |   │                       #   examples user guide, APIs & usage, performance analysis)
    │   │   |   ├── config/             # Configuration guide (optional)
    │   │   |   ├── examples/           # Examples user guide (optional)
    │   │   |   ├── api_description/    # API detail description (optional)
    │   │   |   └── performance/        # Performance analysis (optional)
    │   |   ├── release/                # Module release note [:need:`wp__module_sw_release_note`]
    │   |   ├── safety_mgt/             # Module safety plan [:need:`wp__module_safety_plan`],
    │   |   │                           #   module safety package [:need:`wp__module_safety_package`],
    │   |   │                           #   formal document and safety analysis reviews [:need:`wp__fdr_reports`]
    │   |   └── security_mgt/           # Module security plan [:need:`wp__module_security_plan`],
    │   |                               #   module security package [:need:`wp__module_security_package`],
    │   |                               #   formal document reviews [:need:`wp__fdr_reports_security`],
    │   |                               #   module SW bill of material [:need:`wp__sw_module_sbom`]
    │   └── verification_report/        # Module verification report,
    │                                   #   module verifications [:need:`wp__verification_module_ver_report`],
    ├── examples/                       # Usage examples for the module / features
    ├── score/                          # Components of the module
    │   ├── tests/                      # Module-level tests (e.g. feature integration tests) [:need:`wp__verification_feat_int_test`]
    │   └── <component_name>/           # Component folder for each component of the module
    │       ├── docs/                   # Documentation of the component
    │       │   ├── architecture/       # Component architecture [:need:`wp__component_arch`]
    │       │   │                       #   (only if lower level components exist)
    |       |   |                       #   architecture review [:need:`wp__sw_arch_verification`],
    │       │   ├── detailed_design/    # Detailed design [:need:`wp__sw_implementation`]
    │       │   │                       #   code inspection [:need:`wp__sw_implementation_inspection`]
    │       │   ├── requirements/       # Component requirements [:need:`wp__requirements_comp`], [:need:`wp__requirements_inspect`]
    │       │   ├── safety_analysis/    # Safety analysis [:need:`wp__sw_component_fmea`], [:need:`wp__sw_component_dfa`], [:need:`wp__requirements_comp_aou`]
    │       │   │                       # Component classification [:need:`wp__sw_component_class`] for pre-existing software
    |       |   |                       #   (only if component architecture exists)
    │       │   ├── security_analysis/  # Security analysis [:need:`wp__sw_component_security_analysis`]
    |       |   |                       #   (only if component architecture exists)
    │       │   │                       #   component AoU requirements [:need:`wp__requirements_comp_aou`]
    │       │   └── manuals/            # User documentation (of a single component, e.g., user manual of a library component, optional)
    │       ├── src/                    # Source files, include files, unit tests [:need:`wp__verification_sw_unit_test`],
    │       │   └── <lower_level_comp>/ # Lower level component (follows <component_name> structure)
    │       └── tests/                  # Component-level tests (e.g., integration or complex unit tests) [:need:`wp__verification_comp_int_test`]
    ├── MODULE.bazel                    # Bazel module definition
    ├── BUILD                           # Root build rules
    ├── project_config.bzl              # Project metadata used by Bazel macros
    └── README.md                       # Entry point of the repository

.. note::

    The feature-specific subfolder under ``docs/features/<feature_name>/`` is only necessary
    if more than one feature is implemented in the module.

Module Folder Structure (Single-Feature Variant)
------------------------------------------------

The following variant keeps the same structure but removes the additional
feature-name nesting under ``docs/features/``. In this case, the ``features/``
subfolder is optional and omitted. This variant is intended for modules that only implement a single feature, to avoid unnecessary nesting.
For identification of the single feature, the repository name or module name should be replicate the feature name.

.. parsed-literal::

    <module_name>/                      # Root folder of the module, subfolder only if more than one module exists in the repository
    ├── .github/
    │   └── workflows/                  # CI/CD pipelines
    ├── docs/                           # Global documentation of the module and the single feature
    │   ├── features/                   # Feature folder parts for each feature which should be in module documentation
    │   │   ├── architecture/           # Feature architecture [:need:`wp__feature_arch`]
    |   |   |                           #   and of architecture review [:need:`wp__sw_arch_verification`]
    │   │   ├── safety_analysis/        # Feature safety analysis artifacts
    |   |   |                           #   ([:need:`wp__feature_fmea`], [:need:`wp__feature_dfa`], [:need:`wp__requirements_feat_aou`])
    │   │   ├── safety_planning/        # Feature safety planning artifacts
    │   │   ├── security_analysis/      # Feature security analysis artifacts
    |   |   |                           #  ([:need:`wp__feature_security_analysis`], [:need:`wp__requirements_feat_aou`])
    │   │   └── security_planning/      # Feature security planning artifacts
    │   ├── module/                     # Module documentation
    │   |   ├── manuals/                # User manual, integration manual, table of assumptions of use
    │   │   |   |                       #   safety manual [:need:`wp__module_safety_manual`],
    │   │   |   |                       #   needs table of [:need:`wp__requirements_feat_aou`]
    │   │   |   |                       #   security manual [:need:`wp__module_security_manual`]
    │   │   |   │                       # Additional optional user centric documentation (e.g. configuration guide,
    │   │   |   │                       #   examples user guide, APIs & usage, performance analysis)
    │   │   |   ├── config/             # Configuration guide (optional)
    │   │   |   ├── examples/           # Examples user guide (optional)
    │   │   |   ├── api_description/    # API detail description (optional)
    │   │   |   └── performance/        # Performance analysis (optional)
    │   |   ├── release/                # Module release note [:need:`wp__module_sw_release_note`]
    │   |   ├── safety_mgt/             # Module safety plan [:need:`wp__module_safety_plan`],
    │   |   │                           #   module safety package [:need:`wp__module_safety_package`],
    │   |   │                           #   formal document and safety analysis reviews [:need:`wp__fdr_reports`]
    │   |   └── security_mgt/           # Module security plan [:need:`wp__module_security_plan`],
    │   |                               #   module security package [:need:`wp__module_security_package`],
    │   |                               #   formal document reviews [:need:`wp__fdr_reports_security`],
    │   |                               #   module SW bill of material [:need:`wp__sw_module_sbom`]
    │   └── verification_report/        # Module verification report,
    │                                   #   module verifications [:need:`wp__verification_module_ver_report`],
    ├── examples/                       # Usage examples for the module / features
    ├── score/                          # Components of the module
    │   ├── tests/                      # Module-level tests (e.g. feature integration tests) [:need:`wp__verification_feat_int_test`]
    │   └── <component_name>/           # Component folder for each component of the module
    │       ├── docs/                   # Documentation of the component
    │       │   ├── architecture/       # Component architecture [:need:`wp__component_arch`]
    │       │   │                       #   (only if lower level components exist)
    |       |   |                       #   architecture review [:need:`wp__sw_arch_verification`],
    │       │   ├── detailed_design/    # Detailed design [:need:`wp__sw_implementation`]
    │       │   │                       #   code inspection [:need:`wp__sw_implementation_inspection`]
    │       │   ├── requirements/       # Component requirements [:need:`wp__requirements_comp`],[:need:`wp__requirements_inspect`]
    │       │   ├── safety_analysis/    # Safety analysis [:need:`wp__sw_component_fmea`], [:need:`wp__sw_component_dfa`], [:need:`wp__requirements_comp_aou`]
    |       |   |                       # Component classification [:need:`wp__sw_component_class`] for pre-existing software
    │       │   │                       #   (only if component architecture exists)
    │       │   ├── security_analysis/  # Security analysis [:need:`wp__sw_component_security_analysis`]
    │       │   │                       #   (only if component architecture exists)
    |       |   |                       #   component AoU requirements [:need:`wp__requirements_comp_aou`]
    │       │   └── manuals/            # User documentation (of a single component, e.g., user manual of a library component, optional)
    │       ├── src/                    # Source files, include files, unit tests [:need:`wp__verification_sw_unit_test`],
    │       │   └── <lower_level_comp>/ # Lower level component (follows <component_name> structure)
    │       └── tests/                  # Component-level tests (e.g., integration or complex unit tests) [:need:`wp__verification_comp_int_test`]
    ├── MODULE.bazel                    # Bazel module definition
    ├── BUILD                           # Root build rules
    ├── project_config.bzl              # Project metadata used by Bazel macros
    └── README.md                       # Entry point of the repository

.. attention::
    This page is describing the module template and shall be deleted in the module's documentation.
