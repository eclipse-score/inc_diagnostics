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

Module
======

<Module sphinx documentation template snippets for the module. The directives and their parameters
should be updated according to the module and it's components.>

.. code-block:: rst

   .. mod:: Module Name
      :id: mod__module_name
      :includes: comp__component_name_template

Module View
-----------

.. code-block:: rst

   .. mod_view_sta:: Module Name Static View
      :id: mod_view_sta__feature_name__module_name
      :includes: comp__component_name_template

      .. needarch::
         :scale: 50
         :align: center

         {{ draw_module(need(), needs) }}

Module Documents
----------------

.. toctree::
   :maxdepth: 1

   manuals/index
   release/release_note
   safety_mgt/index
   security_mgt/index
