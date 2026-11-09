// Windows console support: UTF-8, VT colours and an interactive line editor
// (history with Up/Down, cursor movement, Ctrl+C/D/L/A/E/U/K).
#ifdef _WIN32
#include <iostream>
#include <string>

#include "core/history.hpp"
#include "core/terminal.hpp"
#include "utilities/platform.hpp"
#include "utilities/win_strings.hpp"

namespace pp {

void Terminal::enableConsoleFeatures() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    DWORD outMode = 0;
    const bool outIsConsole = hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &outMode);
    bool vt = false;
    if (outIsConsole) {
        vt = SetConsoleMode(hOut, outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        SetConsoleTitleW(L"PROXY PRODIGY");
    }

    DWORD inMode = 0;
    const bool inIsConsole = hIn != INVALID_HANDLE_VALUE && GetConsoleMode(hIn, &inMode);

    interactive_ = outIsConsole && inIsConsole;
    color_ = vt && !platform::getEnv("NO_COLOR");
}

namespace {

void writeW(HANDLE h, const std::wstring& s) {
    DWORD written = 0;
    WriteConsoleW(h, s.data(), static_cast<DWORD>(s.size()), &written, nullptr);
}

bool isLowSurrogate(wchar_t c) { return c >= 0xDC00 && c <= 0xDFFF; }
bool isHighSurrogate(wchar_t c) { return c >= 0xD800 && c <= 0xDBFF; }

}  // namespace

std::optional<std::string> Terminal::readLineInteractive(const std::string& prompt,
                                                         const History& history) {
    out_.flush();
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD oldMode = 0;
    GetConsoleMode(hIn, &oldMode);
    // Raw-ish mode for the duration of this call only: we do our own echo/editing and
    // receive Ctrl+C as a key (so a typo'd line can be cancelled without killing the shell).
    SetConsoleMode(hIn, oldMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT));
    struct Restore {
        HANDLE h;
        DWORD m;
        ~Restore() { SetConsoleMode(h, m); }
    } restore{hIn, oldMode};

    const std::wstring wprompt = win::utf8ToWide(prompt);
    std::wstring buf;
    size_t pos = 0;
    const auto& entries = history.entries();
    size_t histIdx = entries.size();  // == size() means "the line being typed"
    std::wstring draft;

    auto redraw = [&] {
        std::wstring s = L"\r" + wprompt + buf + L"\x1b[K";
        const size_t back = buf.size() - pos;
        if (back > 0) s += L"\x1b[" + std::to_wstring(back) + L"D";
        writeW(hOut, s);
    };
    auto prevPos = [&](size_t p) {
        if (p == 0) return p;
        --p;
        if (p > 0 && isLowSurrogate(buf[p]) && isHighSurrogate(buf[p - 1])) --p;
        return p;
    };
    auto nextPos = [&](size_t p) {
        if (p >= buf.size()) return p;
        if (isHighSurrogate(buf[p]) && p + 1 < buf.size() && isLowSurrogate(buf[p + 1])) return p + 2;
        return p + 1;
    };
    auto recall = [&](size_t idx) {
        if (idx == entries.size()) {
            buf = draft;
        } else {
            buf = win::utf8ToWide(entries[idx]);
        }
        histIdx = idx;
        pos = buf.size();
    };

    redraw();

    INPUT_RECORD rec;
    DWORD got = 0;
    for (;;) {
        if (!ReadConsoleInputW(hIn, &rec, 1, &got) || got == 0) return std::nullopt;
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) continue;

        const KEY_EVENT_RECORD& k = rec.Event.KeyEvent;
        const WORD vk = k.wVirtualKeyCode;
        const wchar_t ch = k.uChar.UnicodeChar;

        if (vk == VK_RETURN) {
            writeW(hOut, L"\r\n");
            return win::wideToUtf8(buf);
        }
        if (ch == 3) {  // Ctrl+C: cancel the current line
            writeW(hOut, L"^C\r\n");
            return std::string();
        }
        if (ch == 4) {  // Ctrl+D: EOF on an empty line, delete-char otherwise
            if (buf.empty()) return std::nullopt;
            if (pos < buf.size()) buf.erase(pos, nextPos(pos) - pos);
            redraw();
            continue;
        }
        if (ch == 12) {  // Ctrl+L
            writeW(hOut, L"\x1b[2J\x1b[3J\x1b[H");
            redraw();
            continue;
        }
        if (ch == 1 || vk == VK_HOME) { pos = 0; redraw(); continue; }
        if (ch == 5 || vk == VK_END) { pos = buf.size(); redraw(); continue; }
        if (ch == 21) { buf.erase(0, pos); pos = 0; redraw(); continue; }  // Ctrl+U
        if (ch == 11) { buf.erase(pos); redraw(); continue; }              // Ctrl+K

        switch (vk) {
            case VK_BACK:
                if (pos > 0) {
                    size_t p = prevPos(pos);
                    buf.erase(p, pos - p);
                    pos = p;
                    redraw();
                }
                continue;
            case VK_DELETE:
                if (pos < buf.size()) {
                    buf.erase(pos, nextPos(pos) - pos);
                    redraw();
                }
                continue;
            case VK_LEFT: pos = prevPos(pos); redraw(); continue;
            case VK_RIGHT: pos = nextPos(pos); redraw(); continue;
            case VK_UP:
                if (histIdx > 0) {
                    if (histIdx == entries.size()) draft = buf;
                    recall(histIdx - 1);
                    redraw();
                }
                continue;
            case VK_DOWN:
                if (histIdx < entries.size()) {
                    recall(histIdx + 1);
                    redraw();
                }
                continue;
            default: break;
        }

        if (ch >= 32 && ch != 127) {  // printable (including UTF-16 surrogate halves)
            buf.insert(buf.begin() + static_cast<std::ptrdiff_t>(pos), ch);
            ++pos;
            if (histIdx != entries.size()) histIdx = entries.size();  // editing a recalled line
            redraw();
        }
        // Tab and other control keys are ignored for now (autocomplete: later phase).
    }
}

}  // namespace pp
#endif
