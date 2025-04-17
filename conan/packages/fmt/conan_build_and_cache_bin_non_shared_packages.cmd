IF [%~1] EQU [] (SET VERSION=11.1.4) ELSE (SET VERSION=%1)
SET PACKAGE=fmt
SET RECIPE=.
SET SHARED=False
..\..\scripts\conan_build_and_cache_bin_shared_packages.cmd
