#include "core/shell.hpp"

#include <exception>

#include "core/parser.hpp"
#include "utilities/platform.hpp"
#include "utilities/strings.hpp"

namespace pp {

Shell::Shell(Terminal& term, History& history, CommandRegistry& registry)
    : ctx_{term, history, registry, {}, 0, true, 0} {}

int Shell::execute(const std::string& line) {
    auto lookup = [this](const std::string& name) -> std::optional<std::string> {
        if (name == "?") return std::to_string(ctx_.lastExit);
        return platform::getEnv(name);
    };
    ParseResult pr = parseLine(line, lookup, platform::homeDir());
    if (!pr.ok) {
        ctx_.term.error(pr.error);
        return ctx_.lastExit = 2;
    }
    if (pr.args.empty()) return ctx_.lastExit;  // blank line or comment: keep previous status

    const CommandInfo* cmd = ctx_.registry.find(pr.args[0]);
    if (!cmd) {
        ctx_.term.error("Command not found: " + pr.args[0]);
        return ctx_.lastExit = 127;
    }

    int rc = 1;
    try {
        rc = cmd->handler(ctx_, pr.args);
    } catch (const std::exception& e) {
        ctx_.term.error("Internal error in '" + cmd->name + "': " + e.what());
        rc = 1;
    } catch (...) {
        ctx_.term.error("Internal error in '" + cmd->name + "'");
        rc = 1;
    }
    return ctx_.lastExit = rc;
}

std::string Shell::buildPrompt() const {
    namespace fs = std::filesystem;
    std::error_code ec;
    std::string cwd = pathToUtf8(fs::current_path(ec));
    if (ec) cwd = "?";

    std::string shown = cwd;
    std::string home = platform::homeDir();
    while (home.size() > 1 && (home.back() == '\\' || home.back() == '/')) home.pop_back();
    if (!home.empty() && cwd.size() >= home.size()) {
#ifdef _WIN32
        const bool prefix = iequals(std::string_view(cwd).substr(0, home.size()), home);
#else
        const bool prefix = cwd.compare(0, home.size(), home) == 0;
#endif
        if (prefix && (cwd.size() == home.size() || cwd[home.size()] == '\\' ||
                       cwd[home.size()] == '/')) {
            shown = "~" + cwd.substr(home.size());
        }
    }

    Terminal& t = ctx_.term;
    return t.paint("proxy@prodigy", Color::Blue) + t.paint(":", Color::White) +
           t.paint(shown, Color::Cyan) + t.paint("$ ", Color::White);
}

void Shell::printBanner() {
    Terminal& t = ctx_.term;
    t.println(t.paint("  PROXY PRODIGY", Color::Blue) + t.paint("  v" PP_VERSION, Color::Gray));
    t.println(t.paint("  One Terminal. Unlimited Control.", Color::Cyan));
    t.println(t.paint("  Type 'help' for the list of commands.", Color::Gray));
    t.println();
}

int Shell::runInteractive() {
    if (ctx_.term.interactive()) printBanner();
    while (ctx_.running) {
        auto line = ctx_.term.readLine(buildPrompt(), ctx_.history);
        if (!line) {  // EOF / Ctrl+D
            if (ctx_.term.interactive()) ctx_.term.println();
            ctx_.exitCode = ctx_.lastExit;
            break;
        }
        if (isBlank(*line)) continue;
        ctx_.history.add(*line);
        execute(*line);
    }
    return ctx_.exitCode;
}

}  // namespace pp
