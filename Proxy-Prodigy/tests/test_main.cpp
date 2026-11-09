// Dependency-free unit/integration tests for Phase 1.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include "commands/core_commands.hpp"
#include "core/command_registry.hpp"
#include "core/history.hpp"
#include "core/parser.hpp"
#include "core/shell.hpp"
#include "utilities/platform.hpp"
#include "utilities/strings.hpp"

namespace fs = std::filesystem;
static int g_total = 0, g_failed = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_total;                                                               \
        if (!(cond)) {                                                           \
            ++g_failed;                                                          \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; \
        }                                                                        \
    } while (0)

#define CHECK_EQ(a, b)                                                           \
    do {                                                                         \
        ++g_total;                                                               \
        auto&& _a = (a);                                                         \
        auto&& _b = (b);                                                         \
        if (!(_a == _b)) {                                                       \
            ++g_failed;                                                          \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #a " == " #b \
                      << "\n   got: [" << _a << "]\n   exp: [" << _b << "]\n";   \
        }                                                                        \
    } while (0)

using namespace pp;

static std::vector<std::string> P(const std::string& line,
                                  std::map<std::string, std::string> env = {},
                                  const std::string& home = "/home/u", bool* ok = nullptr,
                                  std::string* err = nullptr) {
    auto lookup = [&](const std::string& n) -> std::optional<std::string> {
        auto it = env.find(n);
        if (it == env.end()) return std::nullopt;
        return it->second;
    };
    ParseResult r = parseLine(line, lookup, home);
    if (ok) *ok = r.ok;
    if (err) *err = r.error;
    return r.args;
}
using V = std::vector<std::string>;

static fs::path makeTempDir(const std::string& tag) {
    auto t = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path p = fs::temp_directory_path() / ("pp_test_" + tag + "_" + std::to_string(t));
    fs::create_directories(p);
    return p;
}

static void testParser() {
    CHECK(P("") == V{});
    CHECK(P("   \t ") == V{});
    CHECK(P("ls") == V{"ls"});
    CHECK(P("  echo   a   b  ") == V({"echo", "a", "b"}));
    CHECK(P("echo \"hello world\" x") == V({"echo", "hello world", "x"}));
    CHECK(P("echo 'a  b' c") == V({"echo", "a  b", "c"}));
    CHECK(P("echo \"\"") == V({"echo", ""}));              // empty quoted arg is kept
    CHECK(P("echo a\"b c\"d") == V({"echo", "ab cd"}));     // adjacent quoting joins
    CHECK(P("echo \"say \\\"hi\\\"\"") == V({"echo", "say \"hi\""}));
    // Windows paths survive: backslashes are literal
    CHECK(P("cd C:\\Users\\me") == V({"cd", "C:\\Users\\me"}));
    CHECK(P("cd \"C:\\Program Files\\App\"") == V({"cd", "C:\\Program Files\\App"}));
    CHECK(P("cd \"C:\\dir\\\\\"") == V({"cd", "C:\\dir\\"}));        // trailing double backslash collapses to one
    CHECK(P("cd \"C:\\a\\\\\\\"x\"") == V({"cd", "C:\\a\\\"x"}));  // 3 backslashes + quote
    CHECK(P("echo \"a\\b\\\\c\"") == V({"echo", "a\\b\\\\c"}));      // not before a quote: literal
    CHECK(P("echo 'C:\\x y'") == V({"echo", "C:\\x y"}));
    // comments
    CHECK(P("echo hi # comment") == V({"echo", "hi"}));
    CHECK(P("# only a comment") == V{});
    CHECK(P("echo a#b") == V({"echo", "a#b"}));
    CHECK(P("echo '#x'") == V({"echo", "#x"}));
    // environment expansion
    std::map<std::string, std::string> env{{"NAME", "Utkarsh"}, {"?", "3"}, {"ProgramFiles(x86)", "C:\\PF86"}};
    CHECK(P("echo $NAME", env) == V({"echo", "Utkarsh"}));
    CHECK(P("echo ${NAME}x", env) == V({"echo", "Utkarshx"}));
    CHECK(P("echo \"$NAME is here\"", env) == V({"echo", "Utkarsh is here"}));
    CHECK(P("echo '$NAME'", env) == V({"echo", "$NAME"}));
    CHECK(P("echo $NOPE.", env) == V({"echo", "."}));
    CHECK(P("echo $?", env) == V({"echo", "3"}));
    CHECK(P("echo %NAME%", env) == V({"echo", "Utkarsh"}));
    CHECK(P("echo %NOPE%", env) == V({"echo", "%NOPE%"}));
    CHECK(P("echo 50% and 60%", env) == V({"echo", "50%", "and", "60%"}));
    CHECK(P("echo %ProgramFiles(x86)%", env) == V({"echo", "C:\\PF86"}));
    CHECK(P("echo $", env) == V({"echo", "$"}));
    CHECK(P("echo $5", env) == V({"echo", "$5"}));
    // expansion results are NOT re-split or re-parsed (no injection through variables)
    std::map<std::string, std::string> evil{{"X", "a b | c > d"}};
    CHECK(P("echo $X", evil) == V({"echo", "a b | c > d"}));
    // tilde
    CHECK(P("cd ~") == V({"cd", "/home/u"}));
    CHECK(P("cd ~/docs") == V({"cd", "/home/u/docs"}));
    CHECK(P("cd ~\\docs", {}, "C:\\Users\\u") == V({"cd", "C:\\Users\\u\\docs"}));
    CHECK(P("echo a~b") == V({"echo", "a~b"}));
    CHECK(P("echo '~'") == V({"echo", "~"}));
    CHECK(P("echo ~user") == V({"echo", "~user"}));
    // errors
    bool ok = true;
    std::string err;
    P("echo \"abc", {}, "", &ok, &err);
    CHECK(!ok);
    CHECK(err.find("double quote") != std::string::npos);
    P("cd \"C:\\dir\\\"", {}, "", &ok, &err);   // trailing backslash-quote is a literal quote, so unterminated
    CHECK(!ok);
    P("echo 'abc", {}, "", &ok, &err);
    CHECK(!ok);
    P("echo ${abc", {}, "", &ok, &err);
    CHECK(!ok);
    P("echo ${}", {}, "", &ok, &err);
    CHECK(!ok);
    for (const char* op : {"echo a | b", "echo a > f", "echo a < f", "echo a & b", "echo a ; b", "a && b"}) {
        P(op, {}, "", &ok, &err);
        CHECK(!ok);
        CHECK(err.find("Unsupported operator") != std::string::npos);
    }
    P("echo \"a|b;c>d\" 'x&y'", {}, "", &ok, &err);
    CHECK(ok);
    CHECK(P("echo \"a|b;c>d\" 'x&y'") == V({"echo", "a|b;c>d", "x&y"}));
    // Unicode passes through untouched
    CHECK(P("echo héllo 日本語 \"🙂 x\"") == V({"echo", "héllo", "日本語", "🙂 x"}));
}

static void testRegistry() {
    CommandRegistry r;
    auto h = [](Context&, const Args&) { return 0; };
    CHECK(r.add({"Foo", "c", "s", "u", {"f", "Fo"}, h}));
    CHECK(r.find("foo") != nullptr);
    CHECK(r.find("FOO") != nullptr);       // case-insensitive
    CHECK(r.find("f") == r.find("foo"));   // alias resolves to the same command
    CHECK(r.find("FO") == r.find("foo"));
    CHECK(r.find("bar") == nullptr);
    CHECK(!r.add({"foo", "c", "s", "u", {}, h}));          // duplicate name
    CHECK(!r.add({"f", "c", "s", "u", {}, h}));            // name collides with alias
    CHECK(!r.add({"baz", "c", "s", "u", {"foo"}, h}));     // alias collides with name
    CHECK(r.find("baz") == nullptr);                        // failed add registered nothing
    CHECK(!r.add({"", "c", "s", "u", {}, h}));
    CHECK(!r.add({"nohandler", "c", "s", "u", {}, nullptr}));
    CHECK(r.add({"alpha", "c", "s", "u", {}, h}));
    auto all = r.all();
    CHECK_EQ(all.size(), size_t(2));
    CHECK_EQ(all[0]->name, std::string("alpha"));  // sorted
}

static void testHistory() {
    fs::path dir = makeTempDir("hist");
    fs::path file = dir / "sub" / "history.txt";  // parent dir created on demand
    {
        History h(3);
        h.setFile(file);
        h.load();
        h.add("one");
        h.add("one");   // immediate repeat ignored
        h.add("   ");   // blank ignored
        h.add("");
        h.add("two");
        h.add("three");
        h.add("four");  // evicts "one"
        CHECK_EQ(h.entries().size(), size_t(3));
        CHECK_EQ(h.entries().front(), std::string("two"));
        CHECK_EQ(h.entries().back(), std::string("four"));
    }
    {
        History h(3);
        h.setFile(file);
        h.load();  // persisted across instances
        CHECK_EQ(h.entries().size(), size_t(3));
        CHECK_EQ(h.entries().back(), std::string("four"));
        h.add("héllo 日本語");
        CHECK_EQ(h.entries().back(), std::string("héllo 日本語"));
    }
    {
        History h(2);  // smaller limit trims on load and rewrites the file
        h.setFile(file);
        h.load();
        CHECK_EQ(h.entries().size(), size_t(2));
        CHECK_EQ(h.entries().back(), std::string("héllo 日本語"));
        h.clear();
        CHECK(h.entries().empty());
    }
    {
        History h(5);
        h.setFile(file);
        h.load();
        CHECK(h.entries().empty());  // clear() truncated the file
    }
    {   // Unwritable location must not throw or crash
        History h(5);
        h.setFile(fs::path("/proc/nonexistent_dir/x/history.txt"));
        h.add("x");
        CHECK_EQ(h.entries().size(), size_t(1));
    }
    {   // No file at all is fine
        History h(5);
        h.load();
        h.add("a");
        CHECK_EQ(h.entries().size(), size_t(1));
    }
    fs::remove_all(dir);
}

struct Rig {
    std::ostringstream out, err;
    Terminal term{out, err};
    History history{100};
    CommandRegistry registry;
    Shell shell{term, history, registry};
    Rig() { registerCoreCommands(registry); }
};

static void testShell() {
    fs::path startDir = fs::current_path();
    fs::path tmp = makeTempDir("shell");
    fs::path spaced = tmp / "dir with spaces";
    fs::create_directories(spaced);
    fs::path file = tmp / "file.txt";
    std::ofstream(file) << "x";

    {   // echo / exit codes / $?
        Rig r;
        CHECK_EQ(r.shell.execute("echo hello world"), 0);
        CHECK_EQ(r.out.str(), std::string("hello world\n"));
        r.out.str("");
        CHECK_EQ(r.shell.execute("echo -n hi"), 0);
        CHECK_EQ(r.out.str(), std::string("hi"));
        r.out.str("");
        CHECK_EQ(r.shell.execute("definitely_not_a_command"), 127);
        CHECK(r.err.str().find("[ERROR] Command not found: definitely_not_a_command") != std::string::npos);
        r.out.str("");
        CHECK_EQ(r.shell.execute("echo $?"), 0);
        CHECK_EQ(r.out.str(), std::string("127\n"));   // $? sees the previous failure
        CHECK_EQ(r.shell.execute("echo \"unterminated"), 2);
        CHECK_EQ(r.shell.execute(""), 2);               // blank line keeps the previous status
    }
    {   // blank/comment lines do not reset status
        Rig r;
        r.shell.execute("nope");
        CHECK_EQ(r.shell.execute("   "), 127);
        CHECK_EQ(r.shell.execute("# comment"), 127);
    }
    {   // pwd / cd
        Rig r;
        CHECK_EQ(r.shell.execute("cd \"" + pathToUtf8(spaced) + "\""), 0);
        r.out.str("");
        CHECK_EQ(r.shell.execute("pwd"), 0);
        CHECK_EQ(r.out.str(), pathToUtf8(fs::canonical(spaced)) + "\n");

        CHECK_EQ(r.shell.execute("cd .."), 0);
        r.out.str("");
        r.shell.execute("pwd");
        CHECK_EQ(r.out.str(), pathToUtf8(fs::canonical(tmp)) + "\n");

        r.out.str("");
        CHECK_EQ(r.shell.execute("cd -"), 0);  // back to the spaced dir, and announces it
        CHECK_EQ(r.out.str(), pathToUtf8(fs::canonical(spaced)) + "\n");

        CHECK_EQ(r.shell.execute("cd does_not_exist"), 1);
        CHECK(r.err.str().find("no such directory") != std::string::npos);
        CHECK_EQ(r.shell.execute("cd \"" + pathToUtf8(file) + "\""), 1);
        CHECK(r.err.str().find("not a directory") != std::string::npos);
        CHECK_EQ(r.shell.execute("cd a b"), 2);  // unquoted spaces -> clear error
        CHECK(r.err.str().find("too many arguments") != std::string::npos);

        // failed cd leaves cwd untouched
        r.out.str("");
        r.shell.execute("pwd");
        CHECK_EQ(r.out.str(), pathToUtf8(fs::canonical(spaced)) + "\n");

        // cd with no args goes home (if HOME is set)
        if (!platform::homeDir().empty() && fs::is_directory(pathFromUtf8(platform::homeDir()))) {
            CHECK_EQ(r.shell.execute("cd"), 0);
            r.out.str("");
            r.shell.execute("pwd");
            CHECK_EQ(r.out.str(), pathToUtf8(fs::canonical(pathFromUtf8(platform::homeDir()))) + "\n");
        }
        Rig r2;
        CHECK_EQ(r2.shell.execute("cd -"), 1);  // no previous dir in a fresh shell
    }
    {   // exit
        Rig r;
        CHECK(r.shell.context().running);
        CHECK_EQ(r.shell.execute("exit 7"), 7);
        CHECK(!r.shell.context().running);
        CHECK_EQ(r.shell.context().exitCode, 7);
        Rig r2;
        CHECK_EQ(r2.shell.execute("exit abc"), 2);
        CHECK(r2.shell.context().running);   // invalid exit does not quit
        CHECK_EQ(r2.shell.execute("exit -1"), 2);
        CHECK_EQ(r2.shell.execute("exit 1 2"), 2);
        CHECK(r2.shell.context().running);
        Rig r3;
        CHECK_EQ(r3.shell.execute("quit"), 0);  // alias
        CHECK(!r3.shell.context().running);
    }
    {   // help / version / clear
        Rig r;
        CHECK_EQ(r.shell.execute("help"), 0);
        for (const char* name : {"help", "version", "clear", "exit", "pwd", "cd", "echo", "history"})
            CHECK(r.out.str().find(name) != std::string::npos);
        r.out.str("");
        CHECK_EQ(r.shell.execute("help cd"), 0);
        CHECK(r.out.str().find("Usage:") != std::string::npos);
        CHECK_EQ(r.shell.execute("help nosuch"), 1);
        CHECK_EQ(r.shell.execute("help cls"), 0);  // alias lookup
        r.out.str("");
        CHECK_EQ(r.shell.execute("version"), 0);
        CHECK(r.out.str().find("PROXY PRODIGY v") != std::string::npos);
        CHECK_EQ(r.shell.execute("version extra"), 2);
        CHECK_EQ(r.shell.execute("clear"), 0);  // no console features -> no escape codes
        CHECK(r.out.str().find('\x1b') == std::string::npos);
    }
    {   // history command
        Rig r;
        r.history.add("alpha");
        r.history.add("beta");
        r.history.add("gamma");
        CHECK_EQ(r.shell.execute("history"), 0);
        CHECK(r.out.str().find("1  alpha") != std::string::npos);
        CHECK(r.out.str().find("3  gamma") != std::string::npos);
        r.out.str("");
        CHECK_EQ(r.shell.execute("history 1"), 0);
        CHECK(r.out.str().find("gamma") != std::string::npos);
        CHECK(r.out.str().find("alpha") == std::string::npos);
        CHECK_EQ(r.shell.execute("history 99"), 0);
        CHECK_EQ(r.shell.execute("history xyz"), 2);
        CHECK_EQ(r.shell.execute("history -c"), 0);
        CHECK(r.history.entries().empty());
    }
    {   // a throwing command cannot take the shell down
        Rig r;
        r.registry.add({"boom", "t", "s", "u", {}, [](Context&, const Args&) -> int { throw std::runtime_error("kaboom"); }});
        CHECK_EQ(r.shell.execute("boom"), 1);
        CHECK(r.err.str().find("kaboom") != std::string::npos);
        CHECK_EQ(r.shell.execute("echo still alive"), 0);
    }
    {   // prompt shape (no colour in tests)
        Rig r;
        std::string p = r.shell.buildPrompt();
        CHECK(p.rfind("proxy@prodigy:", 0) == 0);
        CHECK(p.size() >= 2 && p.substr(p.size() - 2) == "$ ");
    }
    {   // runInteractive over non-interactive stdin is covered by the shell-level smoke test script.
    }
    fs::current_path(startDir);
    fs::remove_all(tmp);
}

int main() {
    testParser();
    testRegistry();
    testHistory();
    testShell();
    std::cout << (g_total - g_failed) << "/" << g_total << " checks passed\n";
    return g_failed == 0 ? 0 : 1;
}
