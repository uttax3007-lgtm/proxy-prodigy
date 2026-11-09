#pragma once
#include <filesystem>
#include <string>

#include "core/command_registry.hpp"
#include "core/history.hpp"
#include "core/terminal.hpp"

#ifndef PP_VERSION
#define PP_VERSION "0.1.0"
#endif

namespace pp {

// Mutable state shared with command handlers.
struct Context {
    Terminal& term;
    History& history;
    CommandRegistry& registry;
    std::filesystem::path previousDir;  // for `cd -`
    int lastExit = 0;
    bool running = true;
    int exitCode = 0;
};

class Shell {
public:
    Shell(Terminal& term, History& history, CommandRegistry& registry);

    // Parses and runs one line. Returns the exit code (also stored in lastExit).
    // 2 = parse/usage error, 127 = command not found.
    int execute(const std::string& line);

    // Interactive (or piped-stdin) read-eval loop. Returns the process exit code.
    int runInteractive();

    std::string buildPrompt() const;
    Context& context() { return ctx_; }

private:
    void printBanner();
    Context ctx_;
};

}  // namespace pp
