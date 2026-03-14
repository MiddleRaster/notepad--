export module VsTdd20;

import tdd20;

export class VsTest : private TDD20::Test
{
    static auto StripOffLine(const std::string& name) { return name.substr(0, name.rfind('?')); };
    static std::set<std::string>& UniqueSet()
    {
        static std::set<std::string> s;
        return s;
    }
    static std::string EnforceUniqueness(const std::string& name)
    {
        if (UniqueSet().insert(StripOffLine(name)).second == true)
            return name;
        return "TWO TESTS WITH THE SAME NAME!  FIX IMMEDIATELY!?ERROR! ERROR! ERROR!?-1";
    }
public:
    VsTest(const std::string& testname, std::function<void()> func, std::source_location loc = std::source_location::current())
        : TDD20::Test(EnforceUniqueness(std::format("{}?{}?{}", testname, loc.file_name(), loc.line())), func)
    {}
};

