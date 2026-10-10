SET PROFILES_PATH=..\..\profiles

conan create ^
    -pr:h=%PROFILES_PATH%\host_msvc_crtdll.txt ^
    -pr:b=%PROFILES_PATH%\build_msvc_crtdll.txt ^
    -s:h compiler.runtime=static ^
    -s:h build_type=Debug ^
    --name=%PACKAGE% ^
    --version=%VERSION% ^
    %RECIPE%

conan create ^
    -pr:h=%PROFILES_PATH%\host_msvc_crtdll.txt ^
    -pr:b=%PROFILES_PATH%\build_msvc_crtdll.txt ^
    -s:h compiler.runtime=static ^
    -s:h build_type=Release ^
    --name=%PACKAGE% ^
    --version=%VERSION% ^
    %RECIPE%

conan create ^
    -pr:h=%PROFILES_PATH%\host_msvc_crtdll.txt ^
    -pr:b=%PROFILES_PATH%\build_msvc_crtdll.txt ^
    -s:h compiler.runtime=dynamic ^
    -s:h build_type=Debug ^
    --name=%PACKAGE% ^
    --version=%VERSION% ^
    %RECIPE%

conan create ^
    -pr:h=%PROFILES_PATH%\host_msvc_crtdll.txt ^
    -pr:b=%PROFILES_PATH%\build_msvc_crtdll.txt ^
    -s:h compiler.runtime=dynamic ^
    -s:h build_type=Release ^
    --name=%PACKAGE% ^
    --version=%VERSION% ^
    %RECIPE%
