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

.. _fault_lib_requirements:

Requirements
############

.. document:: Fault Library Requirements
   :id: doc__fault_lib_requirements
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__requirements_comp
   :tags: requirements, fault_lib

Functional Requirements
=======================

.. comp_req:: Component Fault Catalog
   :id: comp_req__fault_lib__catalog
   :reqtype: Interface
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_lib_catalog
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_lib

   The fault library shall let a component define its faults in a fault catalog, given as JSON or in code, and
   shall refuse a catalog that defines a fault ID twice.

.. comp_req:: Fault Reporting
   :id: comp_req__fault_lib__reporting
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_lib
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_lib

   The fault library shall let a component report a fault of its catalog with its lifecycle stage, the time of
   the report and optional environment data, through a reporter bound once to the fault's descriptor.

.. comp_req:: Framework Agnostic
   :id: comp_req__fault_lib__framework_agnostic
   :reqtype: Non-Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_lib
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_lib

   The fault library shall be usable from synchronous code without an async runtime and from several threads.

.. comp_req:: Debouncing
   :id: comp_req__fault_lib__debounce
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_lib_debounce
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_lib

   The fault library shall debounce reports per fault before relaying them, with a policy configured in the
   fault catalog: a minimum count within a time window, a hold time, or an edge with a cooldown.

.. comp_req:: Enabling Conditions
   :id: comp_req__fault_lib__enabling_conditions
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_lib
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_lib

   The fault library shall let a component report the status of an enabling condition and let fault monitors
   query it.

.. comp_req:: Relay to the Fault Manager
   :id: comp_req__fault_lib__relay
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_lib, feat_req__diagnostics__fault_manager
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_lib

   The fault library shall relay reports to the diagnostic fault manager and, while it does not answer, keep
   them in a bounded queue that drops the oldest report when full and is retried with growing delays.

.. comp_req:: Reporting Errors
   :id: comp_req__fault_lib__report_errors
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__fault_lib
   :status: valid
   :version: 1
   :satisfied_by: comp__fault_lib

   The fault library shall return an error to the reporting component when a report cannot be relayed or
   queued.

Assumptions of Use (AoU)
========================

.. aou_req:: Fault IDs
   :id: aou_req__fault_lib__fault_ids
   :reqtype: Non-Functional
   :security: NO
   :safety: QM
   :status: valid
   :version: 1

   The integrator shall give the faults of a component's catalog IDs that are unique within that catalog and
   shall use only ASCII in text fault IDs and environment data.

Verification
============

The requirements are verified by the tests of Eclipse OpenSOVD fault-lib at commit ``12dac50``. fault-lib does
not tag its tests with requirement IDs yet, so they are listed here instead of being linked automatically.

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Requirement
     - Tests in fault-lib
   * - :need:`comp_req__fault_lib__catalog`
     - `duplicate_fault_id_returns_error <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/common/src/catalog.rs>`__, `catalog_builder_rejects_duplicate_text_fault_id <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_error_paths.rs>`__, `json_string_with_debounce_and_reset <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_json_catalog.rs>`__
   * - :need:`comp_req__fault_lib__reporting`
     - `publish_success <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/reporter.rs>`__, `report_fault_with_env_data_and_query <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_report_and_query.rs>`__
   * - :need:`comp_req__fault_lib__framework_agnostic`
     - `req1_reporter_requires_no_async_runtime <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/reporter.rs>`__, `req1_reporter_is_send_sync <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/reporter.rs>`__, `concurrent_process_and_query_does_not_panic <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_concurrent_access.rs>`__
   * - :need:`comp_req__fault_lib__debounce`
     - `count_with_window_reports_only_after_min_count_within_window <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/common/src/debounce.rs>`__, `holdtime_requires_continuous_duration_before_report <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/common/src/debounce.rs>`__, `edge_with_cooldown_reports_first_then_suppresses_during_cooldown <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/common/src/debounce.rs>`__, `req3_descriptor_supports_reporter_side_debounce <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/reporter.rs>`__
   * - :need:`comp_req__fault_lib__enabling_conditions`
     - `enabling_condition_status_transitions <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_debounce_aging_cycles_ec.rs>`__
   * - :need:`comp_req__fault_lib__relay`
     - `cache_for_retry_evicts_oldest_when_full <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/ipc_worker.rs>`__, `backoff_delay_grows_exponentially <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/ipc_worker.rs>`__, `handle_send_failure_caches_on_transient_error <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/ipc_worker.rs>`__
   * - :need:`comp_req__fault_lib__report_errors`
     - `req2_sink_error_propagates_to_caller <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/reporter.rs>`__, `publish_fail <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/src/fault_lib/src/reporter.rs>`__
   * - :need:`aou_req__fault_lib__fault_ids`
     - `unicode_in_text_fault_id_rejected <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_boundary_values.rs>`__, `unicode_in_env_data_rejected <https://github.com/eclipse-opensovd/fault-lib/blob/12dac502616701734f90a61edca1326ae2ac6506/tests/integration/src/test_boundary_values.rs>`__
