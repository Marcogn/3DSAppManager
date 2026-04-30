/*
 * test_utils.c — tests for formatSize and sanitizeName.
 */
#include "../vendor/greatest.h"

/* Pull in the real implementations */
#include "../../source/utils.c"

TEST test_formatSize_bytes(void) {
    char buf[32];
    formatSize(512, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "512 B");
    GREATEST_PASS();
}

TEST test_formatSize_kb(void) {
    char buf[32];
    formatSize(2048, buf, sizeof(buf));
    /* 2048 / 1024 = 2.00 KB */
    ASSERT(strncmp(buf, "2.00 KB", 7) == 0);
    GREATEST_PASS();
}

TEST test_formatSize_mb(void) {
    char buf[32];
    formatSize((u64)3 * 1024 * 1024, buf, sizeof(buf));
    ASSERT(strncmp(buf, "3.00 MB", 7) == 0);
    GREATEST_PASS();
}

TEST test_formatSize_gb(void) {
    char buf[32];
    formatSize((u64)2 * 1024 * 1024 * 1024, buf, sizeof(buf));
    ASSERT(strncmp(buf, "2.00 GB", 7) == 0);
    GREATEST_PASS();
}

TEST test_sanitizeName_clean(void) {
    char name[] = "Pokemon Sun";
    sanitizeName(name);
    ASSERT_STR_EQ(name, "Pokemon Sun");
    GREATEST_PASS();
}

TEST test_sanitizeName_strips_bad_chars(void) {
    char name[] = "My:Game<Title>";
    sanitizeName(name);
    /* bad chars replaced with spaces, trailing spaces trimmed */
    ASSERT(strchr(name, ':') == NULL);
    ASSERT(strchr(name, '<') == NULL);
    ASSERT(strchr(name, '>') == NULL);
    GREATEST_PASS();
}

TEST test_sanitizeName_empty_becomes_unknown(void) {
    char name[32] = "   ";
    sanitizeName(name);
    ASSERT_STR_EQ(name, "Unknown Title");
    GREATEST_PASS();
}

TEST test_sanitizeName_strips_control_chars(void) {
    char name[] = "Hello\x01World";
    sanitizeName(name);
    ASSERT_STR_EQ(name, "HelloWorld");
    GREATEST_PASS();
}

SUITE(suite_utils) {
    RUN_TEST(test_formatSize_bytes);
    RUN_TEST(test_formatSize_kb);
    RUN_TEST(test_formatSize_mb);
    RUN_TEST(test_formatSize_gb);
    RUN_TEST(test_sanitizeName_clean);
    RUN_TEST(test_sanitizeName_strips_bad_chars);
    RUN_TEST(test_sanitizeName_empty_becomes_unknown);
    RUN_TEST(test_sanitizeName_strips_control_chars);
}

int main(void) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(suite_utils);
    GREATEST_MAIN_END();
}
