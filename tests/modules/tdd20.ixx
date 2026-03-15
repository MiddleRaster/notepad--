// Copyright (c) 2025 MiddleRaster
// Licensed under the MIT License
// See LICENSE file in the project root for full license information.

export module tdd20;

import std;

export namespace TDD20
{
    template<typename T>            inline std::string ToString(const            T&) { static_assert(sizeof(T) == 0, "test writer must write a specialization for this type"); return {}; }
    template<typename T>            inline std::string ToString(                 T* t) { return std::format("0x{:X}", reinterpret_cast<std::uintptr_t>(t)); } // generic pointer converted to hex string
    template<std::integral       T> inline std::string ToString(const            T& t) { return std::to_string(t); }
    template<std::floating_point T> inline std::string ToString(const            T& t) { return std::format("{:.15f}", t); } // 15 digits of precision
                                    inline std::string ToString(const         bool& t) { return t ? "true" : "false"; }      // an overload, not a specialization
    template <>                     inline std::string ToString(const  std::string& t) { return t; }
    template <>                     inline std::string ToString(const         char* t) { return std::string(t); }
    template <>                     inline std::string ToString(const std::wstring& t)
    {
        std::string s;
        for (wchar_t wc : t)
            s += static_cast<char>(wc); // lossy:  drops high bits
        return s;
    }
    template <>                     inline std::string ToString(const      wchar_t* t) { return ToString(std::wstring(t)); }

    struct AssertException : public std::exception
    {
        const std::string message, file;
        const int line;
        AssertException(const std::string& msg, int line, const std::string& file) : message(msg), file(file), line(line) {}
        const char* what() const noexcept override { return message.c_str(); }
    };
    struct Assert
    {
        static void Fail(const std::string& msg="", std::source_location loc=std::source_location::current()) { throw AssertException(msg, loc.line(), loc.file_name()); }
        template<typename S, typename T> static void AreEqual(const S& expected, const T& actual, const std::string& message="", std::source_location loc=std::source_location::current())
        {
            std::string Expected{ToString(expected)}, Actual{ToString(actual)};
            if (Expected != Actual)
                throw AssertException(std::format("Expected <{}> Actual <{}>{}", Expected, Actual, message.empty() ? "" : " - " + message), loc.line(), loc.file_name());
        }
        template<typename S, typename T> static void AreNotEqual(const S& expected, const T& actual, const std::string& message="", std::source_location loc=std::source_location::current())
        {
            std::string Actual{ToString(actual)};
            if (ToString(expected) == Actual)
                throw AssertException(std::format("Unexpected equality <{}>{}", Actual, message.empty() ? "" : " - " + message), loc.line(), loc.file_name());
        }
        static void AreWithin(double expected, double actual, double tolerance, const std::string& message="", std::source_location loc=std::source_location::current())
        {
            if (std::fabs(expected - actual) > tolerance)
                throw AssertException(std::format("Expected <{}> to be within <{}> of <{}>{}", ToString(expected), ToString(tolerance), ToString(actual), message.empty() ? "" : " - " + message), loc.line(), loc.file_name());
        }
        static void IsFalse(bool actual, const std::string& message="", std::source_location loc=std::source_location::current()) { AreEqual(false, actual, message, loc); }
        static void IsTrue (bool actual, const std::string& message="", std::source_location loc=std::source_location::current()) { AreEqual(true,  actual, message, loc); }
        template <typename E, typename L> static void ExpectingException(L l, const std::string& message=std::string(), std::source_location loc=std::source_location::current())
        {
            try { l(); }
            catch (const E&) { return; }
            catch (...) { throw AssertException(std::format("exception of wrong type thrown; was expecting exception of type '{}'{}", typeid(E).name(), message.empty() ? "" : " - " + message), loc.line(), loc.file_name()); }
            throw               AssertException(std::format(           "no exception thrown; was expecting exception of type '{}'{}", typeid(E).name(), message.empty() ? "" : " - " + message), loc.line(), loc.file_name());
        }
    };

    class Test
    {
        inline static std::vector<std::pair<std::string, std::function<void()>>> tests;
    public:
        static std::pair<int, int> RunTests(auto&& matcher, auto&& out)
        {
            int passed = 0, failed = 0;
            for (auto& [name, func] : tests) {
                if (false == matcher.WantTest(name))
                    continue;

                try {
                    func();
                    ++passed;
                    continue;
                }
                catch (const AssertException& e) { out << std::format("{}({}) : warning unit-test: \"{}\" failed with: {}\n", e.file, e.line, name, e.what()); }
                catch (const std::exception & e) { out << std::format("{}({}) : warning unit-test: \"{}\" failed with: {}\n",    "?",      1, name, e.what()); }
                catch (...)                      { out << std::format("{}({}) : warning unit-test: \"{}\" failed with: {}\n",    "?",      1, name, "unknown exception caught"); }
                ++failed;
            }
            return {passed, failed};
        }
        Test(const std::string& name, std::function<void()> func) { tests.push_back(std::pair{name, func}); }
    };
}
