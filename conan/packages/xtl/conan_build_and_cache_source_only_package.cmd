IF [%~1] EQU [] (SET VERSION=0.8.0) ELSE (SET VERSION=%1)
SET PACKAGE=xtl
SET RECIPE=.
..\..\scripts\conan_build_and_cache_source_only_package.cmd
