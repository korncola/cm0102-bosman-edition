/* Faults of the original that long games reach (0.6.2 slice 11, tools/recomp/original_faults.py). The fixes act in
 * every game, the base game too (Q23, Q24, Q27). The test switch RECOMP_ORIGINAL_FAULTS turns them off: "keep" all
 * of them, "keep:" with a list of groups some of them ("keep:money,age"). The groups are managers, money, age,
 * olympics, ire_super, nsl, holiday, callups and confed. */
#include "win.h"
#include "season_intl.h"
#include "../rt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLUB_COUNT 0xae235cu     /* clubs, national teams and under-21 teams */
#define NATION_COUNT 0xae2350u

enum { MANAGERS, MONEY, AGE, OLYMPICS, IRE_SUPER, NSL, HOLIDAY, CALLUPS, CONFED, GROUPS };

static int fixes_on_group(int group)
{
    static int on[GROUPS] = {-1};
    if (on[0] < 0) {
        static const char *const names[GROUPS] = {"managers", "money", "age", "olympics", "ire_super", "nsl",
                                                   "holiday", "callups", "confed"};
        const char *v = getenv("RECOMP_ORIGINAL_FAULTS");
        if (v && strcmp(v, "keep") && strncmp(v, "keep:", 5))
            fprintf(stderr, "recomp: RECOMP_ORIGINAL_FAULTS=%s is not keep or keep:<groups>: every fix acts\n", v);
        const char *list = v && !strncmp(v, "keep:", 5) ? v + 5 : "";
        for (const char *p = list; *p; p += strcspn(p, ",") + (p[strcspn(p, ",")] == ',')) {
            size_t n = strcspn(p, ",");
            int known = 0;
            for (int g = 0; g < GROUPS; g++) known |= n == strlen(names[g]) && !strncmp(p, names[g], n);
            if (n && !known)        /* a typo would leave its fixes on, and a same-save run would still pass */
                fprintf(stderr,
                        "recomp: RECOMP_ORIGINAL_FAULTS: unknown group %.*s (managers, money, age, olympics, "
                        "ire_super, nsl, holiday, callups, confed)\n",
                        (int)n, p);
        }
        for (int g = 0; g < GROUPS; g++) {
            on[g] = 1;
            if (v && !strcmp(v, "keep")) on[g] = 0;
            if (v && !strncmp(v, "keep:", 5)) {
                size_t n = strlen(names[g]);
                for (const char *p = v + 5; *p; p += strcspn(p, ",") + (p[strcspn(p, ",")] == ',')) {
                    if (strcspn(p, ",") == n && !strncmp(p, names[g], n)) on[g] = 0;
                }
            }
        }
    }
    return on[group];
}

static int money_fixes_on(void)    /* the fixes of money above 2,147,483,647 (faults 27 and 28) */
{
    return fixes_on_group(MONEY);
}

/* A name of the game for a log line, with each byte above 0x7e as '?': the test scripts read the log as UTF-8, and
 * the game's names are Latin-1 ("S\xe3o Tom\xe9"). */
static const char *log_name(uint32_t at)
{
    static char out[64];
    size_t i = 0;
    for (const char *p = win_str(at); *p && i < sizeof out - 1; p++)
        out[i++] = (unsigned char)*p > 0x7e ? '?' : *p;
    out[i] = 0;
    return out;
}

/* `game_run_to_start_date` 0x5cc850 (called from create_game at 0x829fd9) plays the days of a new game up to its
 * start date, across a change of month. The fix stays off there, so a new game starts as the original's does (about
 * 95 of 213 national teams of the 3.9.68 data have no manager then) and changes only at the first month change in
 * play (Q152: the same-save set ends before it). */
static int creating;

void o_005cc850(void);

RT_WRAP(005cc850, run_to_start_date)
{
    creating = 1;
    o_005cc850();
    creating = 0;
}

/* 1 when a vacancy opens for this club although `club_can_be_managed` 0x544820 refuses it: a senior national team,
 * by the club id range of 0x544835..0x544857, with a nation (+0x53, 0x54482e) that exists. The original asks for more
 * than 150 players of the nation (0x544859), or 100 in `add_job_vacancy` (0x69bafb). A nation that no longer exists
 * has no continent (+0x71 null; the data load 0x5315a0 links an id of -1 as null): in the 3.9.68 data East and West
 * Germany, the Soviet Union, the Commonwealth of Soviet States, Czechoslovakia and Zaire (and Pays Basque and Timor,
 * not FIFA members then), and in a season game the old record of a renamed nation (season_intl.c dead_records). The original's pools leave such a nation out, so it gets
 * no manager either (Q43). */
uint32_t rt_national_vacancy(uint32_t club, uint32_t site)
{
    static const char *const why[] = {"the vacancy", "a retirement", "a move to another national team", "the month check",
                                       "a departure"};
    if (!fixes_on_group(MANAGERS) || creating || !club) return 0;
    uint32_t nation = rd32(g_mem, club + 0x53u);
    if (!nation || !rd32(g_mem, nation + 0x71u)) return 0;
    int32_t id = (int32_t)rd32(g_mem, club), clubs = (int32_t)rd32(g_mem, CLUB_COUNT);
    int32_t nations = (int32_t)rd32(g_mem, NATION_COUNT);
    if (id < clubs - 2 * nations || id >= clubs - nations) return 0;
    if (site)    /* site 0, the bound in add_job_vacancy: no line (it mostly follows one of the others) */
        fprintf(stderr, "recomp: national manager: a vacancy for %s after %s (%d players)\n", log_name(club + 4u),
                why[site < 5u ? site : 0u], (int32_t)rd32(g_mem, nation + 0x88u));
    return 1;
}

/* `database_get_staff_age` 0x53ab90 (cdecl: staff, date) gives the age in ax (0x53ac0a..0x53ac3a), and its callers keep
 * al. Above 127 it gives 127, so no caller sees a negative age (Q27). A null staff goes to the original's assert. */
void o_0053ab90(void);

RT_WRAP(0053ab90, staff_age)
{
    uint32_t staff = rt_arg(0);
    o_0053ab90();
    if (rt_unwinding || !fixes_on_group(AGE) || !staff) return;
    if ((int16_t)(cpu.eax & 0xffffu) > 127) cpu.eax = (cpu.eax & 0xffff0000u) | 127u;
}

/* Olympic squads (open item 102). `NationalTeams::select_squad` 0x76c120 picks the Olympic squad in slot 2 (the
 * under-21 side, 0x76c26d). A candidate in his nation's senior squad (+0x24 the senior side, flag 4 in +0x5e,
 * 0x76cdf1..0x76ce05) is asked of `national_teams_can_change_squad` 0x76db00 (nation, slot 0, player, removing) at
 * 0x76ce15. For the Olympics (fixture +0 is the key at 0x9cf7a0, 0x76ce2d) he may then leave the senior squad.
 * 0x76db00 refuses while the senior squad's window runs (its end at record +0x3a after today, 0x76deb5..0x76dee0),
 * but only for a squad marked selected (+0x44, 0x76de03). Only the human's squad screens set that mark
 * (set_squad_selected 0x76c0f0, from 0x779f7e, 0x784641, 0x78469a). So an AI senior squad lost its young players
 * to an Olympic pick made between its own pick and its match. The squads of a fixture are picked when it is 14 days
 * away (select_squads_for_fixture 0x76bab0 from the daily pass 0x76b100). A senior match in the 14 days before the
 * Olympics therefore meets it. In the run set game a2001offs3 (changing rules off, main 738c1269) Holland kept 10
 * of 26 for a World Cup qualifier on 31 August 2052 and stopped on the box match_eng..cpp 612.
 * Here, at 0x76ce1c (the jump that skips the candidate), the Olympic pick also skips a player of an AI senior squad
 * whose window ends today or later. On the last day of a window the original asks 0x5acee0 (not read) before it
 * allows a change (0x76dee5..0x76df87). The fix keeps the player on that day too. Other picks of slot 1 or 2 skip a
 * senior squad player anyway (0x76ce38). With the group off (keep:olympics) the line says what the original allows. */
#define NATIONAL_TEAMS 0xb74318u
#define OLYMPICS_KEY 0x9cf7a0u
#define TODAY 0xae2c90u

uint32_t rt_olympic_keeps_senior(uint32_t player, uint32_t nation, uint32_t slot, uint32_t fixture)
{
    if ((slot & 0xffu) == 0 || !player || !nation || !fixture) return 0;
    uint32_t teams = rd32(g_mem, NATIONAL_TEAMS);
    if (rd32(g_mem, fixture) != rd32(g_mem, OLYMPICS_KEY) || !teams) return 0;
    uint32_t senior = rd32(g_mem, teams + 4u) + 110u * rd32(g_mem, nation);     /* slot 0 record, as 0x76dd16 */
    int end_day = (int16_t)rd16(g_mem, senior + 0x3au), end_year = (int16_t)rd16(g_mem, senior + 0x3cu);
    int day = (int16_t)rd16(g_mem, TODAY), year = (int16_t)rd16(g_mem, TODAY + 2u);
    if (end_year < year || (end_year == year && end_day < day)) return 0;
    int on = fixes_on_group(OLYMPICS);
    if ((int8_t)g_mem[player + 0x18u] > 23) return (uint32_t)on;     /* too old for this pick anyway (0x76cee3) */
    char name[32];
    snprintf(name, sizeof name, "%s", log_name(nation + 4u));
    fprintf(stderr, "recomp: olympic squad: %s %s staff %d %s its senior squad on day %d of %d (window to day %d of "
                    "%d)\n", name, on ? "keeps" : "may take", (int32_t)rd32(g_mem, player), on ? "in" : "from", day,
            year, end_day, end_year);
    return (uint32_t)on;
}

/* The Irish Super Cup (open item 127, tools/recomp/original_faults.py). At 0x63b2ae the last loop of
 * `IreSuperCup::create_table` 0x63b0c0 has found a Premier Division club. The table (+0xb1) holds rows of 0x3b bytes
 * with the club at +0 (League::add_club_to_table 0x687430), and the loops have filled the first `rows` (ebx). A club
 * that is one of them is skipped, so the table holds four clubs and the edition can complete. With the group off
 * (keep:ire_super) the line says that the original takes the club again. */
uint32_t rt_ire_super_has(uint32_t cup, uint32_t club, uint32_t rows)
{
    uint32_t table = rd32(g_mem, cup + 0xb1u);
    int found = 0;
    for (uint32_t i = 0; table && i < rows && i < 4u && !found; i++) found = rd32(g_mem, table + 0x3bu * i) == club;
    if (!found) return 0;
    int on = fixes_on_group(IRE_SUPER);
    fprintf(stderr, "recomp: Irish Super Cup %d: %s %s, already in the table\n", (int16_t)rd16(g_mem, cup + 0x40u),
            on ? "skips" : "takes again", log_name(club + 4u));
    return (uint32_t)on;
}

/* Irish Super Cup, saves of a game that met the fault (open item 127). The broken edition never completes, so the
 * season pass never calls IreSuperCup::start_new_season 0x63b550 (all_stages_complete 0x48ce10 stays false), and
 * IreSuperCup::update_for_date 0x63b300 draws only while +0x40 is the game year ([0xde1f58]+0x439, 0x63b311..
 * 0x63b331). The cup is then gone for every later season. The real cup is played every season. In a game with no
 * fault, start_new_season runs after each final, so +0x40 is never below the game year (the trace of a2001offs3,
 * 2051 to 2057). So when the game year moves on, +0x40 is below it and the table holds one club twice (the state of
 * the fault, ire_super_broken), the wrap runs start_new_season. It runs outside the season pass, so
 * season_change_start runs no hooks, but only a broken cup gets here. When the cup is drawn anew (+0x45 0) and still
 * behind, the wrap sets +0x40 to the game year. The original then draws the cup on that day, as it does in a game
 * with no fault. The broken edition leaves no history row (checked on the
 * batch O save of a2001offs3). A load or a new game forgets the year seen (rt_ire_super_forget, from
 * season_dates_forget), so the restart waits for a year turn that the game plays.
 * A cup that the rules table parks in a later year (season_kind_cup_breaks.c, 2999) is never below the game year. */
void o_0063b300(void);
static int ire_super_seen;              /* the game year at the last update; 0 after a load or a new game */

void rt_ire_super_forget(void)
{
    ire_super_seen = 0;
}

/* 1 when the table of the cup (+0xb1, rows of 0x3b bytes, club at +0) holds one club twice in its four rows: the
 * state that the fault leaves. With changing rules on, the rules table stops the cup after 2001
 * (season_kind_cup_breaks.c skips start_new_season), and that cup keeps a whole table and a year below the game
 * year: it must stay stopped. */
static int ire_super_broken(uint32_t cup)
{
    uint32_t table = rd32(g_mem, cup + 0xb1u);
    if (!table) return 0;
    for (uint32_t i = 0; i < 4u; i++)
        for (uint32_t j = i + 1; j < 4u; j++) {
            uint32_t a = rd32(g_mem, table + 0x3bu * i);
            if (a && a == rd32(g_mem, table + 0x3bu * j)) return 1;
        }
    return 0;
}

RT_WRAP(0063b300, ire_super_update)
{
    uint32_t self = cpu.ecx;
    int game = (int16_t)rd16(g_mem, rd32(g_mem, 0xde1f58u) + 0x439u);
    int year = (int16_t)rd16(g_mem, self + 0x40u);
    int turned = ire_super_seen && game == ire_super_seen + 1;
    ire_super_seen = game;
    if (turned && year < game && ire_super_broken(self)) {
        int on = fixes_on_group(IRE_SUPER);
        fprintf(stderr, "recomp: Irish Super Cup: the cup of %d is %s in %d\n", year, on ? "restarted" : "left stopped",
                game);
        if (on) {
            Cpu saved = cpu;
            cpu.ecx = self;
            rt_callback(0x63b550u, 0, NULL);
            if (rt_unwinding) return;
            cpu = saved;
            if (!rd32(g_mem, self + 0x45u) && (int16_t)rd16(g_mem, self + 0x40u) < game)
                wr16(g_mem, self + 0x40u, (uint16_t)game);
        }
    }
    cpu.ecx = self;
    o_0063b300();
}

/* Australian NSL stages (open item 128, original_faults.py). `AusNsl::start_new_season` 0x411de0 keeps the stage
 * index (+0x30) and the stages of the season before, so from the second season the game builds no finals series
 * and no Grand Final (`AusNsl::advance_stage` 0x411f70 acts only while the index is below the count - 1). At
 * 0x411e1c each stage 0..index is freed by its slot 0 with 1, as `ArgPrm::start_new_season` 0x405720,
 * `BraNatFirst::start_new_season` 0x42a640 and the AusNsl destructor 0x4110e0 do, its slot of the array (+0xc) set to 0, and the index set to -1, as the ctor 0x410e90
 * leaves it. */
#define NSL_STAGES 0xcu
#define NSL_STAGE_COUNT 0x2cu
#define NSL_STAGE_NOW 0x30u

void rt_aus_nsl_new_season(uint32_t self)
{
    if (!fixes_on_group(NSL)) return;
    int32_t now = (int32_t)rd32(g_mem, self + NSL_STAGE_NOW), count = (int32_t)rd32(g_mem, self + NSL_STAGE_COUNT);
    uint32_t stages = rd32(g_mem, self + NSL_STAGES);
    if (now < 0) return;
    for (int32_t i = 0; stages && i <= now && i < count; i++) {
        uint32_t stage = rd32(g_mem, stages + 4u * (uint32_t)i);
        if (!stage) continue;
        wr32(g_mem, stages + 4u * (uint32_t)i, 0u);
        Cpu saved = cpu;
        cpu.ecx = stage;
        const uint32_t one = 1u;
        rt_callback(rd32(g_mem, rd32(g_mem, stage)), 1, &one);      /* the scalar deleting destructor, slot 0 */
        if (rt_unwinding) return;
        cpu = saved;
    }
    wr32(g_mem, self + NSL_STAGE_NOW, 0xffffffffu);
    fprintf(stderr, "recomp: aus nsl: a new season from stage %d of %d on day %d of %d\n", now, count,
            (int16_t)rd16(g_mem, TODAY), (int16_t)rd16(g_mem, TODAY + 2u));
}

/* Money (Q24). `Finance::get_transfer_budget` 0x598800 works on a 64-bit budget, but cuts three values to 32 bits.
 * The first two (0x598f4f, 0x598f8b, 0x598f92) re-extend the low word with cdq: here the high word is used. */
uint32_t rt_budget_hi(uint32_t lo, uint32_t hi)
{
    return money_fixes_on() ? hi : (uint32_t)((int32_t)lo >> 31);
}

/* The third is the floor R = balance - 10m (or 15m) (0x599135, 0x599185), a 32-bit value in [esp+0x5c] that the
 * budget falls back to (0x5991d0..0x599201: above R only a tenth counts). Above 2,147,483,647 the store keeps
 * 2,147,483,647 and the full value waits here for the three reads of R. A new call resets it (the wrap below). */
static int64_t floor_wide;
static int floor_on, floor_used;

uint32_t rt_budget_floor(uint32_t finance, uint32_t less)
{
    int64_t r = (int64_t)rd64(g_mem, finance) - (int64_t)less;
    floor_on = money_fixes_on() && r > INT32_MAX;
    if (!floor_on) return rd32(g_mem, finance);
    floor_wide = r;
    return (uint32_t)INT32_MAX + less;      /* the sub that follows leaves 2,147,483,647 */
}

uint32_t rt_budget_floor_low(uint32_t r)
{
    floor_used = floor_on && r == (uint32_t)INT32_MAX;
    return floor_used ? (uint32_t)floor_wide : r;
}

uint32_t rt_budget_floor_high(uint32_t lo)
{
    return floor_used ? (uint32_t)((uint64_t)floor_wide >> 32) : (uint32_t)((int32_t)lo >> 31);
}

int64_t rt_budget_floor_wide(uint32_t r)
{
    return floor_on && r == (uint32_t)INT32_MAX ? floor_wide : (int64_t)(int32_t)r;
}

/* The 37 callers of 0x598800 (by the return address): these 14 keep the 64-bit result (the docs file), three of
 * them only to show it (0x463b20, 0x808ee9, 0x8ea2b4, with rt_cash_wide). The other 23 keep eax: the clamp gives
 * them 2,147,483,647 for a larger budget, where eax alone would be 0 or a wrong amount. */
static const uint32_t budget_wide_callers[] = {
    0x463afdu, 0x463b20u, 0x4d7541u, 0x6970a3u, 0x6a3b8cu, 0x7b6d5fu, 0x7b6e25u, 0x808ee9u, 0x8c0eb6u, 0x8e74a7u,
    0x8e83a1u, 0x8ea2b4u, 0x8f4b28u, 0x8f4bd5u,
};

void o_00598800(void);

static uint32_t news_club;    /* the club of the last budget above 2,147,483,647 that a caller got clamped */
static int64_t news_full;

static int money_log(void)
{
    static int on = -1;
    if (on < 0) on = getenv("RECOMP_MONEY_LOG") != NULL;
    return on;
}

RT_WRAP(00598800, get_transfer_budget)
{
    uint32_t from = rd32(g_mem, cpu.esp), finance = cpu.ecx;
    floor_on = floor_used = 0;
    o_00598800();
    if (rt_unwinding) return;
    int64_t v = (int64_t)(((uint64_t)cpu.edx << 32) | cpu.eax);
    int wide = 0;
    for (size_t i = 0; i < sizeof budget_wide_callers / sizeof budget_wide_callers[0]; i++)
        wide |= from == budget_wide_callers[i];
    if (money_fixes_on() && !wide) {
        if (v > INT32_MAX) {
            cpu.eax = (uint32_t)INT32_MAX, cpu.edx = 0;
            news_club = rd32(g_mem, finance + 8u);    /* rt_news_wide: a news item may carry this budget */
            news_full = v;
        } else if (rd32(g_mem, finance + 8u) == news_club) {
            news_full = 0;    /* the club's budget fits now: an amount of exactly 2,147,483,647 is its own */
        }
        else if (v < INT32_MIN) cpu.eax = 0x80000000u, cpu.edx = 0xffffffffu;
    }
    /* test switch (tests/seasons/original_faults_run.py money): the budgets of a club above 2,147,483,647 */
    if (money_log() && (int64_t)rd64(g_mem, finance) > INT32_MAX) {
        uint32_t club = rd32(g_mem, 0xae23b4u) + rd32(g_mem, finance + 8u) * 0x245u;
        fprintf(stderr, "recomp: money budget: %s from 0x%x: %lld, the caller sees %lld\n", log_name(club + 4u), from,
                (long long)v, wide ? (long long)v : (long long)(int32_t)cpu.eax);
    }
}

/* The money text. A Cash holds 32 bits, and `Cash::get_display_amount` 0x43f460 starts with `fild dword [ecx]`;
 * after that the original works on 64 bits (the rounding step 0x4426e0 and the text `cash_format_money`
 * 0x441c10 take an int64). A screen that puts a 64-bit amount into a Cash registers it here with the number of reads
 * of that site (one, or two for the club info balance, which formats it again when the text is long, 0x4639c8..0x4639e6,
 * and ends at 0x4639ee). Those reads take the full amount while the Cash keeps its low word. */
static uint32_t wide_at, wide_reads, wide_stored;
static int64_t wide_amount;

/* The Cash at cash holds stored; its next reads give amount. */
static void cash_wide(uint32_t cash, uint32_t stored, int64_t amount, uint32_t reads)
{
    wide_reads = money_fixes_on() && amount != (int64_t)(int32_t)stored ? reads : 0u;
    wide_at = cash;
    wide_stored = stored;
    wide_amount = amount;
}

void rt_cash_wide(uint32_t cash, uint32_t lo, uint32_t hi, uint32_t reads)
{
    cash_wide(cash, lo, (int64_t)(((uint64_t)hi << 32) | lo), reads);
}

/* The bid screen (0x8ea2c0): the Cash keeps the budget clamped to 2,147,483,647, because the fee colour reads it
 * again (0x8ea54c, 0x8eaf95, 0x8eb3b0); only the text read gives the full amount. Returns the value to store. */
uint32_t rt_cash_wide_clamped(uint32_t cash, uint32_t lo, uint32_t hi, uint32_t reads)
{
    int64_t v = (int64_t)(((uint64_t)hi << 32) | lo);
    uint32_t stored = !money_fixes_on() ? lo : v > INT32_MAX ? (uint32_t)INT32_MAX : v < INT32_MIN ? 0x80000000u : lo;
    cash_wide(cash, stored, v, reads);
    return stored;
}

void rt_cash_wide_end(void)
{
    wide_reads = 0;
}

int64_t rt_cash_amount(uint32_t cash)
{
    uint32_t lo = rd32(g_mem, cash);
    if (wide_reads && cash == wide_at && lo == wide_stored) {
        wide_reads--;
        if (money_log())     /* test switch: tests/seasons/original_faults_run.py money */
            fprintf(stderr, "recomp: money text: %lld\n", (long long)wide_amount);
        return wide_amount;
    }
    return (int64_t)(int32_t)lo;
}

/* `Finance::get_max_wage` 0x5968d0: the wage term plus ftol(1.3 x balance) (ftol at 0x5968e3, only eax kept at
 * 0x5968ee), in 32 bits at 0x597019. 0x5968e8 hands over the high word in edx; the sum takes it when the low word is
 * still the one of that product, and a sum above 2,147,483,647 is clamped to it. Any other sum, a debt among them,
 * is the original's. */
static uint32_t wage_high, wage_low, wage_set;

void rt_max_wage_high(uint32_t edx, uint32_t eax)
{
    wage_high = edx;
    wage_low = eax;
    wage_set = 1;
}

uint32_t rt_max_wage_sum(uint32_t term, uint32_t lo)
{
    int had = wage_set && lo == wage_low;
    wage_set = 0;
    if (!money_fixes_on() || !had) return term + lo;
    int64_t sum = (int64_t)(int32_t)term + (int64_t)(((uint64_t)wage_high << 32) | lo);
    return sum > INT32_MAX ? (uint32_t)INT32_MAX : term + lo;
}

/* News (Q46). A news item keeps 50 32-bit slots at item + 5 + 4 x slot (`NewsItem::set_slot` 0x785790), saved raw in
 * news.dat. The job offer (0x6a5021) and the finance news (0x59f36b) put a transfer budget in slot 2, clamped to
 * 2,147,483,647 by the wrap of 0x598800. When that budget was clamped for the same club, its full amount also goes into
 * slots 8 (low word) and 9 (high word), which these items do not use and which the constructor clears
 * (0x77bc1b..0x77bc29): below 2,147,483,647 the item, and so the save, is unchanged. */
#define NEWS_SLOT(item, n) ((item) + 5u + 4u * (n))

void rt_news_wide(uint32_t item, uint32_t club, uint32_t value)
{
    if (!money_fixes_on() || value != (uint32_t)INT32_MAX || club != news_club || news_full <= INT32_MAX) return;
    wr32(g_mem, NEWS_SLOT(item, 8u), (uint32_t)news_full);
    wr32(g_mem, NEWS_SLOT(item, 9u), (uint32_t)((uint64_t)news_full >> 32));
}

/* The text writers read slot 2 (0x6a51dd, 0x59f5ab) and later put it into a Cash (0x6a533d, 0x59f5c7). */
static int64_t news_pending;

void rt_news_read(uint32_t item, uint32_t value)
{
    uint64_t full = ((uint64_t)rd32(g_mem, NEWS_SLOT(item, 9u)) << 32) | rd32(g_mem, NEWS_SLOT(item, 8u));
    news_pending = money_fixes_on() && value == (uint32_t)INT32_MAX && (int64_t)full > INT32_MAX ? (int64_t)full : 0;
}

void rt_cash_news(uint32_t cash, uint32_t value)
{
    if (news_pending && value == (uint32_t)INT32_MAX) cash_wide(cash, value, news_pending, 1u);
    news_pending = 0;
}

/* Holiday offers (open item 132, original_faults.py). The settings of "Go on holiday ?" are the bytes at human record
 * +0x9b23 (HUMAN_MANAGER::get_holiday_option 0x5fea50): option 0 is 1 for "Reject all offers for players" and 2 for
 * "Reject all offers below asking price" (the confirm build 0x6b2d80, its input 0x6b35c1). A player's asking price is
 * staff +0x52 (TRANSFER_MANAGER::set_player_squad_status 0x8ce1e9). */
#define HUMANS 0xb63c98u          /* the human manager object: +0xc the count of humans on holiday */
#define STAFF_RECORDS 0xae23bcu
#define IS_HUMAN_CLUB 0x600a50u   /* cdecl (club, ignore_holiday, quick, manager, any) */
#define CLUB_MANAGER 0x53ca90u    /* cdecl (club) */
#define ON_HOLIDAY 0x5fd210u      /* thiscall HUMANS (manager) */
#define HOLIDAY_OPTION 0x5fea50u  /* thiscall HUMANS (option, manager) */
#define RELEASE_CLAUSE 0x867b50u  /* thiscall contract: a release clause in force */

/* The value of option 0 when a human manager on holiday runs the club, else 0. The test is the one that
 * `shortlist_manager_predict_selling_club_response` 0x84af00 makes at 0x84b05e..0x84b096, but with the human test of
 * `ai_respond_to_bid` 0x8c81ea (0x600a50 with ignore_holiday 1), so that a club on holiday passes it. A contract with
 * a release clause in force keeps the original's answer (0x84b05e), and so does every club while no human is on
 * holiday (the count at [0xb63c98] + 0xc, kept by `set_on_holiday` 0x5fd2f0). */
static uint32_t holiday_option(uint32_t club, uint32_t contract)
{
    if (!fixes_on_group(HOLIDAY) || !club || !rd32(g_mem, rd32(g_mem, HUMANS) + 0xcu)) return 0;
    Cpu saved = cpu;
    uint32_t r = 0;
    uint32_t a[5] = {club, 1u, 1u, 0u, 0u};
    if (!rt_callback(IS_HUMAN_CLUB, 5, a) || rt_unwinding) goto done;
    if (contract) {
        cpu.ecx = contract;
        if (rt_callback(RELEASE_CLAUSE, 0, NULL) || rt_unwinding) goto done;
    }
    uint32_t m[1] = {club};
    uint32_t manager = rt_callback(CLUB_MANAGER, 1, m);
    if (rt_unwinding) goto done;
    uint32_t h[1] = {manager};
    cpu.ecx = rd32(g_mem, HUMANS);
    if (!rt_callback(ON_HOLIDAY, 1, h) || rt_unwinding) goto done;
    uint32_t o[2] = {0u, manager};
    cpu.ecx = rd32(g_mem, HUMANS);
    r = rt_callback(HOLIDAY_OPTION, 2, o) & 0xffu;
done:
    if (rt_unwinding) return 0;
    cpu = saved;
    return r == 1u || r == 2u ? r : 0;
}

/* The predictor 0x84af00 for a transfer (0x84b02f, the computer path of a club on holiday): 1 for "Reject all
 * offers" (the answer 2, reject, at 0x84b01c), 2 for "below asking price" when the player has an asking price
 * (staff +0x52 above 0, the price that the dead test takes at 0x84b0a8), else 0: the original's computer path at
 * 0x84b0ad. */
uint32_t rt_holiday_predict(uint32_t club, uint32_t contract, uint32_t player)
{
    uint32_t opt = holiday_option(club, contract);
    if (opt == 2u && (int32_t)rd32(g_mem, player + 0x52u) <= 0) return 0;
    return opt;
}

/* The predictor 0x84af00 for a loan (0x84af77): 1 when the club is on holiday with "Reject all offers" (the answer 2
 * at 0x84b162). */
uint32_t rt_holiday_refuses_loan(uint32_t club)
{
    return holiday_option(club, 0) == 1u;
}

/* The bid reply 0x8c8090 at 0x8c8264, after its human and holiday tests (0x8c81ea..0x8c8216): 1 when option 0 (al) is
 * "below asking price" and the bid's fee (offer +0x1b) is below the player's asking price, so the bid takes the
 * reject of "Reject all offers" at 0x8c8266. esi holds the offer slot. */
uint32_t rt_holiday_below_asking(uint32_t option, uint32_t slot)
{
    if (!fixes_on_group(HOLIDAY) || (option & 0xffu) != 2u) return 0;
    uint32_t offer = rd32(g_mem, slot);
    if (!offer) return 0;
    uint32_t player = rd32(g_mem, STAFF_RECORDS) + rd32(g_mem, offer + 0xcu) * 0x6eu;
    int32_t ask = (int32_t)rd32(g_mem, player + 0x52u), fee = (int32_t)rd32(g_mem, offer + 0x1bu);
    return ask > 0 && fee < ask;
}

/* Senior call-ups (open item 143, original_faults.py). `NationalTeams::select_squad` 0x76c120 refuses a candidate at a
 * club on another continent (club +0x53 nation, nation +0x71 continent, 0x76cc3d..0x76cc59 and 0x76cc9f..0x76ccbd):
 * for a friendly (fixture +0 the key at 0x9cf668) when his club's reputation (+0x80) x 0.8 (0x9689b0) is over the
 * opponent's (0x76ccdd..0x76cd3e), for another record when his division's reputation (club +0x57, +0x69) x 0.7
 * (0x968e40) is over the competition's (0x76cc61..0x76cc9a). 1 when the pick is for a senior squad (slot 0, the
 * byte argument at [esp + 0x8e4], 0x76cd9c) and the fix acts: the candidate then goes on as a player at a club of his
 * own continent does. 'B' and under-21 picks keep the original's rule. */
uint32_t rt_callup_released(uint32_t slot)
{
    return fixes_on_group(CALLUPS) && (slot & 0xffu) == 0u;
}

/* The Confederations Cup in June (open items 157 to 159): season_intl_date_shift (season_intl_rules.c) moves an edition
 * that keeps the original's dates, 29 July to 9 August, to the Saturday on or before 18 June,
 * so all of it plays in June. It acts from the new game on:
 * the base game builds its 2003 edition there (the stage builder 0x58bdb0). */
int season_intl_confed_june(void)
{
    return fixes_on_group(CONFED);
}
