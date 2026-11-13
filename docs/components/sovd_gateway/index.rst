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

SOVD Gateway
############

.. document:: SOVD Gateway
   :id: doc__sovd_gateway
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :tags: sovd_gateway
   :realizes: wp__cmpt_request

.. toctree::
   :hidden:

   requirements/index.rst

Abstract
========

The S-CORE SOVD gateway (``//score/opensovd-core:opensovd-gateway``) is the single SOVD entry point of a
system. It is built on Eclipse OpenSOVD Core: it hosts the entities of the S-CORE platform itself and forwards
everything else to an upstream SOVD server, such as the :doc:`classic diagnostic adapter <../cda/index>`
for UDS ECUs. Data of its own entities comes from the :doc:`UDS data bridge <../uds_data_bridge/index>`.

Motivation and Rationale
========================

Testers need one SOVD API for the whole vehicle instead of one per network domain or protocol
(:need:`feat_req__diagnostics__sovd_gateway`).

Component
=========

.. comp:: SOVD Gateway
   :id: comp__sovd_gateway
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: sovd_gateway
   :belongs_to: feat__diagnostics

Specification
=============

The requirements describe only what this component adds on top of ISO 17978 (SOVD). Resources, methods, payload
formats and error responses of the SOVD API itself are defined by the standard
(:need:`feat_req__diagnostics__sovd_std`) and are not repeated here.

The v1.0 gateway has no authentication, TLS or service discovery and forwards to one upstream over TCP.
These are tracked as v1.0 work items in https://github.com/eclipse-score/inc_diagnostics/issues/66; until
then :need:`aou_req__sovd_gateway__trusted_network` applies.

* :need:`comp_req__sovd_gateway__single_entry_point`
* :need:`comp_req__sovd_gateway__upstream_config`
* :need:`comp_req__sovd_gateway__forwarding`
* :need:`comp_req__sovd_gateway__merged_discovery`
* :need:`comp_req__sovd_gateway__reference_rewrite`
* :need:`comp_req__sovd_gateway__upstream_unavailable`
* :need:`comp_req__sovd_gateway__hosted_entities`
* :need:`comp_req__sovd_gateway__vehicle_lock`
* :need:`comp_req__sovd_gateway__discovery_timeout`
* :need:`comp_req__sovd_gateway__dot_segments`

OpenSOVD Core Usage
===================

The gateway uses these parts of OpenSOVD Core. Each is covered by a requirement of this or another component of
this repository, or is not used and ruled out by an assumption of use.

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - OpenSOVD Core
     - Covered by
   * - ``opensovd_server``: ``Server``, ``Topology``, ``VendorInfo``
     - :need:`comp_req__sovd_gateway__single_entry_point`, :need:`comp_req__sovd_gateway__upstream_config`,
       :need:`comp_req__sovd_gateway__hosted_entities`
   * - ``opensovd_core``: ``Component``, ``App``
     - :need:`comp_req__sovd_gateway__hosted_entities`
   * - ``opensovd_providers``: ``DataProviderBuilder``, ``Constant``, ``ReadableDataResource``, ``Value``
     - :need:`comp_req__sovd_gateway__hosted_entities`, :need:`comp_req__uds_data_bridge__read_handlers`,
       :need:`comp_req__uds_data_bridge__utf8_data`
   * - ``opensovd_core::DataError``, ``opensovd_models``: ``GenericError``, ``DataCategory``
     - :need:`comp_req__uds_data_bridge__nrc_errors`, :need:`comp_req__uds_data_bridge__read_timeout`
   * - ``opensovd_extra``: ``Federation``, ``Upstream``, ``ForwardRoute`` ``Entities``, ``Functions``,
       ``Collections``, ``CustomCollections``
     - :need:`comp_req__sovd_gateway__forwarding`, :need:`comp_req__sovd_gateway__merged_discovery`,
       :need:`comp_req__sovd_gateway__discovery_timeout`, :need:`comp_req__sovd_gateway__dot_segments`
   * - ``opensovd_extra``: ``ForwardRoute::Locks``
     - :need:`comp_req__sovd_gateway__vehicle_lock`
   * - ``opensovd_extra``: ``ServerUri``
     - :need:`comp_req__sovd_gateway__reference_rewrite`
   * - ``opensovd_extra``: ``ForwardRoute::Authorize``, ``opensovd_server``: ``Authenticator``, ``Authorizer``
     - Not used: :need:`aou_req__sovd_gateway__trusted_network`
   * - ``opensovd_client``: ``Client``
     - Used only by tests and the ``sovd-client`` example, not part of a component of this repository
