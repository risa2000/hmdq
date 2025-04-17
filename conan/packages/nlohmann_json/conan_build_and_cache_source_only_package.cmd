IF [%~1] EQU [] (SET VERSION=3.12.0) ELSE (SET VERSION=%1)
SET PACKAGE=nlohmann_json
SET RECIPE=.
..\..\scripts\conan_build_and_cache_source_only_package.cmd
