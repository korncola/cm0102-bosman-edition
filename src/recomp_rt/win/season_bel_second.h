/* Belgium: U23 sides stay below their first team and out of the top tier (open item 115), and from 2015/16 the
 * Second Division's 16th goes down directly (open item 124); season_bel_second.c. */
#ifndef RECOMP_SEASON_BEL_SECOND_H
#define RECOMP_SEASON_BEL_SECOND_H

#include <stdint.h>

/* 1 when club (a club record) is a Belgian U23 side and div (a club_comp record) is the Belgian First Division, or a
 * division at the level of its first team or above: the club moves of a new game do not put it there. */
int season_bel_barred(uint32_t club, uint32_t div);

#endif
