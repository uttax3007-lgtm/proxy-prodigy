#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

namespace pp {

class History;

enum class Color { Reset, Blue, Cyan, Green, Red, Yellow, Gray, White };

// All console I/O goes through here so the rest of the code is testable with
// plain streams. By default colour and interactivity are OFF; main() calls
// enableConsoleFeatures() to switch on the real console (UTF-8, VT colours,
// raw line editor).
class Terminal {
public:
    Terminal(std::ostream& out, std::ostream& err) : out_(out), err_(err) {}

    // Platform specific (terminal_win.cpp / terminal_posix.cpp).
    void enableConsoleFeatures();

    bool interactive() const { return interactive_; }
    bool colorEnabled() const { return color_; }

    std::string paint(std::string_view text, Color c) const;

    void print(std::string_view s);
    void println(std::string_view s = {});
    void success(std::string_view msg);  // [SUCCESS] green
    void info(std::string_view msg);     // [INFO] cyan
    void warn(std::string_view msg);     // [WARNING] yellow
    void error(std::string_view msg);    // [ERROR] red, written to stderr
    void clearScreen();

    // Reads one line (UTF-8, no newline). nullopt = end of input (Ctrl+D / EOF).
    // An empty string means "nothing entered" (e.g. Ctrl+C cancelled the line).
    std::optional<std::string> readLine(const std::string& prompt, const History& history);

private:
    std::optional<std::string> readLineInteractive(const std::string& prompt,
                                                   const History& history);  // platform specific

    std::ostream& out_;
    std::ostream& err_;
    bool color_ = false;
    bool interactive_ = false;
};

}  // namespace pp
