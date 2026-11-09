#include "core/history.hpp"

#include <fstream>

#include "utilities/strings.hpp"

namespace pp {

void History::load() {
    entries_.clear();
    fileLines_ = 0;
    if (file_.empty()) return;
    std::ifstream in(file_, std::ios::binary);
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        entries_.push_back(line);
        ++fileLines_;
    }
    if (entries_.size() > limit_) {
        entries_.erase(entries_.begin(), entries_.end() - static_cast<std::ptrdiff_t>(limit_));
        rewriteFile();
    }
}

void History::add(const std::string& line) {
    if (isBlank(line)) return;
    if (line.find('\n') != std::string::npos || line.find('\r') != std::string::npos) return;
    if (!entries_.empty() && entries_.back() == line) return;
    entries_.push_back(line);
    if (entries_.size() > limit_) entries_.erase(entries_.begin());
    appendToFile(line);
    if (fileLines_ > limit_ * 2) rewriteFile();  // keep the file from growing forever
}

void History::clear() {
    entries_.clear();
    rewriteFile();
}

void History::appendToFile(const std::string& line) {
    if (file_.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(file_.parent_path(), ec);
    std::ofstream out(file_, std::ios::binary | std::ios::app);
    if (!out) return;
    out << line << '\n';
    ++fileLines_;
}

void History::rewriteFile() {
    fileLines_ = 0;
    if (file_.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(file_.parent_path(), ec);
    std::ofstream out(file_, std::ios::binary | std::ios::trunc);
    if (!out) return;
    for (const auto& e : entries_) {
        out << e << '\n';
        ++fileLines_;
    }
}

}  // namespace pp
