/* France: a reserve side does not go up from the CFA into the National (open item 116, 0.7.0).
 *
 * The real rule, 2001/02 to 2025/26: a reserve side of a professional club plays at most in the CFA (National 2 from
 * 2017/18), tier 4. It never goes up to the National (tier 3). When it wins its group, the next club of the group
 * that may go up takes the place (Bordeaux B won CFA group D in 2007/08 and Bayonne went up; Rennes B won group A in
 * 2016/17 and Cholet went up). Sources, read on 3 November 2026: https://fr.wikipedia.org/wiki/%C3%89quipe_r%C3%A9serve
 * ("les équipes réserves ne pouvant pas monter en National"), https://fr.vikidia.org/wiki/%C3%89quipe_r%C3%A9serve
 * (the two cases), and the rules table research docs/research/rules-0.6.0/fra.md (the CFA rows: "reserve teams
 * cannot go up"). The FFF text itself was not read. 2026/27 (Ligue 3, tier 4 named National 1): the rule is not
 * known (the FFF regulation 2026-2027 refused the fetch), so the game keeps the bar of the seasons before.
 *
 * The original. The CFA is not played for promotion: at the start its clubs go to A Lower Division [0x9cf764], and
 * the record +0x60 of a club (its zone) marks the CFA [0x9cf5d8]. FraFirst::start_new_season 0x5bc4d0 runs the chain
 * of Ligue 1, Ligue 2 and the National (vtable +0xb0 at 0x5bc4f7) and then FraFirst::exchange_cfa_national_clubs
 * 0x5bc780 (at 0x5bc4ff). That function lists every club with a division, a nation and a zone (0x5bc83f..0x5bc852)
 * whose division is A Lower Division (0x5bc854..0x5bc85c) and whose zone is the CFA (0x5bc85e..0x5bc868), with no
 * name or reserve check. It sorts the list by reputation and shuffles the top 4 (comp_util_sort_and_shuffle_top_clubs
 * 0x4afbd0 at 0x5bc8a6), moves the first 6 (FraFirst +0x40 is 2001, 0x5bc8ae) or 4 into the National
 * (League::move_club_to_division 0x6830b0 at 0x5bc8e6), and sends the National rows of status 3 to A Lower Division
 * with the CFA zone (0x6831a0 at 0x5bc92f, +0x60 at 0x5bc957). It moves its count of clubs whatever the list holds
 * (0x5bc8bc..0x5bc8ef), so a list shorter than the count reads past its end. club_get_linked_b_team 0x540a50 links no
 * French club (its nation test 0x540af8..0x540b10 is Spain, Germany and Portugal).
 *
 * The fix. A French reserve side (season_clubs_reserve_name: a last word B, II or 2, a word U19 or U21, and the
 * other reserve forms) is no part of that list: for the call of 0x5bc780 its zone reads as none, then it gets the
 * CFA back. The next club of the list goes up, as the real rule. The name is the only link the data gives, and the
 * real rule bars every reserve side, so the test needs no club. When the clubs left in the list are fewer than the
 * count, the best reserve sides stay in it (logged), so the original never reads past the list (no database holds
 * so few CFA clubs). The club moves of a new game put no French reserve side into the National, Ligue 2 or Ligue 1
 * (season_fra_barred, season_clubs.c).
 *
 * Season games only (season_game_changes): the base game (3.9.68, which has no French reserve side) keeps the
 * original. Test switches: RECOMP_FRA_SECOND=off lists the reserve sides as the original. RECOMP_FRA_SECOND_LOG=1
 * writes each French reserve side with its division and zone, the reserve sides held back and the clubs that go up.
 * RECOMP_TEST_FRA_SECOND_REP=1 (tests only) gives each listed French reserve side the highest reputation before the
 * call, so that the original would take them first.
 *
 * Which reserve sides the list holds: a start of November 2020 (France, season game) gave the CFA zone to the 12
 * reserve sides that the database holds in French CFA (the start moves its clubs to A Lower Division), and none to the
 * 31 of the French Lower Division. So the fault occurs in a normal game when such a side's reputation reaches the top
 * four of the list. Which code writes the zone at the start: not found. */
#include "win.h"
#include "season.h"
#include "season_clubs.h"
#include "season_names.h"
#include "season_fra_second.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_FRANCE 0x9cf300u
#define KEY_LIGUE1 0x9cf5ccu
#define KEY_LIGUE2 0x9cf5d0u
#define KEY_NATIONAL 0x9cf5d4u
#define KEY_CFA 0x9cf5d8u
#define KEY_LOWER 0x9cf764u             /* A Lower Division */
#define CLUBS 0xae23b4u
#define CLUB_COUNT 0xae235cu
#define COMPS 0xae23d0u
#define CLUB_SIZE 0x245u
#define COMP_SIZE 0x6bu
#define CLUB_NAME 0x04u
#define CLUB_NATION 0x53u
#define CLUB_DIVISION 0x57u
#define CLUB_ZONE 0x60u
#define CLUB_REPUTATION 0x80u
#define FRA_YEAR 0x40u                  /* FraFirst +0x40, the season's year (0x5bc8ae) */

static int readable(uint32_t p) { return p >= 0x10000u && p < 0xfff00000u; }

static int env_is(const char *name, const char *value)
{
    const char *v = getenv(name);
    return v && !strcmp(v, value);
}

static int switched_off(void)
{
    static int off = -1;
    if (off < 0) off = env_is("RECOMP_FRA_SECOND", "off");
    return off;
}

static int log_on(void)
{
    static int log = -1;
    if (log < 0) log = env_is("RECOMP_FRA_SECOND_LOG", "1");
    return log;
}

/* 1 when the record p (a club_comp or nation record) has the id of the key. */
static int is_key(uint32_t p, uint32_t key)
{
    return readable(p) && (int32_t)rd32(g_mem, p) == (int32_t)rd32(g_mem, key);
}

static int french_reserve(uint32_t club)
{
    return readable(club) && is_key(rd32(g_mem, club + CLUB_NATION), KEY_FRANCE) &&
           season_clubs_reserve_name(win_str(club + CLUB_NAME));
}

static uint32_t comp_of(uint32_t key)
{
    return rd32(g_mem, COMPS) + rd32(g_mem, key) * COMP_SIZE;
}

int season_fra_barred(uint32_t club, uint32_t div)
{
    if (switched_off() || !readable(div)) return 0;
    if (!is_key(div, KEY_NATIONAL) && !is_key(div, KEY_LIGUE2) && !is_key(div, KEY_LIGUE1)) return 0;
    return french_reserve(club);
}

/* 1 when the club is in the list of 0x5bc780 (0x5bc83f..0x5bc868). */
static int listed(uint32_t club)
{
    uint32_t div = rd32(g_mem, club + CLUB_DIVISION), zone = rd32(g_mem, club + CLUB_ZONE);
    return div && rd32(g_mem, club + CLUB_NATION) && zone && is_key(div, KEY_LOWER) && is_key(zone, KEY_CFA);
}

static int by_reputation(const void *a, const void *b)
{
    int ra = (int16_t)rd16(g_mem, *(const uint32_t *)a + CLUB_REPUTATION);
    int rb = (int16_t)rd16(g_mem, *(const uint32_t *)b + CLUB_REPUTATION);
    return rb - ra;
}

/* FraFirst::exchange_cfa_national_clubs 0x5bc780, thiscall (FraFirst), no arguments. */
void o_005bc780(void);
RT_WRAP(005bc780, fra_second_exchange)
{
    static int test_rep = -1;
    if (test_rep < 0) test_rep = env_is("RECOMP_TEST_FRA_SECOND_REP", "1");
    uint32_t table = rd32(g_mem, CLUBS), self = cpu.ecx;
    int count = (int)rd32(g_mem, CLUB_COUNT);
    if (!season_game_changes() || !readable(table) || count <= 0 || !readable(self)) {
        o_005bc780();
        return;
    }
    uint32_t national = comp_of(KEY_NATIONAL);
    int places = (int16_t)rd16(g_mem, self + FRA_YEAR) == 2001 ? 6 : 4;     /* 0x5bc8ae..0x5bc8b8 */
    uint32_t *held = malloc(sizeof *held * (size_t)count);
    uint32_t *zone_of = malloc(sizeof *zone_of * (size_t)count);
    char *was_national = calloc((size_t)count, 1);
    int nheld = 0, others = 0;
    for (int i = 0; held && zone_of && was_national && i < count; i++) {
        uint32_t club = table + (uint32_t)i * CLUB_SIZE;
        was_national[i] = rd32(g_mem, club + CLUB_DIVISION) == national;
        int reserve = french_reserve(club);
        if (reserve && log_on()) {
            uint32_t d = rd32(g_mem, club + CLUB_DIVISION), z = rd32(g_mem, club + CLUB_ZONE);
            fprintf(stderr, "recomp: fra second: reserve side %.51s in %.51s, zone %.51s\n", win_str(club + CLUB_NAME),
                    readable(d) ? win_str(d + 4) : "none", readable(z) ? win_str(z + 4) : "none");
        }
        if (reserve && test_rep && listed(club)) wr16(g_mem, club + CLUB_REPUTATION, 10000);
        if (!listed(club)) continue;
        if (!reserve) {
            others++;
            continue;
        }
        if (log_on()) fprintf(stderr, "recomp: fra second: listed reserve side %.51s, reputation %d\n",
                              win_str(club + CLUB_NAME), (int16_t)rd16(g_mem, club + CLUB_REPUTATION));
        if (switched_off()) continue;
        held[nheld++] = club;
    }
    /* The list must keep the count of clubs: the best reserve sides stay in it when too few others are left. */
    int keep = places - others;
    if (keep > 0 && nheld) {
        qsort(held, (size_t)nheld, sizeof *held, by_reputation);
        if (keep > nheld) keep = nheld;
        for (int k = 0; k < keep; k++)
            fprintf(stderr, "recomp: fra second: %.51s stays in the list (only %d other CFA clubs for %d places)\n",
                    win_str(held[k] + CLUB_NAME), others, places);
        memmove(held, held + keep, sizeof *held * (size_t)(nheld - keep));
        nheld -= keep;
    }
    for (int k = 0; k < nheld; k++) {
        zone_of[k] = rd32(g_mem, held[k] + CLUB_ZONE);
        wr32(g_mem, held[k] + CLUB_ZONE, 0);
    }
    o_005bc780();
    for (int k = 0; k < nheld; k++) {
        wr32(g_mem, held[k] + CLUB_ZONE, zone_of[k]);
        if (log_on()) fprintf(stderr, "recomp: fra second: %.51s stays in the CFA\n", win_str(held[k] + CLUB_NAME));
    }
    if (log_on() && was_national && !rt_unwinding)
        for (int i = 0; i < count; i++) {
            uint32_t club = table + (uint32_t)i * CLUB_SIZE;
            if (!was_national[i] && rd32(g_mem, club + CLUB_DIVISION) == national)
                fprintf(stderr, "recomp: fra second: %.51s goes up to the National%s\n", win_str(club + CLUB_NAME),
                        french_reserve(club) ? " (a reserve side)" : "");
        }
    free(held);
    free(zone_of);
    free(was_national);
}
