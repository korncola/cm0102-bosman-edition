/* The OFC Nations Cup of 8 teams (0.6.2 open item 45). The finals table (season_intl_table.c) gives 8 teams in
 * 2002 (int-continental-21), 2012 (-120), 2016 (-164) and 2024 (-252), and after the table every edition takes the
 * 8 of the last row (0.6.2 decision A). Each of these editions has two groups of 4, the top two of each group to
 * the semi-finals, then the final. The base game (season_intl_on() 0) and every edition of another count (6 in
 * 2004, 4 in 2008) take the original's path: each site and wrap here gives the original's value or calls the
 * original then.
 *
 * The class OceaniaNations (oceania_nations.cpp, ctor 0x7988b0, vtable 0x96e0cc) plays 6 nations. The League
 * object is group A, stage[0] (+0xc, count +0x2c = 2 at 0x798980) is group B and stage[1] the Cup_Stage of the
 * semi-finals. On its draw day update_for_date 0x79a100 calls:
 * - select_and_draw_teams 0x799e30: 6 team pointers at +0xee (+0xf2 the count as a word, 6 at 0x799e42; a block of
 *   0x18 at 0x799e4d). Australia and New Zealand (key ids at 0x9cf224 and 0x9cf3f4) go to places 0 and 3 in an
 *   order drawn by utils_random 0x90cfc0 (0x799f92), two Melanesian nations (0x53d900) to places 1 and 4 and two
 *   Polynesian nations (0x53d940) to places 2 and 5, each pair from the top of its list (0x4afbd0 at 0x79a03c and
 *   0x79a04d) in a drawn order (0x79a052, 0x79a096). From 2006 the wrap in season_intl.c (ofc_australia) puts the
 *   Oceania nation with the highest reputation in Australia's place;
 * - set_predicted_positions 0x79a320 (vtable +0x5c, 0x79a1df): 6 places 1, 2, 3, 3, 5, 5 (0x79a425..0x79a46b);
 * - create_group_table 0x7995a0: a table of 3 rows (+0x3e = 3 at 0x7995ae, 0xb1 bytes at 0x7995a9) with the teams
 *   0, 1 and 2 (League::init_table_row 0x687430 at 0x799659);
 * - create_group_stage 0x799a60 (0x79a2c7, group 0): a League_Stage (0x68b100 at 0x799b79) of 3 teams (0x799b74),
 *   the teams 3, 4 and 5 copied to a block of 0xc (0x799a80, the copy at 0x799ac2..0x799ae3).
 * Then +0xee is freed and +0xf2 cleared (0x79a2d3..0x79a2e1), so the draw and the groups are made on one day and a
 * save holds only the tables: the League save and load keep the row count +0x3e (0x689461, 0x688379). The group
 * template create_schedule 0x798b60 has 3 match days (0x798bc8), as the Asian Cup's groups of 4 (0x40eb00); the
 * League scheduler makes rows / 2 fixtures a day (0x6838d6..0x6838e5). Each day of the OFC template has one
 * kick-off slot (slot 0, 0x798ca2..0x798d0a; slot 1 ends the list, 0x798c33), the Asian Cup's two (0x40ec3e,
 * 0x40ec4f; slot 2 ends it, 0x40ebb5). The knockout create_knockout_stage 0x799c50 takes rows 0 and 1 of each
 * group (League table row 0x6826d0 at 0x799cb7..0x799ce6), so it is the same for groups of 4. The rule bytes of
 * set_up_rules 0x799690 (+0x43, +0xc2..+0xc7, +0x4a) are those of the Asian Cup's groups of 4 (0x40f4c0).
 *
 * With 8 teams: the draw adds the two Oceania nations with the highest reputation that are not in the six, one to
 * the end of each group (places 3 and 7; the six move to 0 to 2 and 4 to 6). The groups have 4 rows; the
 * predicted places are 1, 2, 3, 3, 5, 5, 7, 7; a group's fourth gets the group stage codes of a third
 * (set_club_progress 0x79a490); each group day gets a second kick-off slot, a copy of its first, so that both
 * matches of the day play at the same time. Sites in tools/recomp/seasons.py ("OFC Nations Cup of 8 teams"). The new
 * places of the two added nations are a decision of this build (the real editions had a qualifying round of the
 * smaller nations, which the game does not play): inference, not a source. */
#include "win.h"
#include "season_intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OFC_VTABLE 0x96e0ccu
#define NATIONS 0xae23a8u               /* nation records, 0x122 bytes */
#define NATION_COUNT 0xae2350u
#define NATION_SIZE 0x122u
#define NATION_NAME 0x4u
#define NATION_CONTINENT 0x71u          /* continent record, its +0 the continent id */
#define NATION_REPUTATION 0x8eu         /* s16 (season_intl.c ofc_australia) */
#define CONTINENT_OCEANIA 0x9cfa18u     /* 0x799ef9 */
#define CLUB_REPUTATION 0x80u           /* s16: the field of the sort compare 0x4b07d0 (0x4b07f0) */

#define GAME_MALLOC 0x00944e46u         /* malloc, cdecl (size) */
#define GAME_FREE 0x009452cau           /* free, cdecl (block) */
#define GAME_QSORT 0x00945333u          /* qsort, cdecl (base, count, size, compare) */
#define CLUB_COMPARE 0x004b07d0u        /* compare of club pointers (0x79a3ff) */
#define ROUND_SLOT 0x0068a1c0u          /* league_round_set_match_slot, cdecl (rounds, round, slot 0 to 7, weekday,
                                         * kind, option, flag): flag at round * 0x41 + slot * 7 + 5, weekday +9,
                                         * kind +0xa, option +0xb (0x68a382..0x68a3c9) */
#define ROUND_SIZE 0x41u
#define GROUP_DAYS 3                    /* 0x798bc8 */
#define NATIONAL_TEAM 0x0053b3d0u       /* national team club of a nation record, cdecl (nation) */
#define SET_FINAL_POSITION 0x004a2540u  /* Comp_Stats::set_club_final_position, thiscall (club, position) */

#define TEAMS_OF 0xeeu                  /* team pointers, 4 bytes each */
#define TEAM_COUNT 0xf2u                /* word (0x799e42) */
#define GROUP_ROWS 0x3eu                /* word: the rows of group A's table (0x7995ae) */

#define TEAMS 8
#define GROUP 4

static int logging(void)
{
    static int on = -1;
    if (on < 0) {
        const char *v = getenv("RECOMP_SEASON_INTL_LOG");
        on = v && *v == '1';
    }
    return on;
}

static int readable(uint32_t p) { return p >= 0x10000u && p < 0xfff00000u; }

static uint32_t call_cdecl(uint32_t fn, int nargs, const uint32_t *args)
{
    Cpu saved = cpu;
    uint32_t r = rt_callback(fn, nargs, args);
    if (!rt_unwinding) cpu = saved;
    return r;
}

static uint32_t call_this(uint32_t fn, uint32_t self, int nargs, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, nargs, args);
    if (!rt_unwinding) cpu = saved;
    return r;
}

static uint32_t game_malloc(uint32_t size)
{
    uint32_t a[1] = {size};
    uint32_t p = call_cdecl(GAME_MALLOC, 1, a);
    if (readable(p)) memset(g_mem + p, 0, size);
    return readable(p) ? p : 0;
}

static void game_free(uint32_t p)
{
    uint32_t a[1] = {p};
    if (p) call_cdecl(GAME_FREE, 1, a);
}

static int year_of(uint32_t self) { return (int16_t)rd16(g_mem, self + 0x40u); }

static int is_ofc(uint32_t self) { return readable(self) && rd32(g_mem, self) == OFC_VTABLE; }

/* 1 when the class holds a drawn edition of 8 teams (the count that season_intl_ofc_draw wrote). */
static int drawn8(uint32_t self) { return is_ofc(self) && (int16_t)rd16(g_mem, self + TEAM_COUNT) == TEAMS; }

/* ---- The draw ---- */

static int nation_count(void) { return (int)rd32(g_mem, NATION_COUNT); }
static uint32_t nation_at(int id) { return rd32(g_mem, NATIONS) + (uint32_t)id * NATION_SIZE; }

static uint32_t continent_of(uint32_t nation)
{
    uint32_t c = rd32(g_mem, nation + NATION_CONTINENT);
    return readable(c) ? rd32(g_mem, c) : 0xffffffffu;
}

static uint32_t team_nation(uint32_t club) { return readable(club) ? rd32(g_mem, club + 0x53u) : 0; }

static int nation_in(const uint32_t *teams, int n, uint32_t nation)
{
    for (int k = 0; k < n; k++)
        if (team_nation(teams[k]) == nation) return 1;
    return 0;
}

/* The national team of the Oceania nation with the highest reputation that is not in yet, or 0. A team counts
 * only when its club names the nation (season_intl.c team_nation: a season database can give an empty row). */
static uint32_t strongest(const uint32_t *teams, int n)
{
    uint32_t best = 0, oceania = rd32(g_mem, CONTINENT_OCEANIA);
    int best_rep = -100000;
    for (int id = 0; id < nation_count(); id++) {
        uint32_t nation = nation_at(id);
        int rep = (int16_t)rd16(g_mem, nation + NATION_REPUTATION);
        if (rep <= best_rep || continent_of(nation) != oceania || nation_in(teams, n, nation)) continue;
        uint32_t a[1] = {nation};
        uint32_t club = call_cdecl(NATIONAL_TEAM, 1, a);
        if (rt_unwinding) return 0;
        if (team_nation(club) != nation) continue;
        best = club;
        best_rep = rep;
    }
    return best;
}

static int club_reputation(uint32_t club) { return readable(club) ? (int16_t)rd16(g_mem, club + CLUB_REPUTATION) : 0; }

/* After the original's draw of 6 (and the Australia swap of season_intl.c), before the hosts come in: for an
 * edition of 8 the block at +0xee becomes 8 teams, group A in places 0 to 3 and group B in 4 to 7. The six keep
 * their groups (places 0 to 2 and 3 to 5 become 0 to 2 and 4 to 6). The two added nations are chosen by the nation
 * reputation, as ofc_australia chooses. The stronger goes to the group whose first place (New Zealand, and
 * Australia or the nation in its place, in the order drawn at 0x799f92) has the lower team reputation, the field
 * that the original's sort of teams compares (0x4b07d0). When the data has fewer than two more Oceania nations the
 * six stay. */
void season_intl_ofc_draw(uint32_t self)
{
    if (!season_intl_on() || !is_ofc(self)) return;
    const SeasonIntlEdition *e = season_intl_edition("oceania_nations", year_of(self));
    uint32_t six = rd32(g_mem, self + TEAMS_OF);
    if (!e || e->teams != TEAMS || !readable(six) || (int16_t)rd16(g_mem, self + TEAM_COUNT) != 6) return;
    uint32_t teams[TEAMS] = {0};
    for (int k = 0; k < 6; k++) teams[k < 3 ? k : k + 1] = rd32(g_mem, six + 4u * (uint32_t)k);
    uint32_t first = strongest(teams, TEAMS);
    if (rt_unwinding) return;
    teams[3] = first;                                   /* so that the second pick leaves it out */
    uint32_t second = first ? strongest(teams, TEAMS) : 0;
    if (rt_unwinding) return;
    if (!second) {
        fprintf(stderr, "recomp: season intl: oceania_nations %d: no two more Oceania nations, 6 teams play\n",
                year_of(self));
        return;
    }
    int a_weaker = club_reputation(teams[0]) <= club_reputation(teams[4]);
    teams[3] = a_weaker ? first : second;
    teams[7] = a_weaker ? second : first;
    uint32_t list = game_malloc(4u * TEAMS);
    if (rt_unwinding || !list) return;
    for (int k = 0; k < TEAMS; k++) wr32(g_mem, list + 4u * (uint32_t)k, teams[k]);
    game_free(six);
    if (rt_unwinding) return;
    wr32(g_mem, self + TEAMS_OF, list);
    wr16(g_mem, self + TEAM_COUNT, TEAMS);
    if (logging())
        fprintf(stderr, "recomp: season intl: oceania_nations %d: 8 teams, %s and %s added\n", year_of(self),
                win_str(team_nation(first) + NATION_NAME), win_str(team_nation(second) + NATION_NAME));
}

/* ---- Sites of seasons.py ---- */

/* The size of a group: 3 in the original (create_group_table 0x7995a9, 0x7995ae; create_group_stage 0x799a80,
 * 0x799ac2, 0x799acb, 0x799b74), 4 when the draw holds 8 teams. +0xf2 keeps the count until create_group_stage
 * has run (0x79a2c7, then cleared at 0x79a2e1). */
uint32_t rt_intl_ofc_group(uint32_t self, uint32_t original)
{
    return drawn8(self) ? (uint32_t)GROUP : original;
}

/* ---- Predicted positions (vtable +0x5c, 0x79a320) ---- */

void o_0079a320(void);

/* The original sorts the teams with 0x4b07d0 (0x79a414) and gives 6 places 1, 2, 3, 3, 5, 5 (0x79a425..0x79a46b).
 * With 8 teams: 1, 2, 3, 3, 5, 5, 7, 7. */
RT_WRAP(0079a320, ofc_predicted)
{
    static const int pos8[TEAMS] = {1, 2, 3, 3, 5, 5, 7, 7};
    uint32_t self = cpu.ecx, stats = readable(self) ? rd32(g_mem, self + 8u) : 0;
    uint32_t teams = readable(self) ? rd32(g_mem, self + TEAMS_OF) : 0;
    if (!drawn8(self) || !readable(stats) || !readable(teams)) {
        o_0079a320();
        return;
    }
    uint32_t list = game_malloc(4u * TEAMS);
    if (rt_unwinding) return;
    if (list) {
        memcpy(g_mem + list, g_mem + teams, 4u * TEAMS);
        uint32_t qa[4] = {list, TEAMS, 4, CLUB_COMPARE};
        call_cdecl(GAME_QSORT, 4, qa);
        if (rt_unwinding) return;
        for (int k = 0; k < TEAMS; k++) {
            uint32_t club = rd32(g_mem, list + 4u * (uint32_t)k);
            if (!club) continue;
            uint32_t pa[2] = {club, (uint32_t)pos8[k]};
            call_this(SET_FINAL_POSITION, stats, 2, pa);
            if (rt_unwinding) return;
        }
        game_free(list);
        if (rt_unwinding) return;
    }
    rt_ret_stdcall(cpu.eax, 0);
}

/* ---- Group stage codes (vtable +0x68, 0x79a490) ---- */

void o_0079a490(void);

/* thiscall (club, stage as a byte, a, b, c), ret 0x14. In a group stage (stage below 1, 0x79a49e) the original maps
 * the codes of a group of 3: a 1, 2, 3 to 1, 3, 5 (0x79a4d5..0x79a506), b 1 and 2 to 1 and 3 to 5
 * (0x79a4a5..0x79a4b7), c 1 and 2 to 3 and 3 to 5 (0x79a4bd..0x79a4d3), and passes any other value on. A group of 4 (group A's table of 4 rows, +0x3e) gives its
 * fourth the codes of a third. */
RT_WRAP(0079a490, ofc_progress)
{
    uint32_t self = cpu.ecx, sp = cpu.esp;
    if (season_intl_on() && is_ofc(self) && (int16_t)rd16(g_mem, self + GROUP_ROWS) == GROUP &&
        (int8_t)rd8(g_mem, sp + 8u) < 1)
        for (uint32_t at = 0xcu; at <= 0x14u; at += 4u)
            if (rd8(g_mem, sp + at) == GROUP) wr8(g_mem, sp + at, 3);
    o_0079a490();
}

/* ---- The group template (create_schedule 0x798b60) ---- */

/* 1 for an edition of 8 in a season game: the template of group A is made at the new season (set_up_rules 0x799740,
 * through vtable +0x3c), before the draw, so the test is the edition, not the drawn count. */
static int edition8(uint32_t self)
{
    if (!season_intl_on() || !is_ofc(self)) return 0;
    const SeasonIntlEdition *e = season_intl_edition("oceania_nations", year_of(self));
    return e && e->teams == TEAMS;
}

/* thiscall (stage as a byte, short *rounds, short *id, int *flag), ret 0x10. Called by the wrap of 0x798b60 in
 * season_intl.c inside its date shift, at the entry state. The original's group templates (stage -1 group A, 0
 * group B; 0x798b90, 0x798eec) give each of the 3 days slot 0 only. For an edition of 8 each day also gets slot 1,
 * a copy of slot 0, and slot 2 ends the list, as the Asian Cup's days (0x40ebb5, 0x40ec3e, 0x40ec4f). */
void season_intl_ofc_template(void (*original)(void))
{
    uint32_t self = cpu.ecx;
    int stage = (int8_t)rd8(g_mem, cpu.esp + 4u);
    original();
    if (rt_unwinding || stage > 0 || !edition8(self)) return;
    uint32_t rounds = cpu.eax;
    if (!readable(rounds)) return;
    for (int r = 0; r < GROUP_DAYS; r++) {
        uint32_t s0 = rounds + (uint32_t)r * ROUND_SIZE + 5u;
        uint32_t one[7] = {rounds, (uint32_t)r, 1, (uint32_t)(int8_t)rd8(g_mem, s0 + 4u),
                           (uint32_t)(int8_t)rd8(g_mem, s0 + 5u), (uint32_t)(int8_t)rd8(g_mem, s0 + 6u),
                           rd32(g_mem, s0)};
        uint32_t end[7] = {rounds, (uint32_t)r, 2, 0xffffffffu, 0xffffffffu, 0xffffffffu, 0};
        call_cdecl(ROUND_SLOT, 7, one);
        if (!rt_unwinding) call_cdecl(ROUND_SLOT, 7, end);
        if (rt_unwinding) return;
    }
}
