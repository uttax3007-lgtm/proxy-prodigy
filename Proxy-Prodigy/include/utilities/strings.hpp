#pragma once
// Small string / path helpers shared by every module.
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>

namespace pp {

inline std::string toLower(std::string_view s) {
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

inline bool iequals(std::string_view a, std::string_view b) {
    return a.size() == b.size() && toLower(a) == toLower(b);
}

inline bool isBlank(std::string_view s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) != 0; });
}

// All text inside Proxy Prodigy is UTF-8. These convert to/from fs::path
// correctly on Windows (UTF-16 native) and POSIX.
inline std::string pathToUtf8(const std::filesystem::path& p) {
    auto u = p.u8string();
    return std::string(u.begin(), u.end());
}

inline std::filesystem::path pathFromUtf8(std::string_view s) {
    return std::filesystem::path(std::u8string(s.begin(), s.end()));
}

}  // namespace pp
