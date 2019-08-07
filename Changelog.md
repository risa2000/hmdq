# `hmdq` Change Log

## 1.0.0 - August 7, 2019
* **Official release of the binary.**
* Added support for OpenVR runtime version.
* Internal: Added `const` qualifiers wherever possible.

## 0.3.1 - August 6, 2019
* Added secure checksum to the output file.
* Removed `use_names` option from the configuration file.
* Internal: Converted all `iostream` outputs to using `{fmt}`.

## 0.3.0 - August 4, 2019
* Added an option for anonymization of sensitive info (basically serial numbers of the tracked devices).
* Added new config values to support anonymization.

>Note: Upgrading users should regenerate the default config and then merge the old one.

## 0.2.4 - August 2, 2019
* **Initial public commit**  
The source code is out. The binary will be released shortly after some closed beta testing.