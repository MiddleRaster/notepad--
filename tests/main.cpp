import std;
import tdd20;

#include <windows.h>

int main(int argc, char* argv[])
{
    if ((argc > 1) && (argv[1] == std::string("-dump")))
    {
        struct Dumper { bool WantTest  (const std::string& name) const { std::cout << name << "\n"; return false; } };
        struct Noop   { void operator<<(const std::string&) {} };
        TDD20::Test::RunTests(Dumper{}, Noop{});
        return 0;
    }

    std::string pipeName = "";
    std::ostringstream oss;
    if (argc > 1)
    {    // does first arg start with "/pipe:"?
        std::string s = argv[1];
        if (s.substr(0, 6) == "/pipe:") {
            pipeName = "\\\\.\\pipe\\" + s.substr(6);
            ++argv, --argc;
        }
    }

    class CmdLineMatcher
    {
        const std::vector<std::string> args;
    public:
        CmdLineMatcher(int argc, char* argv[]) : args(argv+1, argv+argc) {}
        bool WantTest(const std::string& name) const { return args.size() == 0 ? true : args.cend() != std::find(args.cbegin(), args.cend(), name); }
    };
    auto [passed, failed] = pipeName == "" ? TDD20::Test::RunTests(CmdLineMatcher{argc, argv}, std::cout)
                                           : TDD20::Test::RunTests(CmdLineMatcher{argc, argv}, oss);

    if (pipeName != "")
    {    // we've been launched via "frameworkHandle.LaunchProcessWithDebuggerAttached()" in TestAdapter, which eats std::cout output, so use a pipe instead
        HANDLE hPipe = CreateFileA(pipeName.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {
            std::string output = oss.str();
            DWORD bytesWritten = 0;
            WriteFile(hPipe, output.c_str(), (DWORD)output.size(), &bytesWritten, NULL);
            CloseHandle(hPipe);
        }
        return 0;
    }
    std::cout << std::format("\n{} failure(s) out of {} test(s) run\n\n", failed, passed + failed);
    return 0;
}

