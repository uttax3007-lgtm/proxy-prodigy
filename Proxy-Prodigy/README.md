# PROXY PRODIGY
**One Terminal. Unlimited Control.** - a Windows-native CLI (C++20, Win32). Developer: Utkarsh J.

**Status: Phase 1 (Core Terminal)** - interactive shell, parser, command registry, history,
`help version clear exit history pwd cd echo`. See `docs/COMMANDS.md`.
Phases 2-8 (filesystem, CMD/PowerShell bridge, Linux compat, system tools, dev tools, scripting/plugins, release) are not started.

## Build (Windows 10/11)

Requirements: Visual Studio 2022 (Desktop C++ workload) or MinGW-w64 GCC 13+, CMake 3.20+.

```bat
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
build\Release\proxy.exe --version
```
(Single-config generators such as Ninja/MinGW: the exe is `build\proxy.exe`.)

Debug build: `cmake --build build --config Debug`.

## Run
```bat
proxy.exe                 :: interactive
proxy.exe -c "echo hi"    :: one command
```
To launch with just `proxy` from any terminal, add the folder containing `proxy.exe` to your user PATH
(Settings -> System -> About -> Advanced system settings -> Environment Variables -> Path -> New).
To uninstall, delete that folder, remove the PATH entry and optionally delete `%APPDATA%\ProxyProdigy\`.

## Layout
```
include/core        parser, registry, history, terminal, shell
include/commands    command registration
include/utilities   strings, platform + Win32 UTF-8 helpers
src/core            parser.cpp command_registry.cpp history.cpp shell.cpp terminal*.cpp
src/commands        core_commands.cpp
src/utilities       platform_win.cpp platform_posix.cpp
src/main.cpp        wmain / main, CLI options
tests/test_main.cpp dependency-free tests (ctest target: pp_tests)
scripts/dev-build.sh  non-Windows dev build (core only; used for CI-style checks)
```

## Known Phase 1 limitations
- Line editor: lines longer than the window width and double-width (CJK/emoji) characters redraw imperfectly.
- History is a UTF-8 text file; the spec's SQLite `history.db` is planned for a later phase.
- Config directory only holds history so far; `config.json` etc. come with the configuration phase.
- Command execution logging (spec section 13) is not implemented yet.
- External programs cannot be launched yet (Phase 3).
