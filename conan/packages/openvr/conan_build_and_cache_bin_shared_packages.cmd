IF [%~1] EQU [] (SET VERSION=1.26.7) ELSE (SET VERSION=%1)
SET PACKAGE=openvr
SET RECIPE=.
SET SHARED=True
..\..\scripts\conan_build_and_cache_bin_shared_packages.cmd
