IF [%~1] EQU [] (SET VERSION=3.13.0) ELSE (SET VERSION=%1)
SET PACKAGE=botan
SET RECIPE=.
SET SHARED=True
..\..\scripts\conan_build_and_cache_bin_shared_packages.cmd
