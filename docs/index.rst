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

Module Template Documentation
=============================

<Brief description of the module and the implemented feature(s).>

<Further documentation of the module
and the implemented feature(s) should be added in the respective sections of the documentation
(e.g., feature architecture, safety analysis, security analysis, manuals, etc.) following the provided
templates and guidelines.>

.. toctree::
   :titlesonly:
   :hidden:
   :glob:

   template/index

Feature Documentation
---------------------

The Feature documentation covers the feature-level definition of <module name>, including architecture and safety planning artifacts.

.. toctree::
   :maxdepth: 1

   features/index

Module Documentation
--------------------

The Module documentation covers the module-level view, including architecture, safety management documents, and the user manual.

.. toctree::
   :maxdepth: 1

   module/index
   verification_report/module_verification_report


Component Documentation
-----------------------

The Components documentation provides detailed documentation for each individual library component, including requirements, architecture, and design decisions:

.. toctree::
   :maxdepth: 1

   components/index

Architecture Modeling Example
-----------------------------

An example of modeling architecture in Sphinx Needs can be found in

.. toctree::
   :maxdepth: 1

   examples/index

Please note, that is not a template for architecture documentation, but an example of how to use Sphinx Needs for architecture modeling. The architecture documentation of the components and features of the module should follow the provided templates and guidelines.


.. _quick-start-building-testing:

Quick Start - Building and Testing
==================================

.. attention::
    This could also contain just a link to the repository's README

To build the entire module:

.. code-block:: bash

   bazel build //src/...

To run all tests:

.. code-block:: bash

   bazel test //...

To run only unit tests:

.. code-block:: bash

   bazel test //src/...

To run only component or feature integration tests:

.. code-block:: bash

   bazel test //tests/...


Module Build Configuration
--------------------------

The ``project_config.bzl`` file at the root of the module defines metadata used by Bazel macros.
This file controls build behavior and project-specific settings. It should follow the S-CORE definition.
See `S-CORE user guide for project_config.bzl <https://eclipse-score.github.io/score/main/users_guide/building_simple_application/first_score_module.html#project-config-bzl>`_ for details.

The configuration enables conditional build behavior:

* **Language-specific tools**: For C++ code, tools like ``clang-tidy`` are used; for Rust code, ``clippy`` is used
* **Safety level**: The ASIL level affects safety-related build settings and validation
* **Source code languages**: The build system optimizes for the configured languages


Release Notes
-------------

The Module documentation covers the module-level view, including architecture, safety management documents, and the user manual.

.. toctree::
   :maxdepth: 1

   release_notes/index
