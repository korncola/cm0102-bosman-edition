/* Test switch RECOMP_TEST_NATION_JOB=<nation> (Q15, run set row a2001nation): the human manager also manages a
 * national team, so that a long test game runs the original's national team code with a human manager. Only the
 * test runs set it. A normal game never reads more than the variable.
 *
 * Why a switch. The original has no short path that a script can take: the Apply for Job button of a national
 * team's screen (add_club_screen_action_button 0x482a40) shows only when the team has no manager
 * (club_get_manager 0x53ca90) and its nation's value +0x88 is over 150 (0x544820), no nation is known to be without a
 * manager at a 2001 start, a vacancy decides 5 to 14 days later (MANAGER_MANAGER::add_job_vacancy 0x69ba50), and
 * MANAGER_MANAGER::daily_update 0x68f9c0 offers the job to a human only after 90 days in his club job.
 *
 * What it does. At the first human turn (human_turn 0x5cde40, testrun.c) on holiday, it takes the first human
 * manager (the last 16 rows of the staff table [0xae23bc], 0x6e bytes a row, in use when +0x5f is not 0; his
 * holiday flag is +0x9a01 of his record in [[0xb63c98]], 0xa6ef bytes a record, holiday_next.c) and a senior national
 * team: a club whose id is from [0xae235c] - 2 * [0xae2350] up to [0xae235c] - [0xae2350] (club_is_national_team
 * 0x53b900, 0x544820), with no manager at +0xcf and a nation (+0x53) whose value +0x88 is over 150, the conditions
 * of the Apply for Job button. The named nation (the club's long name at +4) if it qualifies, else the first such
 * team in club order. Then it calls MANAGER_MANAGER::appoint_manager_to_club 0x69ebc0, thiscall on [0xb67a44]
 * (club, staff), the call that ends an application. For a national team it sets the staff's national team +0x24 and
 * job +0x28 (5) and the club's manager +0xcf, and leaves the club job +0x39 as it is (0x69ebc0, the branch of
 * 0x53b900 != 0). A human who manages a national team already is left as he is (a loaded game).
 * It waits for the holiday because a national team adds an item to the left bar menu: the test script's click on
 * Go on Holiday would hit the item below it (Holiday until next match, holiday_next.c), and that holiday ends
 * before the club's next match, where the script has no clicks.
 *
 * The check. At the first human turn after the last day of the first FIFA window after the appointment (the run of
 * days from season_fifa_year_days), it logs whether the human still manages that team. It does not show that the
 * team played a match in that window. tests/seasons/future/run_set.py reads both lines. */
#include "win.h"
#include "season_uefa_days.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GAME_DATE 0xae2c90u          /* s16 day of the year (0 is 1 January), s16 year */
#define STAFF_TABLE 0xae23bcu
#define STAFF_COUNT 0xae2364u
#define STAFF_SIZE 0x6eu
#define STAFF_HUMAN 0x5fu
#define STAFF_NATION_TEAM 0x24u      /* the national team the staff manages (0x69ebc0) */
#define CLUB_TABLE 0xae23b4u
#define CLUB_SIZE 0x245u
#define CLUB_TOTAL 0xae235cu
#define NATIONS 0xae2350u
#define CLUB_LONG 0x4u
#define CLUB_NATION 0x53u
#define CLUB_MANAGER 0xcfu
#define NATION_VALUE 0x88u
#define MANAGER_MANAGER 0xb67a44u
#define APPOINT 0x0069ebc0u
#define HUMANS 0xb63c98u             /* the human records, 0xa6ef bytes each (holiday_next.c) */
#define HUMAN_RECORD 0xa6efu
#define ON_HOLIDAY 0x9a01u

static int state = -1;               /* -1 not read, 0 off, 1 to appoint, 2 appointed, 3 checked */
static uint32_t human, team;
static int check_year, check_doy;

static const char *long_name(uint32_t club)
{
    static char name[64];
    snprintf(name, sizeof name, "%.50s", (const char *)(g_mem + club + CLUB_LONG));
    return name;
}

static int qualifies(uint32_t club)
{
    uint32_t nation = rd32(g_mem, club + CLUB_NATION);
    return !rd32(g_mem, club + CLUB_MANAGER) && nation && (int32_t)rd32(g_mem, nation + NATION_VALUE) > 150;
}

static uint32_t pick(const char *want)
{
    uint32_t table = rd32(g_mem, CLUB_TABLE);
    int total = (int)rd32(g_mem, CLUB_TOTAL), n = (int)rd32(g_mem, NATIONS);
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < total; i++) {
            uint32_t club = table + (uint32_t)i * CLUB_SIZE;
            int id = (int)rd32(g_mem, club);
            if (id < total - 2 * n || id >= total - n) continue;
            if (pass == 0 && strcmp(long_name(club), want)) continue;
            if (pass == 0 && !qualifies(club)) {
                fprintf(stderr, "recomp: nation job test: %s does not qualify (manager %s, nation value %d)\n", want,
                        rd32(g_mem, club + CLUB_MANAGER) ? "set" : "none",
                        rd32(g_mem, club + CLUB_NATION) ?
                            (int)rd32(g_mem, rd32(g_mem, club + CLUB_NATION) + NATION_VALUE) : 0);
                break;
            }
            if (qualifies(club)) return club;
        }
    return 0;
}

static int first_fifa_window_end(int year, int doy, int *out_year, int *out_doy)
{
    for (int y = year; y <= year + 1; y++) {
        int days[SEASON_FIFA_YEAR_MAX], n = season_fifa_year_days(y, days, SEASON_FIFA_YEAR_MAX), best = -1;
        for (int i = 0; i < n; i++)
            if ((y > year || days[i] > doy) && (best < 0 || days[i] < best)) best = days[i];
        if (best >= 0) {
            for (int more = 1; more; ) {            /* the last day of the window that starts there */
                more = 0;
                for (int i = 0; i < n; i++)
                    if (days[i] == best + 1) { best++; more = 1; }
            }
            *out_year = y;
            *out_doy = best;
            return 1;
        }
    }
    return 0;
}

void nation_job_turn(void)
{
    if (state < 0) {
        const char *v = getenv("RECOMP_TEST_NATION_JOB");
        state = v && *v ? 1 : 0;
    }
    if (!state || state == 3) return;
    int doy = (int16_t)rd16(g_mem, GAME_DATE), year = (int16_t)rd16(g_mem, GAME_DATE + 2);
    if (state == 1) {
        uint32_t staff_table = rd32(g_mem, STAFF_TABLE);
        int count = (int)rd32(g_mem, STAFF_COUNT), slot = -1;
        for (int i = count - 16; i < count && !human; i++)
            if (i >= 0 && g_mem[staff_table + (uint32_t)i * STAFF_SIZE + STAFF_HUMAN]) {
                human = staff_table + (uint32_t)i * STAFF_SIZE;
                slot = i - count + 16;
            }
        if (!human) {
            state = 3;
            fprintf(stderr, "recomp: nation job test: no human manager\n");
            return;
        }
        uint32_t record = rd32(g_mem, rd32(g_mem, HUMANS)) + (uint32_t)slot * HUMAN_RECORD;
        if (!rd32(g_mem, record + ON_HOLIDAY)) {
            human = 0;                                      /* not on holiday yet: look again at the next turn */
            return;
        }
        state = 3;
        uint32_t held = rd32(g_mem, human + STAFF_NATION_TEAM);
        if (held) {
            fprintf(stderr, "recomp: nation job test: the human manages %s already\n", long_name(held));
            return;
        }
        team = pick(getenv("RECOMP_TEST_NATION_JOB"));
        if (!team) {
            fprintf(stderr, "recomp: nation job test: no national team without a manager and a value over 150\n");
            return;
        }
        uint32_t args[2] = {team, human};
        Cpu saved = cpu;
        cpu.ecx = rd32(g_mem, MANAGER_MANAGER);
        rt_callback(APPOINT, 2, args);
        cpu = saved;
        int now = rd32(g_mem, human + STAFF_NATION_TEAM) == team && rd32(g_mem, team + CLUB_MANAGER) == human;
        fprintf(stderr, "recomp: nation job test: the human manages %s from day %d of %d (%s)\n", long_name(team),
                doy + 1, year, now ? "appointed" : "the call did not appoint him");
        if (now && first_fifa_window_end(year, doy, &check_year, &check_doy)) state = 2;
        return;
    }
    if (year < check_year || (year == check_year && doy <= check_doy)) return;
    state = 3;
    int still = rd32(g_mem, human + STAFF_NATION_TEAM) == team && rd32(g_mem, team + CLUB_MANAGER) == human;
    fprintf(stderr, "recomp: nation job test: on day %d of %d (the first FIFA window ends on day %d) the human %s %s\n",
            doy + 1, year, check_doy + 1, still ? "still manages" : "no longer manages", long_name(team));
}
