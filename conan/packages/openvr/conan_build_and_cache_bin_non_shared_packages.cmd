IF [%~1] EQU [] (SET VERSION=2.2.3) ELSE (SET VERSION=%1)
SET PACKAGE=openvr
SET RECIPE=.
SET SHARED=False
..\..\scripts\conan_build_and_cache_bin_shared_packages.cmd
