/* The missing UEFA season coefficient of some season databases (0.6.2 open item 106).
 *
 * A nation record holds six season coefficients, doubles at +0xe0 to +0x108, oldest first. At a new game in start
 * year S the game expects the seasons (S-6)/(S-5) to (S-1)/S there: the first season ranks the nations by +0xe0 to
 * +0x100 (UefaSeeding::roll_coefficients 0x903b50 makes no shift in the first season, 0x903b6f), the European
 * Performance Spots of the first season read +0x108 (season_uefa_seeding.c), and each later season change shifts
 * the six down one place and writes the game's own points to +0x108 (0x903b50). So +0x108, the season before the
 * start, counts in the five-season totals of the next five seasons.
 *
 * "2025 Data Update" (December 2025) and "May 2026 Data Update" (May 2026) hold 0 at +0x108 for every nation: the
 * 2024/25 season is missing, and a 2025 game counts a zero season in every total until 2029/30. The October 2026
 * database holds 2024/25 (Max, 3 November 2026: the 2025/26 databases take these values). At the database load of
 * a new game (season_dates.c, after the shift and the history fill), the game writes +0x108 of each nation of the
 * table. The database files stay unchanged. A loaded save needs nothing: it holds the nation records.
 *
 * A row is written only when the record of its nation id holds that id, its +0x100 is the value of the known
 * database and its +0x108 is 0 or already the value (a nation.dat that a player changed keeps its own values).
 * Nothing runs with RECOMP_SEASON_CODE=off or for a database the table does not name.
 *
 * The table (season_coeffs_table.c) is made by tools/seasons/make_coeff_table.py from
 * docs/research/2026-11-03-database-coefficients.tsv, which the tool measures from the databases. */
#ifndef RECOMP_SEASON_COEFFS_H
#define RECOMP_SEASON_COEFFS_H

#include <stdint.h>

typedef struct {
    uint16_t id;                    /* the nation id (the record index) in the database */
    const char *nation;             /* its name in the database, for the log */
    double check_100;               /* the database's +0x100 (the season before the missing one) */
    double value_108;               /* the missing season */
} SeasonCoeff;

typedef struct {
    const char *db;                 /* the neutral name of the database (seasons_db.h) */
    const SeasonCoeff *rows;
    int n_rows;
} SeasonCoeffDb;

extern const SeasonCoeffDb season_coeff_dbs[];          /* season_coeffs_table.c */
extern const int season_coeff_dbs_count;

/* After the shift of a database load (season_dates.c): the rows of database `name` (NULL: none). */
void season_coeffs_fill(const char *name);

#endif
