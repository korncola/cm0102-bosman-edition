/* The option manager_ability_grows: the ability of a human manager follows his reputation.
 *
 * A staff member without player data keeps his ability and potential in his non-player data (staff +0x69): ability
 * at +4, potential at +6, his three reputations at +8, +0xa and +0xc (each up to 10000). game_daily_staff_update
 * 0x5ce2d0 moves the ability of every non-player with a club a little each day, and keeps it at most at the
 * potential (0x5d347a..0x5d3520), but it leaves out every manager (job 5, 0x5d341b). So a human manager keeps the
 * ability the game drew for him at the start, 17 or 59 in two real games, while the managers of the clubs he may
 * apply to range up to 190. score_staff_for_job 0x69d1b0 reads that ability (0x69d6fe) when a club picks its man.
 *
 * With the option on, after the original's daily pass, each of the 16 human manager rows (the last rows of the staff
 * table, see staff_reserve.c) gets potential 200 and its ability moves one point a day towards the sum of its three
 * reputations over 128, at most the potential. A manager at 10000, 10000, 9750 lands near 230 and so at 200; an AI
 * manager with those reputations has 175 to 190. The aim and the potential follow the patch that Tapani made for the
 * original, which moved the ability one point at each background match. Only the host of a network game runs it. */
#include "win.h"
#include "options.h"
#include "../rt.h"

#define STAFF_COUNT 0xae2364u
#define STAFF_TABLE 0xae23bcu
#define STAFF_ROW 0x6eu
#define STAFF_NON_PLAYER 0x69u
#define HUMAN_ROWS 16
#define ABILITY 4u
#define POTENTIAL 6u
#define REPUTATIONS 8u
#define TOP 200

void o_005ce2d0(void);

static void grow(uint32_t data)
{
    if ((int16_t)rd16(g_mem, data + POTENTIAL) < TOP) wr16(g_mem, data + POTENTIAL, TOP);
    int sum = 0;
    for (uint32_t i = 0; i < 3; i++) sum += (int16_t)rd16(g_mem, data + REPUTATIONS + 2 * i);
    int aim = sum / 128, ability = (int16_t)rd16(g_mem, data + ABILITY);
    if (aim > TOP) aim = TOP;
    if (ability < aim) ability++;
    else if (ability > aim) ability--;
    wr16(g_mem, data + ABILITY, (uint16_t)ability);
}

RT_WRAP(005ce2d0, daily_staff_update)
{
    o_005ce2d0();
    if (!options_bool(OPT_MANAGER_ABILITY_GROWS)) return;
    int count = (int)rd32(g_mem, STAFF_COUNT);
    uint32_t table = rd32(g_mem, STAFF_TABLE);
    for (int row = count - HUMAN_ROWS; row < count; row++) {
        if (row < 0) continue;
        uint32_t data = rd32(g_mem, table + (uint32_t)row * STAFF_ROW + STAFF_NON_PLAYER);
        if (data) grow(data);
    }
}
