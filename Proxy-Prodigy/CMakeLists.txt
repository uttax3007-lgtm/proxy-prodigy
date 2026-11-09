cmake_minimum_required(VERSION 3.20)
project(ProxyProdigy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- core library: everything except main() ------------------------------
set(PP_CORE_SOURCES
  src/core/parser.cpp
  src/core/command_registry.cpp
  src/core/history.cpp
  src/core/terminal.cpp
  src/core/shell.cpp
  src/commands/core_commands.cpp
  src/utilities/platform_win.cpp     # contents guarded by #ifdef _WIN32
  src/utilities/platform_posix.cpp   # contents guarded by #ifndef _WIN32
  src/core/terminal_win.cpp
  src/core/terminal_posix.cpp)

add_library(pp_core STATIC ${PP_CORE_SOURCES})
target_include_directories(pp_core PUBLIC include)
target_compile_definitions(pp_core PUBLIC PP_VERSION="${PROJECT_VERSION}")

if(WIN32)
  target_compile_definitions(pp_core PUBLIC
    UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
endif()

if(MSVC)
  target_compile_options(pp_core PUBLIC /W4 /permissive- /utf-8 /EHsc)
else()
  target_compile_options(pp_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

# ---- executable: proxy(.exe) ---------------------------------------------
add_executable(proxy src/main.cpp)
target_link_libraries(proxy PRIVATE pp_core)
if(MINGW)
  target_link_options(proxy PRIVATE -municode)
endif()

# ---- tests ---------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
  add_executable(pp_tests tests/test_main.cpp)
  target_link_libraries(pp_tests PRIVATE pp_core)
  add_test(NAME pp_tests COMMAND pp_tests)
endif()

install(TARGETS proxy RUNTIME DESTINATION bin)
