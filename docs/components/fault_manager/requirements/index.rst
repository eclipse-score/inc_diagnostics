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

.. _fault_manager_requirements:

Requirements
############

.. document:: Diagnostic Fault Manager Requirements
   :id: doc__fault_manager_requirements
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__requirements_comp
   :tags: requirements, fault_manager

Functional Requirements
=======================

.. comp_req:: Catalog Aggregation
   :id: comp_req__fault_manager__catalog_aggregation
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_catalog_agg
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_manager

   The diagnostic fault manager shall be configured with the fault catalogs of all fault libraries and shall
   keep the faults of different catalogs apart, also when they use the same fault ID.

.. comp_req:: Fault Status
   :id: comp_req__fault_manager__status
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_manager, feat_req__diagnostics__db
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_manager

   The diagnostic fault manager shall keep the status of every fault of its catalogs, derived from the
   reports of the fault libraries, including faults that were never reported.

.. comp_req:: Environment Data
   :id: comp_req__fault_manager__env_data
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_manager, feat_req__diagnostics__db
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_manager

   The diagnostic fault manager shall store the environment data reported with a fault together with its
   status.

.. comp_req:: Aging
   :id: comp_req__fault_manager__aging
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_manager
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_manager

   The diagnostic fault manager shall reset a fault that no longer occurs by the policy of its catalog entry:
   after a number of power cycles or named operation cycles, after a stable time, or only on request of a
   tester.

.. comp_req:: Persistence
   :id: comp_req__fault_manager__persistence
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__db_persistence
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_manager

   The diagnostic fault manager shall store the status of all faults persistently, so that it survives a
   restart, and shall remove the stored status of faults that are deleted.

.. comp_req:: Fault Queries
   :id: comp_req__fault_manager__queries
   :reqtype: Interface
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_manager, feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_manager

   The diagnostic fault manager shall answer the SOVD server's queries for the faults of a catalog and their
   environment data, and shall delete faults on its request, scoped to that catalog.

Assumptions of Use (AoU)
========================

.. aou_req:: Single Fault Manager
   :id: aou_req__fault_manager__single_instance
   :reqtype: Non-Functional
   :security: NO
   :safety: QM
   :status: valid
   :version: 1

   The integrator shall run one diagnostic fault manager per system and give it the fault catalogs of all
   fault libraries that report to it.

Verification
============

The requirements are verified by the tests of Eclipse OpenSOVD fault-lib at commit ``12dac50``. fault-lib does
not tag its tests with requirement IDs yet, so they are listed here instead of being linked automatically.

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Requirement
     - Tests in fault-lib
   * - :need:`comp_req__fault_manager__catalog_aggregation`
     - `multi_catalog_fault_isolation <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_multi_catalog.rs>`__, `same_fault_id_different_catalogs_independent <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_multi_catalog.rs>`__
   * - :need:`comp_req__fault_manager__status`
     - `from_state_converts_all_flags <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/dfm_lib/src/sovd_fault_manager.rs>`__, `report_single_fault_and_query_via_sovd <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_report_and_query.rs>`__, `unreported_faults_have_default_status <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_report_and_query.rs>`__
   * - :need:`comp_req__fault_manager__env_data`
     - `report_fault_with_env_data_and_query <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_report_and_query.rs>`__
   * - :need:`comp_req__fault_manager__aging`
     - `power_cycles_trigger_met <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/dfm_lib/src/aging_manager.rs>`__, `operation_cycles_named_counter <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/dfm_lib/src/aging_manager.rs>`__, `stable_for_trigger <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/dfm_lib/src/aging_manager.rs>`__, `tool_only_never_auto_resets <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/dfm_lib/src/aging_manager.rs>`__
   * - :need:`comp_req__fault_manager__persistence`
     - `fault_state_survives_dfm_restart <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_persistent_storage.rs>`__, `delete_fault_clears_persistent_state <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_persistent_storage.rs>`__, `to_and_from_kvs <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/dfm_lib/src/sovd_fault_storage.rs>`__
   * - :need:`comp_req__fault_manager__queries`
     - `direct_query_delete_single_fault <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_ipc_query.rs>`__, `query_unknown_path_returns_error <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_report_and_query.rs>`__, `delete_faults_scoped_to_catalog <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_multi_catalog.rs>`__
   * - :need:`aou_req__fault_manager__single_instance`
     - `multi_catalog_fault_isolation <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_multi_catalog.rs>`__
