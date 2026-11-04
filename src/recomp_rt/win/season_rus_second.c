/* Russia: second teams ("-2" sides) of Russian clubs (open item 120, 0.7.0).
 *
 * The real rule. A second team plays below its club's first team: it never goes up into the Premier League or its
 * play-off (the next club takes the place), it goes up from the third tier only while its first team plays in the
 * Premier League, and it goes down when its first team comes down into its division, as one of the clubs that go
 * down. Sources (read 3 November 2026, notes in run-logs/0.6.2-s111c-rus/research.md):
 * - FNL Regulation 2015/16 art. 4.3 and 4.4 (RFS executive decision 166/4 of 24 June 2015), FNL Regulations 2021/22
 *   and 2022/23 art. 3.3, 3.3.1 and 3.4 (1fnl.ru PDFs): a second team of an RPL club in places 1 to 6 gets no right
 *   to go up or to the play-off, the clubs below it get it; when its club comes down into the FNL it goes down
 *   whatever its place and counts among the clubs that go down;
 * - First League Regulation 2024/25 art. 3.3.1 and 3.4: the same. The First League Regulations 2023/24, 2025/26 and
 *   2026/27 have no such clause. Second League Division A 2023/24 and 2024/25 art. 3.19 and 3.41, 2025/26 and 2026/27
 *   art. 3.12, 3.14 and 3.30: two teams of one club never play in one competition or go up into a league where
 *   another team of the club plays, the second team of a First League club does not go up while its first team plays
 *   there (the next club does), and a second team counts among the clubs that go down when its first team comes down
 *   into its league. So the rule holds in those seasons too (no second team played the First League in them);
 * - 2001 to 2014/15: no regulation text found. Russian Wikipedia (Система футбольных лиг России, uncited) says that
 *   a second team could go up from the PFL to the FNL only while its first team played in the top flight; none
 *   played the FNL before 2015/16 (Spartak-2 went up from PFL West 2014/15). The same rule is used.
 *
 * The original. The game plays two Russian divisions: the Premier League (RusPrm, key 0x9cf930) and the First
 * Division (RusFirst, key 0x9cf92c). The six zones of the Second Division (keys 0x9cf914..0x9cf928) only hold clubs.
 * RusPrm::start_new_season 0x7eb800 first calls the promotion chain (vtable +0xb0, League::process_promotion_
 * relegation_chain 0x689c20, at 0x7eb813), which moves the rows of status 0 or 5 of the First Division up and the
 * rows of status 3 of the Premier League down (league_promote_and_relegate 0x689c80). Then it calls
 * rus_prm_exchange_first_and_second_division_clubs 0x7ebe20 (at 0x7eb81b). That function lists every club whose
 * division is one of the six zones (0x7ebf87..0x7ebfa8), sorts the list by reputation and shuffles the top 3
 * (comp_util_sort_and_shuffle_top_clubs 0x4afbd0 at 0x7ec009), moves the first club of zones 1 or 2, of zones 3 or 4
 * and of zones 5 or 6 into the First Division (League::move_club_to_division 0x6830b0 at 0x7ec0be), and then sends
 * the clubs of the last three rows of the First Division table (0x6826d0: table +0xb1 plus the row times 0x3b, rows
 * +0x3e - 1, - 2 and - 3) to zones 0x9cf918, 0x9cf91c and 0x9cf924 (League::relegate_club_to_division 0x6831a0 at
 * 0x7ec100, 0x7ec11c, 0x7ec138). It reads only the club of each row (+0), so the rows give the order of last
 * season's table. No step tests a second team: club_get_linked_b_team 0x540a50 holds pairs of Spain, Germany and
 * Portugal only (the nation test at 0x540af6..0x540b12). So a "-2" side with a high reputation goes up from its zone
 * in every season, wins promotion to the Premier League by table, and can play in its own club's division.
 *
 * The link. No field of the club record names the club of a second team. A Russian club is a second team when its
 * name, in plain words (letters and digits, without the words "FK", "FC" and the like), has a word "2", "II" or "M"
 * ("Zenit 2 St. Petersburg", "Arsenal-2 Tula", "CSKA Moscow II", "Fakel-M Voronezh"; not "Krasnodar-2000"), and a
 * Russian club whose plain name is the same without that word exists (its parent). A small table gives the parent of
 * the names that differ from it ("Torpedo 2 Moskau" for Torpedo Moscow). A club with no parent in the data is no
 * second team here. This keeps first teams with such names out of the rule: "Volgar-Gazprom-2 Astrakhan" (the club
 * of Astrakhan in 2009 to 2011) and "FK Orenburg 2" (FK Orenburg itself in the First Division of the March 2018
 * database, the same club id as "FK Orenburg" of 2023), whose plain names have no parent. Evidence: the names of
 * every test database, run-logs/0.6.2-s111c-rus/namecheck.txt.
 *
 * The fix, season games only (season_game_changes):
 * - the zone exchange: a second team that may not play in the First Division is no part of the list of 0x7ebe20
 *   (for the call its division reads as none, then it gets its zone back), so the next club of its zones goes up;
 * - the same call: a second team in the First Division that may not stay there takes, for the call only, the club
 *   of the highest of the last three rows (row +0x3e - 3, then - 2, then - 1) that is no such team, so the original
 *   sends it to that row's zone and the club of the row stays up: it counts among the clubs that go down. More such
 *   teams than three rows: the rest stay one more season (logged);
 * - the First Division table: before each League::update_positions_and_status 0x684cd0 of RusFirst every second
 *   team gets the original's mark 0xfc (st_mark, season_second_teams.c), so the next club takes promotion. A save
 *   made after the table was complete: before the chain the places go on to the next clubs (st_shift_up);
 * - the Premier League (a game of an older build): before the chain each second team gets status 3 in exchange for
 *   the best placed club of status 3, which stays up (the chain moves pairs). It comes down into the First Division
 *   and goes on to a zone as above when it may not stay there either;
 * - the club moves of a new game (season_clubs.c) put no second team into a division where it may not play.
 *
 * Switches. RECOMP_RUS_SECOND=off (test) gives the original. RECOMP_RUS_SECOND_LOG=1 logs the clubs held, moved and
 * marked, and a census of each division before and after each season change. Tests only: RECOMP_TEST_RUS_SECOND_REP=1
 * gives each second team of the zones that may not go up the reputation 10000 and every other one 9000 before
 * 0x7ebe20, so the original takes them first;
 * RECOMP_TEST_RUS_SECOND_PTS=1 gives each second team of the First Division 500 points at its first 0x684cd0 of a
 * season, so the original promotes it; RECOMP_TEST_RUS_SECOND_PARENT_DOWN=1 sends the parent of a second team of the
 * First Division down from the Premier League at the next season change (status 3, in exchange for the best placed
 * club of status 3); RECOMP_TEST_RUS_SECOND_OLD=<n> plays as the original until n Russian season changes are done, as
 * a game of an older build. */
#include "win.h"
#include "season.h"
#include "season_names.h"
#include "season_second_teams.h"
#include "season_rus_second.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_RUSSIA 0x9cf448u
#define KEY_PREMIER 0x9cf930u
#define KEY_FIRST 0x9cf92cu
#define KEY_ZONE1 0x9cf914u             /* the six zones of the Second Division, 0x9cf914..0x9cf928 */
#define ZONES 6
#define ROW_POINTS 0x19u                /* s16: the points of a row in the sort 0x683410 (0x683466, kind +0xad 0) */
#define TAG "rus second"

/* ---- switches ---- */

static int old_left = -1;               /* RECOMP_TEST_RUS_SECOND_OLD: season changes left to play as the original */

static int switched_off(void)
{
    static int off = -1;
    if (off < 0) off = st_env_is("RECOMP_RUS_SECOND", "off");
    if (old_left < 0) {
        const char *v = getenv("RECOMP_TEST_RUS_SECOND_OLD");
        old_left = v ? atoi(v) : 0;
        if (old_left < 0) old_left = 0;
    }
    return off || old_left > 0;
}

static int rule_on(void) { return !switched_off() && season_game_changes(); }

static int log_on(void)
{
    static int log = -1;
    if (log < 0) log = st_env_is("RECOMP_RUS_SECOND_LOG", "1");
    return log;
}

static int test_on(const char *name)
{
    const char *v = getenv(name);
    return v && !strcmp(v, "1");
}

/* ---- names ---- */

static int russian(uint32_t club) { return st_readable(club) && st_nation_is(club, KEY_RUSSIA); }

/* The plain words of a name, lower case, joined by one space (the drop list of season_clubs.c plain_words, copied:
 * that function is static there). *token gets 1 when a word is "2", "ii" or "m", which out leaves out. */
static void plain(const char *in, char *out, size_t room, int *token)
{
    static const char *const drop[] = {"fc", "cf", "sc", "ac", "ec", "ca", "cd", "sd", "ud", "if", "fk", "sk", "bk",
                                       "afc", "club", "clube", "futebol", "football", "ltda", "de", "da", "do", "das",
                                       "dos", "the", "e", "y", "and"};
    char word[64];
    size_t n = 0, w = 0;
    out[0] = 0;
    if (token) *token = 0;
    for (const unsigned char *p = (const unsigned char *)in;; p++) {
        int ch = *p;
        if (ch >= 'A' && ch <= 'Z') ch += 32;
        if (((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) && w < sizeof word - 1) {
            word[w++] = (char)ch;
            continue;
        }
        if (w) {
            word[w] = 0;
            int skip = 0;
            for (size_t k = 0; k < sizeof drop / sizeof drop[0] && !skip; k++) skip = !strcmp(word, drop[k]);
            if (!skip && (!strcmp(word, "2") || !strcmp(word, "ii") || !strcmp(word, "m"))) {
                skip = 1;
                if (token) *token = 1;
            }
            if (!skip && n + w + 2 < room) {
                if (n) out[n++] = ' ';
                memcpy(out + n, word, w);
                n += w;
                out[n] = 0;
            }
            w = 0;
        }
        if (!*p || p - (const unsigned char *)in >= 51) break;
    }
}

/* Second teams whose parent has another name in the data (plain words: the second team, then its parent). */
static const char *const exceptions[][2] = {
    {"torpedo moskau", "torpedo moscow"},         /* Torpedo 2 Moskau (April 2023 to December 2025) */
    {"chertanovo moskau", "chertanovo moscow"},   /* Chertanovo Moskau II, Chertanovo 2 Moskau */
    {"ural ekaterinburg", "ural yekaterinburg"},  /* Ural 2 Ekaterinburg */
    {"akron togliatti", "akron tolyatti"},        /* Akron-2 Togliatti (May 2026) */
};

uint32_t season_rus_parent(uint32_t club)
{
    if (!russian(club)) return 0;
    char want[128], name[128];
    int token;
    plain(win_str(club + ST_CLUB_NAME), want, sizeof want, &token);
    if (!token || !want[0]) return 0;
    for (size_t k = 0; k < sizeof exceptions / sizeof exceptions[0]; k++)
        if (!strcmp(want, exceptions[k][0])) {
            snprintf(want, sizeof want, "%s", exceptions[k][1]);
            break;
        }
    int count;
    uint32_t table = st_clubs(&count);
    for (int i = 0; i < count; i++) {
        uint32_t other = table + (uint32_t)i * ST_CLUB_SIZE;
        if (other == club || !russian(other)) continue;
        plain(win_str(other + ST_CLUB_NAME), name, sizeof name, &token);
        if (!token && !strcmp(name, want)) return other;
    }
    return 0;
}

/* ---- divisions ---- */

/* 0 the Premier League, 1 the First Division, 2 a zone, 3 any other division or none. */
static int level_of(uint32_t div)
{
    if (!st_readable(div)) return 3;
    int32_t id = (int32_t)rd32(g_mem, div);
    if (id == (int32_t)rd32(g_mem, KEY_PREMIER)) return 0;
    if (id == (int32_t)rd32(g_mem, KEY_FIRST)) return 1;
    for (uint32_t k = 0; k < ZONES; k++)
        if (id == (int32_t)rd32(g_mem, KEY_ZONE1 + 4u * k)) return 2;
    return 3;
}

static int level_of_club(uint32_t club) { return level_of(rd32(g_mem, club + ST_CLUB_DIVISION)); }

/* 1 when a second team may play at level: below its parent (level 0 is the top). */
static int may_play(int level, int parent_level) { return level > parent_level; }

/* A second team that may not take promotion from the First Division (every second team: the Premier League is never
 * below its parent). */
static int barred_up(uint32_t club) { return season_rus_parent(club) != 0; }

/* A second team that may not stay in or go up to the First Division. */
static int barred_first(uint32_t club)
{
    uint32_t parent = season_rus_parent(club);
    return parent && !may_play(1, level_of_club(parent));
}

static void census(const char *when)
{
    if (!log_on()) return;
    int count, n[2] = {0, 0}, s[2] = {0, 0};
    uint32_t table = st_clubs(&count);
    for (int i = 0; i < count; i++) {
        uint32_t club = table + (uint32_t)i * ST_CLUB_SIZE;
        int l = level_of_club(club);
        if (l > 1 || !russian(club)) continue;
        n[l]++;
        s[l] += season_rus_parent(club) != 0;
    }
    fprintf(stderr, "recomp: " TAG ": census %s: Premier League %d clubs, %d second teams; First Division %d clubs, "
            "%d second teams\n", when, n[0], s[0], n[1], s[1]);
}

/* ---- the First Division table: before each 0x684cd0 ---- */

static void before_status(uint32_t league)
{
    if (!league || league != st_league_of(KEY_FIRST) || !season_game_changes()) return;
    if (test_on("RECOMP_TEST_RUS_SECOND_PTS")) {
        int n;
        uint32_t table = st_table_of(league, &n);
        for (int i = 0; i < n; i++) {
            uint32_t row = table + (uint32_t)i * ST_ROW_SIZE;
            int pts = (int16_t)rd16(g_mem, row + ROW_POINTS);
            if (barred_up(rd32(g_mem, row)) && pts < 500) wr16(g_mem, row + ROW_POINTS, (uint16_t)(pts + 500));
        }
    }
    if (rule_on()) st_mark(league, barred_up, TAG, "the First Division", log_on());
}

/* ---- the promotion chain of the Premier League: before 0x689c20 ---- */

/* Up to k rows of status 3 that are no second teams, the best placed first, stay up (0xff); skip is left alone.
 * Returns how many. */
static int stay_up(uint32_t league, int k, uint32_t skip)
{
    int n, done = 0;
    uint32_t table = st_table_of(league, &n);
    for (int i = 0; i < n && done < k; i++) {
        uint32_t row = table + (uint32_t)i * ST_ROW_SIZE, club = rd32(g_mem, row);
        if (g_mem[row + ST_ROW_STATUS] != ST_DOWN || club == skip || barred_up(club)) continue;
        g_mem[row + ST_ROW_STATUS] = ST_NONE;
        done++;
        if (log_on()) fprintf(stderr, "recomp: " TAG ": %.51s stays up in the Premier League\n", win_str(club + ST_CLUB_NAME));
    }
    return done;
}

/* RECOMP_TEST_RUS_SECOND_PARENT_DOWN=1: the parent of the first second team of the First Division table that plays
 * in the Premier League goes down, and a club of status 3 stays up for it. */
static void test_parent_down(uint32_t prm)
{
    int n, np;
    uint32_t first = st_table_of(st_league_of(KEY_FIRST), &n), top = st_table_of(prm, &np);
    for (int i = 0; i < n; i++) {
        uint32_t parent = season_rus_parent(rd32(g_mem, first + (uint32_t)i * ST_ROW_SIZE));
        if (!parent || level_of_club(parent) != 0) continue;
        for (int j = 0; j < np; j++) {
            uint32_t row = top + (uint32_t)j * ST_ROW_SIZE;
            if (rd32(g_mem, row) != parent || g_mem[row + ST_ROW_STATUS] == ST_DOWN) continue;
            if (!stay_up(prm, 1, parent)) return;
            g_mem[row + ST_ROW_STATUS] = ST_DOWN;
            fprintf(stderr, "recomp: " TAG ": test: %.51s goes down from the Premier League\n",
                    win_str(parent + ST_CLUB_NAME));
            return;
        }
    }
}

static void before_chain(uint32_t league)
{
    if (!league || league != st_league_of(KEY_PREMIER) || !season_game_changes()) return;
    census("before");
    if (test_on("RECOMP_TEST_RUS_SECOND_PARENT_DOWN")) test_parent_down(league);
    if (!rule_on()) return;
    st_shift_up(st_league_of(KEY_FIRST), barred_up, TAG, "at the season change", log_on());
    /* A second team in the Premier League (a game of an older build) goes down, in exchange for the best placed club
     * of status 3 (the chain moves pairs, 0x689c60); the lowest placed first. */
    int n;
    uint32_t table = st_table_of(league, &n);
    for (int i = n - 1; i >= 0; i--) {
        uint32_t row = table + (uint32_t)i * ST_ROW_SIZE, club = rd32(g_mem, row);
        if (g_mem[row + ST_ROW_STATUS] == ST_DOWN || !barred_up(club)) continue;
        if (!stay_up(league, 1, 0)) {
            if (log_on()) fprintf(stderr, "recomp: " TAG ": %.51s stays in the Premier League one more season (no "
                                  "place goes down for it)\n", win_str(club + ST_CLUB_NAME));
            continue;
        }
        g_mem[row + ST_ROW_STATUS] = ST_DOWN;
        if (log_on()) fprintf(stderr, "recomp: " TAG ": %.51s goes down from the Premier League\n",
                              win_str(club + ST_CLUB_NAME));
    }
}

/* ---- the club moves of a new game ---- */

static int barred_at_start(uint32_t club, uint32_t div)
{
    if (switched_off() || !russian(club)) return 0;
    return level_of(div) == 0 && season_rus_parent(club);
}

static const SecondTeamHooks rus_hooks = {before_status, before_chain, NULL, barred_at_start};
SEASON_SECOND_HOOKS(rus_hooks)

/* ---- the zone exchange 0x7ebe20, thiscall (RusPrm), no arguments ---- */

void o_007ebe20(void);
RT_WRAP(007ebe20, rus_second_exchange)
{
    int count;
    uint32_t table = st_clubs(&count);
    if (!season_game_changes() || !count) {
        o_007ebe20();
        return;
    }
    int on = rule_on(), log = log_on(), rep = test_on("RECOMP_TEST_RUS_SECOND_REP");
    uint32_t first = st_comp_of(KEY_FIRST);
    int *held = malloc(sizeof *held * (size_t)count);
    uint32_t *div_of = malloc(sizeof *div_of * (size_t)count);
    char *was = calloc((size_t)count, 1);       /* 1 the First Division, 2 a zone, before the call */
    int nheld = 0;
    for (int i = 0; held && div_of && was && i < count; i++) {
        uint32_t club = table + (uint32_t)i * ST_CLUB_SIZE, div = rd32(g_mem, club + ST_CLUB_DIVISION);
        int l = level_of(div);
        was[i] = l == 1 ? 1 : l == 2 ? 2 : 0;
        if (l != 2 || !russian(club) || !season_rus_parent(club)) continue;
        int barred = barred_first(club);
        if (rep) wr16(g_mem, club + ST_CLUB_REPUTATION, barred ? 10000 : 9000);
        if (!on || !barred) continue;
        held[nheld] = i;
        div_of[nheld++] = div;
        wr32(g_mem, club + ST_CLUB_DIVISION, 0);
    }
    /* Second teams of the First Division that may not stay: each takes, for the call, the highest of the last three
     * rows whose club may stay. */
    uint32_t rows_at[3] = {0, 0, 0}, rows_was[3] = {0, 0, 0}, swap_at[3] = {0, 0, 0}, swap_was[3] = {0, 0, 0};
    int nrows = 0, n;
    uint32_t ft = st_table_of(st_league_of(KEY_FIRST), &n);
    if (on && ft && n >= 3) {
        for (int i = 0; i < count; i++) {
            uint32_t club = table + (uint32_t)i * ST_CLUB_SIZE;
            if (rd32(g_mem, club + ST_CLUB_DIVISION) != first || !barred_first(club)) continue;
            int at = -1;
            for (int r = n - 3; r < n && at < 0; r++)
                if (rd32(g_mem, ft + (uint32_t)r * ST_ROW_SIZE) == club) at = r;
            if (at >= 0) {
                if (log) fprintf(stderr, "recomp: " TAG ": %.51s goes down from the First Division (row %d)\n",
                                 win_str(club + ST_CLUB_NAME), at + 1);
                continue;
            }
            int slot = -1;
            for (int r = n - 3; r < n && slot < 0; r++) {
                uint32_t c = rd32(g_mem, ft + (uint32_t)r * ST_ROW_SIZE);
                int used = 0;
                for (int k = 0; k < nrows; k++) used |= rows_at[k] == ft + (uint32_t)r * ST_ROW_SIZE;
                if (!used && !barred_first(c)) slot = r;
            }
            if (slot < 0) {
                if (log) fprintf(stderr, "recomp: " TAG ": %.51s stays in the First Division one more season (no "
                                 "place goes down for it)\n", win_str(club + ST_CLUB_NAME));
                continue;
            }
            uint32_t row = ft + (uint32_t)slot * ST_ROW_SIZE, displaced = rd32(g_mem, row);
            rows_at[nrows] = row;
            rows_was[nrows] = displaced;
            /* The team's own row (none when it came down from the Premier League) takes the displaced club. */
            for (int r = 0; r < n; r++)
                if (rd32(g_mem, ft + (uint32_t)r * ST_ROW_SIZE) == club) {
                    swap_at[nrows] = ft + (uint32_t)r * ST_ROW_SIZE;
                    swap_was[nrows] = club;
                    wr32(g_mem, swap_at[nrows], displaced);
                }
            wr32(g_mem, row, club);
            nrows++;
            if (log) fprintf(stderr, "recomp: " TAG ": %.51s goes down from the First Division in place of row %d; "
                             "%.51s stays up\n", win_str(club + ST_CLUB_NAME), slot + 1, win_str(displaced + ST_CLUB_NAME));
        }
    }
    o_007ebe20();
    for (int k = nrows - 1; k >= 0; k--) {
        wr32(g_mem, rows_at[k], rows_was[k]);
        if (swap_at[k]) wr32(g_mem, swap_at[k], swap_was[k]);
    }
    for (int k = 0; k < nheld; k++) {
        uint32_t club = table + (uint32_t)held[k] * ST_CLUB_SIZE;
        wr32(g_mem, club + ST_CLUB_DIVISION, div_of[k]);
        uint32_t parent = season_rus_parent(club);
        if (log) fprintf(stderr, "recomp: " TAG ": %.51s stays in %.51s (its club %.51s plays in %.51s)\n",
                         win_str(club + ST_CLUB_NAME), win_str(div_of[k] + 4), win_str(parent + ST_CLUB_NAME),
                         st_readable(rd32(g_mem, parent + ST_CLUB_DIVISION)) ?
                         win_str(rd32(g_mem, parent + ST_CLUB_DIVISION) + 4) : "no division");
    }
    if (log && was && !rt_unwinding)
        for (int i = 0; i < count; i++) {
            uint32_t club = table + (uint32_t)i * ST_CLUB_SIZE, parent = season_rus_parent(club);
            if (rd32(g_mem, club + ST_CLUB_DIVISION) != first) continue;
            if (was[i] != 1)
                fprintf(stderr, "recomp: " TAG ": %.51s goes up to the First Division%s\n", win_str(club + ST_CLUB_NAME),
                        !parent ? "" : barred_first(club) ? " (a barred second team)" : " (a second team)");
            else if (parent && barred_first(club))
                fprintf(stderr, "recomp: " TAG ": %.51s stays in the First Division (a barred second team)\n",
                        win_str(club + ST_CLUB_NAME));
        }
    free(held);
    free(div_of);
    free(was);
    if (!rt_unwinding) {
        census("after");
        if (old_left > 0) old_left--;
    }
}
