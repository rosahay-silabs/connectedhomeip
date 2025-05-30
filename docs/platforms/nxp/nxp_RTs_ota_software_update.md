# Matter Over-The-Air Software Update with NXP RTs example applications

## Overview

This document describes OTA feature on NXP devices:

-   [RW61x](./nxp_rw61x_guide.md)
-   [RT1060_EVK-C](./nxp_rt1060_guide.md)
-   [RT1170_EVK-B](./nxp_rt1170_guide.md)

The OTA Requestor feature enables the device to be informed of, download and
apply a software update from an OTA Provider.

This section explains how to perform an OTA Software Update with NXP RTs example
applications. Throughout this guide, the all-clusters application is used as an
example.

In general, the Over-The-Air Software Update process consists of the following
steps :

-   The OTA Requestor queries an update image from the OTA Provider which
    responds according to its availability.
-   The update image is received in blocks and stored in the external flash of
    the device.
-   Once the update image is fully downloaded, the bootloader is notified and
    the device resets applying the update in test-mode.
-   If the test is successful, the update is applied permanently. Otherwise, the
    bootloader reverts back to the primary application, preventing any
    downgrade.

### Flash Memory Layout

The RTs Flash is divided into different regions as follow :

-   Bootloader : MCUBoot resides at the base of the flash.
-   Primary application partition : The example application which would be run
    by the bootloader (active application). The size reserved for this partition
    is 4.4 MB.
-   Secondary application partition : Update image received with the OTA
    (candidate application). The size reserved for the partition is 4.4 MB.

Notes :

-   For RW61x: The CPU1/CPU2 firmware are embedded in the CPU3 example
    application.
-   The sizes of the primary and secondary applications are provided as an
    example (currently 4.4 MB is reserved for each partition). The size can be
    changed by modifying the `m_app_max_sectors` value in the linker script of
    the application .

### MCUBoot Bootloader

MCUBoot is an open-source secure bootloader used by RW61x to apply the
self-upgrade. For more details, please refer to the
[MCUBoot documentation](https://github.com/mcu-tools/mcuboot/blob/main/docs/design.md).

For RTs platform, the bootloader is configured to use the flash remapping
mechanism by default, in order to perform the image upgrade. This is achieved by
using the `MCUBoot DIRECT-XIP` upgrade mode.

## OTA Software Update process for RTs example application

> Important note : When building the NXP Matter examples with CMake build
> system, the mcuboot binary, the signed application image, and the .ota file
> are all automatically generated when OTA SU is enabled. The auto-generation is
> handled in `third_party/nxp/nxp_matter_support/cmake/build_helpers.cmake`.
> Therefore, you may skip the generation process described below.
>
> To build with OTA enabled, you can refer to the 'Building' section of the
> platform [dedicated readme](./nxp_examples_freertos_platforms.md#building). By
> default, the software version is 1. For building with software version 2 you
> can use a `prj_<custom>.conf` which has `v2` suffix.

### Generating and Flashing the bootloader

#### Generating MCUBoot bootloader

> Note : For applications generated with CMake, this section can be skipped.
> Auto-generated MCUBoot binary can be found under the path
> `<build_dir>/modules/chip/mcuboot/` with `.elf` and `.bin` formats.

MCUBoot application can be built with NXP MCUX SDK installed, using instructions
below.

-   Locate the MCUX SDK directory :

```
user@ubuntu: cd ~/Desktop/connectedhomeip/third_party/nxp/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk
```

-   Export your ARM GCC toolchain :

```
user@ubuntu: export ARMGCC_DIR=  # with ARMGCC_DIR referencing the compiler path recommended by the SDK.
```

-   Build `mcuboot` example from the root directory of MCUX SDK using
    `west build` command :

```
user@ubuntu: west build -d mcuboot_build -b <RT_board> examples/ota_examples/mcuboot_opensource
```

> Note : For RT1170 platform, `-Dcore_id=cm7` argument should be added to the
> build command-line above.

Replace `RT_board` with :

-   For RW61x platform, either `rdrw612bga` or `frdmrw612` based on the board
    you are targeting.
-   For `RT1170-EVKB`, use `evkbmimxrt1170`.
-   For `RT1060-EVKC`, use `evkcmimxrt1060`.

MCUBoot binary can be found under `mcuboot_build` folder under the SDK root.

#### Flashing MCUBoot bootloader

In order for the device to perform the software update, the MCUBoot bootloader
must be flashed first at the base of the flash. A step-by-step guide is given
below.

-   It is recommended to start with erasing the external flash of the device,
    for this JLink from Segger can be used. It can be downloaded and installed
    from https://www.segger.com/products/debug-probes/j-link. Once installed,
    JLink can be run using the command line :

```
$ JLink
```

Run the following commands :

Connect J-Link debugger to device:

```sh
J-Link > connect
Device> ? # you will be presented with a dialog -> select `RW612` for RW61x, `MIMXRT1062XXX6B` for RT1060, `MIMXRT1176xxxA_M7` for RT1170
Please specify target interface:
J) JTAG (Default)
S) SWD
T) cJTAG
TIF> S
Specify target interface speed [kHz]. <Default>: 4000 kHz
Speed> # <enter>
```

Erase flash:

```
J-Link > exec EnableEraseAllFlashBanks
```

For RW61x

```
J-Link > erase 0x8000000, 0x88a0000
```

For RT1060-EVK-C

```
J-Link > erase 0x60000000, 0x61000000
```

For RT1170-EVK-B

```
J-Link > erase 0x30000000, 0x34000000
```

-   Program the generated binary to the target board.

```
J-Link > loadfile <path_to_mcuboot>/mcuboot_opensource.elf
```

-   If it runs successfully, the following logs will be displayed on the
    terminal :

```
hello sbl.
Disabling flash remapping function
Bootloader Version 2.0.0
Image 0 Primary slot: Image not found
Image 0 Secondary slot: Image not found
No slot to load for image 0
Unable to find bootable image
```

Note : By default, mcuboot application considers the primary and secondary
partitions to be the size of 4.4 MB. If the size is to be changed, the partition
addresses could be modified in the
`third_party/nxp/nxp_matter_support/cmake/rt/<rt_platform>/bootloader.conf` for
a CMake build where the mcuboot application is automatically generated.
Otherwise, to generate it manually, the partition addresses should be modified
in `flash_partitioning.h` accordingly. For more information about the flash
partitioning with mcuboot, please refer to the dedicated `readme.txt` located in
`<matter_repo_root>/third_party/nxp/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/examples/ota_examples/mcuboot_opensource`.

### Generating and flashing the signed application image

#### Generating the signed application image

> Note : For applications generated with CMake, this section can be skipped.
> Auto-generated signed application image can be found under the path
> `<build_dir>/app_SIGNED.bin`.

After flashing the bootloader, the application can be programmed to the board.
The image must have the following format :

-   Header : contains general information about the image (version, size,
    magic...)
-   Code of the application : generated binary
-   Trailer : contains metadata needed by the bootloader such as the image
    signature, the upgrade type, the swap status...

The all-clusters application can be generated using the instructions from the
[CHIP NXP Examples Guide 'Building'](./nxp_examples_freertos_platforms.md#building).
The application is automatically linked to be executed from the primary image
partition, taking into consideration the offset imposed by mcuboot.

For an application generated with GN build system, the resulting executable file
found in
out/release/chip-<a href="#4" id="4-ref">`board`<sup>4</sup></a>-all-cluster-example
needs to be converted into raw binary format as shown below.

_<a id="4" href="#4-ref"><sup>4</sup></a> `rw61x` for RW61x, `rt1060` for
RT1060-EVK-C, `rt1170` for RT1170-EVK-B_

```sh
arm-none-eabi-objcopy -R .flash_config -R .NVM -O binary chip-<"board">-all-cluster-example chip-<"board">-all-cluster-example.bin
```

To sign the image and wrap the raw binary of the application with the header and
trailer, "`imgtool`" is provided in the SDK and can be found in
"`<matter_repo_root>/third_party/nxp/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/middleware/mcuboot_opensource/scripts/`".

The following commands can be run (make sure to replace the /path/to/file/binary
with the adequate files):

```sh
user@ubuntu: cd ~/Desktop/<matter_repo_root>/third_party/nxp/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/middleware/mcuboot_opensource/scripts/

user@ubuntu: python3 imgtool.py sign --key ~/Desktop/<matter_repo_root>/third_party/nxp/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/middleware/mcuboot_opensource/boot/nxp_mcux_sdk/keys/sign-rsa2048-priv.pem --align 4 --header-size 0x1000 --pad-header --pad --confirm --slot-size 0x440000 --max-sectors 1088 --version "1.0" ~/Desktop/connectedhomeip/examples/all-clusters-app/nxp/rt/<"rt_board">/out/debug/chip-<"rt_board">-all-cluster-example.bin ~/Desktop/connectedhomeip/examples/all-clusters-app/nxp/rt/<"rt_board">/out/debug/chip-<"rt_board">-all-cluster-example_SIGNED.bin
```

Notes :

-   The arguments `slot-size` and `max-sectors` are aligned with the size of the
    partitions reserved for the primary and the secondary applications. (By
    default the size considered is 4.4 MB for each application). If the size of
    these partitions are modified, the `slot-size` and `max-sectors` should be
    adjusted accordingly.
-   In this example, the image is signed with the private key provided by the
    SDK as an example
    (`<matter_repo_root>/third_party/nxp/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/middleware/mcuboot_opensource/boot/nxp_mcux_sdk/keys/sign-rsa2048-priv.pem`),
    MCUBoot is built with its corresponding public key which would be used to
    verify the integrity of the image. It is possible to generate a new pair of
    keys using the following commands. This procedure should be done prior to
    building the mcuboot application.

-   To generate the private key :

```
user@ubuntu: python3 imgtool.py keygen -k priv_key.pem -t rsa-2048
```

-   To extract the public key :

```
user@ubuntu: python3 imgtool.py getpub -k priv_key.pem
```

-   The extracted public key can then be copied to the
    `<matter_repo_root>/third_party/nxp/nxp_matter_support/github_sdk/sdk_next/repo/mcuxsdk/middleware/mcuboot_opensource/boot/nxp_mcux_sdk/keys/sign-rsa2048-pub.c`,
    given as a value to the rsa_pub_key[] array.

The resulting output is the signed binary of the application version "1.0".

#### Flashing the signed application image

JLink can be used to flash the application using the command :

For RW61x

```
J-Link > loadbin <application>_SIGNED.bin 0x8020000
```

For RT1060-EVK-C

```
J-Link > loadbin <application>_SIGNED.bin 0x60040000
```

For RT1170-EVK-B

```
J-Link > loadbin <application>_SIGNED.bin 0x30040000
```

The bootloader should then be able to jump directly to the start of the
application and run it.

### Generating the OTA Update Image

> Note : For applications generated with CMake, this section can be skipped.
> Auto-generated `.ota` file can be found under `<build_dir>/app.ota` .

To generate the OTA update image, the same procedure can be followed from the
[Generating and flashing the signed application image](#generating-and-flashing-the-signed-application-image)
sub-section, replacing the "--version "1.0"" argument with "--version "2.0""
(recent version of the update), without arguments "--pad" "--confirm" when
running `imgtool` script during OTA Update Image generation.

Note : When building the update image, the build arguments
`nxp_software_version=2` `nxp_software_version_string=\"2.0\"` can be added to
the gn gen command in order to specify the upgraded version. For a CMake build,
this can be done by adding to the command line the arguments
`-DCONFIG_CHIP_DEVICE_SOFTWARE_VERSION=2`
`-DCONFIG_CHIP_DEVICE_SOFTWARE_VERSION_STRING="2.0"`, or using `prj_*_v2.conf`
configuration files.

In order to have a correct OTA process, the OTA header version should be the
same as the binary embedded software version.

When the signed binary of the update is generated, the file should be converted
into OTA format. To do so, the `scripts/tools/nxp/ota/ota_image_tool.py` is
provided in the repo and can be used to convert a binary file into an .ota file.
This NXP script is a wrapper over the standard tool `src/app/ota_image_tool.py`,
and can be used to generate an OTA image with the following format :

```
    | OTA image header | TLV1 | TLV2 | ... | TLVn |
```

where each TLV is in the form `|tag|length|value|`.

Note that "standard" TLV format is used. Matter TLV format is only used for
factory data TLV value.

Please find more information about the script and supported arguments in the
[OTA image tool guide](../../../scripts/tools/nxp/ota/README.md).

Example of command to generate the `.ota` file :

```sh
user@ubuntu:~/connectedhomeip$ : ./scripts/tools/nxp/ota/ota_image_tool.py create -v 0xDEAD -p 0xBEEF -vn 2 -vs "2.0" -da sha256 --app-input-file chip-<"rt_board">-all-cluster-example_SIGNED.bin chip-rw61x-all-cluster-example.ota
```

#### OTA with encryption

A user can choose to enable the encryption of the OTA update image. This can be
done by adding `--enc_enable` and `--input_ota_key <aes_128_key>` to the
ota_image_tool script, by replacing `<aes_128_key>` with the encryption key.
This will generate an encrypted `.ota` file.

Note that the application must enable the encryption also to be able to
successfully process the update image. This can be done by adding to the build :

-   For a GN build, add to the `gn gen` args `chip_with_ota_encryption=true` and
    `chip_with_ota_key=<aes_128_key>`.
-   For a CMake build, add to the `west build` command line the following
    Kconfig `-DCONFIG_CHIP_OTA_ENCRYPTION=y` and
    `-DCONFIG_CHIP_OTA_ENCRYPTION_KEY=<aes_128_key>`.

The `<aes_128_key>` value must match between the application build and the
`.ota` generation.

> Note : For applications generated with CMake, if the build configuration has
> `CONFIG_CHIP_OTA_ENCRYPTION=y` Kconfig enabled, then the auto-generated `.ota`
> file will be encrypted with the specified key.

The generated OTA file can be used to perform the OTA Software Update. The
instructions below describe the procedure step-by-step.

### Performing the OTA Software Update

Setup example :

-   [Chip-tool](../../development_controllers/chip-tool/chip_tool_guide.md)
    application running on the RPi.
-   OTA Provider application built on the same RPi (as explained below).
-   RT board programmed with the example application (with the instructions
    above).

Before starting the OTA process, the Linux OTA Provider application can be built
on the RPi (if not already present in the pre-installed apps) :

```
user@ubuntu:~/connectedhomeip$ : ./scripts/examples/gn_build_example.sh examples/ota-provider-app/linux out/ota-provider-app chip_config_network_layer_ble=false
user@ubuntu:~/connectedhomeip$ : rm -rf /tmp/chip_*
```

```sh
user@ubuntu:~/connectedhomeip$ : ./out/ota-provider-app/chip-ota-provider-app -f chip-<"rt_board">-all-cluster-example.ota
```

The OTA Provider should first be provisioned with chip-tool by assigning it the
node id 1, and then granted the ACL entries :

```
user@ubuntu:~/connectedhomeip$ : ./out/chip-tool-app/chip-tool pairing onnetwork 1 20202021
user@ubuntu:~/connectedhomeip$ : ./out/chip-tool-app/chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": null, "targets": null}]' 1 0
```

The second step is to provision the device with the node id 2 using ble-wifi or
ble-thread commissioning. For example :

```
user@ubuntu:~/connectedhomeip$ : ./out/chip-tool-app/chip-tool pairing ble-wifi 2 WIFI_SSID WIFI_PASSWORD 20202021 3840
```

Once commissioned, the OTA process can be initiated with the
"announce-otaprovider" command using chip-tool (the given numbers refer
respectively to [ProviderNodeId][vendorid] [AnnouncementReason][endpoint]
[node-id][endpoint-id]) :

```
user@ubuntu:~/connectedhomeip$ : ./out/chip-tool-app/chip-tool otasoftwareupdaterequestor announce-otaprovider 1 0 0 0 2 0
```

When the full update image is downloaded and stored, the bootloader will be
notified and the device will reboot with the update image.
