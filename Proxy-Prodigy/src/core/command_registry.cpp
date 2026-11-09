#include "core/command_registry.hpp"

#include "utilities/strings.hpp"

namespace pp {

bool CommandRegistry::add(CommandInfo info) {
    info.name = toLower(info.name);
    if (info.name.empty() || !info.handler) return false;
    if (commands_.count(info.name) || aliasToName_.count(info.name)) return false;
    for (auto& a : info.aliases) {
        a = toLower(a);
        if (a == info.name || commands_.count(a) || aliasToName_.count(a)) return false;
    }
    for (const auto& a : info.aliases) aliasToName_[a] = info.name;
    commands_.emplace(info.name, std::move(info));
    return true;
}

const CommandInfo* CommandRegistry::find(std::string_view nameOrAlias) const {
    std::string key = toLower(nameOrAlias);
    if (auto it = commands_.find(key); it != commands_.end()) return &it->second;
    if (auto a = aliasToName_.find(key); a != aliasToName_.end()) return &commands_.at(a->second);
    return nullptr;
}

std::vector<const CommandInfo*> CommandRegistry::all() const {
    std::vector<const CommandInfo*> v;
    v.reserve(commands_.size());
    for (const auto& kv : commands_) v.push_back(&kv.second);
    return v;
}

}  // namespace pp
