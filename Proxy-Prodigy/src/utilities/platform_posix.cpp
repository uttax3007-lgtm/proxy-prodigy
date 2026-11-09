// POSIX implementation: used only so the portable core can be built and
// unit-tested off Windows. Proxy Prodigy itself targets Windows 10/11.
#ifndef _WIN32
#include <cstdlib>

#include "utilities/platform.hpp"

namespace pp::platform {

std::optional<std::string> getEnv(const std::string& name) {
    const char* v = std::getenv(name.c_str());
    if (!v) return std::nullopt;
    return std::string(v);
}

std::string homeDir() { return getEnv("HOME").value_or(""); }

std::filesystem::path configDir() {
    if (auto x = getEnv("XDG_CONFIG_HOME"); x && !x->empty())
        return std::filesystem::path(*x) / "ProxyProdigy";
    return std::filesystem::path(homeDir()) / ".config" / "ProxyProdigy";
}

}  // namespace pp::platform
#endif
