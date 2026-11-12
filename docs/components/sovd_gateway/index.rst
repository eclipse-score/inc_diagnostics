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
everything else to an upstream SOVD server, such as the classic diagnostic adapter (CDA) for UDS ECUs.

Motivation and Rationale
========================

Testers need one SOVD API for the whole vehicle instead of one per network domain or protocol
(:need:`feat_req__diagnostics__sovd_gateway`). Data of S-CORE components is provided by existing C++ UDS
handlers of ``score::mw::diag``, so the gateway reads them through a C++/Rust bridge instead of
requiring a second implementation.

Specification
=============

The requirements describe only what the S-CORE integration adds on top of ISO 17978 (SOVD). Resources,
methods, payload formats and error responses of the SOVD API itself are defined by the standard and are not
repeated here (:need:`feat_req__diagnostics__sovd_std`).

Forwarding and discovery:

* :need:`comp_req__sovd_gateway__single_entry_point`
* :need:`comp_req__sovd_gateway__upstream_config`
* :need:`comp_req__sovd_gateway__forwarding`
* :need:`comp_req__sovd_gateway__merged_discovery`
* :need:`comp_req__sovd_gateway__reference_rewrite`
* :need:`comp_req__sovd_gateway__upstream_unavailable`

Data from UDS handlers of S-CORE components:

* :need:`comp_req__sovd_gateway__uds_data_bridge`
* :need:`comp_req__sovd_gateway__utf8_data`
* :need:`comp_req__sovd_gateway__nrc_errors`
* :need:`comp_req__sovd_gateway__read_timeout`
* :need:`comp_req__sovd_gateway__read_admission`
* :need:`comp_req__sovd_gateway__non_blocking_reads`

The v1.0 gateway has no authentication, TLS or service discovery and forwards to one upstream over TCP.
These are tracked as v1.0 work items in https://github.com/eclipse-score/inc_diagnostics/issues/66; until
then :need:`aou_req__sovd_gateway__trusted_network` applies.
