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


.. _classic_diagnostic_adapter_requirements:

Requirements
############

.. document:: Classic Diagnostic Adapter Requirements
   :id: doc__classic_diagnostic_adapter_requirements
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__requirements_comp
   :tags: requirements, classic_diagnostic_adapter

Functional Requirements
=======================

.. comp_req:: CDA HTTP Server Port
   :id: comp_req__cda__http_server_port
   :reqtype: Interface
   :security: YES
   :safety: QM
   :derived_from: feat_req__diagnostics__classic_adapter
   :status: valid
   :version: 1
   :satisfied_by: comp__classic_diagnostic_adapter

   The classic diagnostic adapter shall fulfil `req~sovd-api-http-server-port <https://eclipse-opensovd.github.io/classic-diagnostic-adapter/02_requirements/02_sovd.html#req~sovd-api-http-server-port>`_.

.. comp_req:: CDA Components Collection
   :id: comp_req__cda__components_collection
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__classic_adapter
   :status: valid
   :version: 1
   :satisfied_by: comp__classic_diagnostic_adapter

   The classic diagnostic adapter shall fulfil `req~sovd-api-components-entity-collection <https://eclipse-opensovd.github.io/classic-diagnostic-adapter/02_requirements/02_sovd.html#req~sovd-api-components-entity-collection>`_.

.. comp_req:: CDA Database Loading
   :id: comp_req__cda__database_loading
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__diagnostics__classic_adapter_odx
   :status: valid
   :version: 1
   :satisfied_by: comp__classic_diagnostic_adapter

   The classic diagnostic adapter shall fulfil `req~dt-database-loading <https://eclipse-opensovd.github.io/classic-diagnostic-adapter/02_requirements/03_diagnostic_tester.html#req~dt-database-loading>`_.
