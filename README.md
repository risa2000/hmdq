# `hmdq`
![hmdq application icon](docs/images/hmdq_128.png)

`hmdq` is a command line tool for collecting different information from an OpenVR headset (a.k.a head mounted display - HMD) and other VR devices for Windows. In order to run it, one needs an OpenVR stack installed, which usually means SteamVR and, of course, the hardware.

[Change Log](Changelog.md)

## Installation
Get the latest binary ZIP from [releases](https://github.com/risa2000/hmdq/releases) and unzip it wherever you want to run it.

## Operation
Since `hmdq` is a command line tool, it is better run from Windows console (or any other terminal window which supports standard I/O).

When the tool runs the first time, it creates a configuration file `hmdq.conf.json` in the same directory.

Another file which is necessary (and is present in the archive) is OpenVR API description file (`openvr_api.json`). This file can be updated independently of `hmdq`.

### Commands

#### `help`
Shows all the commands and options with short descriptions.

```bash
$ hmdq help
Usage:
        hmdq (geom|props|version|all|help) [-a <name>] [-o <name>] [-v [<level>]] [-n]
Options:
        geom        show only geometry data
        props       show only device properties
        version     show version and other info
        all         show all data (default choice)
        help        show this help page
        -a, --api_json <name>
                    OpenVR API JSON definition file [openvr_api.json]

        -o, --out_json <name>
                    JSON output file

        -v, --verb <level>
                    verbosity level [0]

        -n, --anonymize
                    anonymize serial numbers in the output [false]
```

#### `geom`
Displays the headset geometry as advertised by the OpenVR subsystem to the application. It includes:

* View geometry (FOVs) for each eye.
* Total stereo FOVs and the overlap.
* Rotation (canting) of the panels.
* IPD value.

Additionally to that, it also shows:

* The recommended render target resolution (for the application in whose context `hmdq` runs).
* The statistics of the _hidden area mask_ (HAM) mesh (if supported by the headset).

Example (excerpt):
```c
Total FOV:
    horizontal: 108.77 deg
    vertical:   111.48 deg
    diagonal:   113.18 deg
    overlap:     93.45 deg

View geometry:
    left panel rotation:     0.0 deg
    right panel rotation:    0.0 deg
    reported IPD:           63.0 mm
```

#### `props`
Prints out all different _tracked device properties_ for all currently detected _tracked devices_. The term _tracked device_ includes not only the headset and the controllers, but also the lighthouses and additional (not tracked) devices as the gamepads. The number of properties shown depends on the _verbosity_ level specified by the user, which, by default, is set to `0`.

Example (excerpt):
```c
Device enumeration:
    Found dev: id=0, class=1, name=HMD

[0:HMD]
    1000 : Prop_TrackingSystemName_String = "lighthouse"
    1001 : Prop_ModelNumber_String = "Vive. MV"
    1002 : Prop_SerialNumber_String = "LHR-ABCDEFGH"
    1003 : Prop_RenderModelName_String = "generic_hmd"
    1005 : Prop_ManufacturerName_String = "HTC"
    1006 : Prop_TrackingFirmwareVersion_String = "1462663157 steamservices@firmware-win32 2016-05-08 FPGA 262(1.6/0/0) BL 0"
    1007 : Prop_HardwareRevision_String = "product 128 rev 2.1.0 lot 2000/0/0 0"
    2002 : Prop_DisplayFrequency_Float = 90
```

#### `version`
Shows the tool version info plus some additional information about the build and the used libraries.

#### `all (default)`
Processes both `geom` and `props`. This is the default command.

### Options

#### `--api_json <filename>`
Allows specifying a custom/different/new JSON file with OpenVR API definitions. Normally, you should not need that. The one included comes directly from [OpenVR repository](https://github.com/ValveSoftware/openvr/tree/master/headers).

You can update this file independently from `hmdq` though to let the tool recognize new properties (if there were any).

#### `--out_json <filename>`
When specified, all the information collected by the tool (not only what is actually displayed in the console, but all possible retrievable information), plus some additional information calculated by the tool in the process (e.g. different _FOV points_, HAM mesh optimized layout, etc.) is stored in the specified file in JSON format.

This file is mostly useful for additional processing, comparison, and future reference.

**NOTE:** _The amount of the information stored in the file, **is not controlled by the verbosity level**. Every time the file is created, all the available information plus the additionally computed data are stored in the file. The reason for that is to have a well defined set of data, which is guaranteed to be present._

#### `-v <level>, --verb <level>`
Verbosity level of the output (to the console). There are five levels defined:

* `--verb -1`  
Silent running. Shows only the basic startup info.
* `--verb 0 (default)`  
The default one, shows just the basic information in both `geom` and `props` modes.
* `--verb 1`  
All the information available for `geom` part is displayed, and the basic information for `props`.
* `--verb 2`  
Adds few more properties in `props` mode (specified in the config file).
* `--verb 3`  
Shows all the properties, which are supported by each device currently detected and identified by the system.
* `--verb 4`  
Adds as well all the properties, which are defined by the OpenVR API, but are neither supported by the devices, nor are supported by the tool itself (the reason is explained by the error message given in the console output).

The levels can be redefined in the configuration file. The values listed above are the default ones.

#### `-n, --anonymize`
If specified, it will anonymize values in pre-selected tracked device properties, basically anything which looks like a serial number. Some are predefined in the default configuration (and therefore in the config file), others could be added to the config, if needed.

The anonymization happens in both the console output and in the output JSON file.

This could be useful for sharing the output data in public, without disclosing the unique identifiers.

The anonymized values are computed by using the secure hash function [Blake2](https://blake2.net) set with 96-bit wide output. The hash is computed over three properties: #1005 (`Prop_ManufacturerName_String`), concatenated with #1001 (`Prop_ModelNumber_String`), and finally with the incriminated value to anonymize. The manufacturer and model number are used to pre-seed the hash with distinct values, so the same serial numbers from different manufacturers, will not anonymize into the same values.

### Configuration
The configuration file `hmdq.conf.json` is always created with the default values, and can be changed later by the user. The tool will not "touch" the configuration file as long as it exists and only create a new one if none is present.

It has a consequence when updating the tool to the new version. It may happen that the new version of the tool introduces new config values, which are not present in the old configuration file. As the config file is never modified by the tool, they will not be added, will always be used with the default presets, and the user will not be able to change them.

If this happens the way how to resolve it is let the new version create a new (default) config file (e.g. by renaming the old one) and then manually copy paste the new values added in the new version to the old config and put it back in place.

There are following configuration options:

* `meta`  
This section is read-only. Changing it has no meaning or impact on the tool operation.
* `control`
    * `anonymize` decides whether the sensitive data (the serial numbers) are anonymized by default. Default value is `false`.
    * `anon_props` defines the list of tracked device properties which are anonymized, if requested either by the user with the  command line option or by specifying the default behavior in the config file.
* `format`  
Defines the indentation (in spaces):
    * `cli_indent` for standard output in the console,
    * `json_indent` for the output JSON file.
* `openvr`  
Sets the operational conditions related to OpenVR. Currently only one is supported:
    * `app_type`  
    Defines how the tool initializes the OpenVR subsystem in `vr::VR_Init` call. The different application types are described [here](https://github.com/ValveSoftware/openvr/wiki/API-Documentation#initialization-and-cleanup).  
    **Note**: _Basically anything different from the default value is untested and unsupported. Putting in a wrong type can also impact the reported values. So only change it if you know what you are doing._
* `verbosity`  
Defines all the different verbosity levels to which the user specified verbosity (set by `--verb` on the command line) is compared.
    * `silent`  
    Defines the level, which, when specified by user, will display only the basic startup info.
    * `default`  
    The default verbosity level which the tool will use, when none is specified on the command line.
    * `geom`  
    The level at which all the geometry properties (displayed in `geom` command) are shown.
    * `max`  
    The level at which all properties _with meaningful values_ are included in the console output.
    * `error`  
    The level which, when specified, will also display unsupported properties (with errors).
    * `props`  
    Defines the individual verbosity levels for listed properties. The default list is more of an example than some sophisticated choice. The number defines the minimal required verbosity level specified by the user, in order to have the property value displayed in the output.

## Theory behind the FOV calculations
While listing the properties of (tracked) devices is a routine task, nothing to write much about, the FOV calculation turned out to be an interesting challenge with few unexpected outcomes.

Since it deserves a separate space on its own, it is in detail dissected here [FOV calculation of a VR headset](https://github.com/risa2000/vr_docs/blob/master/docs/fov_calculation.md).

## Building the tool from the source code
The tool was developed as a fun project, so it is probably overdone on many levels, but should never the less be buildable relatively easily. There are few external dependencies though.

### Required external libraries
* [`muellan/clipp`](https://github.com/muellan/clipp)
for parsing the command line arguments.
* [`QuantStack/xtensor`](https://github.com/QuantStack/xtensor) for all 2D, 3D vectors and matrices operations.
* [`QuantStack/xtl`](https://github.com/QuantStack/xtl) required by `QuantStack/xtensor`.
* [`nlohmann/json`](https://github.com/nlohmann/json) for all JSON parsing and creating.
* [`nlohmann/fifo_map`](https://github.com/nlohmann/fifo_map) for supporting ordered JSON output.
* [`ValveSoftware/openvr`](https://github.com/ValveSoftware/openvr) for obvious reasons.
* [`randombit/botan`](https://github.com/randombit/botan) for secure hash implementation.
* [`fmtlib/fmt`](https://github.com/fmtlib/fmt) for comfortable printing and formatting of the console output.

On top of that you will also need `cmake` version 3.15 or higher.

### Optional external libraries
* [`Catch2`](https://github.com/catchorg/Catch2) to build unit tests.

### Building
The project was developed as a "CMake project" in Visual Studio 2019, while using `ninja` as a build driver. The binary can be successfully built by native MSVC compiler `cl` or by LLVM `clang-cl` (Clang drop in replacement for MSVC compiler).

Building with `clang-cl` may provide some additional challenges which however should not surprise anyone who is already using this compiler as he should be accustomed to some roughing.

Because of the particular use of `nlohmann/json` library, it is also necessary to patch `QuantStack/xtensor` JSON support with this [patch](https://github.com/QuantStack/xtensor/compare/master...risa2000:xjson_patch). At least until this patch is merged or some other solution to the issue is implemented.

To have the automatic versioning working correctly, CMake scripts expect the build to happen in a locally cloned `git` repository.
