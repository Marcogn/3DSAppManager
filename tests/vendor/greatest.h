/*
 * greatest.h — minimal single-header test framework (subset).
 * Adapted for 3ds-app-manager host tests.
 */
#ifndef GREATEST_H
#define GREATEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned passed;
    unsigned failed;
    unsigned skipped;
    const char *name;
} greatest_suite_t;

static greatest_suite_t greatest_suite_info = {0, 0, 0, "all"};

#define GREATEST_PASS()  do { greatest_suite_info.passed++;  return 1; } while(0)
#define GREATEST_FAIL()  do { greatest_suite_info.failed++;  fprintf(stderr, "  FAIL %s:%d\n", __FILE__, __LINE__); return 0; } while(0)

#define ASSERT(cond)  do { if (!(cond)) { greatest_suite_info.failed++; fprintf(stderr, "  FAIL [%s:%d] assertion: %s\n", __FILE__, __LINE__, #cond); return 0; } } while(0)
#define ASSERT_EQ(a,b) do { if ((a) != (b)) { greatest_suite_info.failed++; fprintf(stderr, "  FAIL [%s:%d] %s != %s\n", __FILE__, __LINE__, #a, #b); return 0; } } while(0)
#define ASSERT_STR_EQ(a,b) do { if (strcmp((a),(b)) != 0) { greatest_suite_info.failed++; fprintf(stderr, "  FAIL [%s:%d] \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); return 0; } } while(0)

#define TEST  static int
#define RUN_TEST(t) do { int _r = t(); if (_r) { printf("  PASS %s\n", #t); } } while(0)
#define SUITE(name) static void name(void)
#define RUN_SUITE(s) do { printf("[Suite] " #s "\n"); s(); } while(0)

#define GREATEST_MAIN_BEGIN() do { printf("=== Running tests ===\n"); } while(0)
#define GREATEST_MAIN_END() do { \
    printf("=== Results: %u passed, %u failed, %u skipped ===\n", \
        greatest_suite_info.passed, greatest_suite_info.failed, greatest_suite_info.skipped); \
    return (greatest_suite_info.failed > 0) ? 1 : 0; \
} while(0)

#endif /* GREATEST_H */
