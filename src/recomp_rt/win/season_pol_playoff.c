/* The I liga's promotion play-off waits for the Ekstraklasa's last stage (0.6.2 open item 104).
 *
 * PolFirst (the Ekstraklasa) plays two stages: the groups (its own table, 14 rounds to November) and the spring stage
 * (stage 1, [+0xc][1], 16 clubs). The spring stage marks its places 13 and 14 for the relegation play-off (status +0x37
 * 2 in the stage's table rows, League::update_positions_and_status 0x684cd0). PolSecond (the I liga) plays its own
 * table and then one stage, the play-off of its places 3 and 4 against those two clubs.
 *
 * League::update_for_date 0x6847c0 calls the I liga's advance_stage (vtable +0x28, 0x6849e0) on each day that its own
 * table and built stages are complete (check_complete at 0x684994 and 0x6849be) and a stage is still to come.
 * PolSecond::advance_stage 0x7cc1c0 tests the Ekstraklasa ([0x9cf858]) with check_complete(0, 1) (0x7cc24a..0x7cc253):
 * the first argument 0 skips its stages (League::check_complete 0x689ad0, the stage test 0x689bb2..0x689be8 runs only
 * for 1), so the test passes from November, when the groups end. It then calls update_positions_and_status of the
 * spring stage (vtable +0x94 at 0x7cc270) and makes the play-off (create_play_off_stage 0x7cc290). That function
 * takes the I liga rows with status 1 and the spring stage rows with status 2 into four team slots (0x7cc300..0x7cc375)
 * and gives them to the Cup_Stage (0x522e00). When the spring stage has not ended, only the places already decided
 * carry status 2, so a slot stays 0. Cup::draw_round 0x51c410 then moves the draw one day on each day while a slot of
 * the round is empty (0x51c47c..0x51c549), so the play-off is never drawn, the I liga is never complete, the
 * Ekstraklasa's chain test (Comp::all_stages_complete 0x48ce10) fails on the Polish update day (entry 25, day 170) and
 * both leagues keep their edition for a year (league-stall).
 *
 * In the base data the spring stage ends about a week before the I liga's last round (day 136 against day 143 in 2200),
 * so the test of the groups alone was enough. In the date jump to 2198 (run set pass 2, jump2198, 2199/00) an
 * Ekstraklasa match of the spring stage came after the I liga's last round on day 143: the play-off was made that day
 * with the spring stage rows 13 727d0fd9 (status 255, still open) and 14 (status 2), so its fourth slot was 0, and its
 * draw moved from day 144 to day 200 and on. Whether a spring match comes late depends on the order of the days'
 * matches: the same leg loaded on day 120 played its spring stage to the end on day 141.
 *
 * So in a season game the I liga's advance_stage waits, one day at a time as update_for_date calls it, until every
 * built stage of the Ekstraklasa is complete and its last stage is built (the test that check_complete(1, ...) makes,
 * read here without its side effects: check_complete sets the complete flag +0xdd and calls vtable +0x94 the first time,
 * 0x689bea..0x689c06). The play-off then takes the final places. Its dates stay those of create_schedule (draw 25 May,
 * first leg 31 May, PolSecond::create_schedule 0x7cbd6a..0x7cbd94): a draw date in the past is drawn on the next update
 * (Cup::update_for_date 0x51f890, draw when the date is today or before, 0x51fa45..0x51fa7d), and a leg date in the past
 * goes to tomorrow (Cup::create_tie_fixtures 0x51cb90). A game where the spring stage ends first is not changed: the
 * test passes on the same day as the original's. The base game and RECOMP_SEASON_CODE=off keep the original (Q152). */
#include "win.h"
#include "../rt.h"
#include "../rt_host.h"
#include "season_names.h"

#include <stdio.h>
#include <stdint.h>

#define COMP_OBJECTS 0x00adadfcu        /* g_comp_object_table */
#define COMP_SLOTS 0x00adadf8u
#define KEY_POL_FIRST 0x009cf858u       /* the Ekstraklasa's competition id (0x7cc245) */
#define TODAY 0x00ae2c90u               /* Date: day of the year +0, year +2 */

static int readable(uint32_t p) { return p >= 0x10000u && p < 0xfff00000u; }

/* League::check_complete 0x689ad0 of one league or stage, read only: a table (+0xb1) that is not locked (+0xb5) of a
 * league that is made (+0x45), and every row (0x3b bytes) with its played count (s8 +0x11) at the matches per club
 * (+0xa7; -1 before the first test, then (clubs - 1) * meetings, 0x689b1d..0x689b36). A league whose complete flag
 * +0xdd is set is complete (0x689b01..0x689b0f). */
static int league_complete(uint32_t lg)
{
    if (!readable(lg) || !readable(rd32(g_mem, lg + 0xb1u)) || g_mem[lg + 0xb5u] || !rd32(g_mem, lg + 0x45u))
        return 0;
    if (rd32(g_mem, lg + 0xddu)) return 1;
    int clubs = (int16_t)rd16(g_mem, lg + 0x3eu), matches = (int16_t)rd16(g_mem, lg + 0xa7u);
    if (matches == -1 && clubs > 0) matches = (int16_t)((clubs - 1) * (int16_t)rd16(g_mem, lg + 0x3cu));
    uint32_t rows = rd32(g_mem, lg + 0xb1u);
    for (int i = 0; i < clubs; i++)
        if ((int8_t)g_mem[rows + 0x3bu * (uint32_t)i + 0x11u] < matches) return 0;
    return 1;
}

/* The stage part of check_complete(1, ...) (0x689bb6..0x689be8): every built stage (0 to +0x30) complete, and the
 * last stage (+0x2c - 1) built. */
int season_pol_first_done(uint32_t lg)
{
    if (!league_complete(lg)) return 0;
    int32_t built = (int32_t)rd32(g_mem, lg + 0x30u), count = (int32_t)rd32(g_mem, lg + 0x2cu);
    uint32_t stages = rd32(g_mem, lg + 0xcu);
    if (built < count - 1) return 0;
    for (int32_t i = 0; i <= built && i < 64; i++)
        if (!readable(stages) || !league_complete(rd32(g_mem, stages + 4u * (uint32_t)i))) return 0;
    return 1;
}

void o_007cc1c0(void);

/* PolSecond::advance_stage, thiscall, no arguments. */
RT_WRAP(007cc1c0, pol_second_advance)
{
    uint32_t self = cpu.ecx, table = rd32(g_mem, COMP_OBJECTS), id = rd32(g_mem, KEY_POL_FIRST);
    uint32_t first = readable(table) && id < rd32(g_mem, COMP_SLOTS) ? rd32(g_mem, table + 4u * id) : 0;
    /* Only the step that makes the play-off (+0x30 from -1 to 0, 0x7cc276..0x7cc281) waits. */
    if (season_game_changes() && (int32_t)rd32(g_mem, self + 0x30u) == -1 && readable(first) &&
        league_complete(first) && !season_pol_first_done(first)) {
        static int logged = -1;                     /* the edition of the last line: one line an edition */
        int edition = (int16_t)rd16(g_mem, self + 0x40u);
        if (logged != edition) {
            fprintf(stderr, "recomp: playoffs: pol_second %d: the play-off waits for the end of the Ekstraklasa's "
                            "last stage (day %d of %d)\n", edition, (int16_t)rd16(g_mem, TODAY),
                    (int16_t)rd16(g_mem, TODAY + 2u));
            logged = edition;
        }
        rt_ret_stdcall(cpu.eax, 0);
        return;
    }
    o_007cc1c0();
}
