conan install -if out/conan/x64-DLL-Debug -pr:h default -pr:b default -s compiler.runtime=MDd -s build_type=Debug .
conan install -if out/conan/x64-DLL-Release -pr:h default -pr:b default -s compiler.runtime=MD -s build_type=Release .
conan install -if out/conan/x64-Static-Debug -pr:h default -pr:b default -s compiler.runtime=MTd -s build_type=Debug .
conan install -if out/conan/x64-Static-Release -pr:h default -pr:b default -s compiler.runtime=MT -s build_type=Release .
conan install -if out/conan/x64-xClang-Static-Debug -pr:h default -pr:b default -s compiler.runtime=MTd -s build_type=Debug .
conan install -if out/conan/x64-xClang-Static-Release -pr:h default -pr:b default -s compiler.runtime=MT -s build_type=Release .
