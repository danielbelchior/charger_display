#pragma once

// Minimal self-contained test harness: no third-party framework, so the tests
// build with any C++17 compiler and nothing has to be downloaded.

#include <stdio.h>
#include <string.h>

namespace test {

using Fn = void (*)();

struct Case {
    const char* name;
    Fn fn;
    Case* next;
};

Case*& registry();
int& failures();
void fail(const char* file, int line, const char* expr);

struct Register {
    Register(Case* c) {
        c->next = registry();
        registry() = c;
    }
};

}  // namespace test

#define TEST_CASE(name)                                     \
    static void name();                                     \
    static test::Case name##_case = {#name, name, nullptr}; \
    static test::Register name##_register(&name##_case);    \
    static void name()

#define CHECK(expr)                                \
    do {                                           \
        if (!(expr)) {                             \
            test::fail(__FILE__, __LINE__, #expr); \
        }                                          \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_STR(a, b) CHECK(((a) != nullptr) && strcmp((a), (b)) == 0)
