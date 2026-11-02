/* A match moved for a call-up stays before its competition's season change (O288: the Danish Superliga stalled one
 * season).
 *
 * The season change pass game_season_update_pass 0x5d5ae0 starts the competitions of a league table entry only on
 * the entry's update day (entry +0x15, the day of the year, compared with the game day at 0x5d5bc8; Denmark: day
 * 170), and only a competition that is ready then (vtable +0xc at 0x5d5ccb; a league is ready when every row has
 * played its matches, League::check_complete 0x689ad0, 0x689b3d..0x689b58). A competition that is not ready waits for
 * the update day of the next year, and so runs one season late from then on.
 *
 * NationalTeams::reschedule_clashing_club_fixtures 0x7777e0 calls FIXTURE_MANAGER::reschedule_club_fixtures_near_date
 * 0x5abb10 (0x77795c) for a club whose players are called up. For each fixture of the club near the date it calls
 * remove_fixture 0x5abc30, find_new_fixture_date 0x5ab510 (0x5abbd2), add_fixture 0x5aa680 with flag 1 (0x5abbdc) and
 * queue_fixture_moved_news 0x5aac70 (0x5abbeb). 0x5ab510 (thiscall (fixture) ret 4; year at fixture +0x28, day at
 * +0x2a, slice at +0x3f) looks for a free weekday up to about 20 days after the old date, else takes today + 2, with
 * no look at the season's end. In the base game the leagues end in May, well before their update days. A season game
 * has international windows in June each year (the Nations League and the qualifiers of slices 23, 28 and 29): in a
 * 2021 Select All game FC Copenhagen's last Superliga match of 2022/23 was moved on days 148, 152, 155 and 156 and
 * came after day 170, so the Superliga was not complete on its update day and started 2023/24 a year late (gate (a)
 * run 7; Denmark alone has too few call-ups to show it).
 *
 * The clash code moves matches too (den-stall-2, run 8, 2021 start, 2022/23): a call-up move put Randers FC v FC
 * Copenhagen on day 169 of 2023, the add_fixture that followed (flag 1, 0x5abbdc) found a clash and moved it with its
 * own call of 0x5ab510 (0x5aa96e) to day 171, and the Superliga missed its update day of 170 again. So the rule
 * covers the moves of 0x5ab510 that take a match away from a clash or a call-up, each with the add_fixture (flag 0)
 * and the news that follow it (SITES): the call-up move 0x5abbd2, add_fixture's move of the new fixture 0x5aa96e and
 * of a weaker stored one 0x5aaa25, and resolve_same_day_clash 0x5ae55c, 0x5ae5d8. The other calls (a date today or
 * past at 0x5aa729, an abandoned match 0x6b6084, the cup and manager calls) keep the original.
 *
 * So in a season game, for those calls: when the new date of 0x5ab510 is on or after the competition's
 * next update day, the match goes to the last free day before that update day, after the old date and not before
 * today + 2 (0x5ab929..0x5ab935). A day is free by 0x5ab510's own test (0x5ab9b6..0x5ab9f2): with the slice of
 * get_time_slice 0x5af2f0, find_fixture_clashes 0x5ad780 gives no clash, or the severity 1, or a severity below the
 * match's priority (+0x3a). An international window of the club fails it (severity 0x15, 0x776fc0). When the update
 * day is in the next year, the search goes on into the rest of the old date's year. When there is no free day and the old date is after today, the match keeps its
 * old date and slice: the leg check record that 0x5ab510 added (list 0xae2a48, round variants 3 and 4) is removed,
 * add_fixture inserts it without its clash moves (flag 0 with the try count 0xae2a40 at 10, as add_fixture does
 * after its tenth try), and no "fixture moved" news is queued. The club plays without its called-up players, as a
 * league plays at the end of its season. When the old date is today or past, the new date of 0x5ab510 stays.
 * A kept date keeps its clash, and that is harmless (the maintainer check, 27 October 2026). In a 2007 start (Select All,
 * November 2007, leg 2008/09) four Primera División (Spain) matches of the last round sat on 18 June 2009 (day 169;
 * update day 170): Sevilla v Atlético de Madrid, Barcelona v Cádiz, Real Madrid v Celta, Valencia v Murcia. Spain
 * played the Confederations Cup on days 166, 168 and 171. On the clash with Spain v South Africa (day 168, severity
 * 10 against priority 2) add_fixture moved each to day 171, so the rule kept day 169: no day lies between the old date
 * and the update day. All four matches and Spain's three matches were played, the clubs without their players on
 * duty for Spain (the team pick leaves out players on duty, get_international_duty_club 0x7757d0; the duty window of
 * a finals squad runs from 10 days before its first match, select_squad 0x76c120, so from day 156 no day would have
 * had them). The rule moves no match before its old date, as 0x5ab510 does not. No box and no forfeit: the Primera División was complete on
 * day 170 and started 2009/10 on time (exact replay, run-logs/hold-primera).
 * A cup tie has a second bound: its next round's match day (jap-cup-stall, gate (b) 2006 on ec579543). The Emperor's
 * Cup plays its rounds a week apart in December and its final on 2 January, with Japan's update day 24. In 2009 the
 * round 3 tie Yokohama F Marinos v Jubilo Iwata (day 346) was moved by clashes on days 340, 343 and 346 to days 349,
 * 356 and 363, past round 4's day 353. Each later tie of its winner was made after its own date, and add_fixture's
 * move of a past date (0x5aa729) keeps the weekday, so each round went a week on: the final came on day 26 of 2010,
 * the cup was not complete on day 24 and held edition 2009 for a year. So a covered move of a cup tie takes the last
 * free day before the next round's match day (the same search), when that day comes before the update day bound.
 * When no day is free, the move of 0x5ab510 stays (logged). The tie is found by its two clubs in a drawn tie of the
 * cup's round table (next_round_day). The bound covers one more call for cup ties only: add_fixture's move of a
 * fixture dated before today (0x5aa729). With the first fix the replay still had two round 5 ties of the Club World
 * Cup clubs made after their day 356, moved to day 363, past round 6 on day 360 (final on day 12 of 2010). Other
 * fixtures keep that call's original move.
 * The two legs of a tie stay in order (item 90). In a 2006 game (run set pass 2, a2006s2, day 169 of 2055) the
 * Segunda División play-off final was drawn on day 162 with its first leg on day 165 and its second on day 169, the day
 * before the league's update day 170. Cup::create_tie_fixtures adds the first leg, then the second. The first met a
 * clash, and the rule put it on day 169, the last free day before day 170, while the second leg was not yet stored; the
 * second, added next, met a clash too and the rule kept its old day 169. With both legs on one day Cup::process_fixture
 * took one as the first leg (its leg count, tie +0x32 plus +0x33, 0x51a3ce..0x51a3e5), found no second leg from the next
 * day (find_next_fixture 0x5ac920 at 0x51a610) and stopped at the box "Cup..cpp 1278" (0x51a672). Whether the box comes
 * depends on the order of the day's matches; the same save played as a normal game puts both legs on day 169 too. So
 * for a leg (the round code's digit 3 or 4, comp_util_get_round_variant 0x4b0850): a first leg's move is bounded by
 * its second leg when that is stored, and a second leg never keeps a day on or before its first leg; it then takes the
 * move of 0x5ab510, which lies after its old date, as the original would. The legs are found as process_fixture finds a
 * second leg (other_leg). A leg's past-date move (0x5aa729) is covered too.
 * The other 7 calls of 0x5ab510, the base game and RECOMP_SEASON_CODE=off keep the original (Q152).
 *
 * Test switch RECOMP_TEST_FIXTURE_HOLD (season games only): "log" prints each covered move; "1" also takes the new
 * date of each covered move later in the same year as the update day, so that the free day search and the kept date
 * act on every such move (forced case of the retest). */
#include "win.h"
#include "../rt.h"
#include "../rt_host.h"
#include "season_change.h"
#include "season_names.h"
#include "fixture_season_end.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTRIES 0x00b63d60u     /* league table entries, 0x48 bytes each (0x5d5bc8) */
#define ENTRY_SIZE 0x48u
#define ENTRY_COUNT 35
#define ENTRY_UPDATE_DAY 0x15u
#define COMP_OBJECTS 0x00adadfcu
#define TODAY 0x00ae2c90u       /* Date: day of the year +0, year +2 */
#define LEG_CHECKS 0x00ae2a48u  /* LLIST: count (word) +0, head +2, tail +6 */
#define ADD_TRIES 0x00ae2a40u   /* add_fixture's try count (word) */
#define FIX_COMP 0x14u          /* the competition's record */
#define FIX_YEAR 0x28u
#define FIX_DAY 0x2au
#define FIX_PRIORITY 0x3au
#define FIX_SLICE 0x3fu
#define FIX_CODE 0x30u          /* the round code (comp_util_get_round_variant 0x4b0850) */

/* The moves of a fixture that the rule covers: the return addresses of the call of 0x5ab510, of the add_fixture that
 * follows it and of the queue_fixture_moved_news after that (0 for none). */
typedef struct {
    uint32_t new_date, add, news;
    const char *what;
    int cup_only;               /* 1: only a cup tie's next round bound applies, other fixtures keep the original */
} MoveSite;
static const MoveSite SITES[] = {
    {0x005abbd7u, 0x005abbe1u, 0x005abbf0u, "call-up"},              /* 0x5abb10: 0x5abbd2, 0x5abbdc, 0x5abbeb */
    {0x005aa973u, 0x005aa97du, 0, "clash"},                         /* add_fixture moves the new fixture: 0x5aa96e, 0x5aa978 */
    {0x005aaa2au, 0x005aaa38u, 0x005aaa4bu, "clash"},               /* add_fixture moves a weaker stored one: 0x5aaa25, 0x5aaa33, 0x5aaa46 */
    {0x005ae561u, 0x005ae56fu, 0x005ae57eu, "same-day clash"},      /* resolve_same_day_clash: 0x5ae55c, 0x5ae56a, 0x5ae579 */
    {0x005ae5ddu, 0x005ae5ebu, 0x005ae5fau, "same-day clash"},      /* resolve_same_day_clash: 0x5ae5d8, 0x5ae5e6, 0x5ae5f5 */
    {0, 0x005aeddbu, 0x005aedf1u, "nearby"},                        /* move_nearby_club_fixtures, no 0x5ab510 */
    {0x005aa72eu, 0, 0, "past date", 1},                            /* add_fixture, a date before today: 0x5aa729 */
};
#define SITE_NEARBY 5
#define SITE_COUNT ((int)(sizeof SITES / sizeof SITES[0]))

#define DATE_NEW 0x00549ef0u            /* Date::Date (day of the year, year), ret 8 */
#define DATE_WEEKDAY 0x0054a3f0u        /* Date::get_weekday, 0 Monday..6 Sunday */
#define TIME_SLICE 0x005af2f0u          /* FIXTURE_MANAGER::get_time_slice (comp, weekday), ret 8 */
#define FIND_CLASHES 0x005ad780u        /* FIXTURE_MANAGER::find_fixture_clashes (fixture, &severity, flag), ret 0xc */
#define LIST_FREE 0x0068d040u           /* LLIST::~LLIST */
#define GAME_DELETE 0x00944c94u         /* operator delete, cdecl */
#define LIST_REMOVE_NODE 0x0068d280u    /* LLIST::remove_node (node), ret 4 */

static uint32_t kept;           /* the fixture that keeps its date, from its 0x5ab510 call to its add (and news) */
static int kept_site;

/* RECOMP_TEST_FIXTURE_HOLD: 0 off, 1 "log", 2 "1" (forced). */
static int test_mode(void)
{
    static int mode = -1;
    if (mode < 0) {
        const char *v = getenv("RECOMP_TEST_FIXTURE_HOLD");
        mode = !v ? 0 : !strcmp(v, "log") ? 1 : !strcmp(v, "1") ? 2 : 0;
    }
    return mode;
}

static int readable(uint32_t p) { return p >= 0x10000u && p < 0xfff00000u; }

static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    if (!rt_unwinding) cpu = saved;
    return r;
}

/* The update day of the entry that holds the competition of record rec, or -1. */
static int update_day(uint32_t rec)
{
    uint32_t table = rd32(g_mem, COMP_OBJECTS);
    if (!readable(rec) || !readable(table)) return -1;
    int32_t id = (int32_t)rd32(g_mem, rec);
    if (id < 0 || id > 0xffff) return -1;
    uint32_t obj = rd32(g_mem, table + 4u * (uint32_t)id);
    if (!readable(obj)) return -1;
    uint32_t comps[128];
    for (int e = 0; e < ENTRY_COUNT; e++) {
        int n = season_change_entry_comps(e, comps, 128);
        for (int i = 0; i < n; i++)
            if (comps[i] == obj) return (int16_t)rd16(g_mem, ENTRIES + ENTRY_SIZE * (uint32_t)e + ENTRY_UPDATE_DAY);
    }
    return -1;
}

static int before(int y1, int d1, int y2, int d2) { return y1 < y2 || (y1 == y2 && d1 < d2); }

/* A tie of a cup round with a next round: 1 with the next round's match day in *y and *d. The cup is the object of
 * record rec (COMP_OBJECTS); a Cup holds its rounds at +0x3c (s16), the round table at +0xa3 (records of 0x68: match
 * day s16 +3, year offset s8 +5 from the year +0x40, tie count s16 +0x1a; Cup::draw_round 0x51c410, 0x51c473 and
 * 0x51c569..0x51c580), the ties of round r at [+0xad] + 4r (0x35 bytes each: home and away places s16 +0 and +2,
 * drawn +0x34; 0x51c449..0x51c4b7) and the places at +0xa7 (6 bytes each, the club +0; 0x51c485..0x51c491). The
 * fixture is a tie of round r when a drawn tie of r has its two clubs (fixture +0x1c and +0x20). A league object
 * has none of these, so no tie matches. */
#define FIX_HOME 0x1cu
#define FIX_AWAY 0x20u
static int next_round_day(uint32_t rec, uint32_t fix, int *y, int *d)
{
    uint32_t table = rd32(g_mem, COMP_OBJECTS);
    if (!readable(rec) || !readable(table)) return 0;
    int32_t id = (int32_t)rd32(g_mem, rec);
    if (id < 0 || id > 0xffff) return 0;
    uint32_t cup = rd32(g_mem, table + 4u * (uint32_t)id);
    if (!readable(cup)) return 0;
    uint32_t rounds_at = rd32(g_mem, cup + 0xa3u), ties_at = rd32(g_mem, cup + 0xadu), places = rd32(g_mem, cup + 0xa7u);
    int rounds = (int16_t)rd16(g_mem, cup + 0x3cu);
    uint32_t home = rd32(g_mem, fix + FIX_HOME), away = rd32(g_mem, fix + FIX_AWAY);
    if (rounds < 2 || rounds > 32 || !readable(rounds_at) || !readable(ties_at) || !readable(places) || !home || !away)
        return 0;
    for (int r = 0; r + 1 < rounds; r++) {
        uint32_t ties = rd32(g_mem, ties_at + 4u * (uint32_t)r);
        int n = (int16_t)rd16(g_mem, rounds_at + 0x68u * (uint32_t)r + 0x1au);
        if (!readable(ties) || n < 1 || n > 512) continue;
        for (int k = 0; k < n; k++) {
            uint32_t tie = ties + 0x35u * (uint32_t)k;
            int h = (int16_t)rd16(g_mem, tie), a = (int16_t)rd16(g_mem, tie + 2u);
            if (!g_mem[tie + 0x34u] || h < 0 || a < 0) continue;
            uint32_t ch = rd32(g_mem, places + 6u * (uint32_t)h), ca = rd32(g_mem, places + 6u * (uint32_t)a);
            if (!((ch == home && ca == away) || (ch == away && ca == home))) continue;
            uint32_t next = rounds_at + 0x68u * (uint32_t)(r + 1);
            *d = (int16_t)rd16(g_mem, next + 3u);
            *y = (int16_t)rd16(g_mem, cup + 0x40u) + (int8_t)g_mem[next + 5u];
            return *d >= 0 && *d < 366;
        }
    }
    return 0;
}

static int days_in(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0 ? 366 : 365; }

/* 1 when day d of year y is free for the fixture: it gets that date and the slice of get_time_slice, and
 * find_fixture_clashes finds no clash. The work space (a Date and the severity) lies below the guest stack. */
static int free_day(uint32_t manager, uint32_t fix, int y, int d)
{
    uint32_t esp = cpu.esp;
    cpu.esp -= 0x40u;
    uint32_t date = cpu.esp + 0x20u, sev = cpu.esp + 0x30u;
    uint32_t a[3] = {(uint32_t)d, (uint32_t)y, 0};
    call_this(DATE_NEW, date, 2, a);
    int weekday = rt_unwinding ? 0 : (int)call_this(DATE_WEEKDAY, date, 0, a);
    int ok = 0;
    if (!rt_unwinding) {
        wr16(g_mem, fix + FIX_YEAR, (uint16_t)y);
        wr16(g_mem, fix + FIX_DAY, (uint16_t)d);
        a[0] = rd32(g_mem, fix + FIX_COMP);
        a[1] = (uint32_t)(int16_t)weekday;
        g_mem[fix + FIX_SLICE] = (uint8_t)call_this(TIME_SLICE, manager, 2, a);
    }
    if (!rt_unwinding) {
        wr32(g_mem, sev, 0);
        a[0] = fix;
        a[1] = sev;
        a[2] = 0;
        uint32_t list = call_this(FIND_CLASHES, manager, 3, a);
        int severity = (int32_t)rd32(g_mem, sev);
        ok = (!list && !severity) || severity == 1 || (int8_t)g_mem[fix + FIX_PRIORITY] > severity;
        if (list && !rt_unwinding) {
            call_this(LIST_FREE, list, 0, a);
            a[0] = list;
            if (!rt_unwinding) rt_callback(GAME_DELETE, 1, a);
        }
    }
    if (!rt_unwinding) cpu.esp = esp;
    return ok;
}

/* Test log (any RECOMP_TEST_FIXTURE_HOLD test mode): the severity that find_fixture_clashes gives a fixture at its
 * date, and the nations of a national side's teams (club +0x53). It calls the original once more, so only the test
 * mode. */
static void log_clash(uint32_t manager, uint32_t fix)
{
    uint32_t esp = cpu.esp;
    cpu.esp -= 0x40u;
    uint32_t sev = cpu.esp + 0x30u, a[3] = {fix, sev, 0};
    wr32(g_mem, sev, 0);
    uint32_t list = call_this(FIND_CLASHES, manager, 3, a);
    int severity = rt_unwinding ? -1 : (int32_t)rd32(g_mem, sev);
    if (list && !rt_unwinding) {
        call_this(LIST_FREE, list, 0, a);
        a[0] = list;
        if (!rt_unwinding) rt_callback(GAME_DELETE, 1, a);
    }
    if (!rt_unwinding) cpu.esp = esp;
    uint32_t h = rd32(g_mem, fix + FIX_HOME), w = rd32(g_mem, fix + FIX_AWAY);
    uint32_t hn = readable(h) ? rd32(g_mem, h + 0x53u) : 0, wn = readable(w) ? rd32(g_mem, w + 0x53u) : 0;
    fprintf(stderr, "recomp: fixtures: test: clash severity %d, priority %d, %s v %s\n", severity,
            (int8_t)g_mem[fix + FIX_PRIORITY], readable(hn) ? win_str(hn + 4u) : "?",
            readable(wn) ? win_str(wn + 4u) : "?");
}

/* The leg digit of the fixture's round code (+0x30): comp_util_get_round_variant 0x4b0850 gives code % 10 below 1000,
 * 3 for a first leg and 4 for a second (find_new_fixture_date adds its leg check only for these, 0x5ab657..0x5ab677). */
static int leg_variant(uint32_t fix)
{
    int code = (int16_t)rd16(g_mem, fix + FIX_CODE);
    return code >= 0 && code < 1000 ? code % 10 : 0;
}

/* The other leg of the tie of a leg on day d of year y (item 90): a stored fixture of the same competition (+0) and
 * clubs whose round code is the leg's code + 1 (the second leg of a first leg) or - 1 (the first leg of a second leg).
 * A league's two meetings share one code, so they never match. The search reads the store's own lists of each day
 * (fix_man_get_fixture_list_for_date 0x5ae6a0, slices 0 to 2, every fixture type, as season_friendly_dates.c side_busy
 * does) node by node and touches no list cursor: FIXTURE_MANAGER::find_next_fixture 0x5ac920 sets the cursor of each
 * list it reads to its head (0x5ac9c6), and MATCH_DAY 0x6b3c10 walks today's list with that cursor while a postponed
 * match is added again (item 84). It looks LEG_DAYS days ahead (from the next day) for a second leg, and back to its
 * own day for a first leg, within year y. The fixture of the move is not in the store then (removed before the call-up
 * move, a copy or not yet added at the clash moves), so it cannot find itself. 1 and the date in *ld when found. */
#define FIX_HOME_CLUB 0x1cu
#define FIX_AWAY_CLUB 0x20u
#define FIXTURES 0x00ae2a58u            /* the fixture manager */
#define DAY_LIST 0x005ae6a0u            /* fix_man_get_fixture_list_for_date, thiscall (Date *, slice, type) */
#define TYPE_COUNT 0x00b64738u          /* s8: the fixture types of a day (season_friendly_dates.c) */
#define LEG_DAYS 60
static int other_leg(uint32_t fix, int y, int d, int forward, int *ld)
{
    uint32_t manager = rd32(g_mem, FIXTURES);
    if (!readable(manager)) return 0;
    int code = (int16_t)rd16(g_mem, fix + FIX_CODE), want = forward ? code + 1 : code - 1;
    uint32_t comp = rd32(g_mem, fix), h = rd32(g_mem, fix + FIX_HOME_CLUB), w = rd32(g_mem, fix + FIX_AWAY_CLUB);
    int types = (int8_t)g_mem[TYPE_COUNT];
    if (types < 2 || types > 40) types = 2;
    for (int k = forward ? 1 : 0; k <= LEG_DAYS && !rt_unwinding; k++) {
        int day = forward ? d + k : d - k;
        if (day < 0 || day >= days_in(y)) break;
        for (int slice = 0; slice < 3 && !rt_unwinding; slice++)
            for (int type = 0; type < types && !rt_unwinding; type++) {
                uint32_t esp = cpu.esp;
                cpu.esp -= 16;
                uint32_t date = cpu.esp;
                wr16(g_mem, date, (uint16_t)day);
                wr16(g_mem, date + 2, (uint16_t)y);
                wr32(g_mem, date + 4, 0);
                uint32_t a[3] = {date, (uint32_t)slice, (uint32_t)type};
                uint32_t list = call_this(DAY_LIST, manager, 3, a);
                if (rt_unwinding) return 0;
                cpu.esp = esp;
                for (uint32_t node = readable(list) ? rd32(g_mem, list + 2) : 0, n = 0; readable(node) && n < 4000;
                     node = rd32(g_mem, node + 6), n++) {
                    uint32_t f = rd32(g_mem, node);
                    if (!readable(f) || rd32(g_mem, f) != comp || (int16_t)rd16(g_mem, f + FIX_CODE) != want) continue;
                    uint32_t fh = rd32(g_mem, f + FIX_HOME_CLUB), fw = rd32(g_mem, f + FIX_AWAY_CLUB);
                    if ((fh == h && fw == w) || (fh == w && fw == h)) {
                        *ld = day;
                        return 1;
                    }
                }
            }
    }
    return 0;
}

void o_005ab510(void);

RT_WRAP(005ab510, fixture_new_date)
{
    uint32_t fix = rt_arg(0), manager = cpu.ecx, ret = rd32(g_mem, cpu.esp);
    int site = 0;
    while (site < SITE_COUNT && SITES[site].new_date != ret) site++;
    if (site == SITE_COUNT || !season_game_changes() || !readable(fix)) {
        o_005ab510();
        return;
    }
    if (test_mode()) {
        log_clash(manager, fix);
        if (rt_unwinding) return;
    }
    kept = 0;               /* a kept fixture's add and news come before the next covered move (none nests there) */
    int year = (int16_t)rd16(g_mem, fix + FIX_YEAR), day = (int16_t)rd16(g_mem, fix + FIX_DAY);
    uint8_t slice = g_mem[fix + FIX_SLICE];
    uint16_t legs = rd16(g_mem, LEG_CHECKS);
    o_005ab510();
    if (rt_unwinding) return;
    uint32_t rec = rd32(g_mem, fix + FIX_COMP);
    int last = update_day(rec);
    int new_year = (int16_t)rd16(g_mem, fix + FIX_YEAR), new_day = (int16_t)rd16(g_mem, fix + FIX_DAY);
    if (test_mode())
        fprintf(stderr, "recomp: fixtures: test: a %s move of %.40s from day %d of %d to day %d of %d (update "
                        "day %d)\n", SITES[site].what, win_str(rec + 4), day, year, new_day, new_year, last);
    if (test_mode() == 2 && new_year == year && new_day > day) last = new_day;
    int last_year = day < last ? year : year + 1;
    /* A cup tie stays before its next round's match day (jap-cup-stall). */
    int round_year, round_day, cup_tie = next_round_day(rec, fix, &round_year, &round_day) &&
                                         before(year, day, round_year, round_day);
    if (cup_tie && (last < 0 || SITES[site].cup_only || before(round_year, round_day, last_year, last)))
        last_year = round_year, last = round_day;
    else
        cup_tie = 0;
    /* A leg stays before the other leg of its tie (item 90; the bounds above are later days). */
    int variant = leg_variant(fix);                         /* 3 a first leg, 4 a second leg */
    int leg_year = year, leg_day, leg = variant == 3 && other_leg(fix, year, day, 1, &leg_day) && !rt_unwinding;
    int prev_year = year, prev_day, prev = variant == 4 && !rt_unwinding && other_leg(fix, year, day, 0, &prev_day);
    if (rt_unwinding) return;
    if (test_mode())
        fprintf(stderr, "recomp: fixtures: test: other leg %d (day %d of %d), previous leg %d (day %d of %d); comp %d "
                        "code %d clubs %d %d\n", leg, leg ? leg_day : -1, leg ? leg_year : -1, prev, prev ? prev_day : -1,
                prev ? prev_year : -1, (int32_t)rd32(g_mem, fix), (int16_t)rd16(g_mem, fix + FIX_CODE),
                (int32_t)rd32(g_mem, fix + 0x10u), (int32_t)rd32(g_mem, fix + 0xcu));
    if (SITES[site].cup_only && !cup_tie && !leg) return;
    if (leg && (last < 0 || before(leg_year, leg_day, last_year, last) || (leg_year == last_year && leg_day == last))) {
        last_year = leg_year, last = leg_day;
        cup_tie = 0;            /* the other leg bounds it: no fallback past it */
    } else {
        leg = 0;
    }
    if (last < 0) return;
    uint8_t new_slice = g_mem[fix + FIX_SLICE];
    if (before(new_year, new_day, last_year, last)) return;
    int today_year = (int16_t)rd16(g_mem, TODAY + 2u), today = (int16_t)rd16(g_mem, TODAY);
    const char *name = win_str(rec + 4);
    /* The last free day before the update day, after the old date and not before today + 2 (0x5ab92d). */
    int min_year = today_year, min_day = today + 2;
    if (min_day >= days_in(min_year)) min_day -= days_in(min_year), min_year++;
    for (int y = last_year, d = last - 1;; d--) {
        if (d < 0) y--, d = days_in(y) - 1;
        if (!before(year, day, y, d) || before(y, d, min_year, min_day)) break;
        if (free_day(manager, fix, y, d)) {
            fprintf(stderr, "recomp: fixtures: a match of %.40s moves from day %d of %d to day %d of %d, before its "
                            "%s on day %d (the %s move gave day %d of %d)\n",
                    name, day, year, d, y, leg ? "other leg" : cup_tie ? "next round" : "season change", last,
                    SITES[site].what,
                    new_day, new_year);
            return;
        }
        if (rt_unwinding) return;
    }
    if (cup_tie) {
        wr16(g_mem, fix + FIX_YEAR, (uint16_t)new_year);
        wr16(g_mem, fix + FIX_DAY, (uint16_t)new_day);
        g_mem[fix + FIX_SLICE] = new_slice;
        fprintf(stderr, "recomp: fixtures: a match of %.40s goes to day %d of %d after its next round on day %d of %d "
                        "(no free day before it, old day %d of %d)\n",
                name, new_day, new_year, last, last_year, day, year);
        return;
    }
    if (!before(today_year, today, year, day)) {
        wr16(g_mem, fix + FIX_YEAR, (uint16_t)new_year);
        wr16(g_mem, fix + FIX_DAY, (uint16_t)new_day);
        g_mem[fix + FIX_SLICE] = new_slice;
        fprintf(stderr, "recomp: fixtures: a match of %.40s goes to day %d of %d after its %s on day %d "
                        "(no free day before it, the old day %d of %d is not in the future)\n",
                name, new_day, new_year, leg ? "other leg" : "season change", last, day, year);
        return;
    }
    /* A kept date on or before the previous leg of its tie would put the legs out of order (item 90): the first
     * leg was moved to this day while this second leg was not yet stored. The match then takes the move of 0x5ab510,
     * which lies after its old date, as the original would. */
    if (prev && !before(prev_year, prev_day, year, day)) {
        wr16(g_mem, fix + FIX_YEAR, (uint16_t)new_year);
        wr16(g_mem, fix + FIX_DAY, (uint16_t)new_day);
        g_mem[fix + FIX_SLICE] = new_slice;
        fprintf(stderr, "recomp: fixtures: a match of %.40s goes to day %d of %d after its %s on day %d (its first leg "
                        "is on day %d of %d, its old day %d of %d)\n",
                name, new_day, new_year, leg ? "other leg" : "season change", last, prev_day, prev_year, day, year);
        return;
    }
    wr16(g_mem, fix + FIX_YEAR, (uint16_t)year);
    wr16(g_mem, fix + FIX_DAY, (uint16_t)day);
    g_mem[fix + FIX_SLICE] = slice;
    if (rd16(g_mem, LEG_CHECKS) == (uint16_t)(legs + 1u)) {
        uint32_t a[1] = {rd32(g_mem, LEG_CHECKS + 6u)};
        call_this(LIST_REMOVE_NODE, LEG_CHECKS, 1, a);
        if (rt_unwinding) return;
    }
    kept = fix;
    kept_site = site;
    fprintf(stderr, "recomp: fixtures: a match of %.40s stays on day %d of %d (no free day before its %s "
                    "on day %d; the %s move gave day %d of %d)\n",
            name, day, year, leg ? "other leg" : "season change", last, SITES[site].what, new_day, new_year);
}

/* The moves by one day that set the date with no call of 0x5ab510 (den-stall-2, the maintainer under O259):
 *   - move_nearby_club_fixtures 0x5ae9d0 moves a club's fixture two days after a new international one a day later
 *     (0x5aed12: add_days 1 on a copy at its esp+0x38, the old one at esp+0x88), then remove_fixture (0x5aedc8),
 *     add_fixture (copy, 0) (0x5aedd6) and the moved news (0x5aedec). Its branch for a fixture two days before
 *     moves it a day earlier (0x5aeb5f, sub_days), which cannot cross.
 *   - League::move_fixtures_for_tv 0x685640 sets a TV date at its esp+0x1c and esp+0x1e (0x685894..0x685964, a
 *     weekday near the old date), copies the fixture to esp+0x50 (0x685aae), sends the TV news to both clubs
 *     (News::send_to_club_manager 0x7861e0 at 0x685bf3 and 0x685c0b), then removes the fixture (0x685c23), writes
 *     the TV date into the copy (0x685c28..0x685c3a) and adds it with flag 1 (0x685c79).
 * Either can take a match on the day before its competition's update day to that day or later. A move of one day has
 * no free day between, so the match keeps its date: the nearby move gets the old date back in its copy at the
 * remove, with no news and no clash moves at its add; the TV move gets the old date back in its date variable at the
 * first TV news, and neither TV news is sent. */
static int tv_kept;

void o_005abc30(void);
RT_WRAP(005abc30, fixture_remove)
{
    uint32_t ret = rd32(g_mem, cpu.esp), old = rt_arg(0);
    if (ret != 0x005aedcdu || !season_game_changes() || !readable(old)) {
        o_005abc30();
        return;
    }
    uint32_t copy = cpu.esp + 8u + 0x38u;               /* the caller's esp after the call, + 0x38 */
    int year = (int16_t)rd16(g_mem, old + FIX_YEAR), day = (int16_t)rd16(g_mem, old + FIX_DAY);
    int new_year = (int16_t)rd16(g_mem, copy + FIX_YEAR), new_day = (int16_t)rd16(g_mem, copy + FIX_DAY);
    int last = update_day(rd32(g_mem, old + FIX_COMP));
    int last_year = day < last ? year : year + 1;
    int today_year = (int16_t)rd16(g_mem, TODAY + 2u), today = (int16_t)rd16(g_mem, TODAY);
    o_005abc30();
    if (rt_unwinding || last < 0 || before(new_year, new_day, last_year, last) || !before(today_year, today, year, day))
        return;
    wr16(g_mem, copy + FIX_YEAR, (uint16_t)year);
    wr16(g_mem, copy + FIX_DAY, (uint16_t)day);
    g_mem[copy + FIX_SLICE] = g_mem[old + FIX_SLICE];
    kept = copy;
    kept_site = SITE_NEARBY;
    fprintf(stderr, "recomp: fixtures: a match of %.40s stays on day %d of %d (the nearby move gave day %d of %d, on or "
                    "after its season change on day %d)\n", win_str(rd32(g_mem, old + FIX_COMP) + 4), day, year,
            new_day, new_year, last);
}

void o_007861e0(void);
RT_WRAP(007861e0, fixture_tv_news)
{
    uint32_t ret = rd32(g_mem, cpu.esp);
    if ((ret != 0x00685bf8u && ret != 0x00685c10u) || !season_game_changes()) {
        o_007861e0();
        return;
    }
    uint32_t frame = cpu.esp + 12u, fix = frame + 0x50u;       /* the caller's esp after the call (ret 8) */
    if (ret == 0x00685bf8u) {
        tv_kept = 0;
        int year = (int16_t)rd16(g_mem, fix + FIX_YEAR), day = (int16_t)rd16(g_mem, fix + FIX_DAY);
        int new_year = (int16_t)rd16(g_mem, frame + 0x1eu), new_day = (int16_t)rd16(g_mem, frame + 0x1cu);
        int last = update_day(rd32(g_mem, fix + FIX_COMP));
        int last_year = day < last ? year : year + 1;
        int today_year = (int16_t)rd16(g_mem, TODAY + 2u), today = (int16_t)rd16(g_mem, TODAY);
        if (last >= 0 && !before(new_year, new_day, last_year, last) && before(today_year, today, year, day)) {
            wr16(g_mem, frame + 0x1eu, (uint16_t)year);
            wr16(g_mem, frame + 0x1cu, (uint16_t)day);
            tv_kept = 1;
            fprintf(stderr, "recomp: fixtures: a match of %.40s stays on day %d of %d (the TV move gave day %d of %d, on "
                            "or after its season change on day %d)\n", win_str(rd32(g_mem, fix + FIX_COMP) + 4), day,
                    year, new_day, new_year, last);
        }
    }
    if (tv_kept) {
        if (ret == 0x00685c10u) tv_kept = 0;
        rt_ret_stdcall(0, 2);
        return;
    }
    o_007861e0();
}

/* add_fixture 0x5aa680 (its wrap is in season_kind_cup_breaks.c): the call at 0x5abbdc for a kept fixture inserts
 * it on its old date with flag 0 and the try count at 10, so the clash code does not move it again. */
void o_005aa680(void);

void fixture_season_end_add_fixture(void)
{
    if (!kept || rt_arg(0) != kept || rd32(g_mem, cpu.esp) != SITES[kept_site].add) {
        o_005aa680();
        return;
    }
    uint16_t tries = rd16(g_mem, ADD_TRIES);
    if (!SITES[kept_site].news) kept = 0;       /* no news call follows */
    wr32(g_mem, cpu.esp + 8u, 0);
    wr16(g_mem, ADD_TRIES, 10);
    o_005aa680();
    if (!rt_unwinding) wr16(g_mem, ADD_TRIES, tries);
}

/* queue_fixture_moved_news 0x5aac70 (thiscall (moved, cause, old date) ret 0xc): no news for a kept fixture. */
void o_005aac70(void);

RT_WRAP(005aac70, fixture_moved_news)
{
    if (kept && rt_arg(0) == kept && rd32(g_mem, cpu.esp) == SITES[kept_site].news) {
        kept = 0;
        rt_ret_stdcall(0, 3);
        return;
    }
    o_005aac70();
}
