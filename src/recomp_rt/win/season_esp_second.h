/* Spain and Portugal: B and C sides newer than the key pairs of 0x540a50 (open items 121 and 122;
 * season_esp_second.c). */
#ifndef RECOMP_SEASON_ESP_SECOND_H
#define RECOMP_SEASON_ESP_SECOND_H

#include <stdint.h>

/* 1 when club (a club record) is a Spanish or Portuguese B or C side and a team above it in its club (its B side or
 * its first team) has div (a club_comp record) as its division now: the real rule bars it from that division. */
int season_esp_barred(uint32_t club, uint32_t div);

/* Around the original's exchange of the Portuguese Second Division B, por_prm_exchange_second_b_with_lower 0x7cff70
 * (the form with the database's zones): phase 0 before the call, 1 after it. nation_key is the key of the nation of
 * the tier ([0x9cf434] Portugal, [0x9cf488] Spain). */
void season_esp_second_pool(uint32_t nation_key, int phase);

#endif
