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
 * (set_club_progress 0x79a490); on the draw day each group day gets a second kick-off slot, a copy of its first,
 * so that both matches of the day play at the same time. Sites in tools/recomp/seasons.py ("OFC Nations Cup of 8 teams"). The new
 * places of the two added nations are a decision of this build (the real editions had a qualifying round of the
 * smaller nations, which the game does not play): inference, not a source.
 *
 * The edition of 4 teams (2008, int-continental-87; https://www.rsssf.org/tables/08oc.html, read 3 November 2026): New
 * Zealand, New Caledonia, Fiji and Vanuatu in one group, home and away, no knockout, from 17 October 2007 to 19
 * November 2008. The game plays it so:
 * - set_up_rules 0x799690 (the constructor 0x798994 and start_new_season 0x799951 call it; a load does not, 0x798961):
 *   each pair meets twice (+0x3c, 1 at 0x7996d2; the League scheduler makes rows - 1 rounds that many times,
 *   0x683ac9..0x683ad3), and the stage count +0x2c (2 at 0x798980) becomes 0, so League::update_for_date 0x6847c0 never calls advance_stage
 *   0x7999d0 (it calls it while +0x30 < +0x2c - 1, 0x6849d1..0x6849e0) and ends the edition when the group's table
 *   is complete: the club history 0x442e80 (0x6849fa), the honours (vtable +0x30, 0x684a8f) and the complete flag
 *   +0x4c (0x684a9e). The draw date (+0xb6 day, +0xb8 year offset; 5 March of the edition year at 0x7996b3..0x799712)
 *   becomes 1 October 2007, after the South Pacific Games of 2007 that chose the three other teams (inference: the
 *   sources give no draw). The next set_up_rules gives +0x2c its 2 again, with a new stage array (a load of a save of
 *   the edition of 4 makes the array of the saved count);
 * - the group template (create_schedule 0x798b60, stage -1, kept at +0xba): 6 match days in place of 3 (0x798bc8),
 *   so the League scheduler makes a double round robin of 2 fixtures a day. The days are the real days of the
 *   edition with two matches, or one match on each of two near days: 17 and 21 November 2007, 14 and 21 June, 6 and
 *   10 September 2008; for a game made after them (a 2008/09 start, made on 1 January 2008) the first 6 of these and
 *   15 October and 19 November 2008 that come at least a week after the game's day (15 October: an international
 *   match day of 2008, inference; 19 November: the real last match day);
 * - the draw: New Zealand (key id 0x9cf3f4, one of the original's six) and the three Oceania nations with the
 *   highest nation reputation (the real three came from the South Pacific Games, which the game does not play:
 *   inference); +0xf2 is 4, so create_group_table makes 4 rows and create_group_stage 0x799a60 (group B) is not
 *   called; the predicted places are 1 to 4;
 * - the honours: slot +0x30 of the class is 0x595160, a jump to stage 1's (the knockout's Cup::record_season_honours
 *   0x519a90). With no stage the class takes League::record_season_honours 0x684640: the group's first is the winner,
 *   the second the runner-up (rows 0 and 1 of +0xb1, 0x68465b..0x68465d). */
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
#define GROUP_A_ROUNDS 0xbau            /* group A's template (set_up_rules 0x799747, freed at 0x7998ff) */

#define STAGES 0xcu                     /* the stage array (8 bytes at 0x798987) */
#define STAGE_COUNT 0x2cu               /* 2 at 0x798980 */
#define MEETINGS 0x3cu                  /* s16: the times each pair meets (1 at 0x7996d2); the League scheduler
                                         * makes rows - 1 rounds that many times (0x683ac9..0x683ad3) */
#define DRAW_DAY 0xb6u                  /* s16 day of the year (0x799712) */
#define DRAW_OFFSET 0xb8u               /* s16 year offset from +0x40 (0x79971e) */
#define GAME_DATE 0xae2c90u             /* s16 day of the year, s16 year */
#define KEY_NEW_ZEALAND 0x9cf3f4u       /* key nation id (0x799fc1) */
#define LEAGUE_HONOURS 0x00684640u      /* League::record_season_honours, thiscall () */

#define TEAMS 8
#define GROUP 4
#define ONE_GROUP 4                     /* the edition of one group, home and away (2008) */
#define ONE_GROUP_DAYS 6

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

/* 1 when the class holds a drawn edition of one group of 4 (the count that season_intl_ofc_draw wrote). */
static int drawn4(uint32_t self) { return is_ofc(self) && (int16_t)rd16(g_mem, self + TEAM_COUNT) == ONE_GROUP; }

/* 1 for the edition of one group, home and away, in a season game (the table's 2008). */
static int one_group(uint32_t self)
{
    if (!season_intl_on() || !is_ofc(self)) return 0;
    const SeasonIntlEdition *e = season_intl_edition("oceania_nations", year_of(self));
    return e && e->teams == ONE_GROUP;
}

static int leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

/* The day of the year from 0 (the Date word +0) of a day, month 1 to 12. */
static int day_of_year(int y, int month, int day)
{
    static const int before[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    return before[month - 1] + (month > 2 && leap(y)) + day - 1;
}

/* A day number to compare days of different years. */
static long day_number(int y, int doy)
{
    long n = doy;
    for (int k = 1900; k < y; k++) n += 365 + leap(k);
    return n;
}

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

static void second_slots(uint32_t rounds);

static int club_reputation(uint32_t club) { return readable(club) ? (int16_t)rd16(g_mem, club + CLUB_REPUTATION) : 0; }

/* After the original's draw of 6 (and the Australia swap of season_intl.c), before the hosts come in: for an
 * edition of 8 the block at +0xee becomes 8 teams, group A in places 0 to 3 and group B in 4 to 7. The six keep
 * their groups (places 0 to 2 and 3 to 5 become 0 to 2 and 4 to 6). The two added nations are chosen by the nation
 * reputation, as ofc_australia chooses. The stronger goes to the group whose first place (New Zealand, and
 * Australia or the nation in its place, in the order drawn at 0x799f92) has the lower team reputation, the field
 * that the original's sort of teams compares (0x4b07d0). When the data has fewer than two more Oceania nations the
 * six stay. */
static void draw_one_group(uint32_t self, uint32_t six);

void season_intl_ofc_draw(uint32_t self)
{
    if (!season_intl_on() || !is_ofc(self)) return;
    const SeasonIntlEdition *e = season_intl_edition("oceania_nations", year_of(self));
    uint32_t six = rd32(g_mem, self + TEAMS_OF);
    if (e && e->teams == ONE_GROUP && readable(six) && (int16_t)rd16(g_mem, self + TEAM_COUNT) == 6) {
        draw_one_group(self, six);
        return;
    }
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
    second_slots(rd32(g_mem, self + GROUP_A_ROUNDS));
    if (rt_unwinding) return;
    if (logging())
        fprintf(stderr, "recomp: season intl: oceania_nations %d: 8 teams, %s and %s added\n", year_of(self),
                win_str(team_nation(first) + NATION_NAME), win_str(team_nation(second) + NATION_NAME));
}

/* The edition of one group: New Zealand from the original's six (else the strongest), then the three Oceania nations
 * with the highest reputation that are not in. With fewer than four in the data the six stay. */
static void draw_one_group(uint32_t self, uint32_t six)
{
    uint32_t teams[ONE_GROUP] = {0}, nz = 0;
    int nz_id = (int)rd32(g_mem, KEY_NEW_ZEALAND);
    for (int k = 0; k < 6; k++) {
        uint32_t t = rd32(g_mem, six + 4u * (uint32_t)k);
        if (nz_id >= 0 && nz_id < nation_count() && readable(t) && team_nation(t) == nation_at(nz_id)) nz = t;
    }
    for (int k = 0; k < ONE_GROUP; k++) {
        teams[k] = k == 0 && nz ? nz : strongest(teams, ONE_GROUP);
        if (rt_unwinding) return;
        if (!teams[k]) {
            fprintf(stderr, "recomp: season intl: oceania_nations %d: fewer than 4 Oceania nations, 6 teams play\n",
                    year_of(self));
            return;
        }
    }
    uint32_t list = game_malloc(4u * ONE_GROUP);
    if (rt_unwinding || !list) return;
    for (int k = 0; k < ONE_GROUP; k++) wr32(g_mem, list + 4u * (uint32_t)k, teams[k]);
    game_free(six);
    if (rt_unwinding) return;
    wr32(g_mem, self + TEAMS_OF, list);
    wr16(g_mem, self + TEAM_COUNT, ONE_GROUP);
    if (logging())
        fprintf(stderr, "recomp: season intl: oceania_nations %d: one group of 4: %s, %s, %s, %s\n", year_of(self),
                win_str(team_nation(teams[0]) + NATION_NAME), win_str(team_nation(teams[1]) + NATION_NAME),
                win_str(team_nation(teams[2]) + NATION_NAME), win_str(team_nation(teams[3]) + NATION_NAME));
}

/* ---- The edition of one group: rules, template, group B, honours ---- */

/* After set_up_rules 0x799690 (the wrap in season_intl.c, inside its date shift, before its late draw): the stage
 * count and the draw date of the edition of one group; the stage count of 2 back for the next edition. */
void season_intl_ofc_rules(uint32_t self)
{
    if (!season_intl_on() || !is_ofc(self)) return;
    if (one_group(self)) {
        int year = year_of(self);
        wr32(g_mem, self + STAGE_COUNT, 0);
        wr16(g_mem, self + MEETINGS, 2);
        wr16(g_mem, self + DRAW_DAY, (uint16_t)day_of_year(year - 1, 10, 1));
        wr16(g_mem, self + DRAW_OFFSET, (uint16_t)-1);
        if (logging()) fprintf(stderr, "recomp: season intl: oceania_nations %d: one group, draw 1 October %d\n", year,
                               year - 1);
        return;
    }
    if (rd32(g_mem, self + STAGE_COUNT) != 0) return;
    /* After the edition of one group (or a load of its save, whose stage array has the saved count 0) */
    uint32_t stages = game_malloc(8);
    if (rt_unwinding || !stages) return;
    game_free(rd32(g_mem, self + STAGES));
    if (rt_unwinding) return;
    wr32(g_mem, self + STAGES, stages);
    wr32(g_mem, self + STAGE_COUNT, 2);
}

/* The real match days of the edition of one group, the year as an offset from the edition year. */
static const SeasonNlDate one_group_days[] = {
    {17, 11, -1}, {21, 11, -1}, {14, 6, 0}, {21, 6, 0}, {6, 9, 0}, {10, 9, 0}, {15, 10, 0}, {19, 11, 0},
};

/* The template of group A for the edition of one group: thiscall (stage, short *rounds, short *id, int *flag), ret
 * 0x10, as the original's group template (flag 1, 3 days, id 0x3e9 + stage, a block of 0x41 a day at
 * 0x798ba6..0x798bd0). Gives 0 for another stage or edition. */
static int one_group_template(uint32_t self)
{
    uint32_t sp = cpu.esp;
    int stage = (int8_t)rd8(g_mem, sp + 4u);
    if (stage != -1 || !one_group(self)) return 0;
    int year = year_of(self), ty = (int16_t)rd16(g_mem, GAME_DATE + 2), td = (int16_t)rd16(g_mem, GAME_DATE);
    int draw = day_of_year(year - 1, 10, 1);
    long from = day_number(year - 1, draw);
    if (ty >= 1900 && day_number(ty, td) > from) from = day_number(ty, td);
    SeasonNlDate days[ONE_GROUP_DAYS];
    int n = 0;
    for (size_t i = 0; i < sizeof one_group_days / sizeof one_group_days[0] && n < ONE_GROUP_DAYS; i++) {
        SeasonNlDate d = one_group_days[i];
        if (day_number(year + d.year, day_of_year(year + d.year, d.month, d.day)) >= from + 7) days[n++] = d;
    }
    if (n < ONE_GROUP_DAYS) {
        fprintf(stderr, "recomp: season intl: oceania_nations %d: %d match days to come, the original's template\n",
                year, n);
        return 0;
    }
    uint32_t rec = game_malloc(ONE_GROUP_DAYS * ROUND_SIZE);
    if (rt_unwinding || !rec) return 0;
    uint32_t rounds = rd32(g_mem, sp + 8u), id = rd32(g_mem, sp + 0xcu), flag = rd32(g_mem, sp + 0x10u);
    if (readable(flag)) wr32(g_mem, flag, 1);
    if (readable(rounds)) wr16(g_mem, rounds, ONE_GROUP_DAYS);
    if (readable(id)) wr16(g_mem, id, (uint16_t)(0x3e9 + stage));
    for (int i = 0; i < ONE_GROUP_DAYS && !rt_unwinding; i++) season_nl_league_round(rec, i, days[i], year);
    if (rt_unwinding) return 1;
    if (logging())
        fprintf(stderr, "recomp: season intl: oceania_nations %d: one group, home and away, from %d/%d/%d\n", year,
                days[0].day, days[0].month, year + days[0].year);
    rt_ret_stdcall(rec, 4);
    return 1;
}

void o_00799a60(void);

/* create_group_stage 0x799a60, thiscall (group), ret 4 (0x799c43): group B. None in the edition of one group. */
RT_WRAP(00799a60, ofc_group_b)
{
    if (season_intl_on() && drawn4(cpu.ecx)) {
        rt_ret_stdcall(1, 1);
        return;
    }
    o_00799a60();
}

/* Slot +0x30 0x595160 (the wrap in season_cup_groups.c), thiscall () ret 0: 1 when the class is the edition of one
 * group with no stage (+0x2c 0): League::record_season_honours has run and the call has returned. */
int season_intl_ofc_honours(uint32_t self)
{
    if (!season_intl_on() || !is_ofc(self) || rd32(g_mem, self + STAGE_COUNT) != 0) return 0;
    call_this(LEAGUE_HONOURS, self, 0, NULL);
    if (!rt_unwinding) rt_ret_cdecl(0);
    return 1;
}

/* ---- Sites of seasons.py ---- */

/* The size of a group: 3 in the original (create_group_table 0x7995a9, 0x7995ae; create_group_stage 0x799a80,
 * 0x799ac2, 0x799acb, 0x799b74), 4 when the draw holds 8 teams or the one group of 4. +0xf2 keeps the count until create_group_stage
 * has run (0x79a2c7, then cleared at 0x79a2e1). */
uint32_t rt_intl_ofc_group(uint32_t self, uint32_t original)
{
    return drawn8(self) || drawn4(self) ? (uint32_t)GROUP : original;
}

/* ---- Predicted positions (vtable +0x5c, 0x79a320) ---- */

void o_0079a320(void);

/* The original sorts the teams with 0x4b07d0 (0x79a414) and gives 6 places 1, 2, 3, 3, 5, 5 (0x79a425..0x79a46b).
 * With 8 teams: 1, 2, 3, 3, 5, 5, 7, 7; with the one group of 4: 1, 2, 3, 4. */
RT_WRAP(0079a320, ofc_predicted)
{
    static const int pos8[TEAMS] = {1, 2, 3, 3, 5, 5, 7, 7}, pos4[ONE_GROUP] = {1, 2, 3, 4};
    uint32_t self = cpu.ecx, stats = readable(self) ? rd32(g_mem, self + 8u) : 0;
    uint32_t teams = readable(self) ? rd32(g_mem, self + TEAMS_OF) : 0;
    int n = drawn8(self) ? TEAMS : drawn4(self) ? ONE_GROUP : 0;
    const int *pos = n == TEAMS ? pos8 : pos4;
    if (!n || !readable(stats) || !readable(teams)) {
        o_0079a320();
        return;
    }
    uint32_t list = game_malloc(4u * (uint32_t)n);
    if (rt_unwinding) return;
    if (list) {
        memcpy(g_mem + list, g_mem + teams, 4u * (uint32_t)n);
        uint32_t qa[4] = {list, (uint32_t)n, 4, CLUB_COMPARE};
        call_cdecl(GAME_QSORT, 4, qa);
        if (rt_unwinding) return;
        for (int k = 0; k < n; k++) {
            uint32_t club = rd32(g_mem, list + 4u * (uint32_t)k);
            if (!club) continue;
            uint32_t pa[2] = {club, (uint32_t)pos[k]};
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

/* The original's group templates (stage -1 group A, 0 group B; 0x798b90, 0x798eec) give each of the 3 days slot 0
 * only. For a draw of 8 each day also gets slot 1, a copy of slot 0, and slot 2 ends the list, as the Asian Cup's
 * days (0x40ebb5, 0x40ec3e, 0x40ec4f). */
static void second_slots(uint32_t rounds)
{
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

/* thiscall (stage as a byte, short *rounds, short *id, int *flag), ret 0x10. Called by the wrap of 0x798b60 in
 * season_intl.c inside its date shift, at the entry state. Group B's template is made on the draw day
 * (create_group_stage 0x799afe), after the draw: with 8 drawn it gets the second slots. Group A's is made at the new
 * season (set_up_rules 0x799740, kept at +0xba), before the draw: season_intl_ofc_draw gives it the second slots. The
 * edition of one group takes its own template of 6 days (one_group_template). */
void season_intl_ofc_template(void (*original)(void))
{
    uint32_t self = cpu.ecx;
    int stage = (int8_t)rd8(g_mem, cpu.esp + 4u);
    if (one_group_template(self)) return;
    original();
    if (rt_unwinding || stage > 0 || !drawn8(self)) return;
    second_slots(cpu.eax);
}
