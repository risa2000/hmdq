conan create -pr:h=..\..\profiles\host_msvc_v193_crtdll.txt -pr:b=..\..\profiles\build_msvc_v193_crtdll.txt --test-folder None -s compiler.runtime=static -s build_type=Debug %RECIPE% %PACKAGE%/%VERSION%@
conan create -pr:h=..\..\profiles\host_msvc_v193_crtdll.txt -pr:b=..\..\profiles\build_msvc_v193_crtdll.txt --test-folder None -s compiler.runtime=static -s build_type=Release %RECIPE% %PACKAGE%/%VERSION%@
conan create -pr:h=..\..\profiles\host_msvc_v193_crtdll.txt -pr:b=..\..\profiles\build_msvc_v193_crtdll.txt --test-folder None -s compiler.runtime=dynamic -s build_type=Debug %RECIPE% %PACKAGE%/%VERSION%@
conan create -pr:h=..\..\profiles\host_msvc_v193_crtdll.txt -pr:b=..\..\profiles\build_msvc_v193_crtdll.txt --test-folder None -s compiler.runtime=dynamic -s build_type=Release %RECIPE% %PACKAGE%/%VERSION%@
