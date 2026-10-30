/* The rules table and the rules report (0.6.0 slice 12; spec docs/specs/2026-10-16-seasons-0.6.0-spec.md,
 * "Testing" and slice 12).
 *
 * The rules table: one row per league or competition per season, 2001/02 to 2027/28. The research parts are
 * docs/research/rules-0.6.0/<part>.tsv. tools/seasons/make_rules.py turns them into one file, Seasons/rules.tsv
 * of the download, with the structured columns as values (its docstring gives the format). The game reads
 * the file once at the start (season_rules_init). This slice changes no rule: later slices (15 to 29) read
 * the rows through this API. A missing or damaged file leaves the table empty and the game plays as before.
 *
 * The rules report (Q265, season_report.c): a test flag that writes the competitions that the game holds at
 * the start of each season. tests/seasons/rules_report_check.py compares the report with the table. */
#ifndef RECOMP_SEASON_RULES_H
#define RECOMP_SEASON_RULES_H

#include <stdint.h>

/* The limits of a row. tools/seasons/make_rules.py has the same, so a bad table stops the build. */
#define SEASON_RULE_FIRST 2001       /* seasons: start years 2001 to 2099 */
#define SEASON_RULE_LAST 2099
#define SEASON_RULE_MAX_COUNT 100000 /* the largest count */

/* A count of the table: a number from 0 to SEASON_RULE_MAX_COUNT, or one of these codes. */
#define SEASON_RULE_NA (-1)          /* n/a: the column does not apply */
#define SEASON_RULE_UNKNOWN (-2)     /* the research found no value */
#define SEASON_RULE_NOT_HELD (-3)    /* the competition is not held in that season */
#define SEASON_RULE_SEE_EACH (-4)    /* see each competition (a row of nation rules) */
#define SEASON_RULE_AS_CL (-5)       /* as the Champions League */
#define SEASON_RULE_TEXT (-6)        /* a description with no single count: read the text */

/* A tier that is not a number. */
#define SEASON_TIER_CUP (-1)
#define SEASON_TIER_SUPER_CUP (-2)
#define SEASON_TIER_OTHER (-3)       /* state_1, international, national_teams, world, nation: tier_code says */
#define SEASON_TIER_NA (-4)

typedef struct {
    const char *id;                  /* eng-28 */
    int season;                      /* start year: 2001 for 2001/02 */
    const char *nation;
    const char *comp_file;           /* source file of the original's class (eng_prm), "new", "na", or "a;b" */
    const char *comp;
    int tier;                        /* 1 = top division, or SEASON_TIER_* */
    const char *tier_code;           /* the value of the file: "1", "cup", "state_1", ... */
    const char *name;                /* the real name of that season */
    int clubs, up, down, subs_used, bench;          /* counts or SEASON_RULE_* */
    const char *playoffs, *split, *format;          /* codes: na, unknown, none, not_held, see_format, ..., text */
    const char *clubs_text, *up_text, *down_text, *playoffs_text, *split_text, *format_text, *subs_used_text,
        *bench_text;
    const char *real_season, *foreign, *work_permit, *registration, *windows, *notes, *sources;
} SeasonRule;

/* Reads the table once at the start: RECOMP_RULES_TABLE (test runs) or Seasons/rules.tsv of the game folder
 * game_dir. It uses host memory only and never the game's memory. Returns the row count, 0 when the file is
 * missing or damaged. */
int season_rules_init(const char *game_dir);

/* Reads a table file (also for tests). Returns the row count, or 0 and keeps no row when the file is
 * damaged. A later call replaces the table. */
int season_rules_load(const char *path);

int season_rules_count(void);
const SeasonRule *season_rules_row(int i);           /* NULL out of range */

/* The n-th row (from 0) of a season whose comp_file holds file (one of the ";" names), or NULL. */
const SeasonRule *season_rules_find(int season, const char *file, int n);
/* The same by a scan of every row, for the test of the index that season_rules_find uses. */
const SeasonRule *season_rules_find_scan(int season, const char *file, int n);

/* The competition classes of the original (comp_classes.c of the build, tools/seasons/make_comp_classes.py). */
typedef enum { SEASON_KIND_COMP, SEASON_KIND_LEAGUE, SEASON_KIND_CUP } SeasonCompKind;
typedef struct {
    uint32_t vtable;
    const char *file;                /* eng_prm */
    const char *cls;                 /* EngPrm */
    SeasonCompKind kind;
} SeasonCompClass;
extern const SeasonCompClass season_comp_classes[];
extern const int season_comp_class_count;

/* The rules report and the season jump of the test variables RECOMP_RULES_REPORT=<file> and
 * RECOMP_SEASON_JUMP=<start year of the last season> (season_report.c). season_report_init reads them at the
 * start (main.c) and stops the program on a bad value. season_report_turn runs at each human turn (testrun.c). */
void season_report_init(void);
void season_report_turn(void);

#endif
