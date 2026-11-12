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


.. _sovd_gateway_requirements:

Requirements
############

.. document:: SOVD Gateway Requirements
   :id: doc__sovd_gateway_requirements
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__requirements_comp
   :tags: requirements, sovd_gateway

Functional Requirements
=======================

.. comp_req:: Single Entry Point
   :id: comp_req__sovd_gateway__single_entry_point
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway, feat_req__diagnostics__sovd_server
   :status: valid

   The SOVD gateway shall serve the entities it hosts and the entities of its upstream SOVD server under one
   SOVD API base URL.

.. comp_req:: Upstream Configuration
   :id: comp_req__sovd_gateway__upstream_config
   :reqtype: Interface
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid

   The SOVD gateway shall take its listen address from ``SCORE_GATEWAY_ADDRESS`` and the base URL of its
   upstream SOVD server from ``SCORE_UPSTREAM_URL``, and shall forward nothing when no upstream is configured.

.. comp_req:: Forwarding to the Upstream
   :id: comp_req__sovd_gateway__forwarding
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway, feat_req__diagnostics__classic_adapter
   :status: valid

   The SOVD gateway shall forward requests for entities it does not host, and for locks, functions and
   collections, to its upstream SOVD server, and shall return the upstream's status, headers and body to the client.

.. comp_req:: Combined Discovery
   :id: comp_req__sovd_gateway__merged_discovery
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid

   The SOVD gateway shall list the entities of its upstream SOVD server together with the entities it hosts.

.. comp_req:: No Upstream References
   :id: comp_req__sovd_gateway__reference_rewrite
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid

   The SOVD gateway shall replace references to the API of its upstream SOVD server in forwarded responses,
   in the ``Location`` header and in JSON bodies, with references to its own API.

.. comp_req:: Unavailable Upstream
   :id: comp_req__sovd_gateway__upstream_unavailable
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid

   While its upstream SOVD server is unreachable, the SOVD gateway shall keep serving the entities it hosts
   and shall answer forwarded requests with HTTP 502 and an error that contains no upstream address.

.. comp_req:: Data from UDS Handlers
   :id: comp_req__sovd_gateway__uds_data_bridge
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid

   The SOVD gateway shall provide read-only data resources whose values are read from C++
   ``ReadDataByIdentifier`` handlers of ``score::mw::diag``.

.. comp_req:: Text Data Values
   :id: comp_req__sovd_gateway__utf8_data
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid

   For data resources declared as text, the SOVD gateway shall return the UDS payload as a JSON string and
   shall return an error instead of a value when the payload is not valid UTF-8.

.. comp_req:: Negative Response Codes
   :id: comp_req__sovd_gateway__nrc_errors
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid

   The SOVD gateway shall answer a read whose UDS handler returns NRC 0x31 (requestOutOfRange) as not found,
   and shall include the NRC in the error of every other negative response.

.. comp_req:: Read Timeout
   :id: comp_req__sovd_gateway__read_timeout
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid

   The SOVD gateway shall answer a read with an error when its UDS handler does not complete within the
   timeout configured for the data resource, and shall request the handler to stop.

.. comp_req:: Read Admission
   :id: comp_req__sovd_gateway__read_admission
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid

   The SOVD gateway shall answer a read with an error, without calling the UDS handler, while eight reads of
   that handler are outstanding.

Non-Functional Requirements
===========================

.. comp_req:: Non-Blocking Reads
   :id: comp_req__sovd_gateway__non_blocking_reads
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid

   Pending reads of UDS handlers shall not delay the SOVD gateway's handling of other requests.

Assumptions of Use (AoU)
========================

.. aou_req:: Trusted Network
   :id: aou_req__sovd_gateway__trusted_network
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid

   The integrator shall make the SOVD gateway and its upstream SOVD server reachable only from trusted
   clients, as the gateway does not authenticate clients and connects to the upstream without TLS.

.. aou_req:: Cooperative UDS Handlers
   :id: aou_req__sovd_gateway__cooperative_handlers
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid

   The user shall provide ``ReadDataByIdentifier`` handlers that return their future without blocking and
   stop their work when the stop token is requested.
