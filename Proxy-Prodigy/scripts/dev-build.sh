#!/bin/sh
# Non-Windows development build (no CMake needed). Windows users: use CMake, see README.
set -e
cd "$(dirname "$0")/.."
mkdir -p build
SRC="src/core/parser.cpp src/core/command_registry.cpp src/core/history.cpp src/core/terminal.cpp \
src/core/shell.cpp src/core/terminal_posix.cpp src/commands/core_commands.cpp src/utilities/platform_posix.cpp"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Iinclude -DPP_VERSION=\"0.1.0\" ${EXTRA_FLAGS:-}"
g++ $FLAGS $SRC src/main.cpp -o build/proxy
g++ $FLAGS $SRC tests/test_main.cpp -o build/pp_tests
echo "built build/proxy and build/pp_tests"
