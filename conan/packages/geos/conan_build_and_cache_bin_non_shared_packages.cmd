IF [%~1] EQU [] (SET VERSION=3.15.0) ELSE (SET VERSION=%1)
SET PACKAGE=geos
SET RECIPE=.
SET SHARED=False
..\..\scripts\conan_build_and_cache_bin_shared_packages.cmd
