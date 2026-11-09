# PROXY PRODIGY - Command reference (Phase 1)

Only commands that are actually implemented are listed. Anything else prints
`[ERROR] Command not found`. Exit codes: `0` success, `1` runtime error,
`2` usage / parse error, `127` command not found.

| Command | Aliases | Usage | Notes |
|---|---|---|---|
| `help` | | `help [command]` | Lists commands by category, or usage + aliases for one. |
| `version` | | `version` | Version, compiler, C++ standard. |
| `clear` | `cls` | `clear` | Clears screen and scrollback (also **Ctrl+L**). |
| `exit` | `quit` | `exit [code]` | No argument = status of the last command (like bash). Code must be a non-negative integer. |
| `history` | | `history [N \| -c]` | Show all / last N entries, or clear (`-c`). Persisted in `%APPDATA%\ProxyProdigy\history.txt`, limit 1000. |
| `pwd` | | `pwd` | Prints the current directory (UTF-8). |
| `cd` | | `cd [dir \| -]` | No argument = home. `-` = previous directory. `cd /d D:\x` accepted (CMD style). Quote paths containing spaces. Drive letters, `\` and `/` both work. |
| `echo` | | `echo [-n] [text...]` | `-n` suppresses the newline. No escape-sequence processing. |

## Command-line syntax

| Syntax | Meaning |
|---|---|
| `a b` | whitespace separates arguments |
| `'text'` | literal; nothing expanded |
| `"text"` | spaces kept; `$VAR`, `${VAR}`, `%VAR%` expanded |
| `$VAR` `${VAR}` | environment variable (undefined -> empty) |
| `%VAR%` | environment variable (undefined -> left as written, like CMD) |
| `$?` | exit code of the previous command |
| `~` | home directory (only at the start of an argument: `~`, `~/x`, `~\x`) |
| `# text` | comment (when `#` starts an argument) |
| `\` | literal everywhere (Windows paths), except directly before a `"` inside double quotes: `\"` is a quote, `\\"` is one backslash then the closing quote. Write `"C:\dir\\"` for a path with a trailing backslash. |
| `\| & ; < >` | **rejected** for now with a clear error (pipes/redirection/chaining are Phase 4). Quote them to use literally. |

Expanded values are never re-parsed, so a variable containing `|` or spaces cannot inject extra arguments or operators.

## Line editor (interactive mode, Windows console)

Up/Down history, Left/Right, Home/End, Backspace/Delete, **Ctrl+C** cancel line,
**Ctrl+D** exit on an empty line, **Ctrl+L** clear, Ctrl+A / Ctrl+E start/end,
Ctrl+U / Ctrl+K kill before/after cursor. Tab completion and Ctrl+R search come in later phases.

## Command-line options

```
proxy                 interactive shell (reads commands from stdin when piped)
proxy -c "command"    run one command and exit with its exit code
proxy --version       print version
proxy --help          print usage
```
