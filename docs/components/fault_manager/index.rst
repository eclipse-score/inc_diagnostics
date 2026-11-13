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

Diagnostic Fault Manager
########################

.. document:: Diagnostic Fault Manager
   :id: doc__fault_manager
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :tags: fault_manager
   :realizes: wp__cmpt_request

.. toctree::
   :hidden:

   requirements/index.rst

Abstract
========

The diagnostic fault manager (``dfm_lib`` in `Eclipse OpenSOVD fault-lib <https://github.com/eclipse-opensovd/fault-lib>`__)
is the central part of fault handling. It receives the reports of all :doc:`fault libraries <../fault_lib/index>`,
keeps the status of every fault in the diagnostic database and answers fault queries of the SOVD server.

Motivation and Rationale
========================

The SOVD server needs one source for the status of all faults in the system
(:need:`feat_req__diagnostics__fault_manager`), configured from the fault catalogs of all fault libraries
(:need:`feat_req__diagnostics__fault_catalog_agg`) and kept across restarts
(:need:`feat_req__diagnostics__db`, :need:`feat_req__diagnostics__db_persistence`).

Component
=========

.. comp:: Diagnostic Fault Manager
   :id: comp__fault_manager
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: fault_manager
   :belongs_to: feat__diagnostics

Specification
=============

The diagnostic fault manager is not integrated into this repository yet: the SOVD gateway does not query it,
so the faults it holds are not served through SOVD. The requirements describe the fault manager as it is
implemented in fault-lib, so that the integration has a baseline. How faults are represented in SOVD is defined
by ISO 17978 and not repeated here.

* :need:`Functional`
* :need:`Functional`
* :need:`Functional`
* :need:`Functional`
* :need:`Functional`
* :need:`Interface`
