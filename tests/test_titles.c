/*
 * test_titles.c — tests for comparators, filter, safety predicate, smdhSelectName.
 */
#include "../vendor/greatest.h"
#include <string.h>

/*
 * Pull in the real implementations.
 * titles.c includes globals.h which needs globals — provided via globals_stub.c at link time.
 * utils.c is included transitively through titles.c includes.
 */
#include "../../source/utils.c"
#include "../../source/titles.c"
/* config.c needed for BACKUP_PATH_OPTIONS (used by titles.c indirectly via globals) */
#include "../../source/config.c"

/* ---- comparator tests ---- */

TEST test_compareByName_order(void) {
    TitleInfo a, b;
    memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b));
    strncpy(a.name, "Bravely Default", sizeof(a.name)-1);
    strncpy(b.name, "Animal Crossing", sizeof(b.name)-1);
    /* "Animal" < "Bravely" */
    ASSERT(compareTitlesByName(&a, &b) > 0);
    ASSERT(compareTitlesByName(&b, &a) < 0);
    ASSERT_EQ(compareTitlesByName(&a, &a), 0);
    GREATEST_PASS();
}

TEST test_compareBySize_descending(void) {
    TitleInfo a, b;
    memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b));
    a.size = 200; b.size = 100;
    /* larger size sorts first (negative = a before b) */
    ASSERT(compareTitlesBySize(&a, &b) < 0);
    ASSERT(compareTitlesBySize(&b, &a) > 0);
    ASSERT_EQ(compareTitlesBySize(&a, &a), 0);
    GREATEST_PASS();
}

TEST test_compareByID_ascending(void) {
    TitleInfo a, b;
    memset(&a, 0, sizeof(a)); memset(&b, 0, sizeof(b));
    a.titleID = 0x0004000000001234ULL;
    b.titleID = 0x0004000000005678ULL;
    ASSERT(compareTitlesByID(&a, &b) < 0);
    ASSERT(compareTitlesByID(&b, &a) > 0);
    ASSERT_EQ(compareTitlesByID(&a, &a), 0);
    GREATEST_PASS();
}

/* ---- safety filter tests ---- */

TEST test_safetyFilter_SD_allows_game(void) {
    u64 tid = 0x0004000000001234ULL; /* hi = 0x00040000 */
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_SD), true);
    GREATEST_PASS();
}

TEST test_safetyFilter_SD_allows_update(void) {
    u64 tid = 0x0004000E00001234ULL; /* hi = 0x0004000E */
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_SD), true);
    GREATEST_PASS();
}

TEST test_safetyFilter_SD_blocks_system(void) {
    u64 tid = 0x0004001000001234ULL; /* hi = 0x00040010 */
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_SD), false);
    GREATEST_PASS();
}

TEST test_safetyFilter_SD_blocks_0x00040030(void) {
    u64 tid = 0x0004003000001234ULL;
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_SD), false);
    GREATEST_PASS();
}

TEST test_safetyFilter_NAND_allows_game(void) {
    u64 tid = 0x0004000000001234ULL;
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_NAND), true);
    GREATEST_PASS();
}

TEST test_safetyFilter_NAND_blocks_0x00040138(void) {
    u64 tid = 0x0004013800001234ULL;
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_NAND), false);
    GREATEST_PASS();
}

TEST test_safetyFilter_NAND_blocks_unknown_range(void) {
    /* hi = 0x00040001 — not in the allowed set for NAND */
    u64 tid = 0x0004000100001234ULL;
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_NAND), false);
    GREATEST_PASS();
}

TEST test_safetyFilter_NAND_allows_DLC(void) {
    u64 tid = 0x0004008C00001234ULL; /* hi = 0x0004008C */
    ASSERT_EQ(titlePassesSafetyFilter(tid, MEDIATYPE_NAND), true);
    GREATEST_PASS();
}

/* ---- smdhSelectName tests ---- */

/* Build a minimal SMDH with ASCII title stored as UTF-16LE */
static void fill_smdh_title(SMDH *smdh, int slot, const char *ascii) {
    size_t len = strlen(ascii);
    if (len >= 0x40) len = 0x3F;
    for (size_t i = 0; i < len; i++)
        smdh->titles[slot].shortDescription[i] = (uint16_t)(unsigned char)ascii[i];
    smdh->titles[slot].shortDescription[len] = 0;
}

TEST test_smdhSelectName_basic(void) {
    SMDH smdh; memset(&smdh, 0, sizeof(smdh));
    fill_smdh_title(&smdh, 1, "Pokemon X"); /* slot 1 = English */
    char out[256]; memset(out, 0, sizeof(out));
    bool ok = smdhSelectName(&smdh, CFG_LANGUAGE_EN, out, sizeof(out));
    ASSERT_EQ(ok, true);
    ASSERT_STR_EQ(out, "Pokemon X");
    GREATEST_PASS();
}

TEST test_smdhSelectName_fallback_to_any(void) {
    SMDH smdh; memset(&smdh, 0, sizeof(smdh));
    /* Only Japanese slot (0) has a name */
    fill_smdh_title(&smdh, 0, "Zelda JP");
    char out[256]; memset(out, 0, sizeof(out));
    bool ok = smdhSelectName(&smdh, CFG_LANGUAGE_EN, out, sizeof(out));
    ASSERT_EQ(ok, true);
    ASSERT_STR_EQ(out, "Zelda JP");
    GREATEST_PASS();
}

TEST test_smdhSelectName_empty_smdh_returns_false(void) {
    SMDH smdh; memset(&smdh, 0, sizeof(smdh));
    char out[256]; memset(out, 0, sizeof(out));
    bool ok = smdhSelectName(&smdh, CFG_LANGUAGE_EN, out, sizeof(out));
    ASSERT_EQ(ok, false);
    GREATEST_PASS();
}

/* ---- updateFilteredList tests ---- */

static void reset_globals_titles(void) {
    titleCount = 0;
    filteredCount = 0;
    cursor = 0; scrollOffset = 0;
    memset(titles, 0, sizeof(TitleInfo) * 10);
}

TEST test_filterAll_includes_all(void) {
    reset_globals_titles();
    currentFilterMode = FILTER_ALL;
    titles[0].titleID = 0x0004000000001111ULL; titles[0].isValid = true; titleCount++;
    titles[1].titleID = 0x0004000E00001111ULL; titles[1].isValid = true; titleCount++;
    titles[2].titleID = 0x0004008C00001111ULL; titles[2].isValid = true; titleCount++;
    updateFilteredList();
    ASSERT_EQ(filteredCount, 3);
    GREATEST_PASS();
}

TEST test_filterUpdates_only_updates(void) {
    reset_globals_titles();
    currentFilterMode = FILTER_UPDATES;
    titles[0].titleID = 0x0004000000001111ULL; titles[0].isValid = true; titleCount++;
    titles[1].titleID = 0x0004000E00001111ULL; titles[1].isValid = true; titleCount++;
    titles[2].titleID = 0x0004008C00001111ULL; titles[2].isValid = true; titleCount++;
    updateFilteredList();
    ASSERT_EQ(filteredCount, 1);
    ASSERT_EQ(filteredIndices[0], 1);
    GREATEST_PASS();
}

TEST test_filterDLC_only_dlc(void) {
    reset_globals_titles();
    currentFilterMode = FILTER_DLC;
    titles[0].titleID = 0x0004000000001111ULL; titles[0].isValid = true; titleCount++;
    titles[1].titleID = 0x0004000E00001111ULL; titles[1].isValid = true; titleCount++;
    titles[2].titleID = 0x0004008C00001111ULL; titles[2].isValid = true; titleCount++;
    updateFilteredList();
    ASSERT_EQ(filteredCount, 1);
    ASSERT_EQ(filteredIndices[0], 2);
    GREATEST_PASS();
}

SUITE(suite_titles) {
    RUN_TEST(test_compareByName_order);
    RUN_TEST(test_compareBySize_descending);
    RUN_TEST(test_compareByID_ascending);
    RUN_TEST(test_safetyFilter_SD_allows_game);
    RUN_TEST(test_safetyFilter_SD_allows_update);
    RUN_TEST(test_safetyFilter_SD_blocks_system);
    RUN_TEST(test_safetyFilter_SD_blocks_0x00040030);
    RUN_TEST(test_safetyFilter_NAND_allows_game);
    RUN_TEST(test_safetyFilter_NAND_blocks_0x00040138);
    RUN_TEST(test_safetyFilter_NAND_blocks_unknown_range);
    RUN_TEST(test_safetyFilter_NAND_allows_DLC);
    RUN_TEST(test_smdhSelectName_basic);
    RUN_TEST(test_smdhSelectName_fallback_to_any);
    RUN_TEST(test_smdhSelectName_empty_smdh_returns_false);
    RUN_TEST(test_filterAll_includes_all);
    RUN_TEST(test_filterUpdates_only_updates);
    RUN_TEST(test_filterDLC_only_dlc);
}

int main(void) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(suite_titles);
    GREATEST_MAIN_END();
}
