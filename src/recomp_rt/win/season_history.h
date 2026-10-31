/* Competition history between 2001 and the start season (0.6.0 slice 11c, Q347).
 *
 * The season databases before 2010/11 hold no competition history between 2001 and the season before the
 * start. (Four databases of 2010/11 and 2011/12 hold no US Open Cup row at all, though Con_Champ::pick_clubs
 * 0x4c11a0 reads its newest winner for CONCACAF place 1: plan.tsv gives them the researched US Open Cup rows only,
 * tools/seasons/history_fill.py, GAP_COMPETITIONS.) At the database load of a new game in the season of such a database, after the year shift of slice
 * 11b, the game adds the researched rows (docs/research/history-0.6.0/, the research of each fact with its
 * sources) to the history table in memory. The database files stay unchanged. A loaded save needs nothing: it
 * holds the rows.
 *
 * For each competition that holds history rows in the database and each label year from 2002 to the start
 * year that the research covers, the rows of the database for that year are cleared (their competition becomes
 * 0, the free row of the original, 0x4afce0), and the researched rows are written. The research wins over the
 * rows of the database: most of those rows hold the result of another season (slice doc
 * docs/research/2026-10-17-competition-history-fill.md).
 *
 * It runs from season_dates.c, right after the shift, and it is undone with the shift when the shift is made
 * again for another year. It does nothing in the base game (start 2001), for a start year that is not the
 * season of the database, for a Custom database, and with RECOMP_SEASON_CODE=off.
 *
 * Duplicate rows. Many season databases hold the same history row twice: the same competition, label year and
 * four places (September 2010 ODB: 175 rows, among them the English Premier Division 2001 Chelsea / Manchester
 * United, which the shift of slice 11b makes 2010; tests/seasons/history_duplicates_check.py lists every
 * database). The winners tab 0x4991f0 then shows the year twice. Before the fill, every season database load
 * with a shift (season_dates.c) clears the second and later copy of each such row in the same way, and the undo
 * gives them back. The base game and its 3.9.68 data (2 such rows) are not touched.
 *
 * Rows after the start (0.6.2 slice 7, open items 3 and 17). The data holds results of editions that the game
 * then plays itself: the 3.9.68 data holds 128 club rows and 3 nation rows of the label year 2002 (the 2001/02
 * season, Arsenal) and the results of August to December 2001 (Charity Shield 2001 Liverpool / Manchester
 * United), in a game that starts in July 2001; the season databases hold the calendar-year leagues and the
 * super cups of their start year. The game then writes the same edition again, so the history shows the year
 * twice, and it reads the newest row (Con_Champ::pick_clubs 0x4c11a0 through 0x4afd80): its 2001 Charity Shield
 * is Liverpool against Arsenal, the 2002 champion. So at the creation of a new game every row with a label after
 * the start year S goes (season_history_drop_after_start, with the undo record; in the official data also the
 * label S of a split season, which that data labels by the year where it starts), and when the game writes a row
 * (0x4afce0), every other row of that competition with a later label goes, and in the calendar year S the row of
 * the data with the same label (not when the data holds two or more of them: the halves of a split season). A row
 * of the year S of a competition that ends before July (the English Premier Division 2001 in a 2001/02 game)
 * stays: the game writes no row of that label. A second edition of a competition in one calendar year (a final that
 * moves across the new year, as the Emperor's Cup) takes the next free label, unless the competition is a split
 * season or the row of that label is one the game wrote on the same day (the Inter-Toto Cup winners). */
#ifndef RECOMP_SEASON_HISTORY_H
#define RECOMP_SEASON_HISTORY_H

#include <stdint.h>

typedef struct {
    const char *comp;               /* competition name (Latin-1, as the data writes it) */
    uint16_t label;                 /* label year whose rows the database loses */
} SeasonHistClear;

typedef struct {
    const char *comp;
    uint16_t label;
    const char *place[4];           /* first, second, third, host: full club or national side names, "" for none */
    const char *nation[4];          /* the nation of each, to choose between clubs of the same name */
} SeasonHistRow;

typedef struct {
    const char *db;                 /* the neutral name of the database (seasons_db.h, season_dates.h) */
    uint16_t start;                 /* the start year that the rows are for: the season of the database */
    const SeasonHistClear *clears;
    uint32_t n_clears;
    const SeasonHistRow *rows;
    uint32_t n_rows;
} SeasonHistDb;

extern const SeasonHistDb season_history_dbs[];          /* season_history_table.c */
extern const int season_history_dbs_count;

/* After the shift of a database load or a new shift (season_dates.c): the rows of database `name` for start
 * year N: first the duplicate rows go (above), then the researched rows. Nothing more when the table has no
 * entry for that database and year. */
void season_history_fill(const char *name, int N);

/* At the creation of a new game, after the shift and the fill: clears every row with a label after S (above), and
 * with official (the 3.9.60, 3.9.65 or 3.9.68 data) the rows of S of a split season. */
void season_history_drop_after_start(int S, int official);

/* 1 when the rows after the start go (the head): the season code is on (and, with FIX_BASE_GAME 0, the game is
 * not the base game). */
int season_history_after_start_on(void);

/* Before a new shift (season_dates.c): gives back the rows and the row count of the last fill. */
void season_history_undo(void);

/* When the loaded data no longer comes from a database load (season_dates_forget): drops the undo record. */
void season_history_forget(void);

/* The number of rows that the last fill added and cleared (a test reads them). */
void season_history_counts(uint32_t *added, uint32_t *cleared, uint32_t *unmatched);

/* At each human turn (testrun.c): the test log of RECOMP_HISTORY_LOG=<competition>;... (season_history.c). */
void season_history_log_turn(void);

#endif
