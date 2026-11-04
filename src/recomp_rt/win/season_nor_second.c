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
 * Old saves (Max, 3 November 2026: "do whatever real life does"). A game of 0.6.1 or earlier can hold second teams
 * in the First Division or in Eliteserien. The real rule bars them there, so each goes down at the next season
 * change and the next club stays up, as § 5-12 (5) fills a place. How the original sets the places: League::
 * update_positions_and_status 0x684cd0 (vtable +0x94) sorts the table (0x6827d0 at 0x684d18), gives each position
 * a zone from the places +0xbe..+0xc1 and sets a row's status (+0x37) when the row can no longer leave its zone:
 * 5 the title, 0 promotion, 1 the play-off, 3 relegation. A row of status 0xfc is no part of the zones from the
 * top (0x684dea..0x684df4, 0x684e46..0x684e53) and gets neither the title, promotion nor the play-off (0x68512e..
 * 0x685137, 0x68517f..0x68518b), so the next row takes its place; in the zone of relegation it gets 3 (0x68525e).
 * This is the original's own mark for a B side whose senior side plays above (set at 0x5dd942 for Germany, through
 * club_get_linked_b_team 0x540a50). The fix:
 * - Before each 0x684cd0 of the First Division, a second team gets status 0xfc (unless it has 3), so the next club
 *   takes the title, promotion and the play-off. NorFirst::create_play_off_stage 0x790e20 takes the row of
 *   status 1 (0x790e97); when a save comes from the days after the table was complete, the wrap of 0x790e20 first
 *   hands the places on to the next clubs (shift_up).
 * - At the season change, before the chain of NorPrm (vtable +0xb0, 0x689c20, called at 0x7927b3; it calls
 *   league_promote_and_relegate 0x689c80 in pairs, 0x689c60), each second team of the First Division gets status 3,
 *   and the best placed club of status 3 gets 0xff and stays up. 0x792960 sends every row of status 3 to group 1
 *   (0x792aef..0x792b1d). When the second teams outnumber the clubs that go down, the extra places of the First
 *   Division go to the clubs of the groups with the highest reputation (League::move_club_to_division 0x6830b0, as
 *   0x792aca), after the original's four, but never to a club that the original just sent down.
 * - A second team of Eliteserien gets status 3 in the same way, but only in exchange for a club of status 3 that
 *   stays up: the chain moves a row down only with a partner that goes up (pairwise), so a second team beyond the
 *   places that go down stays one more season (logged). Each one that reaches the First Division so goes on to group
 *   1 in the wrap of 0x792960 (League::relegate_club_to_division 0x6831a0, as 0x792b1d), so before the chain one
 *   more club of status 3 of the First Division stays up for it (or, with none left, one more club of the groups
 *   goes up). Such a club gets two relegation rows in its history (0x4cd030 in 0x689c80 and in 0x6831a0), as it
 *   goes down two divisions. 0x792b40 (at 0x7927c2) then shares the groups out by latitude, as for every club that
 *   goes down.
 * A club that stays up or takes a place keeps the news and the history row of its earlier status (0x684cd0 sends
 * them through vtable +0x40 when it sets a status).
 * Eliteserien gets no 0xfc: its play-off takes the row of status 2 (NorFirst 0x790e20), which a row of 0xfc never
 * gets (0x685274..0x68527f).
 *
 * Season games only (season_game_changes, as the other season code): the base game (3.9.68, which has no Norwegian
 * second team) keeps the original. The test switch RECOMP_NOR_SECOND=off lists and moves the second teams as the
 * original does (the log and the test switch below still work). RECOMP_NOR_SECOND_LOG=1 writes the clubs that stay
 * out, with the group they keep, the clubs that go up and down, and a count of the second teams before and after
 * each season change. RECOMP_TEST_NOR_SECOND_REP=1 (tests only) gives each second team of the eight groups the
 * highest reputation before the call, so that the original would take them first. RECOMP_TEST_NOR_SECOND_OLD=<n>
 * (tests only) plays as the original until n Norwegian season changes are done, as a game of an older build.
 * RECOMP_TEST_NOR_SECOND_NO_MARK=1 (tests only) gives no 0xfc before 0x684cd0, as a save of an older build made
 * after the table was complete, so that the wraps of 0x790e20 and the season change hand the places on. */
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
#define LEAGUES 0xadadfcu               /* the competition objects, by competition id (0x792966) */
#define LEAGUE_CLUBS 0x3eu              /* s16 rows */
#define LEAGUE_TABLE 0xb1u              /* rows of 0x3b bytes: +0 the club, +0x37 the status */
#define ROW_SIZE 0x3bu
#define ROW_STATUS 0x37u
#define ST_TITLE 5
#define ST_UP 0
#define ST_PLAYOFF 1
#define ST_DOWN 3
#define ST_BARRED 0xfc
#define ST_NONE 0xff
#define F_MOVE_UP 0x006830b0u           /* League::move_club_to_division (this unused), (club, comp, 1) ret 0xc */
#define F_MOVE_DOWN 0x006831a0u         /* League::relegate_club_to_division (this unused), (club, comp, 1) ret 0xc */

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

/* RECOMP_TEST_NOR_SECOND_OLD=<n>: the season changes left to play as the original. Read once. */
static int old_left = -1;

/* RECOMP_NOR_SECOND=off (test switch): the original's list and club moves. Read once. */
static int switched_off(void)
{
    static int off = -1;
    if (off < 0) off = env_is("RECOMP_NOR_SECOND", "off");
    if (old_left < 0) {
        const char *v = getenv("RECOMP_TEST_NOR_SECOND_OLD");
        old_left = v ? atoi(v) : 0;
        if (old_left < 0) old_left = 0;
    }
    return off || old_left > 0;
}

static int log_on(void)
{
    static int log = -1;
    if (log < 0) log = env_is("RECOMP_NOR_SECOND_LOG", "1");
    return log;
}

/* The league object of a division key (0 when none). */
static uint32_t league_of(uint32_t key)
{
    uint32_t leagues = rd32(g_mem, LEAGUES);
    int32_t id = (int32_t)rd32(g_mem, key);
    if (!readable(leagues) || id < 0 || id > 0xffff) return 0;
    uint32_t league = rd32(g_mem, leagues + 4u * (uint32_t)id);
    return readable(league) ? league : 0;
}

static int rule_on(void) { return !switched_off() && season_game_changes(); }

/* The table of a league and its row count (0 when none). */
static uint32_t table_of(uint32_t league, int *n)
{
    *n = 0;
    if (!readable(league)) return 0;
    uint32_t table = rd32(g_mem, league + LEAGUE_TABLE);
    int rows = (int16_t)rd16(g_mem, league + LEAGUE_CLUBS);
    if (!readable(table) || rows <= 0 || rows > 64) return 0;
    *n = rows;
    return table;
}

static int barred_club(uint32_t club)
{
    return readable(club) && norwegian(club) && lower_team(win_str(club + CLUB_NAME));
}

static int up_status(int s) { return s == ST_TITLE || s == ST_UP || s == ST_PLAYOFF; }

/* Before 0x684cd0 of the First Division: each second team gets the original's mark 0xfc (none has 3 yet). */
static void mark_first(uint32_t league)
{
    int n;
    uint32_t table = table_of(league, &n);
    for (int i = 0; i < n; i++) {
        uint32_t row = table + (uint32_t)i * ROW_SIZE;
        uint8_t s = g_mem[row + ROW_STATUS];
        if (s == ST_DOWN || s == ST_BARRED || !barred_club(rd32(g_mem, row))) continue;
        g_mem[row + ROW_STATUS] = ST_BARRED;
        if (log_on()) fprintf(stderr, "recomp: nor second: %.51s may not go up from the First Division\n",
                              win_str(rd32(g_mem, row) + CLUB_NAME));
    }
}

/* A complete table where a second team holds the title, promotion or the play-off (a save of an older build): the
 * clubs below take the places in table order, and each second team gets 0xfc. The rows are in table order (the sort
 * 0x6827d0 of the last 0x684cd0). Runs again with no change. */
static void shift_up(uint32_t league, const char *when)
{
    int n;
    uint32_t table = table_of(league, &n);
    int any = 0;
    for (int i = 0; i < n && !any; i++) {
        uint32_t row = table + (uint32_t)i * ROW_SIZE;
        any = up_status(g_mem[row + ROW_STATUS]) && barred_club(rd32(g_mem, row));
    }
    if (!any) return;
    uint8_t places[64];
    int np = 0;
    for (int i = 0; i < n; i++) {
        uint8_t s = g_mem[table + (uint32_t)i * ROW_SIZE + ROW_STATUS];
        if (up_status(s)) places[np++] = s;
    }
    int k = 0;
    for (int i = 0; i < n; i++) {
        uint32_t row = table + (uint32_t)i * ROW_SIZE;
        uint8_t s = g_mem[row + ROW_STATUS];
        if (barred_club(rd32(g_mem, row))) {
            if (s != ST_DOWN) g_mem[row + ROW_STATUS] = ST_BARRED;
            continue;
        }
        if (s == ST_DOWN) continue;
        uint8_t want = k < np ? places[k] : (up_status(s) ? ST_NONE : s);
        k++;
        if (want != s && log_on())
            fprintf(stderr, "recomp: nor second: %.51s takes status %d in place of a second team %s\n",
                    win_str(rd32(g_mem, row) + CLUB_NAME), want == ST_NONE ? -1 : want, when);
        g_mem[row + ROW_STATUS] = want;
    }
}

/* Up to k clubs of status 3 (not second teams), the best placed first, stay up (0xff). Returns how many. */
static int stay_up(uint32_t league, int k, const char *what)
{
    int n, done = 0;
    uint32_t table = table_of(league, &n);
    for (int i = 0; i < n && done < k; i++) {
        uint32_t row = table + (uint32_t)i * ROW_SIZE;
        if (g_mem[row + ROW_STATUS] != ST_DOWN || barred_club(rd32(g_mem, row))) continue;
        g_mem[row + ROW_STATUS] = ST_NONE;
        done++;
        if (log_on()) fprintf(stderr, "recomp: nor second: %.51s stays up in %s\n", win_str(rd32(g_mem, row) + CLUB_NAME),
                              what);
    }
    return done;
}

/* At the season change: each second team of the league goes down (status 3), and the best placed club of status 3
 * stays up (0xff) in its place. limit 1 (Eliteserien): a second team goes down only in exchange for such a club.
 * *down gets the second teams that go down. Returns those with no club staying up in exchange. */
static int season_down(uint32_t league, int limit, const char *what, int *down)
{
    int n;
    uint32_t table = table_of(league, &n);
    int stay[64], nstay = 0;
    for (int i = 0; i < n; i++) {
        uint32_t row = table + (uint32_t)i * ROW_SIZE;
        if (g_mem[row + ROW_STATUS] == ST_DOWN && !barred_club(rd32(g_mem, row))) stay[nstay++] = i;
    }
    int used = 0, extra = 0, k = 0;
    for (int i = n - 1; i >= 0; i--) {         /* the lowest placed second team first */
        uint32_t row = table + (uint32_t)i * ROW_SIZE, club = rd32(g_mem, row);
        if (g_mem[row + ROW_STATUS] == ST_DOWN || !barred_club(club)) continue;
        if (limit && used == nstay) {
            if (log_on()) fprintf(stderr, "recomp: nor second: %.51s stays in %s one more season (no place goes "
                                  "down for it)\n", win_str(club + CLUB_NAME), what);
            continue;
        }
        g_mem[row + ROW_STATUS] = ST_DOWN;
        k++;
        if (log_on()) fprintf(stderr, "recomp: nor second: %.51s goes down from %s\n", win_str(club + CLUB_NAME), what);
        if (used < nstay) {
            uint32_t up = table + (uint32_t)stay[used++] * ROW_SIZE;
            g_mem[up + ROW_STATUS] = ST_NONE;
            if (log_on()) fprintf(stderr, "recomp: nor second: %.51s stays up in %s\n",
                                  win_str(rd32(g_mem, up) + CLUB_NAME), what);
        } else {
            extra++;
        }
    }
    *down = k;
    return extra;
}

/* The second teams of a division, and its clubs. */
static void census(uint32_t div, int *clubs, int *seconds)
{
    *clubs = *seconds = 0;
    uint32_t table = rd32(g_mem, CLUBS);
    int count = (int)rd32(g_mem, CLUB_COUNT);
    for (int i = 0; readable(table) && readable(div) && i < count; i++) {
        uint32_t club = table + (uint32_t)i * CLUB_SIZE;
        if (rd32(g_mem, club + CLUB_DIVISION) != div) continue;
        ++*clubs;
        *seconds += barred_club(club);
    }
}

static uint32_t comp_of(uint32_t key)
{
    return rd32(g_mem, COMPS) + rd32(g_mem, key) * COMP_SIZE;
}

static void log_census(const char *when)
{
    if (!log_on()) return;
    int c1, s1, c2, s2;
    census(comp_of(KEY_PREMIER), &c1, &s1);
    census(comp_of(KEY_FIRST), &c2, &s2);
    fprintf(stderr, "recomp: nor second: census %s: Eliteserien %d clubs, %d second teams; First Division %d clubs, "
            "%d second teams\n", when, c1, s1, c2, s2);
}

static uint32_t call_this(uint32_t fn, uint32_t self, int nargs, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, nargs, args);
    if (!rt_unwinding) cpu = saved;
    return r;
}

/* The places of the First Division that the clubs of the groups take beyond the original's four (more second teams
 * than clubs of status 3), and the Eliteserien second teams that the chain sends down to it. */
static int first_extra, prm_down;

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
    int on = rule_on();
    uint32_t self = cpu.ecx, first = comp_of(KEY_FIRST);
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
    if (on && !rt_unwinding) {
        uint32_t group1 = comp_of(KEY_GROUP1);
        /* A second team in the First Division now came down from Eliteserien: it goes on to group 1. Its place went
         * to a club of status 3 that stayed up before the chain (wrap of 0x689c20). */
        int arrived = 0;
        for (int i = 0; i < count && !rt_unwinding; i++) {
            uint32_t club = table + (uint32_t)i * CLUB_SIZE;
            if (rd32(g_mem, club + CLUB_DIVISION) != first || !barred_club(club)) continue;
            uint32_t a[3] = {club, group1, 1};
            call_this(F_MOVE_DOWN, self, 3, a);
            arrived++;
            if (log) fprintf(stderr, "recomp: nor second: %.51s goes down from the First Division\n",
                             win_str(club + CLUB_NAME));
        }
        if (arrived > prm_down) first_extra += arrived - prm_down;
        /* Places left over go to the clubs of the groups with the highest reputation. The original sorts its list
         * by reputation and shuffles the top 12 (0x4afbd0 at 0x792a9c), this takes no random number. A club that
         * the original just sent down from the First Division is not one of them. */
        for (; first_extra > 0 && !rt_unwinding; first_extra--) {
            int best = -1, best_rep = -1;
            for (int i = 0; i < count; i++) {
                uint32_t club = table + (uint32_t)i * CLUB_SIZE;
                if ((was_first && was_first[i]) || !in_group(rd32(g_mem, club + CLUB_DIVISION)) || !norwegian(club) ||
                    barred_club(club))
                    continue;
                int rep = (int16_t)rd16(g_mem, club + CLUB_REPUTATION);
                if (rep > best_rep) best = i, best_rep = rep;
            }
            if (best < 0) break;
            uint32_t a[3] = {table + (uint32_t)best * CLUB_SIZE, first, 1};
            call_this(F_MOVE_UP, self, 3, a);
        }
        first_extra = prm_down = 0;
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
    if (!rt_unwinding) {
        log_census("after");
        if (old_left > 0) old_left--;
    }
}

/* League::update_positions_and_status 0x684cd0 (vtable +0x94), thiscall, no arguments. */
void o_00684cd0(void);
RT_WRAP(00684cd0, nor_second_status)
{
    static int no_mark = -1;
    if (no_mark < 0) no_mark = env_is("RECOMP_TEST_NOR_SECOND_NO_MARK", "1");
    if (cpu.ecx && cpu.ecx == league_of(KEY_FIRST) && !no_mark && rule_on()) mark_first(cpu.ecx);
    o_00684cd0();
}

/* NorFirst::create_play_off_stage 0x790e20, thiscall, no arguments. */
void o_00790e20(void);
RT_WRAP(00790e20, nor_second_playoff)
{
    if (cpu.ecx == league_of(KEY_FIRST) && rule_on()) shift_up(cpu.ecx, "before the play-off");
    o_00790e20();
}

/* The promotion chain 0x689c20 (vtable +0xb0), thiscall (pairwise) ret 4: NorPrm::start_new_season calls it first
 * (0x7927b3). For Eliteserien the second teams go down before it. */
void o_00689c20(void);
RT_WRAP(00689c20, nor_second_chain)
{
    uint32_t self = cpu.ecx;
    if (!readable(self) || self != league_of(KEY_PREMIER) || !season_game_changes()) {
        o_00689c20();
        return;
    }
    log_census("before");
    first_extra = prm_down = 0;
    uint32_t prm = comp_of(KEY_PREMIER), table = rd32(g_mem, CLUBS);
    int count = readable(table) ? (int)rd32(g_mem, CLUB_COUNT) : 0;
    char *was_prm = log_on() && count > 0 ? calloc((size_t)count, 1) : NULL;
    for (int i = 0; was_prm && i < count; i++)
        was_prm[i] = rd32(g_mem, table + (uint32_t)i * CLUB_SIZE + CLUB_DIVISION) == prm;
    if (rule_on()) {
        uint32_t first = league_of(KEY_FIRST);
        shift_up(first, "at the season change");
        int down;
        first_extra = season_down(first, 0, "the First Division", &down);
        season_down(self, 1, "Eliteserien", &prm_down);
        /* Each Eliteserien second team that goes down leaves the First Division again (wrap of 0x792960), so one
         * more club of status 3 stays up there. */
        first_extra += prm_down - stay_up(first, prm_down, "the First Division");
    }
    o_00689c20();
    for (int i = 0; was_prm && !rt_unwinding && i < count; i++) {
        uint32_t club = table + (uint32_t)i * CLUB_SIZE;
        if (!was_prm[i] && rd32(g_mem, club + CLUB_DIVISION) == prm)
            fprintf(stderr, "recomp: nor second: %.51s goes up to Eliteserien%s\n", win_str(club + CLUB_NAME),
                    barred_club(club) ? " (a second team)" : "");
    }
    free(was_prm);
}
