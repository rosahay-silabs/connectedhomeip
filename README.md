[![Silicon Labs](./docs/silabs/images/silabs-logo.jpg)](https://www.silabs.com)

# Silicon Labs Matter

Welcome to the Silicon Labs Matter Github repo. This is your one stop shop for
all things related to Silicon Labs and Matter development.

**To develop a Matter application with Silicon Labs please start here:**

**[Silicon Labs Matter Table of Contents](./docs/silabs/README.md)**

---

[![Builds](https://github.com/project-chip/connectedhomeip/workflows/Builds/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/build.yaml)

**Examples:**
[![Examples - EFR32](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20EFR32/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-efr32.yaml)
[![Examples - ESP32](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20ESP32/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-esp32.yaml)
[![Examples - i.MX Linux](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20i.MX%20Linux/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-linux-imx.yaml)
[![Examples - K32W with SE051](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20K32W%20with%20SE051/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-k32w.yaml)
[![Examples - Linux Standalone](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20Linux%20Standalone/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-linux-standalone.yaml)
[![Examples - nRF Connect SDK](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20nRF%20Connect%20SDK/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-nrfconnect.yaml)
[![Examples - QPG](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20QPG/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-qpg.yaml)
[![Examples - TI CC26X2X7](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20TI%20CC26X2X7/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-cc13x2x7_26x2x7.yaml)
[![Examples - TI CC32XX](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20TI%20CC32XX/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-cc32xx.yaml)
[![Build example - Infineon](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-infineon.yaml/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-infineon.yaml)
[![Build example - BouffaloLab](https://github.com/project-chip/connectedhomeip/workflows/Build%20example%20-%20BouffaloLab/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/examples-bouffalolab.yaml)

**Platforms:**
[![Android](https://github.com/project-chip/connectedhomeip/workflows/Android/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/android.yaml)

**Tests:**
[![Unit / Integration Tests](https://github.com/project-chip/connectedhomeip/workflows/Unit%20/%20Integration%20Tests/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/unit_integration_test.yaml)
[![Cirque](https://github.com/project-chip/connectedhomeip/workflows/Cirque/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/cirque.yaml)
[![QEMU](https://github.com/project-chip/connectedhomeip/workflows/QEMU/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/qemu.yaml)

**Tools:**
[![ZAP Templates](https://github.com/project-chip/connectedhomeip/workflows/ZAP/badge.svg)](https://github.com/project-chip/connectedhomeip/actions/workflows/zap_templates.yaml)

# Directory Structure

The Matter repository is structured as follows:

| File/Folder        | Content                                                                                                                                               |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| build              | Build system support content and built output directories                                                                                             |
| build_overrides    | Build system parameter customization for different platforms                                                                                          |
| config             | Project configurations                                                                                                                                |
| credentials        | Development and test credentials                                                                                                                      |
| docs               | Documentation, including guides. Visit the [Matter SDK documentation page](https://project-chip.github.io/connectedhomeip-doc/index.html) to read it. |
| examples           | Example firmware applications that demonstrate use of Matter                                                                                          |
| integrations       | 3rd Party integrations                                                                                                                                |
| scripts            | Scripts needed to work with the Matter repository                                                                                                     |
| src                | Implementation of Matter                                                                                                                              |
| third_party        | 3rd party code used by Matter                                                                                                                         |
| zzz_generated      | zap generated template code - Revolving around cluster information                                                                                    |
| BUILD.gn           | Build file for the gn build system                                                                                                                    |
| CODE_OF_CONDUCT.md | Code of conduct for Matter and contribution to it                                                                                                     |
| CONTRIBUTING.md    | Guidelines for contributing to Matter                                                                                                                 |
| LICENSE            | Matter license file                                                                                                                                   |
| REVIEWERS.md       | PR reviewers                                                                                                                                          |
| gn_build.sh        | Build script for specific projects such as Android, EFR32, etc.                                                                                       |
| README.md          | This File                                                                                                                                             |

# License

Matter is released under the [Apache 2.0 license](./LICENSE).
