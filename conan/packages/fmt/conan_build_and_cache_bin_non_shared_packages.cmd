IF [%~1] EQU [] (SET VERSION=12.2.0) ELSE (SET VERSION=%1)
SET PACKAGE=fmt
SET RECIPE=.
SET SHARED=False
..\..\scripts\conan_build_and_cache_bin_shared_packages.cmd
