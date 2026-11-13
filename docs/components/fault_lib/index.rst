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

Fault Library
#############

.. document:: Fault Library
   :id: doc__fault_lib
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :tags: fault_lib
   :realizes: wp__cmpt_request

.. toctree::
   :hidden:

   requirements/index.rst

Abstract
========

The fault library (``fault_lib`` in `Eclipse OpenSOVD fault-lib <https://github.com/eclipse-opensovd/fault-lib>`__)
is the decentral part of fault handling. Applications and platform components link it to report faults of
their own fault catalog. It debounces the reports and relays them to the
:doc:`diagnostic fault manager <../fault_manager/index>`, which serves them through SOVD.

Motivation and Rationale
========================

Applications and platform components need one interface to report faults, independent of the framework they
run in (:need:`feat_req__diagnostics__fault_lib`). Debouncing in the library keeps short glitches off the IPC
(:need:`feat_req__diagnostics__fault_lib_debounce`).

Component
=========

.. comp:: Fault Library
   :id: comp__fault_lib
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: fault_lib
   :belongs_to: feat__diagnostics

Specification
=============

The fault library is not integrated into this repository yet: no S-CORE component reports faults through it
and the SOVD gateway does not serve faults of S-CORE entities. The requirements describe the fault library as
it is implemented in fault-lib, so that the integration has a baseline.

The fault library relays faults over iceoryx2. :need:`feat_req__diagnostics__internal_com` asks for
S-CORE::COM, which is open.

* :need:`comp_req__fault_lib__catalog`
* :need:`comp_req__fault_lib__reporting`
* :need:`comp_req__fault_lib__framework_agnostic`
* :need:`comp_req__fault_lib__debounce`
* :need:`comp_req__fault_lib__enabling_conditions`
* :need:`comp_req__fault_lib__relay`
* :need:`comp_req__fault_lib__report_errors`
