#include <cstdio>
#include <exception>

#include "test_framework.h"

namespace pskbtest {

std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

void Fail(const char* file, int line, const std::string& message) {
    throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + message);
}

}  // namespace pskbtest

int main() {
    int failed = 0;
    for (const auto& test : pskbtest::Registry()) {
        try {
            test.fn();
            std::printf("[ PASS ] %s\n", test.name);
        } catch (const std::exception& e) {
            ++failed;
            std::printf("[ FAIL ] %s\n         %s\n", test.name, e.what());
        } catch (...) {
            ++failed;
            std::printf("[ FAIL ] %s (unknown exception)\n", test.name);
        }
    }
    const auto total = pskbtest::Registry().size();
    std::printf("\n%zu tests, %d failed\n", total, failed);
    return failed == 0 ? 0 : 1;
}
