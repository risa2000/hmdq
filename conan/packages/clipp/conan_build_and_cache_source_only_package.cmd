IF [%~1] EQU [] (SET VERSION=cci.20230831) ELSE (SET VERSION=%1)
SET PACKAGE=clipp
SET RECIPE=.
..\..\scripts\conan_build_and_cache_source_only_package.cmd
