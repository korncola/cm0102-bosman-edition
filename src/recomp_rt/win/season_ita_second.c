/* Italy: a second team never plays in its first team's division or above (open item 117, 0.7.0).
 *
 * The real rule. From 2018/19 only a Serie A club may have a second team, and it plays Serie C. The FIGC's yearly
 * Comunicato Ufficiale "Seconde Squadre: disposizioni partecipazione al Campionato Serie C", Part I, says in every
 * text read: the second team may be promoted to Serie B, "ma non potrà mai partecipare al medesimo Campionato della
 * prima squadra, né ad un Campionato superiore"; and when the first and the second team end a season in the same
 * division, the second team plays the professional division below. Relegation to Serie D: up to CU 227/A (2024/25)
 * a relegated second team may not enter Serie D (it may only fill a vacant Serie C place); from CU 304/A (2025/26)
 * it may (Milan Futuro, relegated in May 2025, played Serie D girone B in 2025/26). Sources, read 3 November 2026:
 * FIGC CU 227/A (2024/25), 304/A (2025/26), 222/A (2026/27), calcioefinanza.it of 11 May 2018 (the first text) and
 * of 18 May 2025; list and links in run-logs/0.6.2-s111b/research/ITA.md. The texts do not say who takes the place
 * of a second team that may not go up: Max, 3 November 2026, the next club in the table takes it. No text says where
 * a second team goes when its first team comes down into Serie C (no professional division is below): this code
 * sends it to the Serie D, the division below (a gap, recorded under item 117).
 *
 * The game. In the Serie C era (2014/15 on) the chain is our own code (season_divisions_ita.c, chain): the final
 * places of each table, league_promote_and_relegate 0x689c80 for Serie A and B (rows of status 3 of the upper league
 * go down, rows of status 0 or 5 of the lower league go up, in pairs: 0x689d26, 0x689d47..0x689d4e), choose_promoted
 * for the groups, 0x689c80 for Serie B and each group, the relegated rows of the groups to the Serie D data division,
 * then the regroup. League::get_table_row 0x6826d0 is table (+0xb1) + 0x3b x row, row i is place i + 1 after the
 * final sort, and the status is row +0x37. No step tests a second team, and no club field names a club's first team
 * (club_get_linked_b_team 0x540a50 holds Spanish, German and Portuguese pairs only, 0x540af8..0x540b10).
 *
 * The link. A pair table of exact names (no fuzzy match), as the databases name the clubs (March 2020 to May 2026):
 * Juventus U23 and Juventus Next Gen to Juventus FC (or Juventus), Atalanta U23 to Atalanta BC (or Atalanta), Milan
 * Futuro to AC Milan (or Milan), Inter U23 to Internazionale (or Inter). Both clubs Italian.
 *
 * The fix (season games only, season_game_changes, as the other season code; with no second team nothing changes):
 * - Serie B: a second team never goes up to Serie A. Its row of status 0 or 5 takes 0xff (the status of a row that
 *   does not move, 0x689df4) and the best placed row of Serie B that does not move takes its status.
 * - The groups: a second team whose first team is not in Serie A after the exchange of Serie A and B does not go up.
 *   The same hand-on in its own group, and choose_promoted passes over it.
 * - Both in one division: a second team in Serie B whose first team plays Serie B or lower next season gets status
 *   3, and the best placed relegated club of Serie B stays up (0xff), so the pairs of 0x689c80 still match. A second
 *   team in Serie A (an old save) goes down the same way. After the chain, a second team still in its first team's
 *   division or above goes down one division at a time (League::relegate_club_to_division 0x6831a0), and the best
 *   placed club that went down from the division it leaves comes back (League::move_club_to_division 0x6830b0). From
 *   Serie C it goes to the Serie D.
 * - The Serie D by season: when the season that starts is before 2025/26, a second team in the Serie D data division
 *   leaves it (no division), so only the regroup's last pool (clubs with no division, by reputation, when the Serie D
 *   is short) can take it back, as the vacancy rule. From 2025/26 it stays in the Serie D.
 * - The regroup's pools pass over a second team whose first team plays Serie C or lower.
 * - The club moves of a new game (season_clubs.c) put no second team into Serie A or Serie B.
 *
 * Test switches. RECOMP_ITA_SECOND=off: the original's moves (the census log still runs). RECOMP_ITA_SECOND_LOG=1:
 * the census and every move. RECOMP_TEST_ITA_SECOND_TOP=1 (tests only): before the exchange, the first second team
 * of Serie B and of each group changes places with the club of row 0 (the first place, with its points and status;
 * the rows are in table order after the final sort), so it ends first. RECOMP_TEST_ITA_SECOND_BOTTOM=1 (tests only):
 * the first second team of each group changes places with the club of the last row (relegated).
 * RECOMP_TEST_ITA_SECOND_DOWN=<first team>[,<first team>] (tests only): that Serie A club swaps its status with the
 * best placed relegated row, so it goes down. RECOMP_TEST_ITA_SECOND_OLD=<n> (tests only): the original's moves for
 * the first n Italian season changes, as a game of an older build. */
#include "win.h"
#include "season.h"
#include "season_names.h"
#include "season_ita_second.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_ITALY 0x9cf360u
#define KEY_A 0x9cf570u
#define KEY_B 0x9cf574u
#define KEY_C1A 0x9cf578u
#define KEY_SERIE_D 0x9cf58cu
static const uint32_t C_KEYS[5] = {0x9cf578u, 0x9cf57cu, 0x9cf580u, 0x9cf584u, 0x9cf588u};
#define CLUBS 0xae23b4u
#define CLUB_COUNT 0xae235cu
#define CLUB_SIZE 0x245u
#define CLUB_NAME 0x04u
#define CLUB_NATION 0x53u
#define CLUB_DIVISION 0x57u
#define COMP_OBJECTS 0xadadfcu
#define COMP_RECORDS 0xae23d0u
#define COMP_RECORD_SIZE 0x6bu
#define LEAGUE_CLUBS 0x3eu
#define LEAGUE_TABLE 0xb1u
#define ROW_SIZE 0x3bu
#define ROW_STATUS 0x37u
#define ST_TITLE 5
#define ST_UP 0
#define ST_DOWN 3
#define ST_NONE 0xff
#define F_RELEGATE_TO 0x006831a0u       /* thiscall (club, record, 1) ret 0xc */
#define F_PROMOTE_TO 0x006830b0u        /* thiscall (club, record, 1) ret 0xc */
#define TIER_A 0
#define TIER_B 1
#define TIER_C 2
#define TIER_D 3
#define TIER_NONE 4

static const char *const TIER_NAME[5] = {"Serie A", "Serie B", "Serie C", "the Serie D", "no division"};

static const struct { const char *second, *first[2]; } PAIRS[] = {
    {"Juventus U23", {"Juventus FC", "Juventus"}},
    {"Juventus Next Gen", {"Juventus FC", "Juventus"}},
    {"Atalanta U23", {"Atalanta BC", "Atalanta"}},
    {"Milan Futuro", {"AC Milan", "Milan"}},
    {"Inter U23", {"Internazionale", "Inter"}},
};
#define NPAIRS (int)(sizeof PAIRS / sizeof PAIRS[0])

static int readable(uint32_t p) { return p >= 0x10000u && p < 0xfff00000u; }

static int env_is(const char *name, const char *value)
{
    const char *v = getenv(name);
    return v && !strcmp(v, value);
}

static int old_left = -1;

static int switched_off(void)
{
    static int off = -1;
    if (off < 0) off = env_is("RECOMP_ITA_SECOND", "off");
    if (old_left < 0) {
        const char *v = getenv("RECOMP_TEST_ITA_SECOND_OLD");
        old_left = v ? atoi(v) : 0;
        if (old_left < 0) old_left = 0;
    }
    return off || old_left > 0;
}

static int rule_on(void) { return !switched_off() && season_game_changes(); }

static int log_on(void)
{
    static int log = -1;
    if (log < 0) log = env_is("RECOMP_ITA_SECOND_LOG", "1");
    return log;
}

/* The club's name, at most 51 characters. */
static void name_of(uint32_t club, char *out)
{
    snprintf(out, 52, "%.51s", win_str(club + CLUB_NAME));
}

static int italian(uint32_t club)
{
    uint32_t nation = rd32(g_mem, club + CLUB_NATION);
    return readable(nation) && rd32(g_mem, nation) == rd32(g_mem, KEY_ITALY);
}

static uint32_t record_of(uint32_t key)
{
    int32_t id = (int32_t)rd32(g_mem, key);
    uint32_t recs = rd32(g_mem, COMP_RECORDS);
    return id >= 0 && readable(recs) ? recs + COMP_RECORD_SIZE * (uint32_t)id : 0;
}

static int tier_of(uint32_t rec)
{
    if (!readable(rec)) return TIER_NONE;
    if (rec == record_of(KEY_A)) return TIER_A;
    if (rec == record_of(KEY_B)) return TIER_B;
    for (int i = 0; i < 5; i++)
        if (rec == record_of(C_KEYS[i])) return TIER_C;
    if (rec == record_of(KEY_SERIE_D)) return TIER_D;
    return TIER_NONE;
}

static int club_tier(uint32_t club) { return tier_of(rd32(g_mem, club + CLUB_DIVISION)); }

/* The pair of an Italian second team (-1 when the club is none). */
static int pair_of(uint32_t club)
{
    if (!readable(club) || !italian(club)) return -1;
    char name[52];
    name_of(club, name);
    for (int i = 0; i < NPAIRS; i++)
        if (!strcmp(name, PAIRS[i].second)) return i;
    return -1;
}

/* The first team of a second team (0 when none): the first name of the pair that an Italian club has, else the
 * second. */
static uint32_t first_of(uint32_t club)
{
    int p = pair_of(club);
    if (p < 0) return 0;
    uint32_t table = rd32(g_mem, CLUBS), found[2] = {0, 0};
    int count = (int)rd32(g_mem, CLUB_COUNT);
    for (int i = 0; readable(table) && i < count; i++) {
        uint32_t c = table + (uint32_t)i * CLUB_SIZE;
        char name[52];
        name_of(c, name);
        for (int k = 0; k < 2; k++)
            if (!found[k] && !strcmp(name, PAIRS[p].first[k]) && italian(c)) found[k] = c;
        if (found[0]) break;
    }
    return found[0] ? found[0] : found[1];
}

static int second_team(uint32_t club) { return pair_of(club) >= 0 && first_of(club); }

static uint32_t call_this(uint32_t fn, uint32_t self, int nargs, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, nargs, args);
    if (!rt_unwinding) cpu = saved;
    return r;
}

/* The rows of a league object (0 when none). */
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

static uint32_t row_at(uint32_t table, int i) { return table + (uint32_t)i * ROW_SIZE; }
static uint8_t status_at(uint32_t table, int i) { return g_mem[row_at(table, i) + ROW_STATUS]; }
static uint32_t club_at(uint32_t table, int i) { return rd32(g_mem, row_at(table, i)); }
static int moves_up(uint8_t s) { return s == ST_UP || s == ST_TITLE; }

static void say(const char *fmt, uint32_t club, const char *more)
{
    if (!log_on()) return;
    char name[52];
    name_of(club, name);
    fprintf(stderr, "recomp: ita second: ");
    fprintf(stderr, fmt, name, more);
    fputc('\n', stderr);
}

/* The census: each second team and its first team, by division. */
static void census(const char *when)
{
    if (!log_on()) return;
    uint32_t table = rd32(g_mem, CLUBS);
    int count = (int)rd32(g_mem, CLUB_COUNT);
    for (int i = 0; readable(table) && i < count; i++) {
        uint32_t c = table + (uint32_t)i * CLUB_SIZE, f = pair_of(c) >= 0 ? first_of(c) : 0;
        if (!f) continue;
        char a[52], b[52];
        name_of(c, a);
        name_of(f, b);
        fprintf(stderr, "recomp: ita second: census %s: %s in %s, first team %s in %s\n", when, a,
                TIER_NAME[club_tier(c)], b, TIER_NAME[club_tier(f)]);
    }
}

/* Row i of the table (a second team of status 0 or 5) does not go up: it takes 0xff, and the best placed row that
 * does not move (not 0, 5 or 3, and no second team) takes its status (Max: the next club in the table). */
static void hand_on(uint32_t table, int n, int i, const char *what)
{
    uint8_t s = status_at(table, i);
    g_mem[row_at(table, i) + ROW_STATUS] = ST_NONE;
    say("%s may not go up from %s", club_at(table, i), what);
    for (int j = 0; j < n; j++) {
        uint8_t t = status_at(table, j);
        if (j == i || moves_up(t) || t == ST_DOWN || second_team(club_at(table, j))) continue;
        g_mem[row_at(table, j) + ROW_STATUS] = s;
        say("%s takes the place in %s", club_at(table, j), what);
        return;
    }
    say("%s: no club takes its place in %s", club_at(table, i), what);
}

/* Row i (a second team) goes down: status 3, and the best placed relegated row that is no second team stays up. */
static int goes_down(uint32_t table, int n, int i, const char *what)
{
    for (int j = 0; j < n; j++) {
        if (status_at(table, j) != ST_DOWN || second_team(club_at(table, j))) continue;
        g_mem[row_at(table, j) + ROW_STATUS] = ST_NONE;
        g_mem[row_at(table, i) + ROW_STATUS] = ST_DOWN;
        say("%s goes down from %s", club_at(table, i), what);
        say("%s stays up in %s", club_at(table, j), what);
        return 1;
    }
    return 0;
}

/* The tier of a first team next season, by its row in Serie A or B (a, b the league objects). */
static int next_tier(uint32_t first, uint32_t a, uint32_t b)
{
    int t = club_tier(first);
    uint32_t league = t == TIER_A ? a : t == TIER_B ? b : 0;
    int n;
    uint32_t table = table_of(league, &n);
    for (int i = 0; i < n; i++) {
        if (club_at(table, i) != first) continue;
        uint8_t s = status_at(table, i);
        if (s == ST_DOWN) return t + 1;
        if (t == TIER_B && moves_up(s)) return TIER_A;
        break;
    }
    return t;
}

/* The clubs of two rows change places (row +0); each row keeps its points and status. */
static void swap_rows(uint32_t table, int i, int j)
{
    uint32_t c = club_at(table, i);
    wr32(g_mem, row_at(table, i), club_at(table, j));
    wr32(g_mem, row_at(table, j), c);
}

/* The test switches (tests only), before the exchange of Serie A and B. */
static void test_setup(uint32_t a, uint32_t b, const uint32_t *g, int groups)
{
    static int top = -1, bottom = -1;
    if (top < 0) {
        top = env_is("RECOMP_TEST_ITA_SECOND_TOP", "1");
        bottom = env_is("RECOMP_TEST_ITA_SECOND_BOTTOM", "1");
    }
    int n;
    uint32_t table;
    for (int k = 0; bottom && k < groups; k++) {
        table = table_of(g[k], &n);
        for (int i = 0; i < n - 1; i++)
            if (pair_of(club_at(table, i)) >= 0) {
                say("test: %s ends last%s", club_at(table, i), "");
                swap_rows(table, n - 1, i);
                break;
            }
    }
    for (int k = -1; top && k < groups; k++) {
        table = table_of(k < 0 ? b : g[k], &n);
        for (int i = 0; log_on() && i < n && i < 6; i++) {
            char st[16];
            snprintf(st, sizeof st, " %d", status_at(table, i));
            say("test: row %s, status%s", club_at(table, i), st);
        }
        for (int i = 1; i < n; i++)
            if (pair_of(club_at(table, i)) >= 0) {
                say("test: %s ends first%s", club_at(table, i), "");
                swap_rows(table, 0, i);
                break;
            }
    }
    const char *down = getenv("RECOMP_TEST_ITA_SECOND_DOWN");
    table = down ? table_of(a, &n) : 0;
    for (int i = 0; table && i < n; i++) {
        char name[52];
        name_of(club_at(table, i), name);
        const char *p = strstr(down, name);
        size_t len = strlen(name);
        if (!len || !p || (p != down && p[-1] != ',') || (p[len] && p[len] != ',') || status_at(table, i) == ST_DOWN)
            continue;
        for (int j = 0; j < n; j++) {
            char other[52];
            name_of(club_at(table, j), other);
            const char *q = strstr(down, other);
            if (status_at(table, j) != ST_DOWN || (other[0] && q)) continue;     /* not a club of the switch */
            g_mem[row_at(table, j) + ROW_STATUS] = status_at(table, i);
            g_mem[row_at(table, i) + ROW_STATUS] = ST_DOWN;
            say("test: %s goes down from Serie A%s", club_at(table, i), "");
            break;
        }
    }
}

void season_ita_second_before_ab(uint32_t a, uint32_t b, const uint32_t *g, int groups)
{
    if (!season_game_changes()) return;
    switched_off();                     /* reads the switches */
    census("before");
    test_setup(a, b, g, groups);
    if (!rule_on()) return;
    int n;
    uint32_t table = table_of(a, &n);
    /* A second team in Serie A (an old save) goes down. */
    for (int i = n - 1; i >= 0; i--)
        if (status_at(table, i) != ST_DOWN && second_team(club_at(table, i)) && !goes_down(table, n, i, "Serie A"))
            say("%s: no relegated club of %s to stay up (left to the season change)", club_at(table, i), "Serie A");
    table = table_of(b, &n);
    /* Serie B: a second team never goes up to Serie A. */
    for (int i = 0; i < n; i++)
        if (moves_up(status_at(table, i)) && second_team(club_at(table, i))) hand_on(table, n, i, "Serie B");
    /* Serie B: a second team whose first team plays Serie B or lower next season goes down. */
    for (int i = n - 1; i >= 0; i--) {
        uint32_t club = club_at(table, i);
        if (status_at(table, i) == ST_DOWN || !second_team(club)) continue;
        if (next_tier(first_of(club), a, b) >= TIER_B && !goes_down(table, n, i, "Serie B"))
            say("%s: no relegated club of %s to stay up (left to the season change)", club, "Serie B");
    }
}

int season_ita_second_up_barred(uint32_t club)
{
    if (!rule_on() || !second_team(club)) return 0;
    return club_tier(first_of(club)) != TIER_A;
}

void season_ita_second_before_promoted(const uint32_t *g, int groups)
{
    if (!rule_on()) return;
    for (int k = 0; k < groups; k++) {
        int n;
        uint32_t table = table_of(g[k], &n);
        for (int i = 0; i < n; i++)
            if (moves_up(status_at(table, i)) && season_ita_second_up_barred(club_at(table, i)))
                hand_on(table, n, i, "Serie C");
    }
}

/* The best placed club of league (a, b) that went down this change and now plays tier: it comes back up. */
static uint32_t comes_back(uint32_t league, int tier)
{
    int n;
    uint32_t table = table_of(league, &n);
    for (int i = 0; i < n; i++) {
        uint32_t club = club_at(table, i);
        if (status_at(table, i) == ST_DOWN && readable(club) && club_tier(club) == tier && !second_team(club))
            return club;
    }
    return 0;
}

static uint32_t object_of(uint32_t key)
{
    int32_t id = (int32_t)rd32(g_mem, key);
    uint32_t table = rd32(g_mem, COMP_OBJECTS);
    if (id < 0 || !readable(table)) return 0;
    uint32_t obj = rd32(g_mem, table + 4u * (uint32_t)id);
    return readable(obj) ? obj : 0;
}

void season_ita_second_settle(uint32_t serie_a, int season, void (*down)(uint32_t serie_a, uint32_t club, uint32_t d))
{
    if (!season_game_changes()) return;
    uint32_t table = rd32(g_mem, CLUBS), serie_d = record_of(KEY_SERIE_D);
    int count = (int)rd32(g_mem, CLUB_COUNT);
    if (rule_on())
        for (int i = 0; readable(table) && i < count && !rt_unwinding; i++) {
            uint32_t club = table + (uint32_t)i * CLUB_SIZE, first = pair_of(club) >= 0 ? first_of(club) : 0;
            if (!first) continue;
            int t1 = club_tier(first), t2 = club_tier(club);
            while (t2 <= t1 && t2 <= TIER_C && !rt_unwinding) {
                if (t2 == TIER_C) {
                    down(serie_a, club, serie_d);
                    say("%s goes to the Serie D (its first team plays %s)", club, TIER_NAME[t1]);
                    break;
                }
                uint32_t to = record_of(t2 == TIER_A ? KEY_B : KEY_C1A);
                uint32_t back = comes_back(object_of(t2 == TIER_A ? KEY_A : KEY_B), t2 + 1);
                uint32_t r[3] = {club, to, 1};
                call_this(F_RELEGATE_TO, serie_a, 3, r);
                say("%s goes down from %s at the season change", club, TIER_NAME[t2]);
                if (back && !rt_unwinding) {
                    uint32_t p[3] = {back, record_of(t2 == TIER_A ? KEY_A : KEY_B), 1};
                    call_this(F_PROMOTE_TO, serie_a, 3, p);
                    say("%s comes back to %s", back, TIER_NAME[t2]);
                }
                t2++;
            }
        }
    /* The Serie D by season: before 2025/26 no second team plays it. */
    if (rule_on() && season && season < 2025)
        for (int i = 0; readable(table) && i < count && !rt_unwinding; i++) {
            uint32_t club = table + (uint32_t)i * CLUB_SIZE;
            if (rd32(g_mem, club + CLUB_DIVISION) != serie_d || !second_team(club)) continue;
            wr32(g_mem, club + CLUB_DIVISION, 0);
            say("%s leaves the Serie D (no second team plays it before 2025/26)%s", club, "");
        }
    if (rt_unwinding) return;
    census("after");
    if (old_left > 0) old_left--;
}

int season_ita_second_pool_skip(uint32_t club)
{
    if (!rule_on() || !second_team(club)) return 0;
    return club_tier(first_of(club)) >= TIER_C;
}

int season_ita_barred(uint32_t club, uint32_t div)
{
    if (switched_off() || !readable(club) || !readable(div) || pair_of(club) < 0) return 0;
    int t = tier_of(div);
    return t == TIER_A || t == TIER_B;
}
