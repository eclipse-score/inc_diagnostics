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

Release Note v0.2.0
===================

.. document:: Module Template Release Note v0.2.0
   :id: doc__module_template_release_note_v020
   :status: valid
   :safety: ASIL_B
   :version: 2
   :security: YES
   :realizes: wp__module_sw_release_note
   :tags:

| **Module Name:** Module Template
| **Release Tag:** v0.2 .0
| **Origin Release Tag:** v0.1.0...v0.2.0
| **Release Date:** 2026-11-02

Overview
^^^^^^^^

The module module_template provides a template for setting up C++ and Rust projects using Bazel.

It provides a standardized project structure, ensuring best practices for:

- Build configuration with Bazel.
- Testing (Component and Feature Integration Tests).
- Documentation setup.
- CI/CD workflows.
- Development environment configuration.

Disclaimer
----------

This release note does not "release for production", as it does not come with a safety
argumentation and a performed safety assessment.
The work products compiled in the safety package are created with care according to a
process satisfying standards, but the project, being a non-profit and open source
organization, can not take over any liability for its content.

Changes to the Module
^^^^^^^^^^^^^^^^^^^^^

New Features
------------

- Added centrally managed S-CORE toolchain configurations, compiler warning policies, and QNX/IFS support, including QNX aarch64 (PRs #160, #180, #199).
- Added an example component with tests and examples of automatically generated module verification reports (PRs #166, #169, #178).

Improvements
------------

- Update dependency score_docs_as_code to v8 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/150
- chore(bazel): align SCORE platform dependencies by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/152
- chore(ci): align SCORE documentation workflows by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/154
- Update dependency score_bazel_cpp_toolchains to v1 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/153
- fix: make needextend local by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/156
- chore: update deps by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/157
- Update dependency score_bazel_cpp_toolchains to v1.0.2 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/159
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from 7b1b65cbb59d5f817e639a731f018db5adb3028a to 7c5c10c1f3f631fd26b8f9b00d5b147b1b9cff00 in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/161
- feat(toolchain): integrate QNX IFS toolchain centrally by @srinivasugithub in https://github.com/eclipse-score/module_template/pull/160
- Update dependency score_bazel_platforms to v1.1.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/164
- Update dependency score_docs_as_code to v8.1.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/163
- Update dependency score_bazel_cpp_toolchains to v1.0.3 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/165
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from 7c5c10c1f3f631fd26b8f9b00d5b147b1b9cff00 to 37eaadadfefc263d94a858d25665735659e916fa in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/168
- Update dependency score_docs_as_code to v8.1.1 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/167
- Update dependency score_rules_imagefs to v0.1.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/170
- Added code and test for example component by @naveena456 in https://github.com/eclipse-score/module_template/pull/169
- chore(github-actions): bump AndreasAugustin/actions-template-sync from 2.5.3 to 2.6.0 in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/171
- Update dependency score_docs_as_code to v8.1.2 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/172
- Refactor tests and improve component structure by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/173
- add example for auto generated module verification report by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/166
- chore(bazel): align development-only dependencies by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/175
- improvement of module verification report by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/178
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from 37eaadadfefc263d94a858d25665735659e916fa to 8c48c65fc9e8927a06214192e6ebce520853087d in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/179
- Update dependency score_devcontainer to v1.11.1 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/181
- Update dependency score_docs_as_code to v8.2.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/182
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from 8c48c65fc9e8927a06214192e6ebce520853087d to 145c53eb965228c981ba0073bbf8adb2b2ab0ebf in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/183
- Update dependency score_docs_as_code to v8.3.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/184
- Update dependency score_bazel_cpp_toolchains to v1.0.4 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/186
- feat(toolchain): centralize S-CORE toolchain configurations and compiler warning policies by @srinivasugithub in https://github.com/eclipse-score/module_template/pull/180
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from 145c53eb965228c981ba0073bbf8adb2b2ab0ebf to d2083d5dac0e309643d9f495e61342dfbaf07ed5 in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/187
- chore(docs): update score_docs_as_code gitignore configuration by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/188
- fix of safety analysis templates by @janis-qorix in https://github.com/eclipse-score/module_template/pull/189
- Addition of doc mgmt plan by @janis-qorix in https://github.com/eclipse-score/module_template/pull/193
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from d2083d5dac0e309643d9f495e61342dfbaf07ed5 to b8c78474a9177cabd2becbad431ff78c63ed5e8c in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/194
- grammar and sentence corrections in the checklist by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/196
- Adapt and improve arch inspection checklist by @aschemmel-tech in https://github.com/eclipse-score/module_template/pull/197
- Add realisation link for User Manual by @PandaeDo in https://github.com/eclipse-score/module_template/pull/195
- Cleanup templates by @PandaeDo in https://github.com/eclipse-score/module_template/pull/198
- chore(devcontainer): sync standard SCORE tool launcher by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/200
- Add score-qnx-aarch64 by @vincio93 in https://github.com/eclipse-score/module_template/pull/199
- chore(devcontainer): update Bazel dependency to 1.11.2 by @lurtz in https://github.com/eclipse-score/module_template/pull/201
- Update dependency score_docs_as_code to v8.4.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/203
- chore(docker): bump eclipse-score/devcontainer from v1.11.0 to v1.11.2 in /.devcontainer in the docker group across 1 directory by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/204
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from b8c78474a9177cabd2becbad431ff78c63ed5e8c to e5295061011ca186d3397e187764c73feed4b67c in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/205
- Change to valid_inspected state by @aschemmel-tech in https://github.com/eclipse-score/module_template/pull/206
- Update dependency score_docs_as_code to v8.5.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/208
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from e5295061011ca186d3397e187764c73feed4b67c to 0bfa9822cc2df460b2172ec18f5acafa65316e68 in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/209
- Update dependency score_cpp_policies to v0.1.2 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/207
- Update dependency score_docs_as_code to v8.5.1 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/211
- Update dependency score_devcontainer to v1.11.3 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/212
- Add release notes v020 by @masc2023 in https://github.com/eclipse-score/module_template/pull/213


Bug Fixes
---------

not applicable

Other changes by Label
----------------------

not applicable

Compatibility
^^^^^^^^^^^^^

Doc-as-code, process_description

For a detailed list checkout here: https://github.com/eclipse-score/module_template/blob/main/MODULE.bazel

Performed Verification
^^^^^^^^^^^^^^^^^^^^^^

Committer reviews, User Feedback

Known Issues
------------

not applicable

Known Vulnerabilities
---------------------

None

Upgrade Instructions
^^^^^^^^^^^^^^^^^^^^

not applicable

Contact Information
For any questions or support, please contact the SW Process Development Community (https://github.com/orgs/eclipse-score/discussions/108) or raise an issue/discussion.
