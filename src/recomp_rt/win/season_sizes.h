/* League sizes, promotion and relegation in existing divisions (0.6.0 slice 18, spec
 * docs/specs/2026-10-16-seasons-0.6.0-spec.md, Q247, Q272, Q281). Research and evidence:
 * docs/research/2026-10-18-league-sizes.md.
 *
 * Only a season game (season_game_changes, slice 15) gets any of it. The base game and RECOMP_SEASON_CODE=off
 * keep the original's counts.
 *
 * The rules of a league in a season: the row of the rules table (season_rules.h) of its source file. A new
 * game, and every season of a game with changing rules off, takes the row of the start season (Q232). With
 * changing rules on a later season takes the row of the edition that the object holds (+0x40): the row whose
 * real season begins with that year (a calendar-year league holds edition Y + 1 in the row of season Y), else
 * the row of that season. After the last row the last row stays (Q248).
 *
 * - Club counts: a class holds no more clubs than its largest constant (its table and date template are made
 *   for that count, tools/recomp/league_sizes.py CAPS), and no fewer than its own count where a cup of the
 *   nation needs the whole division (FLOORS, the cups are slice 21). The club moves of a new game
 *   (season_clubs.c) fill each division of tools/recomp/league_sizes.py to the count of its start season row. create_table then takes the clubs that the division holds
 *   (rt_league_clubs, the patched stores of league_sizes.py). In later seasons the game's own results decide who
 *   moves (Q247), so a division holds the count that promotion and relegation give it. With a new count the
 *   matches per club (+0xa7) go back to -1, so League::check_complete 0x689ad0 counts them again (most classes
 *   keep the first season's value, 0x689b1d..0x689b36). The French classes get a date template of the rounds
 *   of their count (rt_league_dates).
 * - Promotion and relegation places: after each set_up_rules (season_size_wraps.c) the places of the row. A
 *   row's count can hold the winner of a play-off that the class plays (the rules report reads it the same way),
 *   so the direct places are the count less one when the class has play-off places.
 * - league_promote_and_relegate 0x689c80: when the places of two size-table divisions differ, the clubs move on
 *   their own only with changing rules on and when both divisions stay within their floor and cap, else in pairs
 *   (the sizes stay). An expected count of the caller that differs from the moves is dropped (Serie A and B pass
 *   4 and 4 at 0x642476).
 * - News: one sentence for each count or place that the game changes in a season (season_change.h). */
#ifndef RECOMP_SEASON_SIZES_H
#define RECOMP_SEASON_SIZES_H

#include <stdint.h>

/* The club count that create_table of a league stores (+0x3e): original in the base game, else the clubs that
 * the division holds (rt.h declares it for the translated code). */
uint32_t rt_league_clubs(uint32_t obj, uint32_t original);
uint32_t rt_league_dates(uint32_t obj, uint32_t original);

/* After set_up_rules of a league class: the promotion and relegation places of the season. */
void season_sizes_rules(uint32_t obj);

/* The club count of a division of a league class in the row of a season (the club moves of a new game), or -1
 * when the class is not in the size table or the row holds no usable count. */
int season_sizes_clubs(const char *file, int season);

/* 1 when the club moves leave a division of this file the clubs of the database, up to the class count, where its
 * row has fewer (a cup takes the division whole). */
int season_sizes_keeps_database(const char *file);

/* The source files of the league classes of tools/recomp/league_sizes.py (season_size_wraps.c). */
extern const char *const season_size_files[];
extern const int season_size_file_count;
extern const int season_size_caps[];         /* the largest count of each file, 0: the class counts its data */
extern const int season_size_floors[];       /* the smallest count, 0: none (a cup of the nation needs it) */

#endif
