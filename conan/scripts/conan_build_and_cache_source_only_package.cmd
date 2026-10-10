SET PROFILES_PATH=..\..\profiles

conan create ^
	-pr:h=%PROFILES_PATH%\host_msvc_crtdll.txt ^
	-pr:b=%PROFILES_PATH%\build_msvc_crtdll.txt ^
	--name=%PACKAGE% ^
	--version=%VERSION% ^
	%RECIPE%
