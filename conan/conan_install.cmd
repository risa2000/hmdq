conan install -if out/conan/x64-DLL-Debug -pr:h ./conan/conan.profile.txt -pr:b ./conan/conan.profile.txt -s compiler.runtime=dynamic -s build_type=Debug .
conan install -if out/conan/x64-DLL-Release -pr:h ./conan/conan.profile.txt -pr:b ./conan/conan.profile.txt -s compiler.runtime=dynamic -s build_type=Release .
conan install -if out/conan/x64-Static-Debug -pr:h ./conan/conan.profile.txt -pr:b ./conan/conan.profile.txt -s compiler.runtime=static -s build_type=Debug .
conan install -if out/conan/x64-Static-Release -pr:h ./conan/conan.profile.txt -pr:b ./conan/conan.profile.txt -s compiler.runtime=static -s build_type=Release .
conan install -if out/conan/x64-xClang-Static-Debug -pr:h ./conan/conan.profile.txt -pr:b ./conan/conan.profile.txt -s compiler.runtime=static -s build_type=Debug .
conan install -if out/conan/x64-xClang-Static-Release -pr:h ./conan/conan.profile.txt -pr:b ./conan/conan.profile.txt -s compiler.runtime=static -s build_type=Release .
