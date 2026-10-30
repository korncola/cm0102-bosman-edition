/* The Tier A switch (0.6.2 slice 3, decisions Q9, Q10, Q16 and Q31 of the future play spec). RECOMP_TIER_A=1 is a
 * test switch, read once at the start and off by default. Off, every wrap below runs the original at once, so the
 * game is the same (Q152). On, a game plays only the competitions, to reach far years quickly: the calendar, draws,
 * entrants, league tables, season changes, history, finances, retirements, new players, and save and load stay.
 * The AI transfer market, background matches, contracts, injuries and AI manager moves stop, and each match gets an
 * instant result from the two sides' reputation. Research: run-logs of slice 3 (research-match.md and
 * research-skips.md, read again for each address cited here) and run-logs/future-3/profile.md (the share of a
 * normal year of each area).
 *
 * The skips, each a full no-op that pops the original's arguments (no caller reads eax, cited per wrap):
 * - The AI transfer market: update_club_shortlist 0x83a730 (ret 4 at 0x83a772, called at 0x5cdd15),
 *   make_club_bids 0x84a020 (ret 4 at 0x84abbf, called at 0x5cdd6d), the position lists 0x839e60 (ret 4 at
 *   0x83a12c, called at 0x5cda46) and 0x83a780 (ret at 0x83a9c4, called at 0x5cda56), and
 *   add_player_to_club_shortlists 0x842c00 (ret 8 at 0x842dfe, 13 callers, none reads eax). The day loop's
 *   club loop 0x5cdc4e..0x5cdd9a still runs, so the media calls 0x754a30 and 0x757c70 in it (their only callers)
 *   keep the story log short. The original's flag 0x9cd1f8 would skip them too, so it is not used.
 * - Background matches: SimulatedStats::play_daily_matches 0x84be20 (ret at 0x84c4fb, called at 0x5cdac9). It
 *   writes no fixture, table or history row, only player stats of clubs with no loaded league.
 * - Contracts: the contract pass 0x4c7ac0 (ret 0xc at 0x4c9e2e, called at 0x5cd406) and the daily staff pass
 *   0x5ce2d0 (cdecl, ret at 0x5d590c, called at 0x5cd9f1), which makes the AI contract offers (0x4cda70 at
 *   0x5d19d7 and five more), morale, disputes, wishes and the shortlist offers. Contracts and loans then never
 *   end, so with no market a squad changes only by retirements and new players. The fee instalments, the nation
 *   rules objects, work permits and the free transfer queue of 0x8c5880 stay (finances).
 * - Injuries: Injury::add_injury 0x62d250 (ret 0x20, wrapped in injury_init.c) returns -1 after the game is made.
 *   The injuries of the database and of game creation stay, and heal by date when read (0x62c680 from 0x62c400
 *   and 0x62ed70), with no daily code.
 * - AI manager moves: MANAGER_MANAGER::consider_sacking_manager 0x69aba0 (ret 8 at 0x69acb8, callers 0x696525,
 *   0x696539, 0x6a59b4, 0x6aa0c1, none reads eax), so no sacking, ultimatum or move between clubs. The vacancy pass
 *   0x68f9c0 stays: retirements (0x7aaeb0 calls add_job_vacancy 0x69ba50) and the season change
 *   (league_promote_and_relegate 0x689c80) open vacancies, and only it fills them. Without it more clubs and
 *   national teams would have no manager each year, as Q31 found for players.
 *
 * The instant result. match_day_can_quick_simulate 0x6b66a0 (cdecl, ret, callers 0x6b4ae3 and 0x6b5c9a push eax
 * as the quick argument of init_match 0x6b7f20) gives 1, after the original has run for its side effect on the
 * human clubs' flags (0x6b66b1..0x6b66df). Then run_match_steps 0x6b98c0 calls quick_simulate_match 0x6beeb0
 * (thiscall, ret at 0x6c0661, one caller 0x6b98d7, eax not read) for every match. Its wrap writes the score that
 * 0x6beeb0 writes, with the same tie rules, and skips the per-minute play, scorers, cards, injuries and ratings:
 * - the fixture copy F is [engine + 0x4792] (0x6bfe2b); its clubs are F+0x1c and F+0x20 (0x6c021c..0x6c0225),
 *   each with the reputation s16 at +0x80 (0x6c0228, 0x6c022f; a national side is a club row with half its
 *   nation's reputation)
 * - half time F+0x45/+0x46 at minute 45 (0x6bfe40, 0x6bfe4f), full time F+0x43/+0x44 at 90 (0x6bfe6d,
 *   0x6bfe7c), the side goals at engine +0xf5bd and +0xf5f3 (0x6bfe67, 0x6bfe76), the minute word +0x8ed0
 *   (0x6bfe01)
 * - level at 90 (one leg: F+0x43 == F+0x44 at 0x6bfed0; two legs, F+0x4b not -1: the aggregates with the first
 *   leg F+0x4b/+0x4c at 0x6bfe96..0x6bfea9): flags F+0x38 0x80 or 0x02 play extra time (0x6bfeb1..0x6bfec5).
 *   Else two legs with flag 4 clear and away goals F+0x44 != F+0x4b decide, unless flag 0x4000 (0x6c001c..
 *   0x6c0049); else flag 0x01 or 0x20 gives penalties (0x6c004f, 0x6c0060)
 * - after 120 the totals go to F+0x47/+0x48 (0x6c0085); still level (one leg 0x6c0102; two legs the aggregate,
 *   and with 0x4000 also F+0x48 == F+0x4b, 0x6c00a5..0x6c00ec): flag 0x01 or 0x20 gives penalties
 * - penalties F+0x49/+0x4a, never equal (0x6c0116..0x6c018f); then two legs add the result to F+0x4b/+0x4c
 *   (0x6c0192..0x6c01c9)
 * A draw with no flag stays a draw: the cup plans its replay (Cup::decide_tie_winner 0x51f2f0). The man of the
 * match 0x6d0f80, the reputation update 0x6d1cc0 and the result apply 0x6b5f00 (table, cup tie, gate money,
 * records, FIFA ranking) run as before. The goals come from a random number of the host, not utils_random
 * 0x90cfc0, so the instant result itself takes no draw of the game. */
#include "win.h"
#include "tier_a.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void o_0083a730(void);
void o_0084a020(void);
void o_00839e60(void);
void o_0083a780(void);
void o_00842c00(void);
void o_0084be20(void);
void o_004c7ac0(void);
void o_005ce2d0(void);
void o_0069aba0(void);
void o_006b66a0(void);
void o_006beeb0(void);

int tier_a(void)
{
    static int on = -1;
    if (on < 0) {
        const char *v = getenv("RECOMP_TIER_A");
        on = v && !strcmp(v, "1");
        if (on) fprintf(stderr, "recomp: Tier A switch on: competitions only, instant results from reputation\n");
    }
    return on;
}

/* A skip pops the arguments as the original's ret does. (The recomp tool finds each wrap by its RT_WRAP line.) */
static int skip(int nargs)
{
    if (!tier_a()) return 0;
    rt_ret_stdcall(0, nargs);
    return 1;
}

RT_WRAP(0083a730, tier_a_update_club_shortlist)
{
    if (!skip(1)) o_0083a730();
}

RT_WRAP(0084a020, tier_a_make_club_bids)
{
    if (!skip(1)) o_0084a020();
}

RT_WRAP(00839e60, tier_a_build_position_lists)
{
    if (!skip(1)) o_00839e60();
}

RT_WRAP(0083a780, tier_a_sort_position_lists)
{
    if (!skip(0)) o_0083a780();
}

RT_WRAP(00842c00, tier_a_add_to_shortlists)
{
    if (!skip(2)) o_00842c00();
}

RT_WRAP(0084be20, tier_a_background_matches)
{
    if (!skip(0)) o_0084be20();
}

RT_WRAP(004c7ac0, tier_a_contract_pass)
{
    if (!skip(3)) o_004c7ac0();
}

RT_WRAP(005ce2d0, tier_a_daily_staff_pass)
{
    if (!skip(0)) o_005ce2d0();
}

RT_WRAP(0069aba0, tier_a_consider_sacking)
{
    if (!skip(2)) o_0069aba0();
}

RT_WRAP(006b66a0, tier_a_can_quick_simulate)
{
    o_006b66a0();
    if (tier_a()) cpu.eax = 1;
}

/* The random numbers of the instant result: splitmix64, the same sequence in each program. */
static uint64_t rng_state = 0x7469657241ull;

static double rng_unit(void)
{
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    z ^= z >> 31;
    return (double)(z >> 11) / 9007199254740992.0;
}

static int poisson(double mean)
{
    double l = exp(-mean), p = 1.0;
    int k = 0;
    do { k++; p *= rng_unit(); } while (p > l && k < 15);
    return k - 1;
}

static int binomial(int n, double p)
{
    int k = 0;
    for (int i = 0; i < n; i++) k += rng_unit() < p;
    return k;
}

/* The mean goals of each side from the two reputations (s16 at club +0x80): about 1.4 and 1.1 between equal
 * sides, and the stronger side scores more as the gap grows. */
static void means(uint32_t f, double *home, double *away)
{
    uint32_t hc = rd32(g_mem, f + 0x1c), ac = rd32(g_mem, f + 0x20);
    int hr = hc ? (int16_t)rd16(g_mem, hc + 0x80) : 0, ar = ac ? (int16_t)rd16(g_mem, ac + 0x80) : 0;
    double d = (hr - ar) / 2500.0;
    if (d > 2.0) d = 2.0;
    if (d < -2.0) d = -2.0;
    *home = 1.4 * exp(0.55 * d);
    *away = 1.1 * exp(-0.55 * d);
}

static int s8(uint32_t a) { return (int8_t)rd8(g_mem, a); }

/* Penalties F+0x49/+0x4a as 0x6c0116..0x6c018f: two numbers from 0 to 4, and a coin when they are equal. */
static void penalties(uint32_t f)
{
    int h = (int)(rng_unit() * 5), a = (int)(rng_unit() * 5);
    if (h == a) { if (rng_unit() < 0.5) h++; else a++; }
    wr8(g_mem, f + 0x49, (uint32_t)h);
    wr8(g_mem, f + 0x4a, (uint32_t)a);
}

RT_WRAP(006beeb0, tier_a_quick_simulate_match)
{
    if (!tier_a()) { o_006beeb0(); return; }
    uint32_t eng = cpu.ecx, f = rd32(g_mem, eng + 0x4792);
    double mh, ma;
    means(f, &mh, &ma);
    int h = poisson(mh), a = poisson(ma);
    wr8(g_mem, f + 0x45, (uint32_t)binomial(h, 0.45));
    wr8(g_mem, f + 0x46, (uint32_t)binomial(a, 0.45));
    wr8(g_mem, f + 0x43, (uint32_t)h);
    wr8(g_mem, f + 0x44, (uint32_t)a);
    int minute = 90;
    uint32_t flags = rd16(g_mem, f + 0x38);
    int legs2 = s8(f + 0x4b) != -1;
    int level = legs2 ? h + s8(f + 0x4b) == a + s8(f + 0x4c) : h == a;
    if (level) {
        if (flags & 0x82) {                         /* extra time */
            minute = 120;
            int eh = poisson(mh / 3), ea = poisson(ma / 3);
            if ((flags & 0x80) && eh + ea > 1) {    /* golden goal: the first goal ends the game */
                int home_first = rng_unit() * (eh + ea) < eh;
                eh = home_first;
                ea = !home_first;
            }
            h += eh;
            a += ea;
            wr8(g_mem, f + 0x47, (uint32_t)h);
            wr8(g_mem, f + 0x48, (uint32_t)a);
            if (legs2) {
                level = h + s8(f + 0x4b) == a + s8(f + 0x4c);
                if (level && (flags & 0x4000)) level = a == s8(f + 0x4b);
            } else
                level = h == a;
            if (level && (flags & 0x21)) penalties(f);
        } else if (legs2 && !(flags & 4) && a != s8(f + 0x4b) && !(flags & 0x4000)) {
            /* away goals decide */
        } else if (flags & 0x21)
            penalties(f);
    }
    if (legs2) {                                    /* the aggregate, 0x6c0192..0x6c01c9 */
        int last_h = s8(f + 0x47) != -1 ? s8(f + 0x47) : s8(f + 0x43);
        int last_a = s8(f + 0x47) != -1 ? s8(f + 0x48) : s8(f + 0x44);
        wr8(g_mem, f + 0x4b, (uint32_t)(s8(f + 0x4b) + last_h));
        wr8(g_mem, f + 0x4c, (uint32_t)(s8(f + 0x4c) + last_a));
    }
    wr8(g_mem, eng + 0xf5bd, (uint32_t)h);
    wr8(g_mem, eng + 0xf5f3, (uint32_t)a);
    wr16(g_mem, eng + 0x8ed0, (uint32_t)minute);
    rt_ret_stdcall(0, 0);
}
