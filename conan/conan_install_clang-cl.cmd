SET PROFILES_PATH=.\profiles
SET CONNAN_OUT_PATH=..\out\conan

conan install ^
    -of=%CONNAN_OUT_PATH%\x64-xClang-Static-Debug ^
    -pr:h=%PROFILES_PATH%\host_clang-cl_crtdll.txt ^
    -pr:b=%PROFILES_PATH%\build_clang-cl_crtdll.txt ^
    --build=missing ^
    -s compiler.runtime=static ^
    -s build_type=Debug ^
    -c tools.cmake.cmaketoolchain:user_presets="" ^
    ..

conan install ^
    -of=%CONNAN_OUT_PATH%\x64-xClang-Static-Release ^
    -pr:h=%PROFILES_PATH%\host_clang-cl_crtdll.txt ^
    -pr:b=%PROFILES_PATH%\build_clang-cl_crtdll.txt ^
    --build=missing ^
    -s compiler.runtime=static ^
    -s build_type=Release ^
    -c tools.cmake.cmaketoolchain:user_presets="" ^
    ..
