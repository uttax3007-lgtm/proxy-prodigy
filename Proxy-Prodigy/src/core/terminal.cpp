#include "core/terminal.hpp"

#include <iostream>

#include "core/history.hpp"

namespace pp {

namespace {
const char* code(Color c) {
    switch (c) {
        case Color::Blue: return "\x1b[1;38;5;39m";  // electric blue
        case Color::Cyan: return "\x1b[38;5;51m";
        case Color::Green: return "\x1b[38;5;41m";
        case Color::Red: return "\x1b[38;5;203m";
        case Color::Yellow: return "\x1b[38;5;220m";
        case Color::Gray: return "\x1b[38;5;245m";
        case Color::White: return "\x1b[38;5;255m";
        case Color::Reset: break;
    }
    return "\x1b[0m";
}
}  // namespace

std::string Terminal::paint(std::string_view text, Color c) const {
    if (!color_) return std::string(text);
    std::string s = code(c);
    s.append(text);
    s += "\x1b[0m";
    return s;
}

void Terminal::print(std::string_view s) { out_ << s; }
void Terminal::println(std::string_view s) { out_ << s << '\n'; }

void Terminal::success(std::string_view m) { out_ << paint("[SUCCESS] ", Color::Green) << m << '\n'; }
void Terminal::info(std::string_view m) { out_ << paint("[INFO] ", Color::Cyan) << m << '\n'; }
void Terminal::warn(std::string_view m) { out_ << paint("[WARNING] ", Color::Yellow) << m << '\n'; }
void Terminal::error(std::string_view m) {
    out_.flush();
    err_ << paint("[ERROR] ", Color::Red) << m << '\n';
    err_.flush();
}

void Terminal::clearScreen() {
    if (color_) out_ << "\x1b[2J\x1b[3J\x1b[H" << std::flush;
}

std::optional<std::string> Terminal::readLine(const std::string& prompt, const History& history) {
    if (interactive_) return readLineInteractive(prompt, history);
    // Non-interactive (piped / redirected stdin): no prompt, plain getline.
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

}  // namespace pp
