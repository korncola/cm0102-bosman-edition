/* Faults of the original that long games reach (0.6.2 slice 11, tools/recomp/original_faults.py). The fixes act in
 * every game, the base game too (Q23, Q24, Q27). The test switch RECOMP_ORIGINAL_FAULTS turns them off: "keep" all
 * of them, "keep:" with a list of groups some of them ("keep:money,age"). The groups are managers, money, age and
 * olympics. */
#include "win.h"
#include "../rt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLUB_COUNT 0xae235cu     /* clubs, national teams and under-21 teams */
#define NATION_COUNT 0xae2350u

enum { MANAGERS, MONEY, AGE, OLYMPICS, GROUPS };

static int fixes_on_group(int group)
{
    static int on[GROUPS] = {-1};
    if (on[0] < 0) {
        static const char *const names[GROUPS] = {"managers", "money", "age", "olympics"};
        const char *v = getenv("RECOMP_ORIGINAL_FAULTS");
        if (v && strcmp(v, "keep") && strncmp(v, "keep:", 5))
            fprintf(stderr, "recomp: RECOMP_ORIGINAL_FAULTS=%s is not keep or keep:<groups>: every fix acts\n", v);
        const char *list = v && !strncmp(v, "keep:", 5) ? v + 5 : "";
        for (const char *p = list; *p; p += strcspn(p, ",") + (p[strcspn(p, ",")] == ',')) {
            size_t n = strcspn(p, ",");
            int known = 0;
            for (int g = 0; g < GROUPS; g++) known |= n == strlen(names[g]) && !strncmp(p, names[g], n);
            if (n && !known)        /* a typo would leave its fixes on, and a same-save run would still pass */
                fprintf(stderr, "recomp: RECOMP_ORIGINAL_FAULTS: unknown group %.*s (managers, money, age, olympics)\n",
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
 * 0x76ce15, and for the Olympics (0x9cf7a0, 0x76ce2d) he may then leave the senior squad. 0x76db00 refuses while the
 * senior squad's window runs (its end at record +0x3a after today, 0x76deb5..0x76dee0), but only for a squad marked
 * selected (+0x44, 0x76de03). Only the human's squad screens set that mark (set_squad_selected 0x76c0f0, from
 * 0x779f7e, 0x784641, 0x78469a), so an AI senior squad lost its young players to an Olympic pick made between its
 * own pick and its match. The squads of a fixture are picked when it is 14 days away (select_squads_for_fixture
 * 0x76bab0 from the daily pass 0x76b100), so a senior match in the 14 days before the Olympics meets it: in the run
 * set game a2001offs3 (changing rules off) Holland kept 10 of 26 for a World Cup qualifier on 31 August 2052. Here the Olympic
 * pick also keeps out a player of an AI senior squad whose window ends today or later (the original looks for a
 * match today when the window ends today; a match that day is the reason to keep him). 0x76ce1c, the jump that
 * skips the candidate. Other picks of slot 1 or 2 skip a senior squad player anyway (0x76ce38). */
#define NATIONAL_TEAMS 0xb74318u
#define OLYMPICS_KEY 0x9cf7a0u
#define TODAY 0xae2c90u

uint32_t rt_olympic_keeps_senior(uint32_t player, uint32_t nation, uint32_t slot, uint32_t fixture)
{
    if (!fixes_on_group(OLYMPICS) || (slot & 0xffu) == 0 || !player || !nation || !fixture) return 0;
    uint32_t comp = rd32(g_mem, fixture + 0x14u), teams = rd32(g_mem, NATIONAL_TEAMS);
    if (!comp || rd32(g_mem, comp) != rd32(g_mem, OLYMPICS_KEY) || !teams) return 0;
    uint32_t senior = rd32(g_mem, teams + 4u) + 110u * rd32(g_mem, nation);     /* slot 0 record, as 0x76dd16 */
    int end_day = (int16_t)rd16(g_mem, senior + 0x3au), end_year = (int16_t)rd16(g_mem, senior + 0x3cu);
    int day = (int16_t)rd16(g_mem, TODAY), year = (int16_t)rd16(g_mem, TODAY + 2u);
    if (end_year < year || (end_year == year && end_day < day)) return 0;
    if ((int8_t)g_mem[player + 0x18u] > 23) return 1;    /* too old for this pick anyway (0x76cee3): no line */
    char name[32];
    snprintf(name, sizeof name, "%s", log_name(nation + 4u));
    fprintf(stderr, "recomp: olympic squad: %s keeps staff %d in its senior squad on day %d of %d (window to day %d of "
                    "%d)\n", name, (int32_t)rd32(g_mem, player), day, year, end_day, end_year);
    return 1;
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
