#include "test.h"

namespace test {

Case*& registry() {
    static Case* head = nullptr;
    return head;
}

int& failures() {
    static int count = 0;
    return count;
}

void fail(const char* file, int line, const char* expr) {
    failures()++;
    printf("  FAIL %s:%d: %s\n", file, line, expr);
}

}  // namespace test

int main(int argc, char** argv) {
    // Registration prepends, so reverse to run in file order.
    test::Case* ordered = nullptr;
    for (test::Case* c = test::registry(); c;) {
        test::Case* next = c->next;
        c->next = ordered;
        ordered = c;
        c = next;
    }

    const char* filter = argc > 1 ? argv[1] : nullptr;
    int run = 0;
    int failed = 0;
    for (test::Case* c = ordered; c; c = c->next) {
        if (filter && !strstr(c->name, filter)) {
            continue;
        }
        const int before = test::failures();
        c->fn();
        run++;
        if (test::failures() != before) {
            failed++;
            printf("FAILED %s\n", c->name);
        }
    }
    printf("%d test(s), %d failed\n", run, failed);
    return failed == 0 && run > 0 ? 0 : 1;
}
