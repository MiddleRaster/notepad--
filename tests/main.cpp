import std;
import tdd20;

int main(int argc, char* argv[])
{
	if ((argc > 1) && (argv[1] == std::string("-dump") || argv[1] == std::string("-d") || argv[1] == std::string("--dump") || argv[1] == std::string("/dump")))
	{
		struct Dumper { bool WantTest  (const std::string& name) const { std::cout << name << "\n"; return false; } };
		struct Noop   { void operator<<(const std::string&) {} };
		TDD20::Test::RunTests(Dumper{}, Noop{});
		return 0;
	}

	class CmdLineMatcher
	{
		const std::vector<std::string> args;
	public:
		CmdLineMatcher(int argc, char* argv[]) : args(argv+1, argv+argc) {}
		bool WantTest(const std::string& name) const { return args.size() == 0 ? true : args.cend() != std::find(args.cbegin(), args.cend(), name); }
	};
	auto [passed, failed] = TDD20::Test::RunTests(CmdLineMatcher{argc, argv}, std::cout);
	std::cout << std::format("\n{} failure(s) out of {} test(s) run\n\n", failed, passed + failed);
    return 0;
}
