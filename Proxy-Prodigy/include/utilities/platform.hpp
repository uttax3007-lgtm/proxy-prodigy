#pragma once
// Thin OS abstraction. Implemented in src/utilities/platform_win.cpp (Win32)
// and platform_posix.cpp (development/test builds only).
#include <filesystem>
#include <optional>
#include <string>

namespace pp::platform {

std::optional<std::string> getEnv(const std::string& name);  // UTF-8
std::string homeDir();                                       // UTF-8, may be empty
std::filesystem::path configDir();                           // %APPDATA%\ProxyProdigy on Windows

}  // namespace pp::platform
