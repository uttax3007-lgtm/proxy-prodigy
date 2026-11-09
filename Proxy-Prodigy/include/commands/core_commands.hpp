#pragma once
#include "core/command_registry.hpp"

namespace pp {
// Phase 1: help, version, clear, exit, pwd, cd, echo, history.
void registerCoreCommands(CommandRegistry& registry);
}  // namespace pp
