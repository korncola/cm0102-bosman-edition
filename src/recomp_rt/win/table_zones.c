/* The option table_zones: the place boxes of a league table in the colour of what the place leads to, in place of
 * the original's dashed lines.
 *
 * comp_screens_draw_league_table_tab 0x491380 draws a dashed line as a row of its own: after the champion (league
 * +0xd9 bit 0, 0x491e8c), after the direct promotion places (+0xbe) and the promotion play-off places (+0xbf), and
 * before the relegation play-off places (+0xc0) and the direct relegation places (+0xc1) (0x491e82..0x491f79). The
 * same five values give the number of those rows (0x491b36..0x491b7d), and the function reads them for nothing
 * else. With the option on, the runtime keeps them, sets them to 0 while the table is drawn, so the table has no
 * lines and no gaps, and puts them back after.
 *
 * The place of each club is a label in column 0 with the background colour [0xaebe04]: cell_button 0x8040a0 at
 * 0x491fd5 when the league has +0x3c, else label_create 0x804100 at 0x49200b. Its fifth argument is the background.
 * The rows come in table order, so the n-th of these labels is place n. Only the Overall view (+0xad 0, written
 * at 0x491e4c) gets colours: in the Home and Away views a place says nothing about the season's end.
 *
 * The colours, from the league's own values, so they show what the game does:
 *   promotion places                 dark green     promotion play-off places   light green
 *   relegation play-off places       orange         relegation places           red
 * In the top division of a European nation (league +0x1c is -1, as League::update_positions_and_status 0x684cd0
 * tests at 0x685388, and the nation's continent is Europe, 0x6853b0..0x6853df) the top places are the European
 * places of the next season instead:
 *   Champions League                 dark green     Europa League                light green
 *   Conference League                light blue
 * They come from the access list of the next season (season_uefa_access_places) at the nation's rank. Uefa_Seeding
 * ranks the nations once a year by the sum of the five season coefficients at nation +0xe0..+0x100, after it has
 * shifted the six values down one place (roll_coefficients 0x903b50). So the rank of the next season is the sum of
 * +0xe8..+0x108 now, and it is known for the whole season. The first Europa League place is the domestic cup
 * winner's, and so is the first Conference League place of a nation with no Europa League place; the last place of
 * the nations that the access list gives to the league cup winner is his (season_uefa_seeding.c). Those places are
 * not league places and get no colour. A season before the access list takes the counts that the original's own
 * table marks read (get_nation_places 0x905c60: nation entry +0xc and +0xd). */
#include "win.h"
#include "options.h"
#include "season_uefa.h"
#include "../rt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEAGUE_RECORD 0x4u
#define LEAGUE_ABOVE 0x1cu
#define LEAGUE_CLUBS 0x3eu
#define LEAGUE_VIEW 0xadu
#define LEAGUE_UP 0xbeu             /* up, up play-offs, down play-offs, down: four bytes */
#define LEAGUE_FLAGS 0xd9u
#define RECORD_NATION 0x5du
#define NATION_CONTINENT 0x71u
#define NATION_COEFFS 0xe0u         /* six doubles, oldest first */
#define KEY_EUROPE 0x9cfa10u
#define SEEDING 0xde1f58u
#define SEEDING_COUNT 0x438u
#define SEEDING_YEAR 0x439u
#define ENTRY 0x12u
#define LEAGUE_TABLE_ENGLAND 0xb64150u

#define DARK_GREEN 0x03C5u          /* (0, 120, 40) */
#define LIGHT_GREEN 0x6E4Au         /* (110, 200, 80) */
#define LIGHT_BLUE 0x451Cu          /* (70, 160, 230) */
#define ORANGE 0xF400u              /* (240, 128, 0) */
#define RED 0xC8E3u                 /* (200, 28, 24) */
#define MAX_PLACES 64

static struct {
    uint32_t league;
    int active, saved_flags, place;
    uint8_t saved[4];
    uint16_t colour[MAX_PLACES + 1];
} zones;

static int readable(uint32_t p) { return p >= 0x10000u && p < 0xfff00000u; }

static int count_tokens(const char *s)
{
    int n = 0;
    for (const char *p = s; p && *p;) {
        while (*p == ' ') p++;
        if (!*p) break;
        n++;
        while (*p && *p != ' ') p++;
    }
    return n;
}

static double next_total(uint32_t nation)
{
    double sum = 0;
    for (uint32_t i = 1; i < 6; i++) {
        uint64_t bits = (uint64_t)rd32(g_mem, nation + NATION_COEFFS + 8 * i) |
                        (uint64_t)rd32(g_mem, nation + NATION_COEFFS + 8 * i + 4) << 32;
        double d;
        memcpy(&d, &bits, sizeof d);
        sum += d;
    }
    return sum;
}

/* The league places of the next season's cups for a nation: CL, EL and ECL places that go to clubs by league
 * position. 0 when the nation has no entry in the seeding. */
static int european_places(uint32_t nation, int *cl, int *el, int *ecl)
{
    uint32_t self = rd32(g_mem, SEEDING);
    if (!readable(self)) return 0;
    int entries = (int8_t)g_mem[self + SEEDING_COUNT], index = -1;
    for (int i = 0; i < entries; i++)
        if (rd32(g_mem, self + ENTRY * (uint32_t)i + 4) == nation) index = i;
    if (index < 0) return 0;
    double own = next_total(nation);
    int rank = 1;
    for (int i = 0; i < entries; i++) {
        uint32_t other = rd32(g_mem, self + ENTRY * (uint32_t)i + 4);
        if (other != nation && readable(other) && next_total(other) > own) rank++;
    }
    int year = (int16_t)rd16(g_mem, self + SEEDING_YEAR) + 1;
    const SeasonUefa *row = season_uefa_row(year);
    const char *c = row ? season_uefa_access_places(row->season, 'c', rank) : NULL;
    if (!c) {
        uint32_t entry = self + ENTRY * (uint32_t)index;
        *cl = (int8_t)g_mem[entry + 0xc];
        *el = (int8_t)g_mem[entry + 0xd] - 1;
        *ecl = 0;
        if (*el < 0) *el = 0;
        return 1;
    }
    int s = row->season;
    int e = count_tokens(season_uefa_access_places(s, 'e', rank));
    int k = count_tokens(season_uefa_access_places(s, 'k', rank));
    int england = nation == rd32(g_mem, LEAGUE_TABLE_ENGLAND);
    *cl = count_tokens(c);
    *el = e > 0 ? e - 1 - (england && season_uefa_access_league_cup(s, "England")) : 0;
    *ecl = k - (k > 0 && e == 0) - (k > 0 && england && season_uefa_access_ecl_league_cup(s, "England"));
    if (*el < 0) *el = 0;
    if (*ecl < 0) *ecl = 0;
    return 1;
}

static void paint(int from, int count, uint16_t colour, int clubs)
{
    for (int p = from; p < from + count && p <= clubs && p <= MAX_PLACES; p++)
        if (p >= 1) zones.colour[p] = colour;
}

/* Called by the wrap of 0x491380 (season_round_names_game.c) before and after the original. */
void table_zones_before(uint32_t league)
{
    memset(&zones, 0, sizeof zones);
    if (!options_bool(OPT_TABLE_ZONES) || !readable(league)) return;
    zones.league = league;
    zones.active = 1;
    memcpy(zones.saved, g_mem + league + LEAGUE_UP, 4);
    zones.saved_flags = g_mem[league + LEAGUE_FLAGS];
    int clubs = (int16_t)rd16(g_mem, league + LEAGUE_CLUBS);
    int up = (int8_t)zones.saved[0], up_po = (int8_t)zones.saved[1];
    int down_po = (int8_t)zones.saved[2], down = (int8_t)zones.saved[3];
    if (up < 0) up = 0;
    if (up_po < 0) up_po = 0;
    if (down_po < 0) down_po = 0;
    if (down < 0) down = 0;
    paint(clubs - down + 1, down, RED, clubs);
    paint(clubs - down - down_po + 1, down_po, ORANGE, clubs);
    uint32_t record = rd32(g_mem, league + LEAGUE_RECORD);
    uint32_t nation = readable(record) ? rd32(g_mem, record + RECORD_NATION) : 0;
    uint32_t continent = readable(nation) ? rd32(g_mem, nation + NATION_CONTINENT) : 0;
    int top = (int32_t)rd32(g_mem, league + LEAGUE_ABOVE) == -1;
    int cl = 0, el = 0, ecl = 0;
    if (top && readable(continent) && rd32(g_mem, continent) == rd32(g_mem, KEY_EUROPE) &&
        european_places(nation, &cl, &el, &ecl)) {
        paint(1, cl, DARK_GREEN, clubs);
        paint(1 + cl, el, LIGHT_GREEN, clubs);
        paint(1 + cl + el, ecl, LIGHT_BLUE, clubs);
    } else {
        paint(1, up, DARK_GREEN, clubs);
        paint(1 + up, up_po, LIGHT_GREEN, clubs);
    }
    /* RECOMP_ZONES_LOG: the values and the European places of each table drawn, for the check of the option. */
    if (getenv("RECOMP_ZONES_LOG"))
        fprintf(stderr, "recomp: table zones clubs %d up %d up_po %d down_po %d down %d top %d europe cl %d el %d ecl %d\n",
                clubs, up, up_po, down_po, down, top, cl, el, ecl);
    memset(g_mem + league + LEAGUE_UP, 0, 4);
    g_mem[league + LEAGUE_FLAGS] = (uint8_t)(zones.saved_flags & ~1);
}

void table_zones_after(void)
{
    if (!zones.active) return;
    memcpy(g_mem + zones.league + LEAGUE_UP, zones.saved, 4);
    g_mem[zones.league + LEAGUE_FLAGS] = (uint8_t)zones.saved_flags;
    zones.active = 0;
}

/* Called by the wraps of 0x804100 and 0x8040a0 at the place label of a row: the background colour of the next
 * place, or 0 to keep the original's. */
uint32_t table_zones_place_colour(void)
{
    if (!zones.active) return 0;
    int place = ++zones.place;
    if (rd32(g_mem, zones.league + LEAGUE_VIEW) != 0 || place > MAX_PLACES) return 0;
    return zones.colour[place];
}
