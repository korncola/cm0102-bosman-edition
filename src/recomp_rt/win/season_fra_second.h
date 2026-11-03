/* France: no reserve side in the National, Ligue 2 or Ligue 1 (open item 116; season_fra_second.c). */
#ifndef RECOMP_SEASON_FRA_SECOND_H
#define RECOMP_SEASON_FRA_SECOND_H

#include <stdint.h>

/* 1 when club (a club record) is a French reserve side and div (a club_comp record) is the National, Ligue 2 or
 * Ligue 1: the real rule keeps a reserve side in the CFA or below. */
int season_fra_barred(uint32_t club, uint32_t div);

#endif
