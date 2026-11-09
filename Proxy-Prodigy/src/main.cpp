#include <iostream>
#include <string>
#include <vector>

#include "commands/core_commands.hpp"
#include "core/shell.hpp"
#include "utilities/platform.hpp"
#include "utilities/strings.hpp"

#ifdef _WIN32
#include "utilities/win_strings.hpp"
#endif

namespace {

void printUsage(std::ostream& o) {
    o << "PROXY PRODIGY v" PP_VERSION " - One Terminal. Unlimited Control.\n\n"
         "Usage:\n"
         "  proxy                 start the interactive shell\n"
         "  proxy -c \"command\"    run one command and exit\n"
         "  proxy --version       print the version\n"
         "  proxy --help          print this help\n";
}

int realMain(const std::vector<std::string>& argv) {
    if (argv.size() == 1 && (argv[0] == "--version" || argv[0] == "-v")) {
        std::cout << "PROXY PRODIGY v" PP_VERSION "\n";
        return 0;
    }
    if (argv.size() == 1 && (argv[0] == "--help" || argv[0] == "-h")) {
        printUsage(std::cout);
        return 0;
    }
    const bool oneShot = argv.size() == 2 && argv[0] == "-c";
    if (!argv.empty() && !oneShot) {
        std::cerr << "[ERROR] Invalid arguments: " << argv[0] << "\n\n";
        printUsage(std::cerr);
        return 2;
    }

    pp::Terminal term(std::cout, std::cerr);
    term.enableConsoleFeatures();

    pp::History history(1000);
    history.setFile(pp::platform::configDir() / "history.txt");

    pp::CommandRegistry registry;
    pp::registerCoreCommands(registry);

    pp::Shell shell(term, history, registry);

    if (oneShot) {
        int rc = shell.execute(argv[1]);
        std::cout.flush();
        return shell.context().running ? rc : shell.context().exitCode;
    }

    history.load();
    return shell.runInteractive();
}

}  // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t** wargv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(pp::win::wideToUtf8(wargv[i]));
    try {
        return realMain(args);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Fatal: " << e.what() << "\n";
        return 1;
    }
}
#else
int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    try {
        return realMain(args);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Fatal: " << e.what() << "\n";
        return 1;
    }
}
#endif
