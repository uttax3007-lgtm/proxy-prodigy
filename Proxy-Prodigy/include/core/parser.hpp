#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pp {

// Looks up a variable. The special name "?" must return the last exit code.
using EnvLookup = std::function<std::optional<std::string>(const std::string&)>;

struct ParseResult {
    bool ok = true;
    std::vector<std::string> args;  // args[0] is the command name
    std::string error;              // set when ok == false
};

// Tokenises one command line.
//
//  * whitespace separates arguments
//  * 'single quotes'  : literal, no expansion
//  * "double quotes"  : spaces kept; $VAR, ${VAR}, %VAR% expand. Backslashes are literal
//                       except directly before a closing/escaped quote (Windows rule):
//                         backslash-quote -> literal quote;  "C:\dir\\" -> C:\dir plus trailing backslash
//                       so "C:\Users\me" works and a trailing backslash is written \\"
//  * outside quotes a backslash is always literal (Windows paths)
//  * $VAR ${VAR} $?   : expanded (undefined -> empty); %VAR% expanded only if defined
//  * ~ at the start of an argument (alone or before / or \) -> home directory
//  * # at the start of an argument begins a comment
//  * unquoted | & ; < > are rejected until pipes/redirection exist (Phase 4)
ParseResult parseLine(std::string_view line, const EnvLookup& env, const std::string& home);

}  // namespace pp
