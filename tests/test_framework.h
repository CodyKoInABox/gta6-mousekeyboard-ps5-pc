#pragma once

// Minimal test harness: no third-party download needed to build the suite.

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pskbtest {

using TestFn = void (*)();

struct TestCase {
    const char* name;
    TestFn fn;
};

std::vector<TestCase>& Registry();

struct Registrar {
    Registrar(const char* name, TestFn fn) { Registry().push_back({name, fn}); }
};

[[noreturn]] void Fail(const char* file, int line, const std::string& message);

template <typename A, typename B>
std::string Describe(const A& a, const B& b) {
    std::ostringstream out;
    out << "left=" << a << " right=" << b;
    return out.str();
}

}  // namespace pskbtest

#define PSKB_TEST(name)                                                  \
    static void name();                                                  \
    static pskbtest::Registrar name##_registrar(#name, &name);           \
    static void name()

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) pskbtest::Fail(__FILE__, __LINE__, "CHECK(" #cond ")"); \
    } while (0)

#define CHECK_EQ(a, b)                                                   \
    do {                                                                 \
        const auto& pskb_a_ = (a);                                       \
        const auto& pskb_b_ = (b);                                       \
        if (!(pskb_a_ == pskb_b_))                                       \
            pskbtest::Fail(__FILE__, __LINE__,                           \
                           "CHECK_EQ(" #a ", " #b ") " +                 \
                               pskbtest::Describe(pskb_a_, pskb_b_));    \
    } while (0)

#define CHECK_NEAR(a, b, tol)                                            \
    do {                                                                 \
        const double pskb_a_ = static_cast<double>(a);                   \
        const double pskb_b_ = static_cast<double>(b);                   \
        if (!((pskb_a_ - pskb_b_) <= (tol) && (pskb_b_ - pskb_a_) <= (tol))) \
            pskbtest::Fail(__FILE__, __LINE__,                           \
                           "CHECK_NEAR(" #a ", " #b ") " +               \
                               pskbtest::Describe(pskb_a_, pskb_b_));    \
    } while (0)
