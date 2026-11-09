#pragma once
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pp {

struct Context;
using Args = std::vector<std::string>;  // args[0] is the name the user typed
using Handler = std::function<int(Context&, const Args&)>;

struct CommandInfo {
    std::string name;      // primary name, lower-case
    std::string category;  // used by `help`
    std::string summary;   // one line
    std::string usage;     // e.g. "cd [dir]"
    std::vector<std::string> aliases;
    Handler handler;
};

class CommandRegistry {
public:
    // Returns false (and registers nothing) if the name or any alias is taken.
    bool add(CommandInfo info);
    // Case-insensitive; resolves aliases.
    const CommandInfo* find(std::string_view nameOrAlias) const;
    std::vector<const CommandInfo*> all() const;  // sorted by name
    size_t size() const { return commands_.size(); }

private:
    std::map<std::string, CommandInfo> commands_;
    std::map<std::string, std::string> aliasToName_;
};

}  // namespace pp
