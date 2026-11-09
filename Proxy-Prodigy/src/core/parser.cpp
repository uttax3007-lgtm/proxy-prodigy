#include "core/parser.hpp"

#include <cctype>

namespace pp {
namespace {

bool isIdentStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool isIdentChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

bool validBracedName(const std::string& n) {
    if (n.empty()) return false;
    for (char c : n)
        if (c == ' ' || c == '\t' || c == '"' || c == '\'' || c == '$' || c == '{') return false;
    return true;
}

bool expandDollar(std::string_view s, size_t& i, std::string& out, const EnvLookup& env,
                  std::string& err) {
    const size_t n = s.size();
    if (i + 1 < n && s[i + 1] == '{') {
        size_t close = s.find('}', i + 2);
        if (close == std::string_view::npos) {
            err = "Unterminated '${' (missing '}')";
            return false;
        }
        std::string name(s.substr(i + 2, close - (i + 2)));
        if (!validBracedName(name)) {
            err = "Invalid variable name in '${...}'";
            return false;
        }
        if (auto v = env(name)) out += *v;
        i = close + 1;
        return true;
    }
    if (i + 1 < n && s[i + 1] == '?') {
        if (auto v = env("?")) out += *v;
        i += 2;
        return true;
    }
    if (i + 1 < n && isIdentStart(s[i + 1])) {
        size_t j = i + 1;
        while (j < n && isIdentChar(s[j])) ++j;
        if (auto v = env(std::string(s.substr(i + 1, j - i - 1)))) out += *v;
        i = j;
        return true;
    }
    out += '$';  // lone '$' is literal
    ++i;
    return true;
}

void expandPercent(std::string_view s, size_t& i, std::string& out, const EnvLookup& env) {
    size_t close = s.find('%', i + 1);
    if (close != std::string_view::npos && close > i + 1) {
        std::string name(s.substr(i + 1, close - i - 1));
        bool plausible = true;
        for (char c : name)
            if (c == ' ' || c == '\t' || c == '"' || c == '\'' || c == '$') plausible = false;
        if (plausible) {
            if (auto v = env(name)) {
                out += *v;
                i = close + 1;
                return;
            }
        }
    }
    out += '%';  // undefined or malformed: keep literally (matches CMD behaviour)
    ++i;
}

}  // namespace

ParseResult parseLine(std::string_view s, const EnvLookup& env, const std::string& home) {
    ParseResult r;
    std::string cur;
    bool inToken = false;
    const size_t n = s.size();
    size_t i = 0;

    auto flush = [&] {
        if (inToken) {
            r.args.push_back(cur);
            cur.clear();
            inToken = false;
        }
    };
    auto fail = [&](std::string msg) {
        r.ok = false;
        r.args.clear();
        r.error = std::move(msg);
        return r;
    };

    while (i < n) {
        const char c = s[i];

        if (c == ' ' || c == '\t') {
            flush();
            ++i;
            continue;
        }
        if (c == '#' && !inToken) break;  // comment

        if (c == '\'') {
            size_t close = s.find('\'', i + 1);
            if (close == std::string_view::npos) return fail("Unterminated single quote");
            cur.append(s.substr(i + 1, close - i - 1));
            inToken = true;
            i = close + 1;
            continue;
        }

        if (c == '"') {
            inToken = true;
            ++i;
            bool closed = false;
            while (i < n) {
                const char d = s[i];
                if (d == '"') {
                    closed = true;
                    ++i;
                    break;
                }
                if (d == '\\') {
                    // Windows rule (same as CommandLineToArgvW), only for backslashes that
                    // run up to a double quote: 2n backslashes -> n backslashes and the quote
                    // closes the string; 2n+1 -> n backslashes and a literal quote.
                    // Backslashes anywhere else are literal ("C:\Users\me" just works).
                    size_t k = 0;
                    while (i + k < n && s[i + k] == '\\') ++k;
                    if (i + k < n && s[i + k] == '"') {
                        cur.append(k / 2, '\\');
                        if (k % 2 == 1) {
                            cur += '"';
                            i += k + 1;
                        } else {
                            i += k;  // leave the quote for the closing branch
                        }
                    } else {
                        cur.append(k, '\\');
                        i += k;
                    }
                } else if (d == '$') {
                    std::string err;
                    if (!expandDollar(s, i, cur, env, err)) return fail(err);
                } else if (d == '%') {
                    expandPercent(s, i, cur, env);
                } else {
                    cur += d;
                    ++i;
                }
            }
            if (!closed) return fail("Unterminated double quote");
            continue;
        }

        if (c == '|' || c == '&' || c == ';' || c == '<' || c == '>') {
            return fail(std::string("Unsupported operator '") + c +
                        "' (pipes, redirection and command chaining arrive in a later phase). "
                        "Quote it to use it literally.");
        }

        if (c == '~' && !inToken && !home.empty() &&
            (i + 1 == n || s[i + 1] == '/' || s[i + 1] == '\\' || s[i + 1] == ' ' ||
             s[i + 1] == '\t')) {
            cur += home;
            inToken = true;
            ++i;
            continue;
        }

        if (c == '$') {
            std::string err;
            if (!expandDollar(s, i, cur, env, err)) return fail(err);
            inToken = true;
            continue;
        }
        if (c == '%') {
            expandPercent(s, i, cur, env);
            inToken = true;
            continue;
        }

        cur += c;
        inToken = true;
        ++i;
    }
    flush();
    return r;
}

}  // namespace pp
