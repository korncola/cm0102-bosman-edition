/* Norway: a second team does not go up into the First Division (open item 111, 0.7.0).
 *
 * The real rule. NFF Kampreglement § 2-7 (1): a club's senior teams play in different divisions; (a) a second team
 * plays at most in the division below the club's higher team, and (b) the second team of a club in Eliteserien or
 * the 1. divisjon goes up no higher than the 2. divisjon. So no second team plays in the top two tiers, which are the
 * two divisions that the game plays. § 5-12 (5): the place of a team that may not go up goes to the best placed team
 * of the same group that may go up. The bar is older than the present text (example: Viking 2
 * won its group of the 2. divisjon in 2005 and FK Haugesund went up). Sources, read on 3 November 2026: the NFF
 * Kampreglement on Lovdata (https://lovdata.no/dokument/NFFF/nfffor/2014-03-09-1, amended to 24 February 2026; the
 * text of § 2-7 dates from 17 January 2023), and https://no.wikipedia.org/wiki/2._divisjon_fotball_for_menn (the
 * 2005 case, and the earlier rule: to 2014 the 2. divisjon team of an Eliteserien club went down to the 3. divisjon
 * when its club went down to the 1. divisjon). The wording of 2001 was not found.
 *
 * The original. The 2. divisjon is not played: its eight groups (keys 0x9cf6c4..0x9cf6e0) only hold clubs. At the
 * start of each season NorPrm::start_new_season 0x7927a0 calls nor_prm_swap_first_and_second_division_clubs
 * 0x792960 (at 0x7927bb). That function lists the Norwegian clubs (club +0x53 the nation [0x9cf408]) whose division
 * (club +0x57) is one of the eight groups (0x792a1b..0x792a7b; a club with no division is not listed, 0x792a1e),
 * sorts the list by reputation and shuffles the top 12 (comp_util_sort_and_shuffle_top_clubs 0x4afbd0 at
 * 0x792a9c), moves the first 4 into the First Division [0x9cf6c0] (League::move_club_to_division 0x6830b0 at
 * 0x792aca), then sends the relegated rows of the First Division (status 3) to group 1 (0x6831a0 at 0x792b1d).
 * The original links no Norwegian second team to its club: club_get_linked_b_team 0x540a50 holds key club pairs
 * of Spain, Germany and Portugal only (the nation test at 0x540af8..0x540b10). So a second team with a high
 * reputation goes up, and in a long game it can meet its own club.
 *
 * The fix. A Norwegian second or third team (season_clubs_reserve_name: a last word 2, II or B and the other
 * reserve forms; or a last word 3 or III) is no part of that list: for the call of 0x792960 its division reads as
 * none, then it gets its group back. This is the original's own rule for Spain (spa_first_exchange_second_division_
 * b_clubs drops a reserve side whose senior side plays in the tier from its list, 0x8504c9..0x8504ec), and the next
 * club of the list takes the place, as § 5-12 (5). The name is the only link the data gives: no field of the club
 * record names its club, and the real rule bars every second team from the top two tiers, so the test needs no
 * club.
 *
 * The club moves of a new game (season_clubs.c) put no Norwegian second or third team into Eliteserien or the First
 * Division either (season_nor_barred), in every fill group and for a club that the database has there.
 *
 * Season games only (season_game_changes, as the other season code): the base game (3.9.68, which has no Norwegian
 * second team) keeps the original. The test switch RECOMP_NOR_SECOND=off lists and moves the second teams as the
 * original does (the log and the test switch below still work). RECOMP_NOR_SECOND_LOG=1 writes the clubs that stay
 * out, with the group they keep, and the clubs that go up. RECOMP_TEST_NOR_SECOND_REP=1 (tests only) gives each
 * second team of the eight groups the highest reputation before the call, so that the original would take them
 * first. */
#include "win.h"
#include "season.h"
#include "season_clubs.h"
#include "season_names.h"
#include "season_nor_second.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_NORWAY 0x9cf408u
#define KEY_PREMIER 0x9cf6bcu           /* Eliteserien */
#define KEY_FIRST 0x9cf6c0u             /* the Norwegian First Division */
#define KEY_GROUP1 0x9cf6c4u            /* the 8 groups of the Second Division, 0x9cf6c4..0x9cf6e0 */
#define GROUPS 8
#define CLUBS 0xae23b4u
#define CLUB_COUNT 0xae235cu
#define COMPS 0xae23d0u
#define CLUB_SIZE 0x245u
#define COMP_SIZE 0x6bu
#define CLUB_NAME 0x04u
#define CLUB_NATION 0x53u
#define CLUB_DIVISION 0x57u
#define CLUB_REPUTATION 0x80u

static int readable(uint32_t p) { return p >= 0x10000u && p < 0xfff00000u; }

static int env_is(const char *name, const char *value)
{
    const char *v = getenv(name);
    return v && !strcmp(v, value);
}

/* 1 when a division record is one of the eight groups. */
static int in_group(uint32_t div)
{
    if (!readable(div)) return 0;
    int32_t id = (int32_t)rd32(g_mem, div);
    for (uint32_t k = 0; k < GROUPS; k++)
        if (id == (int32_t)rd32(g_mem, KEY_GROUP1 + 4u * k)) return 1;
    return 0;
}

static int norwegian(uint32_t club)
{
    uint32_t nation = rd32(g_mem, club + CLUB_NATION);
    return readable(nation) && (int32_t)rd32(g_mem, nation) == (int32_t)rd32(g_mem, KEY_NORWAY);
}

/* 1 for a second or third team: the reserve names of season_clubs_reserve_name, or a last word 3 or III (the data
 * has "Odds BK 3" and "Rosenborg 3"; § 2-7 places every lower team of a club below its higher team). */
static int lower_team(const char *name)
{
    if (season_clubs_reserve_name(name)) return 1;
    char last[8];
    size_t n = 0, w = 0;
    while (n < 51 && name[n]) n++;
    while (n && name[n - 1] == ' ') n--;
    while (n && name[n - 1] != ' ' && w < sizeof last - 1) last[w++] = (char)(name[--n] | 0x20);   /* reversed */
    last[w] = 0;
    return n && (!strcmp(last, "3") || !strcmp(last, "iii"));
}

/* RECOMP_NOR_SECOND=off (test switch): the original's list and club moves. Read once. */
static int switched_off(void)
{
    static int off = -1;
    if (off < 0) off = env_is("RECOMP_NOR_SECOND", "off");
    return off;
}

int season_nor_barred(uint32_t club, uint32_t div)
{
    if (switched_off() || !readable(club) || !readable(div) || !norwegian(club)) return 0;
    int32_t id = (int32_t)rd32(g_mem, div);
    if (id != (int32_t)rd32(g_mem, KEY_PREMIER) && id != (int32_t)rd32(g_mem, KEY_FIRST)) return 0;
    return lower_team(win_str(club + CLUB_NAME));
}

void o_00792960(void);
RT_WRAP(00792960, nor_second_exchange)
{
    static int log = -1, test_rep = -1;
    if (log < 0) {
        log = env_is("RECOMP_NOR_SECOND_LOG", "1");
        test_rep = env_is("RECOMP_TEST_NOR_SECOND_REP", "1");
    }
    uint32_t table = rd32(g_mem, CLUBS);
    int count = (int)rd32(g_mem, CLUB_COUNT);
    if (!season_game_changes() || !readable(table) || count <= 0) {
        o_00792960();
        return;
    }
    uint32_t first = rd32(g_mem, COMPS) + rd32(g_mem, KEY_FIRST) * COMP_SIZE;
    /* The second teams of the groups, with their divisions, and the clubs of the First Division before the call. */
    int *held = malloc(sizeof *held * (size_t)count);
    uint32_t *div_of = malloc(sizeof *div_of * (size_t)count);
    char *was_first = calloc((size_t)count, 1);
    int nheld = 0;
    for (int i = 0; held && div_of && was_first && i < count; i++) {
        uint32_t club = table + (uint32_t)i * CLUB_SIZE, div = rd32(g_mem, club + CLUB_DIVISION);
        was_first[i] = div == first;
        if (!in_group(div) || !norwegian(club) || !lower_team(win_str(club + CLUB_NAME))) continue;
        if (test_rep) wr16(g_mem, club + CLUB_REPUTATION, 10000);
        if (switched_off()) continue;
        held[nheld] = i;
        div_of[nheld++] = div;
        wr32(g_mem, club + CLUB_DIVISION, 0);
    }
    o_00792960();
    for (int k = 0; k < nheld; k++) {
        uint32_t club = table + (uint32_t)held[k] * CLUB_SIZE;
        wr32(g_mem, club + CLUB_DIVISION, div_of[k]);
        if (log) fprintf(stderr, "recomp: nor second: %.51s stays in %.51s\n", win_str(club + CLUB_NAME),
                         win_str(div_of[k] + 4));
    }
    if (log && was_first && !rt_unwinding)
        for (int i = 0; i < count; i++) {
            uint32_t club = table + (uint32_t)i * CLUB_SIZE;
            if (!was_first[i] && rd32(g_mem, club + CLUB_DIVISION) == first)
                fprintf(stderr, "recomp: nor second: %.51s goes up to the First Division%s\n",
                        win_str(club + CLUB_NAME), lower_team(win_str(club + CLUB_NAME)) ? " (a second team)" : "");
        }
    free(held);
    free(div_of);
    free(was_first);
}
