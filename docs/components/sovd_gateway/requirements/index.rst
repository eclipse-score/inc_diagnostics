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
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall serve the entities it hosts and the entities of its upstream SOVD server under one
   SOVD API base URL.

.. comp_req:: Upstream Configuration
   :id: comp_req__sovd_gateway__upstream_config
   :reqtype: Interface
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall take its listen address from ``SCORE_GATEWAY_ADDRESS`` and the base URL of its
   upstream SOVD server from ``SCORE_UPSTREAM_URL``, and shall forward nothing when no upstream is configured.

.. comp_req:: Forwarding to the Upstream
   :id: comp_req__sovd_gateway__forwarding
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway, feat_req__diagnostics__classic_adapter
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall forward requests for entities it does not host, and for locks, functions and
   collections, to its upstream SOVD server, and shall return the upstream's status, headers and body to the client.

.. comp_req:: Combined Discovery
   :id: comp_req__sovd_gateway__merged_discovery
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall list the entities of its upstream SOVD server together with the entities it hosts.

.. comp_req:: No Upstream References
   :id: comp_req__sovd_gateway__reference_rewrite
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall replace references to the API of its upstream SOVD server in forwarded responses,
   in the ``Location`` header and in JSON bodies, with references to its own API.

.. comp_req:: Unavailable Upstream
   :id: comp_req__sovd_gateway__upstream_unavailable
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   While its upstream SOVD server is unreachable, the SOVD gateway shall keep serving the entities it hosts
   and shall answer forwarded requests with HTTP 502 and an error that contains no upstream address.

Assumptions of Use (AoU)
========================

.. aou_req:: Trusted Network
   :id: aou_req__sovd_gateway__trusted_network
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1

   The integrator shall make the SOVD gateway and its upstream SOVD server reachable only from trusted
   clients, as the gateway does not authenticate clients and connects to the upstream without TLS.
