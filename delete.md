$ powershell.exe -NoProfile -Command "& '$(cygpath -w ./dashboard/windows-build.bat)'"
========================================
  Inferno Dashboard Build Script
========================================

[INFO] Build type: Release

[INFO] Working directory: C:\Users\user\Documents\C++\inferno\dashboard

[OK] Qt6 found
'qmake6' n’est pas reconnu en tant que commande interne
ou externe, un programme exécutable ou un fichier de commandes.

[INFO] Building from: C:\Users\user\Documents\C++\inferno\dashboard

[INFO] Cleaning old build directory...
[INFO] Trying to force clean...
========================================
Step 1: Configuring with CMake
========================================

[INFO] Qt root: C:\Qt
[INFO] Qt libraries: C:\Qt\6.10.2\mingw_64\bin

[INFO] MinGW toolchain: C:\Qt\Tools\mingw1310_64\bin

[OK] All required tools found

[INFO] Using compilers:
  GCC:  C:\Qt\Tools\mingw1310_64\bin\gcc.exe
  G++:  C:\Qt\Tools\mingw1310_64\bin\g++.exe
  MAKE: C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe
  WINDEPLOYQT: C:\Qt\6.10.2\mingw_64\bin\windeployqt.exe

[INFO] PATH updated (temporary - only for this build)

[OK] Found MinGW OpenSSL:
     C:\msys64\mingw64

-- The CXX compiler identification is GNU 13.1.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Qt/Tools/mingw1310_64/bin/g++.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Performing Test HAVE_STDATOMIC
-- Performing Test HAVE_STDATOMIC - Success
-- Found WrapAtomic: TRUE
-- Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR) 
-- Found OpenSSL: C:/msys64/mingw64/lib/libcrypto.dll.a (found version "3.6.0")
-- Configuring done (3.2s)
-- Generating done (0.3s)
-- Build files have been written to: C:/Users/user/Documents/C++/inferno/dashboard/build
CMake returned 0

[OK] Configuration successful

========================================
Step 2: Building Client
========================================

[  0%] Built target transport_lib_autogen_timestamp_deps
[  3%] Automatic MOC and UIC for target transport_lib
[  3%] Built target transport_lib_autogen
[  6%] Building CXX object transport/CMakeFiles/transport_lib.dir/transport_lib_autogen/mocs_compilation.cpp.obj
[  9%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/frame_transport.cpp.obj
[ 12%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/convert_endian.cpp.obj
[ 15%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/protocol_parser.cpp.obj
[ 18%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/protocol_serializer.cpp.obj
[ 21%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/metrics_serializer.cpp.obj
[ 25%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/metrics_parser.cpp.obj
[ 28%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/protocol_helper.cpp.obj
[ 31%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/system_info_parser.cpp.obj
[ 34%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/codec/system_info_serializer.cpp.obj
[ 37%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/socket/socket_factory.cpp.obj
[ 40%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/socket/tls_socket.cpp.obj
[ 43%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/socket/tls_socket_factory.cpp.obj
[ 46%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/poller/poller.cpp.obj
[ 50%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/socket/windows_socket.cpp.obj
[ 53%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/env_helper.cpp.obj
C:\Users\user\Documents\C++\inferno\transport\src\poller\poller.cpp: In member function 'int Poller::indexOf(int) const':      
C:\Users\user\Documents\C++\inferno\transport\src\poller\poller.cpp:22:58: warning: comparison of integer expressions of different signedness: 'const SOCKET' {aka 'const long long unsigned in'} and 'int' [-Wsign-compare]                                     
   22 |     if (fileDescriptors_[static_cast<std::size_t>(i)].fd == fileDescriptor) {
[ 56%] Building CXX object transport/CMakeFiles/transport_lib.dir/src/logger.cpp.obj
[ 59%] Linking CXX static library libtransport_lib.a
[ 59%] Built target transport_lib
[ 59%] Built target dashboard_autogen_timestamp_deps
[ 62%] Automatic MOC and UIC for target dashboard
[ 62%] Built target dashboard_autogen
[ 65%] Building CXX object CMakeFiles/dashboard.dir/dashboard_autogen/mocs_compilation.cpp.obj
[ 68%] Building CXX object CMakeFiles/dashboard.dir/src/main.cpp.obj
[ 71%] Building CXX object CMakeFiles/dashboard.dir/src/mainwindow.cpp.obj
[ 75%] Building CXX object CMakeFiles/dashboard.dir/src/agentitemwidget.cpp.obj
[ 78%] Building CXX object CMakeFiles/dashboard.dir/src/linechartwidget.cpp.obj
[ 81%] Building CXX object CMakeFiles/dashboard.dir/src/processtablewidget.cpp.obj
[ 84%] Building CXX object CMakeFiles/dashboard.dir/src/metriccardswidget.cpp.obj
[ 87%] Building CXX object CMakeFiles/dashboard.dir/src/uiutils.cpp.obj
[ 90%] Building CXX object CMakeFiles/dashboard.dir/src/serverclient.cpp.obj
[ 93%] Building CXX object CMakeFiles/dashboard.dir/src/dashboardsession.cpp.obj
C:\Users\user\Documents\C++\inferno\dashboard\src\serverclient.cpp: In member function 'bool ServerClient::connectToServer(const QString&, quint16)':                                           
C:\Users\user\Documents\C++\inferno\dashboard\src\serverclient.cpp:53:24: error: 'QCoreApplication' has not been declared      
   53 |     QString certPath = QCoreApplication::applicationDirPath() + "/../../certs/ca.crt";
      |                        ^~~~~~~~~~~~~~~~                  
C:\Users\user\Documents\C++\inferno\dashboard\src\serverclient.cpp:54:45: error: cannot convert 'QString' to 'const std::string&' {aka 'const std::__cxx11::basic_string<char>&'}               
   54 |     socket = TLSSocketFactory::createClient(certPath);   
      |                                             ^~~~~~~~     
      |                                             |            
      |                                             QString      
In file included from C:\Users\user\Documents\C++\inferno\dashboard\src\serverclient.cpp:14:                                   
C:/Users/user/Documents/C++/inferno/transport/include/socket/tls_socket_factory.hpp:7:69: note:   initializing argument 1 of 'static std::unique_ptr<ISocket> TLSSocketFactory::createClient(const std::string&)'                                                
    7 |     static std::unique_ptr<ISocket> createClient(const std::string& caFile);                                              
      |                                                  ~~~~~~~~~~~~~~~~~~~^~~~~~                                                
mingw32-make[2]: *** [CMakeFiles\dashboard.dir\build.make:203: CMakeFiles/dashboard.dir/src/serverclient.cpp.obj] Error 1
mingw32-make[1]: *** [CMakeFiles\Makefile2:105: CMakeFiles/dashboard.dir/all] Error 2
mingw32-make: *** [Makefile:135: all] Error 2

[FAILED] Build failed
Check the errors above

Appuyez sur une touche pour continuer...