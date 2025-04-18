IF [%~1] EQU [] (SET VERSION=3.8.1) ELSE (SET VERSION=%1)
SET PACKAGE=catch2
SET RECIPE=.
SET SHARED=False
..\..\scripts\conan_build_and_cache_bin_shared_packages.cmd
