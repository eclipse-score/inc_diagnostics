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

Classic Diagnostic Adapter
##########################

.. document:: Classic Diagnostic Adapter
   :id: doc__classic_diagnostic_adapter
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :tags: classic_diagnostic_adapter
   :realizes: wp__cmpt_request

.. toctree::
   :hidden:

   requirements/index.rst

Abstract
========

The classic diagnostic adapter (CDA, ``//score/opensovd-cda:opensovd-cda``) of Eclipse OpenSOVD translates SOVD
requests into UDS for classic ECUs. It is the upstream SOVD server of the :doc:`SOVD gateway <../sovd_gateway/index>`.

Motivation and Rationale
========================

The CDA is specified and developed in https://github.com/eclipse-opensovd/classic-diagnostic-adapter. The
requirements below are stubs: each one names a CDA requirement that the v1.0 integration relies on and links
to it, so the CDA requirements are traced to :need:`feat_req__diagnostics__classic_adapter` without being
repeated here.

Component
=========

.. comp:: Classic Diagnostic Adapter
   :id: comp__classic_diagnostic_adapter
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: classic_diagnostic_adapter
   :belongs_to: feat__diagnostics

Specification
=============

Content and acceptance criteria are those of the linked CDA requirement in its published documentation.

* :need:`comp_req__cda__http_server_port`
* :need:`comp_req__cda__components_collection`
* :need:`comp_req__cda__database_loading`
