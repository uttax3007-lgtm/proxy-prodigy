#include "commands/core_commands.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <iomanip>
#include <map>
#include <sstream>

#include "core/shell.hpp"
#include "utilities/platform.hpp"
#include "utilities/strings.hpp"

namespace pp {
namespace {

namespace fs = std::filesystem;

std::string buildInfo() {
    std::ostringstream o;
#if defined(_MSC_VER)
    o << "MSVC " << _MSC_VER;
#elif defined(__clang__)
    o << "clang " << __clang_major__ << '.' << __clang_minor__;
#elif defined(__GNUC__)
    o << "gcc " << __GNUC__ << '.' << __GNUC_MINOR__;
#else
    o << "unknown compiler";
#endif
    o << ", C++" << (__cplusplus / 100 % 100) << ", ";
#ifdef _WIN32
    o << "Windows";
#else
    o << "non-Windows dev build";
#endif
    return o.str();
}

// ---------------------------------------------------------------- help
int cmdHelp(Context& c, const Args& a) {
    Terminal& t = c.term;
    if (a.size() > 2) {
        t.error("help: too many arguments");
        return 2;
    }
    if (a.size() == 2) {
        const CommandInfo* cmd = c.registry.find(a[1]);
        if (!cmd) {
            t.error("help: no such command: " + a[1]);
            return 1;
        }
        t.println(t.paint(cmd->name, Color::Blue) + "  -  " + cmd->summary);
        t.println("Usage:   " + cmd->usage);
        if (!cmd->aliases.empty()) {
            std::string al;
            for (const auto& x : cmd->aliases) al += (al.empty() ? "" : ", ") + x;
            t.println("Aliases: " + al);
        }
        return 0;
    }

    std::map<std::string, std::vector<const CommandInfo*>> byCat;
    size_t width = 0;
    for (const CommandInfo* ci : c.registry.all()) {
        byCat[ci->category].push_back(ci);
        width = std::max(width, ci->name.size());
    }
    t.println(t.paint("PROXY PRODIGY", Color::Blue) + " v" PP_VERSION " - available commands");
    for (const auto& [cat, list] : byCat) {
        t.println();
        t.println(t.paint(cat, Color::Cyan));
        for (const CommandInfo* ci : list) {
            std::string pad(width - ci->name.size() + 2, ' ');
            t.println("  " + ci->name + pad + ci->summary);
        }
    }
    t.println();
    t.println("Type 'help <command>' for usage details.");
    return 0;
}

// ---------------------------------------------------------------- version
int cmdVersion(Context& c, const Args& a) {
    if (a.size() > 1) {
        c.term.error("version: takes no arguments");
        return 2;
    }
    c.term.println(c.term.paint("PROXY PRODIGY", Color::Blue) + " v" PP_VERSION);
    c.term.println("Build: " + buildInfo());
    return 0;
}

// ---------------------------------------------------------------- clear
int cmdClear(Context& c, const Args& a) {
    if (a.size() > 1) {
        c.term.error(a[0] + ": takes no arguments");
        return 2;
    }
    c.term.clearScreen();
    return 0;
}

// ---------------------------------------------------------------- exit
int cmdExit(Context& c, const Args& a) {
    if (a.size() > 2) {
        c.term.error("exit: too many arguments");
        return 2;
    }
    int code = c.lastExit;  // like bash: no argument -> status of the last command
    if (a.size() == 2) {
        const std::string& s = a[1];
        auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), code);
        if (ec != std::errc{} || p != s.data() + s.size() || code < 0) {
            c.term.error("exit: invalid exit code: " + s);
            return 2;
        }
    }
    c.running = false;
    c.exitCode = code;
    return code;
}

// ---------------------------------------------------------------- pwd
int cmdPwd(Context& c, const Args& a) {
    if (a.size() > 1) {
        c.term.error("pwd: takes no arguments");
        return 2;
    }
    std::error_code ec;
    fs::path p = fs::current_path(ec);
    if (ec) {
        c.term.error("pwd: cannot determine current directory: " + ec.message());
        return 1;
    }
    c.term.println(pathToUtf8(p));
    return 0;
}

// ---------------------------------------------------------------- cd
int cmdCd(Context& c, const Args& a) {
    Terminal& t = c.term;
    std::vector<std::string> v(a.begin() + 1, a.end());
    if (!v.empty() && (v[0] == "/d" || v[0] == "/D")) v.erase(v.begin());  // CMD: "cd /d D:\x"

    if (v.size() > 1) {
        t.error("cd: too many arguments (quote paths that contain spaces)");
        return 2;
    }

    fs::path target;
    bool announce = false;
    if (v.empty()) {
        std::string home = platform::homeDir();
        if (home.empty()) {
            t.error("cd: home directory is not set");
            return 1;
        }
        target = pathFromUtf8(home);
    } else if (v[0] == "-") {
        if (c.previousDir.empty()) {
            t.error("cd: no previous directory");
            return 1;
        }
        target = c.previousDir;
        announce = true;
    } else {
        target = pathFromUtf8(v[0]);
    }

    std::error_code ec;
    fs::path before = fs::current_path(ec);

    bool exists = fs::exists(target, ec);
    if (ec) {
        t.error("cd: " + pathToUtf8(target) + ": " + ec.message());
        return 1;
    }
    if (!exists) {
        t.error("cd: no such directory: " + pathToUtf8(target));
        return 1;
    }
    if (!fs::is_directory(target, ec)) {
        t.error("cd: not a directory: " + pathToUtf8(target));
        return 1;
    }
    fs::current_path(target, ec);
    if (ec) {
        if (ec == std::errc::permission_denied)
            t.error("cd: access denied: " + pathToUtf8(target));
        else
            t.error("cd: " + pathToUtf8(target) + ": " + ec.message());
        return 1;
    }
    c.previousDir = before;
    if (announce) {
        fs::path now = fs::current_path(ec);
        if (!ec) t.println(pathToUtf8(now));
    }
    return 0;
}

// ---------------------------------------------------------------- echo
int cmdEcho(Context& c, const Args& a) {
    size_t first = 1;
    bool newline = true;
    if (a.size() > 1 && a[1] == "-n") {
        newline = false;
        first = 2;
    }
    std::string out;
    for (size_t i = first; i < a.size(); ++i) {
        if (i > first) out += ' ';
        out += a[i];
    }
    if (newline) c.term.println(out);
    else c.term.print(out);
    return 0;
}

// ---------------------------------------------------------------- history
int cmdHistory(Context& c, const Args& a) {
    Terminal& t = c.term;
    if (a.size() > 2) {
        t.error("history: too many arguments");
        return 2;
    }
    if (a.size() == 2 && a[1] == "-c") {
        c.history.clear();
        t.success("History cleared");
        return 0;
    }
    const auto& e = c.history.entries();
    size_t count = e.size();
    if (a.size() == 2) {
        size_t n = 0;
        auto [p, ec] = std::from_chars(a[1].data(), a[1].data() + a[1].size(), n);
        if (ec != std::errc{} || p != a[1].data() + a[1].size()) {
            t.error("history: invalid argument: " + a[1] + " (expected a number or -c)");
            return 2;
        }
        count = std::min(n, e.size());
    }
    const size_t start = e.size() - count;
    const int w = static_cast<int>(std::to_string(e.size()).size());
    for (size_t i = start; i < e.size(); ++i) {
        std::ostringstream o;
        o << std::setw(w + 2) << (i + 1) << "  " << e[i];
        t.println(o.str());
    }
    return 0;
}

}  // namespace

void registerCoreCommands(CommandRegistry& r) {
    r.add({"help", "Shell", "List commands or show help for one", "help [command]", {}, cmdHelp});
    r.add({"version", "Shell", "Show the Proxy Prodigy version", "version", {}, cmdVersion});
    r.add({"clear", "Shell", "Clear the terminal screen", "clear", {"cls"}, cmdClear});
    r.add({"exit", "Shell", "Exit the terminal", "exit [code]", {"quit"}, cmdExit});
    r.add({"history", "Shell", "Show or clear command history", "history [N | -c]", {}, cmdHistory});
    r.add({"pwd", "Navigation", "Print the current directory", "pwd", {}, cmdPwd});
    r.add({"cd", "Navigation", "Change the current directory",
           "cd [dir | -]   (no argument: home; '-': previous; Windows: 'cd /d D:\\path' accepted)",
           {}, cmdCd});
    r.add({"echo", "Output", "Print text", "echo [-n] [text...]", {}, cmdEcho});
}

}  // namespace pp
