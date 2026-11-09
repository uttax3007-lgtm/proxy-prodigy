#ifdef _WIN32
#include "utilities/platform.hpp"
#include "utilities/strings.hpp"
#include "utilities/win_strings.hpp"

namespace pp::platform {

std::optional<std::string> getEnv(const std::string& name) {
    std::wstring wname = win::utf8ToWide(name);
    DWORD need = GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
    if (need == 0) {
        // 0 means "not found" or "set but empty"; GetLastError tells them apart.
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) return std::nullopt;
        return std::string();
    }
    std::wstring buf(need, L'\0');
    DWORD got = GetEnvironmentVariableW(wname.c_str(), buf.data(), need);
    buf.resize(got);
    return win::wideToUtf8(buf);
}

std::string homeDir() {
    if (auto p = getEnv("USERPROFILE"); p && !p->empty()) return *p;
    auto d = getEnv("HOMEDRIVE");
    auto h = getEnv("HOMEPATH");
    if (d && h) return *d + *h;
    return {};
}

std::filesystem::path configDir() {
    if (auto a = getEnv("APPDATA"); a && !a->empty()) return pathFromUtf8(*a) / "ProxyProdigy";
    return pathFromUtf8(homeDir()) / ".proxyprodigy";
}

}  // namespace pp::platform
#endif
