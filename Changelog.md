# HMDQ Tools Change Log

## 2.2.1 - April 20, 2024

Cosmetic:

- Restored the order of the generated JSON, which was changed by previous changes.

## 2.2.0 - April 20, 2024

Major update:

- Fixed an algorithm which calculates HAM area (see below).
- Cleaned up the code which does FOV and HAM calculations.
- Changed project structure.
- Added more local builds for conan packages.

Until now the HAM calculation assumed that all HAMs were specified inside [(0,0), (1,1)] rectangle and _did not check it_. Thus calculating the total mesh area as a plain sum of all triangles' areas.

Some HAMs however overlap slightly the nominal rectangle and the sum of all triangles' areas gives slightly higher value. I thought originally the error would be small, but it turned out that the difference is perceivable.

The affected headsets which were reported in HMDGDB with slightly bigger HAM area, are following:

```python
Oculus Rift CV1 - original: 14.07 %, corrected: 10.03 %
Oculus Rift S   - original: 11.26 %, corrected:  9.25 %
Oculus Quest    - original:  5.69 %, corrected:  3.68 %
Oculus Quest 2  - original:  9.13 %, corrected:  7.12 %
Meta Quest Pro  - original: 14.11 %, corrected: 11.92 %
Meta Quest 3    - original: 12.89 %, corrected: 10.66 %
```

The HMDGDB is updated to show the correct values now.

## 2.1.8 - January 20, 2024

Maintenance release:

- Bumped dependencies versions.
- Migrated CMakeSettings.json -> CMakePresets.json
- Bumped OpenVR version to 2.2.3

## 2.1.7 - October 8, 2023

Maintenance release:

- Fixed localized path handling in CLI.

## 2.1.6 - September 3, 2023

Maintenance release:

- Bumped OpenVR version to 1.26.7.

## 2.1.5 - January 31, 2023

Maintenance release:

- Switched code compliance to C++20.
- Added local conan recipes for packages not in conancenter.
- Revamped conan integration.

## 2.1.4 - November 7, 2022

Quick fix:

- Fixed hmdv not saving anonymized values (when instructed with -n).

## 2.1.3 - September 24, 2022

Quick fix:

- Fixed a crash in hmdv (forgotten fix from hmdq).

## 2.1.2 - August 28, 2022

Maintenance release:

- Updated OpenVR API JSON to OpenVR SDK 1.23.7.
- Updated the build process to use conan packages.
- Updated external lib references to their current versions.
- Updated linked LibOVR to v32.0.

## 2.1.1 - October 12, 2020

- Added a workaround for invalid FOV data (Quest 2).
- Some code clean up.
- Updated to OpenVR SDK 1.14.15.

## 2.1.0 - September 12, 2020

- Changed FOV calculation algorithm to accept raw total horizontal FOV > 180 deg. (StarVR One).
- Updated to OpenVR SDK 1.12.5.

## 2.0.1 - April 25, 2020

- Updated to OpenVR SDK 1.11.11.

## 2.0.0 - April 18, 2020

Major release which includes mainly:

- Completely redesigned internal architecture.
- Added support for Oculus runtime, which means that now Oculus headsets can be queried directly without a SteamVR intervention and can provide more information than what SteamVR exposes for them.

## 1.3.90 - April 13, 2020

- Release candidate RC1 before 2.0 release.

## 1.3.3 - January 4, 2020

- Updated OpenVR API JSON to OpenVR SDK 1.9.15.

## 1.3.2 - November 6, 2019

- Updated OpenVR API JSON to OpenVR SDK 1.8.19.

## 1.3.1 - September 18, 2019

- Added OpenVR build version to the version info.
- Updated OpenVR API JSON to OpenVR SDK 1.7.15.
- Internal: Changed version info generation.
- Internal: Updated all external libraries.

## 1.3.0 - September 10, 2019

- Added `Prop_RegisteredDeviceType_String` to the default anonymized properties list. This caused a new config version (4) and thus requires the manual update of the config file.

## 1.2.5 - September 10, 2019

- Changed
  - All applicable fixes to `hmdq` data file (either concerning format, units, or values), are now implemented in separate module and applied in `hmdv` accordingly to the version they affect.
  - Saving data into JSON file in `hmdv` will now save updated version and mark the file with `<file>["misc"]["hmdv_ver"]` value, indicating, which `hmdv` version was used to alter the data.

## 1.2.4 - September 8, 2019

- Changed
  - Total vertical FOV calculation formula. Instead of using combined minimum from both eyes it now uses arithmetic average.
  - Fix has been added to `hmdv` to compensate for this, so resaving the data file will save it with fixed value.
- Internal
  - Changed the way data collection and additional data calculation are handled. Now the collection is separate from the calculation.
  - In the same spirit the anonymization is also separated from the data collection.

## 1.2.3 - September 7, 2019

- Changed: reported IPD is now stored in meters in the JSON output file.

## 1.2.2 - August 29, 2019

- Fixed: `hmdv` compatibility with older data files.

## 1.2.1 - August 27, 2019

- Internal: printing routines refactored.

## 1.2.0 - August 25, 2019

- Added new tool `hmdv` for data file processing.
- Internal: code and CMake files refactoring.

## 1.0.1 - August 20, 2019

- Fixed relative path handling for default and specified files.
- Internal: converting command line arguments to UTF-8 and handling them this way.

## 1.0.0 - August 7, 2019

- **Official release of the binary.**
- Added support for OpenVR runtime version.
- Internal: added `const` qualifiers wherever possible.

## 0.3.1 - August 6, 2019

- Added secure checksum to the output file.
- Removed `use_names` option from the configuration file.
- Internal: converted all `iostream` outputs to using `{fmt}`.

## 0.3.0 - August 4, 2019

- Added an option for anonymization of sensitive info (basically serial numbers of the tracked devices).
- Added new config values to support anonymization.

> Note: Upgrading users should regenerate the default config and then merge the old one.

## 0.2.4 - August 2, 2019

- **Initial public commit**  
  The source code is out. The binary will be released shortly after some closed beta testing.
