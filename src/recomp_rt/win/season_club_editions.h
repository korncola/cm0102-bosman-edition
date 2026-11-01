/* The editions of the club cups of the rules table (0.6.2 slice 9a; spec docs/specs/2026-10-30-future-play-0.6.2-spec.md,
 * slice 9a). It reads only the rules table (season_rules.h) and no game memory, so ctest steps it to 2301.
 *
 * season_club_cups.c plays the Copa Sudamericana, the Recopa, the Suruga Bank Championship, the FIFA Intercontinental
 * Cup and AFC Champions League Two from the dates and club counts of the table's format text. The table ends with
 * 2027/28, and its last rows of these cups say "unknown". The after-table rule (decision A, Q2): after the last
 * edition that the table gives, a cup whose last row is not "not held" plays every year in the format of that last
 * edition, on the same days and months. The Suruga Bank Championship (last rows not held, Q7) never comes back.
 *
 * The FIFA Club World Cup (season_kind_club_world.c, Q3): after the last row of the table (2027/28, not held) it
 * plays every 4 years from the season of its last held row (2024/25, the 32 clubs of June and July 2025), so in the
 * seasons 2028/29, 2032/33 and on, with that row: the editions of 2029, 2033 and on.
 *
 * The Copa Libertadores places (Q2 review note, slice 9a): the row of a season with no "places:" text takes the
 * places of the latest row before it that has one. */
#ifndef RECOMP_SEASON_CLUB_EDITIONS_H
#define RECOMP_SEASON_CLUB_EDITIONS_H

#include "season_rules.h"

#define SEASON_CWC_CYCLE 4              /* the Club World Cup after the table, in years (Q3) */

/* A club cup of the table: its `comp`, 1 for one edition a calendar year (0: one a season), 1 when the dates start
 * after the first ";" of the text (ACL Two: its qualifying comes first). */
typedef struct {
    const char *comp;
    int calendar;
    int skip_first_clause;
} SeasonClubCup;

typedef struct {
    int first, last;                    /* days of the year (from 0) of `year` and `last_year` */
    int year, last_year;                /* the calendar years of the first and the last date */
    int clubs;                          /* the club count of the edition */
    int legs, final_legs;               /* 1 or 2 */
    const SeasonRule *row;              /* the table row that gives the edition */
    int open_end;                       /* 1 while the text gives no last day (see season_club_edition_in_table) */
} SeasonClubEdition;

/* Day of the year from 0, with the month starts of Date.cpp (0x9b8448, 0x9b8460) and its leap test. */
int season_club_is_leap(int y);
int season_club_day_of_year(int day, int month, int year);

/* The edition of year y (a calendar year, or a season for a cup of one edition a season) by the rules of y,
 * with the after-table rule: 1 when it is held and its dates and clubs are known. */
int season_club_edition(const SeasonClubCup *k, int y, SeasonClubEdition *e);

/* The edition of the table alone (no after-table rule). */
int season_club_edition_in_table(const SeasonClubCup *k, int y, SeasonClubEdition *e);

/* The last year with an edition of the table, 0 when none. */
int season_club_last_edition(const SeasonClubCup *k);

/* 1 when the cup continues after its last table edition: its last row is not "not held". */
int season_club_continues(const SeasonClubCup *k);

/* The first year from y on with an edition (season_club_edition), 0 when none ever comes. */
int season_club_next_edition(const SeasonClubCup *k, int y);

/* Edition e moved by `years`, with the same days and months (29 February becomes 1 March). */
void season_club_edition_move(SeasonClubEdition *e, int years);

/* The row of the Club World Cup class `file` in season s: the latest row up to s; after the table the last held row
 * every SEASON_CWC_CYCLE years from its season when the last row is not held (Q3). NULL with no row. Other files:
 * the latest row up to s. */
const SeasonRule *season_club_world_row(const char *file, int s);

/* The row of comp in season s with a "places:" text: the latest such row up to s, NULL when none. */
const SeasonRule *season_club_places_row(const char *comp, int s);

#endif
