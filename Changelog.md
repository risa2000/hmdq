# `hmdq` Change Log

## 1.3.1 - September 18, 2019
* Added OpenVR build version to the version info.
* Updated OpenVR API JSON to OpenVR SDK 1.17.15.
* Internal: Changed version info generation.
* Internal: Updated all external libraries.

## 1.3.0 - September 10, 2019
* Added `Prop_RegisteredDeviceType_String` to the default anonymized properties list. This caused a new config version (4) and thus requires the manual update of the config file.

## 1.2.5 - September 10, 2019
* Changed  
  * All applicable fixes to `hmdq` data file (either concerning format, units, or values), are now implemented in separate module and applied in `hmdv` accordingly to the version they affect.
  * Saving data into JSON file in `hmdv` will now save updated version and mark the file with `<file>["misc"]["hmdv_ver"]` value, indicating, which `hmdv` version was used to alter the data.

## 1.2.4 - September 8, 2019
* Changed  
  * Total vertical FOV calculation formula. Instead of using combined minimum from both eyes it now uses arithmetic average.
  * Fix has been added to `hmdv` to compensate for this, so resaving the data file will save it with fixed value.
* Internal  
  * Changed the way data collection and additional data calculation are handled. Now the collection is separate from the calculation.
  * In the same spirit the anonymization is also separated from the data collection.

## 1.2.3 - September 7, 2019
* Changed: reported IPD is now stored in meters in the JSON output file.

## 1.2.2 - August 29, 2019
* Fixed: `hmdv` compatibility with older data files.

## 1.2.1 - August 27, 2019
* Internal: printing routines refactored.

## 1.2.0 - August 25, 2019
* Added new tool `hmdv` for data file processing.
* Internal: code and CMake files refactoring.

## 1.0.1 - August 20, 2019
* Fixed relative path handling for default and specified files.
* Internal: converting command line arguments to UTF-8 and handling them this way.

## 1.0.0 - August 7, 2019
* **Official release of the binary.**
* Added support for OpenVR runtime version.
* Internal: added `const` qualifiers wherever possible.

## 0.3.1 - August 6, 2019
* Added secure checksum to the output file.
* Removed `use_names` option from the configuration file.
* Internal: converted all `iostream` outputs to using `{fmt}`.

## 0.3.0 - August 4, 2019
* Added an option for anonymization of sensitive info (basically serial numbers of the tracked devices).
* Added new config values to support anonymization.

>Note: Upgrading users should regenerate the default config and then merge the old one.

## 0.2.4 - August 2, 2019
* **Initial public commit**  
The source code is out. The binary will be released shortly after some closed beta testing.