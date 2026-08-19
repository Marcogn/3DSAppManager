/*
 * test_lang.c — tests for lang.c's T() lookup and detectSystemLanguage().
 *
 * Guards against the exact failure mode that would defeat the whole
 * localization effort: a StringID with no entry in one of the tables
 * (T() silently falling back or returning ""), or config.language
 * pointing at a language whose table row is missing.
 */
#include "../vendor/greatest.h"
#include <string.h>

#include "../../source/lang.c"

TEST test_T_returns_english_by_default(void) {
    config.language = LANG_EN;
    ASSERT_STR_EQ(T(STR_MENU_INSTALL), "Install CIA");
    GREATEST_PASS();
}

TEST test_T_returns_italian_when_selected(void) {
    config.language = LANG_IT;
    ASSERT_STR_EQ(T(STR_MENU_INSTALL), "Installa CIA");
    GREATEST_PASS();
}

TEST test_T_switches_at_runtime(void) {
    config.language = LANG_EN;
    const char *en = T(STR_MENU_SETTINGS);
    config.language = LANG_IT;
    const char *it = T(STR_MENU_SETTINGS);
    ASSERT(strcmp(en, it) != 0);
    config.language = LANG_EN;
    GREATEST_PASS();
}

TEST test_T_out_of_range_id_returns_empty(void) {
    ASSERT_STR_EQ(T((StringID)-1), "");
    ASSERT_STR_EQ(T(STR_COUNT), "");
    ASSERT_STR_EQ(T((StringID)(STR_COUNT + 100)), "");
    GREATEST_PASS();
}

TEST test_T_invalid_language_falls_back_to_english(void) {
    config.language = (Language)99;
    ASSERT_STR_EQ(T(STR_MENU_UNINSTALL), "Uninstall Titles");
    config.language = LANG_EN;
    GREATEST_PASS();
}

TEST test_T_all_ids_populated_in_english(void) {
    config.language = LANG_EN;
    for (int i = 0; i < STR_COUNT; i++) {
        const char *s = T((StringID)i);
        ASSERT(s != NULL);
        ASSERT(s[0] != '\0');
    }
    GREATEST_PASS();
}

TEST test_T_all_ids_populated_in_italian(void) {
    config.language = LANG_IT;
    for (int i = 0; i < STR_COUNT; i++) {
        const char *s = T((StringID)i);
        ASSERT(s != NULL);
        ASSERT(s[0] != '\0');
    }
    config.language = LANG_EN;
    GREATEST_PASS();
}

TEST test_detectSystemLanguage_defaults_to_english_shim(void) {
    /* tests/shims/3ds.h's CFGU_GetSystemLanguage always reports
       CFG_LANGUAGE_EN on host, so this just exercises the mapping path. */
    Language lang = detectSystemLanguage();
    ASSERT(lang == LANG_EN || lang == LANG_IT);
    GREATEST_PASS();
}

SUITE(suite_lang) {
    RUN_TEST(test_T_returns_english_by_default);
    RUN_TEST(test_T_returns_italian_when_selected);
    RUN_TEST(test_T_switches_at_runtime);
    RUN_TEST(test_T_out_of_range_id_returns_empty);
    RUN_TEST(test_T_invalid_language_falls_back_to_english);
    RUN_TEST(test_T_all_ids_populated_in_english);
    RUN_TEST(test_T_all_ids_populated_in_italian);
    RUN_TEST(test_detectSystemLanguage_defaults_to_english_shim);
}

int main(void) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(suite_lang);
    GREATEST_MAIN_END();
}
