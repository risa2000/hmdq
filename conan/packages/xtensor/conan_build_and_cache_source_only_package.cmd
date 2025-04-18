IF [%~1] EQU [] (SET VERSION=0.26.0) ELSE (SET VERSION=%1)
SET PACKAGE=xtensor
SET RECIPE=.
..\..\scripts\conan_build_and_cache_source_only_package.cmd
