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

UDS Data Bridge
###############

.. document:: UDS Data Bridge
   :id: doc__uds_data_bridge
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :tags: uds_data_bridge
   :realizes: wp__cmpt_request

.. toctree::
   :hidden:

   requirements/index.rst

Abstract
========

The UDS data bridge (``//score/opensovd-core:read_did_bridge`` and ``bridge/read_did.rs``) provides SOVD data
resources of the :doc:`SOVD gateway <../sovd_gateway/index>` whose values are read from existing C++
``ReadDataByIdentifier`` handlers of ``score::mw::diag``.

Motivation and Rationale
========================

Data of S-CORE components is already provided by C++ UDS handlers. The bridge makes it available over SOVD
without a second implementation (:need:`feat_req__diagnostics__sovd_server`).

Component
=========

.. comp:: UDS Data Bridge
   :id: comp__uds_data_bridge
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: uds_data_bridge
   :belongs_to: feat__diagnostics

Specification
=============

The requirements describe only what this component adds on top of ISO 17978 (SOVD). Resources, methods, payload
formats and error responses of the SOVD API itself are defined by the standard
(:need:`feat_req__diagnostics__sovd_std`) and are not repeated here.

* :need:`comp_req__uds_data_bridge__read_handlers`
* :need:`comp_req__uds_data_bridge__utf8_data`
* :need:`comp_req__uds_data_bridge__nrc_errors`
* :need:`comp_req__uds_data_bridge__read_timeout`
* :need:`comp_req__uds_data_bridge__read_admission`
* :need:`comp_req__uds_data_bridge__non_blocking_reads`
