#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace pp {

// Command history, persisted as UTF-8 text (one command per line).
// (SQLite-backed history.db from the spec is planned for a later phase.)
// Persistence is best-effort: I/O failures never interrupt the shell.
class History {
public:
    explicit History(size_t limit = 1000) : limit_(limit ? limit : 1) {}

    void setFile(std::filesystem::path file) { file_ = std::move(file); }
    void load();                      // reads file_, trims to limit
    void add(const std::string& line);  // ignores blank lines and immediate repeats
    void clear();                     // forgets in memory and truncates the file

    const std::vector<std::string>& entries() const { return entries_; }
    size_t limit() const { return limit_; }

private:
    void appendToFile(const std::string& line);
    void rewriteFile();

    size_t limit_;
    std::filesystem::path file_;
    std::vector<std::string> entries_;
    size_t fileLines_ = 0;
};

}  // namespace pp
