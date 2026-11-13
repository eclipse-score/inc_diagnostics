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

.. comp_req:: Hosted Entities
   :id: comp_req__sovd_gateway__hosted_entities
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_server
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall host the S-CORE platform as a component and every application registered in its
   process as an app of that component, and shall answer requests below these entities itself, without
   forwarding them.

.. comp_req:: Vehicle Lock
   :id: comp_req__sovd_gateway__vehicle_lock
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall acquire a lock of the whole vehicle from its upstream SOVD server, answer it with a
   lock ID of its own and release what it acquired when the upstream refuses, without keeping lock state.

.. comp_req:: Discovery Time Limit
   :id: comp_req__sovd_gateway__discovery_timeout
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   When its upstream SOVD server does not answer a discovery request within 5 s, the SOVD gateway shall list
   the entities it hosts; forwarded requests other than discovery shall have no time limit.

.. comp_req:: Dot Segments
   :id: comp_req__sovd_gateway__dot_segments
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__sovd_gateway
   :status: valid
   :version: 1
   :satisfied_by: comp__sovd_gateway

   The SOVD gateway shall not forward a request whose path contains a ``.`` or ``..`` segment, also
   percent-encoded.

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

Verification in OpenSOVD Core
=============================

The tests of this repository are linked to the requirements they verify. Behavior that the gateway takes over
from ``opensovd-extra``'s ``Federation`` unchanged is also verified by the tests of OpenSOVD Core at commit
``3fd3148``. These do not carry requirement IDs, so they are listed here.

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Requirement
     - Tests in OpenSOVD Core
   * - :need:`comp_req__sovd_gateway__vehicle_lock`
     - `a_single_upstream_also_gets_a_lock_of_the_server <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/vehicle_locks.rs>`__, `a_refused_lock_releases_what_was_acquired <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/vehicle_locks.rs>`__, `acquiring_again_renews_the_lock <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/vehicle_locks.rs>`__
   * - :need:`comp_req__sovd_gateway__hosted_entities`
     - `the_servers_own_entities_never_go_upstream <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/upstreams.rs>`__
   * - :need:`comp_req__sovd_gateway__discovery_timeout`
     - `a_list_stands_without_an_upstream_that_hangs <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/upstreams.rs>`__, `an_upstream_that_hangs_answers_for_an_unknown_entity <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/upstreams.rs>`__
   * - :need:`comp_req__sovd_gateway__dot_segments`
     - `a_path_with_a_dot_segment_goes_to_no_upstream <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/upstreams.rs>`__
   * - :need:`comp_req__sovd_gateway__merged_discovery`
     - `lists_hold_the_entities_of_all_upstreams_once <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/upstreams.rs>`__, `the_root_holds_the_links_each_upstream_serves <https://github.com/eclipse-opensovd/opensovd-core/blob/3fd31487c89b344985c8201ad9b27c0dd5b0262b/opensovd-extra/tests/upstreams.rs>`__
