/* Promotion play-offs in leagues whose class has none (0.6.0 slice 19). See season_playoffs.c. */
#ifndef RECOMP_SEASON_PLAYOFFS_H
#define RECOMP_SEASON_PLAYOFFS_H

#include <stdint.h>

/* The wrap of the empty function 0x5a8f60 (advance_stage, season_uefa_cup.c) calls it first: 1 when obj is a
 * league of this kind whose table is complete and the play-off stage is built now (the wrap then returns). */
int season_playoffs_advance(uint32_t obj);

/* 1 when the rules row of file and season gives a play-off that this kind plays (4 places in a row just below
 * the direct places); *direct (may be NULL) gets the direct promotion places. */
int season_playoffs_shape(const char *file, int season, int *direct);

/* The play-off winners that the rules row of file and season sends up although this kind plays no play-off for
 * them: 2 for 4 places just below the direct places in two ties whose winners both go up (Série B from 2025:
 * "two-legged ties 3rd v 6th and 4th v 5th, the two winners go up"), else 0. The game plays no such play-off, so
 * the nearest form (Q247) sends the first two play-off places up directly: season_sizes.c adds them to the direct
 * places. Without them Série A sent 4 down and took 2 up, and lost 2 clubs every season (0.6.2 open item 63).
 * season_playoff_winners.c, host code only. */
int season_playoffs_winners_up(const char *file, int season);

/* After set_up_rules of a league (season_sizes_rules calls it): the places and the stage count of the play-off
 * of the season, or none. */
void season_playoffs_rules(uint32_t obj);

/* Around game_season_update_pass 0x5d5ae0 (season_change.c): an entry whose play-off stage is not complete
 * waits for it (season_playoffs.c). hold returns 1 when it changed an update day; release puts it back. */
int season_playoffs_hold(void);
void season_playoffs_release(int held);

#endif
