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

Release Note v0.1.0
===================

.. document:: Module Template Release Note v0.1.0
   :id: doc__module_template_release_note_v010
   :status: valid
   :safety: ASIL_B
   :version: 1
   :security: YES
   :realizes: wp__module_sw_release_note
   :tags:

| **Module Name:** Module Template
| **Release Tag:** v0.1.0
| **Origin Release Tag:** v0.0.1...v0.0.1
| **Release Date:** 2026-09-02

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

Initial Release of the module_template, providing a standardized project structure and best
practices for C++ and Rust projects using Bazel.

Improvements
------------

- template: initial template by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/1
- Fix: README.md and  directory by @nradakovic in https://github.com/eclipse-score/module_template/pull/2
- ci: Removed docs workflows by @nicu1989 in https://github.com/eclipse-score/module_template/pull/8
- Update cr_checker version by @nicu1989 in https://github.com/eclipse-score/module_template/pull/7
- cicd: integrated reusable workflows by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/10
- template: added formatting checker by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/11
- dash: rename of module by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/13
- infra: Add starpls and basedpyright by @nicu1989 in https://github.com/eclipse-score/module_template/pull/14
- template: integrate docs-as-code by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/15
- template: added docs workflows by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/17
- template: added missing permissions to workflows by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/18
- docs: permissions by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/19
- template: bug by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/20
- Dcalavrezo target by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/21
- template: bug by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/24
- license: uplift dash checker version by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/26
- Update for gitlint, docs and rules_python by @umaucher in https://github.com/eclipse-score/module_template/pull/28
- Update LICENSE and NOTICE files by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/29
- Upgrade to docs-as-code 1.0.0 by @MaximilianSoerenPollak in https://github.com/eclipse-score/module_template/pull/32
- Update default and custom Bazel modules for this repo by @Aymen-Soussi-01 in https://github.com/eclipse-score/module_template/pull/34
- gitlint: Use gitlint from score by @nicu1989 in https://github.com/eclipse-score/module_template/pull/35
- Update bazel modules and set dev_dependency by @NEOatNHNG in https://github.com/eclipse-score/module_template/pull/38
- tests: separate UT and CIT/FIT by @PiotrKorkus in https://github.com/eclipse-score/module_template/pull/42
- locking: enforce Bzlmod lockfile consistency by @dcalavrezo-qorix in https://github.com/eclipse-score/module_template/pull/44
- Add sync workflow by @FScholPer in https://github.com/eclipse-score/module_template/pull/46
- chore(ci): pin cicd-workflows reusable workflows to commit SHA by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/47
- Add reuse to pre commit hooks by @lurtz in https://github.com/eclipse-score/module_template/pull/49
- chore(deps): bump eclipse-score/devcontainer from v1.2.0 to v1.3.0 in /.devcontainer in the all-in-one group across 1 directory by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/56
- Load user settings in bazel by @lurtz in https://github.com/eclipse-score/module_template/pull/55
- add documentation examples by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/58
- rename folders according to score definition by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/69
- Feat: Upgrade deps by @MaximilianSoerenPollak in https://github.com/eclipse-score/module_template/pull/67
- increase copyright year by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/70
- chore(deps): bump the all-in-one group across 1 directory with 7 updates by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/68
- chore(deps): bump eclipse-score/devcontainer from v1.3.0 to v1.5.0 in /.devcontainer in the all-in-one group across 1 directory by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/60
- chore(deps): bump the all-in-one group across 1 directory with 4 updates by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/66
- Hotfix/fix consumer test bump by @FScholPer in https://github.com/eclipse-score/module_template/pull/71
- update safety analysis template by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/74
- Change implementation documentation by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/78
- Work on detail design warnings by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/79
- fix: update test work product references by @pahmann in https://github.com/eclipse-score/module_template/pull/81
- increase bazel version to 8.6 by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/80
- update docu for single feature repo structure by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/83
- chore(deps): bump actions/checkout from 3 to 6 by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/51
- chore(deps): bump googletest from 1.17.0 to 1.17.0.bcr.2 by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/54
- chore(deps): bump rules_python from 1.8.3 to 2.0.2 by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/77
- chore(deps): bump aspect_rules_lint from 1.10.2 to 2.6.0 by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/76
- Update GitHub workflows for improved scheduling and cleanup by @AlexanderLanin in https://github.com/eclipse-score/module_template/pull/88
- Rename requirements links by @aschemmel-tech in https://github.com/eclipse-score/module_template/pull/93
- Add codeowners by @PandaeDo in https://github.com/eclipse-score/module_template/pull/94
- chore(github-actions): bump the github-actions group across 1 directory with 3 updates by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/90
- FMEA template bug fix and feature/component alignment by @aschemmel-tech in https://github.com/eclipse-score/module_template/pull/96
- enhance implementation checklist by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/95
- Move documentation to right folders and add user manual by @RolandJentschETAS in https://github.com/eclipse-score/module_template/pull/97
- Update dependency score_docs_as_code to v4.6.0 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/99
- Update dependency score_process to v2 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/100
- chore(github-actions): bump the github-actions group with 2 updates by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/98
- chore(docker): bump eclipse-score/devcontainer from v1.5.0 to v1.7.0 in /.devcontainer in the docker group across 1 directory by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/89
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from f57b605a284ca117bcfd9f83ea427096faaac7d1 to 17318d27366522721504b60040590dc822264a38 in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/103
- remove duplicated headers by @masc2023 in https://github.com/eclipse-score/module_template/pull/105
- Remove invalid sec analysis and correct aou template by @aschemmel-tech in https://github.com/eclipse-score/module_template/pull/101
- chore(pre-commit): bump https://github.com/eclipse-score/tooling from 31ff8eee214e4e97ef8f5cb46e443273515b63ec to d5dbc41a14d28c8be40077735492bb7808e98fb9 in the pre-commit group across 1 directory by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/104
- chore(docker): bump eclipse-score/devcontainer from v1.7.0 to v1.8.0 in /.devcontainer in the docker group across 1 directory by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/107
- Update dependency score_process to v2.0.1 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/106
- Add checkpoint to req inspection by @aschemmel-tech in https://github.com/eclipse-score/module_template/pull/108
- Bug fix: correct link in fdr checklist by @aschemmel-tech in https://github.com/eclipse-score/module_template/pull/109
- chore(pre-commit): bump https://github.com/eclipse-score/tooling from d5dbc41a14d28c8be40077735492bb7808e98fb9 to ade2a09e66423f788d207f8624358047f48ba209 in the pre-commit group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/110
- update component request template to consider new FEP proposal by @masc2023 in https://github.com/eclipse-score/module_template/pull/112
- Update dependency score_process to v2.0.2 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/114
- Update dependency score_docs_as_code to v4.6.1 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/111
- chore(pre-commit): bump https://github.com/eclipse-score/tooling from ade2a09e66423f788d207f8624358047f48ba209 to c82be4d499a5a5ec024602329e9733714657f500 in the pre-commit group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/113
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from 17318d27366522721504b60040590dc822264a38 to 4ce43f957a0e164827ad2906a2ec69fd7f420b5d in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/115
- chore(pre-commit): bump https://github.com/eclipse-score/tooling from c82be4d499a5a5ec024602329e9733714657f500 to 50ce3b964e4f7ee60b8dd36146a4b3647a4707f4 in the pre-commit group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/116
- Update dependency score_docs_as_code to v5 by @eclipse-score-bot in https://github.com/eclipse-score/module_template/pull/117
- chore(docker): bump eclipse-score/devcontainer from v1.8.0 to v1.9.0 in /.devcontainer in the docker group across 1 directory by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/118
- chore(pre-commit): bump https://github.com/eclipse-score/tooling from 50ce3b964e4f7ee60b8dd36146a4b3647a4707f4 to 9baca712bb4d23fcb9b09c62aa38ddf9c9a038ee in the pre-commit group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/121
- chore(github-actions): bump eclipse-score/cicd-workflows/.github/workflows/daily.yml from 4ce43f957a0e164827ad2906a2ec69fd7f420b5d to 3eb9c766349cf0c2225fef29f76601c6e7858688 in the github-actions group by @dependabot[bot] in https://github.com/eclipse-score/module_template/pull/120

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
