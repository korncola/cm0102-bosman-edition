/* The option manager_ability_grows: the ability of a human manager follows his reputation, as the ability of the
 * AI managers of the same game does.
 *
 * A staff member without player data keeps his ability and potential in his non-player data (staff +0x69): ability
 * at +4, potential at +6, his three reputations at +8, +0xa and +0xc (each up to 10000). The ratings of his
 * attributes depend on that ability: non_player_attribute_rating 0x544410 rates (ability + 2 x the stored byte), so
 * a stored 0 shows as 8 at ability 17, 13 at 101 and 20 at 190.
 *
 * game_daily_staff_update 0x5ce2d0 (once a game day, called at 0x5cd9f1) moves the ability of every non-player
 * with non-player data and a club each day, at most up to his potential (0x5d3367..0x5d3520). It leaves out every
 * manager: job 5 jumps past that code (0x5d341b). So a human manager keeps the ability that the game drew for him
 * at the start for the whole game.
 *
 * With the option on, after the original's pass, each human manager (one of the 16 last rows of the staff table,
 * see staff_reserve.c, whose kind +0x5f says it has non-player data; an unused row has kind 0) gets potential 200, and its ability moves one point a day towards an aim: the ability
 * that the AI managers of this game have at his reputation. The aim comes from a least squares line of ability
 * over the sum of the three reputations, through every AI manager (job 5) with non-player data. In a 2026/27 game
 * of 3918 such managers the line is about 57 + sum / 241, in a 2001/02 game of England 36 + sum / 179, so it is
 * fitted anew each day rather than fixed. With fewer than MIN_MANAGERS the aim is (sum + 96) / 128, the aim of the
 * patch that Tapani made for the original. Only the host of a network game runs it. */
#include "win.h"
#include "options.h"
#include "../rt.h"

#include <stdio.h>
#include <stdlib.h>

#define STAFF_COUNT 0xae2364u
#define STAFF_TABLE 0xae23bcu
#define STAFF_ROW 0x6eu
#define STAFF_JOB 0x3du
#define STAFF_NON_PLAYER 0x69u
#define STAFF_KIND 0x5fu       /* 1 non-player data, 3 player and non-player data, 0 an unused row */
#define JOB_MANAGER 5
#define HUMAN_ROWS 16
#define ABILITY 4u
#define POTENTIAL 6u
#define REPUTATIONS 8u
#define TOP 200
#define MIN_MANAGERS 20

void o_005ce2d0(void);

static int reputation_sum(uint32_t data)
{
    int sum = 0;
    for (uint32_t i = 0; i < 3; i++) sum += (int16_t)rd16(g_mem, data + REPUTATIONS + 2 * i);
    return sum;
}

/* The line ability = a + b * sum through the AI managers. 0 when there are too few of them to fit one. */
static int fit_line(uint32_t table, int ai_rows, double *a, double *b)
{
    double n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int row = 0; row < ai_rows; row++) {
        uint32_t person = table + (uint32_t)row * STAFF_ROW;
        if (rd8(g_mem, person + STAFF_JOB) != JOB_MANAGER) continue;
        uint32_t data = rd32(g_mem, person + STAFF_NON_PLAYER);
        if (!data) continue;
        double x = reputation_sum(data), y = (int16_t)rd16(g_mem, data + ABILITY);
        n++, sx += x, sy += y, sxx += x * x, sxy += x * y;
    }
    double spread = n * sxx - sx * sx;
    if (n < MIN_MANAGERS || spread <= 0) return 0;
    *b = (n * sxy - sx * sy) / spread;
    *a = (sy - *b * sx) / n;
    return 1;
}

static void grow(uint32_t data, int fitted, double a, double b)
{
    if ((int16_t)rd16(g_mem, data + POTENTIAL) < TOP) wr16(g_mem, data + POTENTIAL, TOP);
    int sum = reputation_sum(data);
    int aim = fitted ? (int)(a + b * sum + 0.5) : (sum + 96) >> 7;
    if (aim > TOP) aim = TOP;
    if (aim < 1) aim = 1;
    int ability = (int16_t)rd16(g_mem, data + ABILITY);
    if (ability < aim) ability++;
    else if (ability > aim) ability--;
    wr16(g_mem, data + ABILITY, (uint16_t)ability);
    /* RECOMP_MANAGER_LOG: the line and the aim, for the check of the option. */
    if (getenv("RECOMP_MANAGER_LOG"))
        fprintf(stderr, "recomp: manager ability %d aim %d (sum %d, %s %.1f + %.5f x sum)\n", ability, aim, sum,
                fitted ? "line" : "fallback", a, b);
}

RT_WRAP(005ce2d0, daily_staff_update)
{
    o_005ce2d0();
    if (!options_bool(OPT_MANAGER_ABILITY_GROWS)) return;
    int count = (int)rd32(g_mem, STAFF_COUNT);
    if (count < HUMAN_ROWS) return;
    uint32_t table = rd32(g_mem, STAFF_TABLE);
    double a = 0, b = 0;
    int fitted = fit_line(table, count - HUMAN_ROWS, &a, &b);
    for (int row = count - HUMAN_ROWS; row < count; row++) {
        uint32_t person = table + (uint32_t)row * STAFF_ROW, data = rd32(g_mem, person + STAFF_NON_PLAYER);
        if (data && (rd8(g_mem, person + STAFF_KIND) & 1)) grow(data, fitted, a, b);
    }
}
