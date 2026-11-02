/* Test of the cups that the rules table does not hold in some seasons (0.6.0 slice 21b,
 * src/recomp_rt/win/season_cup_breaks.c), with the rules table of the build.
 *
 * Usage: season_cup_breaks_test <rules.tsv made by tools/seasons/make_rules.py>
 * ctest makes the table first (test season_rules_table). The values come from the rules table parts
 * (docs/research/rules-0.6.0/<part>.tsv): for example the Coupe de la Ligue is not held from 2020/21, the Norwegian
 * Cup of 2020 (row 2019/20, a calendar-year cup) is not held. */
#include "season_cups.h"
#include "season_rules.h"

#include <stdio.h>

static int failures;

/* A failed check prints what the check expects. */
#define CHECK(cond, ...) \
    do { if (!(cond)) { failures++; printf("FAIL line %d, expected: ", __LINE__); printf(__VA_ARGS__); \
         printf("\n"); } } while (0)

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: season_cup_breaks_test <rules.tsv>\n");
        return 2;
    }
    if (season_rules_load(argv[1]) <= 0) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    /* Split-season cups: the edition of year Y is the row of season Y. */
    CHECK(season_cups_calendar("fra_lge_cup") == 0, "the Coupe de la Ligue is a split-season cup");
    CHECK(season_cups_held("fra_lge_cup", 2019) == 1, "the Coupe de la Ligue is held in 2019/20");
    CHECK(season_cups_held("fra_lge_cup", 2020) == 0, "the Coupe de la Ligue is not held in 2020/21");
    CHECK(season_cups_next("fra_lge_cup", 2019) == 2019, "the next Coupe de la Ligue from 2019 is 2019");
    CHECK(season_cups_next("fra_lge_cup", 2020) == SEASON_CUPS_NEVER,
          "the Coupe de la Ligue never comes back after 2019/20");
    CHECK(season_cups_next("fra_lge_cup", 2040) == SEASON_CUPS_NEVER,
          "the Coupe de la Ligue stays away after the last row");
    CHECK(season_cups_next("wel_prm_cup", 2008) == SEASON_CUPS_NEVER,
          "the Welsh Premier Cup never comes back after 2007/08");
    CHECK(season_cups_next("wel_prm_cup", 2007) == 2007, "the Welsh Premier Cup is held in 2007/08");
    /* A gap: the Leinster Cup is not held 2002/03 to 2008/09 and 2019/20 to 2021/22. */
    CHECK(season_cups_next("ire_leinster_cup", 2002) == 2009, "the next Leinster Cup from 2002 is 2009, got %d",
          season_cups_next("ire_leinster_cup", 2002));
    CHECK(season_cups_next("ire_leinster_cup", 2019) == 2022, "the next Leinster Cup from 2019 is 2022");
    CHECK(season_cups_next("ire_lge_cup", 2002) == 2003, "the next Irish League Cup from 2002 is 2003");
    CHECK(season_cups_next("wel_lge_cup", 2020) == 2021, "the next Welsh League Cup from 2020 is 2021");
    /* Calendar-year cups: the edition of year Y is the row of season Y - 1 (the table's rule, club-world.md). */
    CHECK(season_cups_calendar("nor_cup") == 1, "the Norwegian Cup is a calendar-year cup");
    CHECK(season_cups_held("nor_cup", 2019) == 1 && season_cups_held("nor_cup", 2020) == 0 &&
              season_cups_held("nor_cup", 2021) == 1, "the Norwegian Cup is held in 2019 and 2021, not in 2020");
    CHECK(season_cups_next("nor_cup", 2020) == 2021, "the next Norwegian Cup from 2020 is 2021");
    CHECK(season_cups_held("nor_cup", 2001) == -1, "the Norwegian Cup of 2001 has no row (it would be row 2000/01)");
    CHECK(season_cups_calendar("kor_league_cup") == 1, "the Korean League Cup is a calendar-year cup");
    CHECK(season_cups_next("kor_league_cup", 2003) == 2004,
          "the next Korean League Cup from 2003 is 2004 (row 2002/03 not held)");
    CHECK(season_cups_next("kor_league_cup", 2012) == SEASON_CUPS_NEVER,
          "the Korean League Cup never comes back after 2011");
    CHECK(season_cups_next("kor_super_cup", 2007) == 2026, "the next K League Super Cup from 2007 is 2026, got %d",
          season_cups_next("kor_super_cup", 2007));
    /* Open item 97: edition 2026 is the 106th Emperor's Cup, 19 August 2026 to the final on 1 January 2027 (row of
     * 2025/26, real season 2026). The game's edition E has its final on 1 January E + 1. */
    CHECK(season_cups_held("jap_emp_cup", 2026) == 1 && season_cups_next("jap_emp_cup", 2026) == 2026,
          "the Emperor's Cup is held in 2026 (the 106th edition)");
    CHECK(season_cups_held("jap_emp_cup", 2027) == 1, "the Emperor's Cup is held in 2027 (the 107th edition)");
    CHECK(season_cups_held("jap_emp_cup", 2025) == 1, "the Emperor's Cup is held in 2025");
    /* A cup that the table always holds, and a file with no row. */
    CHECK(season_cups_next("eng_fa_cup", 2010) == 2010, "the FA Cup is held in 2010");
    CHECK(season_cups_held("eng_no_such_cup", 2010) == -1 && season_cups_next("eng_no_such_cup", 2010) == 0,
          "a file with no row gives -1 and 0");
    /* After the last row the rule stays (Q248). */
    CHECK(season_cups_held("nor_cup", 2099) == 1, "the Norwegian Cup is held in 2099 (the last row stays)");
    if (failures) {
        printf("%d failures\n", failures);
        return 1;
    }
    printf("season cup breaks: all checks pass\n");
    return 0;
}
