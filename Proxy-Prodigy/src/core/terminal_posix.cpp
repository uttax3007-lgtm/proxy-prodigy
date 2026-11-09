// POSIX console support: development/test builds only (no line editing).
#ifndef _WIN32
#include <unistd.h>

#include <iostream>

#include "core/history.hpp"
#include "core/terminal.hpp"
#include "utilities/platform.hpp"

namespace pp {

void Terminal::enableConsoleFeatures() {
    interactive_ = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    color_ = isatty(STDOUT_FILENO) && !platform::getEnv("NO_COLOR");
}

std::optional<std::string> Terminal::readLineInteractive(const std::string& prompt,
                                                         const History&) {
    out_ << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    return line;
}

}  // namespace pp
#endif
