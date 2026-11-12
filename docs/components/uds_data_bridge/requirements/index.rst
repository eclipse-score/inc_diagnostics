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


.. _uds_data_bridge_requirements:

Requirements
############

.. document:: UDS Data Bridge Requirements
   :id: doc__uds_data_bridge_requirements
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__requirements_comp
   :tags: requirements, uds_data_bridge

Functional Requirements
=======================

.. comp_req:: Data from UDS Handlers
   :id: comp_req__uds_data_bridge__read_handlers
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__uds_data_bridge

   The UDS data bridge shall provide read-only data resources whose values are read from C++
   ``ReadDataByIdentifier`` handlers of ``score::mw::diag``.

.. comp_req:: Text Data Values
   :id: comp_req__uds_data_bridge__utf8_data
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__uds_data_bridge

   For data resources declared as text, the UDS data bridge shall return the UDS payload as a JSON string and
   shall return an error instead of a value when the payload is not valid UTF-8.

.. comp_req:: Negative Response Codes
   :id: comp_req__uds_data_bridge__nrc_errors
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__uds_data_bridge

   The UDS data bridge shall answer a read whose UDS handler returns NRC 0x31 (requestOutOfRange) as not found,
   and shall include the NRC in the error of every other negative response.

.. comp_req:: Read Timeout
   :id: comp_req__uds_data_bridge__read_timeout
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__uds_data_bridge

   The UDS data bridge shall answer a read with an error when its UDS handler does not complete within the
   timeout configured for the data resource, and shall request the handler to stop.

.. comp_req:: Read Admission
   :id: comp_req__uds_data_bridge__read_admission
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__uds_data_bridge

   The UDS data bridge shall answer a read with an error, without calling the UDS handler, while eight reads of
   that handler are outstanding.

Non-Functional Requirements
===========================

.. comp_req:: Non-Blocking Reads
   :id: comp_req__uds_data_bridge__non_blocking_reads
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__uds_data_bridge

   Pending reads of UDS handlers shall not delay the handling of other requests by the SOVD gateway.

Assumptions of Use (AoU)
========================

.. aou_req:: Cooperative UDS Handlers
   :id: aou_req__uds_data_bridge__handler_contract
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :version: 1

   The user shall provide ``ReadDataByIdentifier`` handlers that return their future without blocking and
   stop their work when the stop token is requested.
