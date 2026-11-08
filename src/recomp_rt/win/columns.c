/* Attribute colours (#19), match columns (#23) and condition and morale colours (#29).
 *
 * Attribute colours: the label create 0x804100 makes every attribute number. The runtime knows the two
 * call sites by their return address (tools/recomp/callsites.py): the value cells of the player profile
 * and the Attributes view of the squad list. For a text of 1 to 20 it changes the text colour argument to
 * the colour of the value band. All other labels, and every label with the option off, keep the colours
 * of the original.
 *
 * Condition and morale colours (#29): the runtime wraps the game's condition getters (0x62c590 outside a
 * match, 0x8aafd0 in a match) and its morale word function 0x4cc2e0, and keeps the last value of each.
 * When the next label is that condition as "N%" or that morale word, the label gets the colour of the
 * band: condition below 75% red, 75 to 89% amber, 90% or more green. The morale words have the same
 * three bands, two words each, in the game's order (0x4cc2e0, morale byte +0x45 of the player): Very Low
 * (below 4) and Low (below 8) red, Ok (below 12) and Good (below 15) amber, Very Good (below 18) and Superb
 * green. So every screen that draws these values with the game's own code gets the colours, with no list
 * of call sites. Option condition_colours, separate from attribute_colours.
 *
 * Tactics positions (#48), in every mode: the Condition view of the tactics screen 0x899ea0 (node slot 10 =
 * 2), in and out of a match, shows the player's positions between the status and the condition, with the
 * same labels as the Match Rating view below. Classic gets narrower column weights.
 *
 * Match columns, in widescreen mode only (the classic screen has no room; RECOMP_COLUMNS_CLASSIC=1 makes
 * a test run show them in classic mode too):
 * - The tactics screen 0x899ea0 in a match, view Match Rating (node slot 10 = 6), shows the player's
 *   positions and condition beside the match rating. Its player list area gets two more columns. The
 *   original cells keep their order: the runtime moves their column numbers, and after the screen code
 *   it adds a position label and a condition label to each row, with the game's own getters (0x8ab480
 *   and 0x53ee50 as in the Position(s) view, 0x8aafd0 as in the Condition view).
 * - The Player Ratings view 0x722ed0 of the match screen shows each player's condition beside the
 *   rating, for both teams. Its area gets one more column per team. The labels come after the screen
 *   code, so that the widget numbers that 0x723350 uses to update the view do not move. 0x723350 runs
 *   each time the view updates, and the runtime then updates the conditions too.
 *
 * Both features only read the game's data and add widgets on the screen, so they change no game state.
 * See docs/research/2026-10-14-columns-and-colours-inventory.md. */
#include "win.h"
#include "options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const uint32_t rt_site_profile_value;
extern const uint32_t rt_site_squad_attribute;
extern const uint32_t rt_site_tactics_area;
extern const uint32_t rt_site_tactics_area_heading;
extern const uint32_t rt_site_tactics_status;
extern const uint32_t rt_site_ratings_area;
extern const uint32_t rt_site_table_area;
extern const uint32_t rt_site_table_against_heading;
extern const uint32_t rt_site_table_for;
extern const uint32_t rt_site_table_against;
extern const uint32_t rt_site_table_place_label;
uint32_t table_zones_place_colour(void);    /* table_zones.c */

/* The colours of the bands, RGB565, light enough for the dark backgrounds of the game. */
#define COLOUR_LOW 0xFA8Au       /* 1 to 5: red (255, 80, 80) */
#define COLOUR_MID 0xFD84u       /* 6 to 10: amber (255, 176, 32) */
#define COLOUR_GOOD 0xA70Cu      /* 11 to 15: light green (160, 224, 96) */
#define COLOUR_TOP 0x37E6u       /* 16 to 20: bright green (48, 255, 48) */

/* The condition and morale bands use red, amber and bright green. */
static uint32_t condition_colour(int percent)
{
    return percent < 75 ? COLOUR_LOW : percent < 90 ? COLOUR_MID : COLOUR_TOP;
}

#define SCREENS 0xb74450u        /* the screen stack, this of 0x804100, 0x801c70, 0x7ff010 */
#define LABEL_CREATE 0x804100u   /* thiscall (area, column, row, style, colour, text_flags, font, text_colour,
                                  * text): the widget number in ax */
#define SET_TEXT 0x801c70u       /* thiscall (widget, text) */
#define NODE_SLOT 0x7ff010u      /* thiscall (slot): the value of a slot of the current screen node */
#define WIDGETS 0xb77c94u + 0xba95eu   /* the widgets of screen manager 0 (names.c) */
#define WIDGET_SIZE 0x18cu
#define MAX_WIDGETS 1200
#define YELLOW 0xaebdecu         /* the colour globals of the original */
#define GREY 0xae317eu
#define PERSONS 0xae23bcu        /* pointer to the person records, 0x6e bytes each */
#define MATCH 0xb67a64u          /* pointer to the match being played: the object at +4 of the tactics
                                  * object (0x8aafd0). Inference: seen only in a run, with no static
                                  * evidence yet (columns-and-colours inventory, match object note) */

/* Scratch text and the column weights: the end of the game's text buffer 0xde1f64 (2000 bytes or more,
 * 0x53ee50 gets 2000 as its size). Screen code only writes short texts at its start. The runtime keeps
 * the bytes it uses and puts them back. */
#define SCRATCH (0xde1f64u + 1600u)
#define SCRATCH_SIZE 256u

/* The attribute colour of a text that is a whole number 1 to 20, or 0 for any other text. */
static uint32_t attribute_colour(const char *s)
{
    int v = 0, n = 0;
    while (*s == ' ') s++;
    for (; *s >= '0' && *s <= '9' && n < 3; s++, n++) v = v * 10 + (*s - '0');
    while (*s == ' ') s++;
    if (!n || *s) return 0;
    return columns_attribute_colour(v);
}

/* The attribute colour of a value 1 to 20, or 0 for any other value. The Best XI tab uses it too. */
uint32_t columns_attribute_colour(int v)
{
    if (v < 1 || v > 20) return 0;
    return v <= 5 ? COLOUR_LOW : v <= 10 ? COLOUR_MID : v <= 15 ? COLOUR_GOOD : COLOUR_TOP;
}

/* Calls of the game's functions. Each keeps the registers of the caller. */
static uint32_t call_this(uint32_t fn, uint32_t self, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t call_cdecl(uint32_t fn, int n, const uint32_t *args)
{
    Cpu saved = cpu;
    uint32_t r = rt_callback(fn, n, args);
    cpu = saved;
    return r;
}

static uint32_t node_slot(int n)
{
    uint32_t a[1] = {(uint32_t)n};
    return call_this(NODE_SLOT, SCREENS, 1, a);
}

static int label(int area, int column, int row, uint32_t text_flags, uint32_t font, uint32_t colour, const char *text)
{
    win_put_str(SCRATCH, text, SCRATCH_SIZE);
    uint32_t a[9] = {(uint32_t)area, (uint32_t)column, (uint32_t)row, 1, 0, text_flags, font, colour, SCRATCH};
    return (int16_t)call_this(LABEL_CREATE, SCREENS, 9, a);
}

static int columns_wanted(void)
{
    static int classic = -1;
    if (classic < 0) {
        const char *v = getenv("RECOMP_COLUMNS_CLASSIC");
        classic = v && *v == '1';
    }
    return options_bool(OPT_MATCH_COLUMNS) && (rt_screen_w > 800u || classic);
}

/* --- Column moves: while a screen builds, the widgets of one area get new column numbers. --- */

static struct {
    int pending;             /* the next area create of the site gets the new columns */
    int area;                /* the area, or -1 */
    int columns;             /* the original's column count */
    const uint8_t *move;     /* new column of each original column */
    const uint8_t *weights;  /* the new weights */
    int new_columns;
} cols = {0, -1, 0, NULL, NULL, 0};

static uint8_t scratch_saved[SCRATCH_SIZE];
static void scratch_keep(void) { memcpy(scratch_saved, g_mem + SCRATCH, SCRATCH_SIZE); }
static void scratch_restore(void) { memcpy(g_mem + SCRATCH, scratch_saved, SCRATCH_SIZE); }

/* Tactics list, 9 columns: number, three boxes, name, the instruction or substitution column, status,
 * rating, goals. New: positions and condition after the status. */
static const uint8_t tactics_move[9] = {0, 1, 2, 3, 4, 5, 6, 9, 10};
static const uint8_t tactics_weights[11] = {14, 1, 5, 1, 52, 10, 14, 26, 13, 8, 7};
enum { TACTICS_POSITIONS = 7, TACTICS_CONDITION = 8 };

/* Tactics positions (#48): the Condition view (node slot 10 = 2), in and out of a match, in every mode. The
 * original's cells in this view: "capt" (text 0xa8bdd4, copied at 0x89cc34) is a widget in column 5 (widget
 * create, return 0x89cd50). The status of 0x8aadb0 (call 0x89e01a: "inj", "off", "sub" or "on" and the
 * minute in a match) is a label in column 6 (label create 0x89e043, push 6 at 0x89e03b, text_flags 1 at
 * 0x89e034). The condition is a label in column 7 (push 7 at 0x89e0b4 and 0x89e0cc). Column 8 (goals, view
 * 6 only) stays empty. New: the positions column after the status (TACTICS_POSITIONS, as in the Match
 * Rating view), so that no two cells share a column. The condition and column 8 move one place. The weights
 * are ours: classic has 330 px for the list, so its name column is narrower (about 118 px), and the status
 * column holds "sub 46". */
static const uint8_t condition_move[9] = {0, 1, 2, 3, 4, 5, 6, 8, 9};
static const uint8_t condition_weights[10] = {14, 1, 5, 1, 52, 10, 14, 26, 13, 7};
static const uint8_t condition_weights_classic[10] = {12, 1, 4, 0, 45, 11, 17, 22, 13, 1};

static int tactics_condition_view(void)
{
    if (!options_bool(OPT_MATCH_COLUMNS) || (int8_t)node_slot(10) != 2) return 0;
    cols.pending = 1;
    cols.columns = 9;
    cols.move = condition_move;
    cols.weights = rt_screen_w > 800u ? condition_weights : condition_weights_classic;
    cols.new_columns = 10;
    return 1;
}

/* Player Ratings, 8 columns per team: number, box, card box, gap, name, event, rating, goals. New: the
 * condition before the rating. */
static const uint8_t ratings_move[16] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17};
static const uint8_t ratings_weights[18] = {14, 1, 5, 1, 60, 21, 12, 7, 7, 14, 1, 5, 1, 60, 21, 12, 7, 7};
enum { RATINGS_CONDITION = 6 };

/* Goal difference (#105): the league table tab, comp_screens_draw_league_table_tab 0x491380. Its table area
 * (area create 0x491c27) has 11 columns with the weights at esp+0x20 (0x4913ae..0x4913df): the colour box (4),
 * the place (2), the club (16), a column whose weight depends on the competition (bl), then Pld, Won, Drn,
 * Lst, For, Ag and Pts (4 each). The headings are labels in row 0, Ag in column 9 (0x491dfe). Each row has
 * labels in columns 1 to 10 (0x4920ae..0x492325), For in column 8 (0x4922ab, the s16 at row +0x15) and Ag
 * in column 9 (0x4922e5, row +0x17), as the number text that 0x944c9f wrote. The Overall, Home and Away
 * views take the same code with other row offsets, so the runtime reads the two numbers from the label
 * texts and works in every view. New: GD in column 10, so Pts moves to 11. */
static void columns_off(void);

static const uint8_t table_move[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11};
enum { TABLE_GD = 10, TABLE_ROWS = 64 };
static uint8_t table_weights[12];
static struct {
    int building, heading_set, count;
    uint32_t heading[9];                       /* the arguments of the Ag heading */
    struct { uint32_t args[9]; int goals_for, goals_against; } row[TABLE_ROWS];
} table;

static void table_weights_from(uint32_t original)
{
    for (int i = 0; i < 10; i++) table_weights[i] = rd8(g_mem, original + (uint32_t)i);
    table_weights[TABLE_GD] = 4;
    table_weights[11] = rd8(g_mem, original + 10);
}

/* Called by the wrap of 0x491380 (season_round_names_game.c) before and after the original. */
void columns_table_before(void)
{
    table.building = 0;
    table.heading_set = 0;
    table.count = 0;
    if (!options_bool(OPT_GOAL_DIFFERENCE)) return;
    columns_off();
    cols.pending = 1;
    cols.columns = 11;
    cols.move = table_move;
    cols.weights = table_weights;
    cols.new_columns = 12;
    table.building = 1;
}

static void table_label_seen(uint32_t ret)
{
    if (!table.building || cols.area < 0 || (int16_t)rt_arg(0) != cols.area) return;
    if (ret == rt_site_table_against_heading) {
        for (int i = 0; i < 9; i++) table.heading[i] = rt_arg(i);
        table.heading_set = 1;
    } else if (ret == rt_site_table_for && table.count < TABLE_ROWS) {
        table.row[table.count].goals_for = atoi(win_str(rt_arg(8)));
    } else if (ret == rt_site_table_against && table.count < TABLE_ROWS) {
        for (int i = 0; i < 9; i++) table.row[table.count].args[i] = rt_arg(i);
        table.row[table.count].goals_against = atoi(win_str(rt_arg(8)));
        table.count++;
    }
}

static int table_label(const uint32_t *args, const char *text)
{
    uint32_t a[9];
    memcpy(a, args, sizeof a);
    a[1] = TABLE_GD;
    win_put_str(SCRATCH, text, SCRATCH_SIZE);
    a[8] = SCRATCH;
    return (int16_t)call_this(LABEL_CREATE, SCREENS, 9, a);
}

void columns_table_after(void)
{
    int area = cols.area, building = table.building;
    table.building = 0;
    if (!building) return;
    columns_off();
    if (rt_unwinding || area < 0 || !table.heading_set) return;
    scratch_keep();
    table_label(table.heading, "GD");
    for (int i = 0; i < table.count; i++) {
        char text[16];
        int gd = table.row[i].goals_for - table.row[i].goals_against;
        snprintf(text, sizeof text, gd > 0 ? "+%d" : "%d", gd);
        table_label(table.row[i].args, text);
    }
    scratch_restore();
}

/* Called by the area create wrap (display.c) before the original: the call sites of the two lists get
 * the new column count and weights. */
void columns_area_before(void)
{
    uint32_t ret = rd32(g_mem, cpu.esp);
    if (!cols.pending || (ret != rt_site_tactics_area && ret != rt_site_tactics_area_heading &&
                          ret != rt_site_ratings_area && ret != rt_site_table_area)) return;
    if ((int)rt_arg(4) != cols.columns) { cols.pending = 0; return; }
    if (ret == rt_site_table_area) table_weights_from(rt_arg(5));
    scratch_keep();
    for (int i = 0; i < cols.new_columns; i++) wr8(g_mem, SCRATCH + (uint32_t)i, cols.weights[i]);
    wr32(g_mem, cpu.esp + 4 + 4 * 4, (uint32_t)cols.new_columns);
    wr32(g_mem, cpu.esp + 4 + 4 * 5, SCRATCH);
    cols.pending = 2;
}

/* Called by the area create wrap after the original, with the new area. The wrap always calls it. When
 * the original unwound, there is no new area: the scratch bytes go back and the pending state clears,
 * but the area stays as it was. */
void columns_area_after(int area, int unwound)
{
    if (cols.pending != 2) return;
    scratch_restore();
    cols.pending = 0;
    if (!unwound) cols.area = area;
}

/* Called by the widget create wrap (display.c) before the original: a widget of the moved area gets its
 * new column. Widget create: (flags, x1, y1, x2, y2, column, row, ..., area as argument 17). */
void columns_widget_before(void)
{
    if (cols.area < 0 || (int16_t)rt_arg(17) != cols.area) return;
    uint32_t c = rt_arg(5);
    if (c < (uint32_t)cols.columns) wr32(g_mem, cpu.esp + 4 + 4 * 5, cols.move[c]);
}

static void columns_off(void) { cols.pending = 0; cols.area = -1; }

/* --- The tactics screen. --- */

typedef struct { int row, index; uint32_t tactics; } TacticsRow;
static TacticsRow tactics_rows[24];
static int tactics_count, tactics_building;

void o_008aadb0(void);

/* The status text of a player, thiscall on the tactics object (index, text, size, colour out, live). The
 * call site in the Condition and Match Rating views gives the row of each player: esi index, ebp row. */
RT_WRAP(008aadb0, tactics_status)
{
    if (tactics_building && cols.area >= 0 && rd32(g_mem, cpu.esp) == rt_site_tactics_status &&
        tactics_count < 24) {
        TacticsRow *r = &tactics_rows[tactics_count++];
        r->row = (int)cpu.ebp;
        r->index = (int8_t)cpu.esi;
        r->tactics = cpu.ecx;
    }
    o_008aadb0();
}

/* The new labels already have their new columns: the column moves end before them. In the Condition view
 * the original draws the condition, so only the positions are new. */
static void tactics_add_labels(int area, int with_condition)
{
    scratch_keep();
    for (int i = 0; i < tactics_count; i++) {
        TacticsRow *r = &tactics_rows[i];
        uint32_t a2[2] = {(uint32_t)r->index, 0}, a3[3] = {(uint32_t)r->index, 0, 0};
        uint32_t on_pitch = call_this(0x8aa9d0u, r->tactics, 3, a3);
        uint32_t colour = rd16(g_mem, on_pitch ? YELLOW : GREY);
        uint32_t player = call_this(0x8ab480u, r->tactics, 2, a2);
        if (player && rd32(g_mem, player + 0x61)) {
            wr8(g_mem, SCRATCH, 0);
            uint32_t p[4] = {rd32(g_mem, player + 0x61), SCRATCH, SCRATCH_SIZE, 1};
            call_cdecl(0x53ee50u, 4, p);
            char text[64];
            snprintf(text, sizeof text, "%s", win_str(SCRATCH));
            label(area, TACTICS_POSITIONS, r->row, 1, 1, colour, text);
        }
        int condition = with_condition ? (int8_t)call_this(0x8aafd0u, r->tactics, 2, a2) : 0;
        if (condition) {
            char text[16];
            if (condition < 1) snprintf(text, sizeof text, "Inj");
            else snprintf(text, sizeof text, "%d%%", condition);
            label(area, TACTICS_CONDITION, r->row, 0xc, 1, colour, text);
        }
    }
    scratch_restore();
}

void o_00899ea0(void);

/* The tactics screen build, a screen node function. */
RT_WRAP(00899ea0, tactics_build)
{
    columns_off();
    tactics_count = 0;
    int condition_view = tactics_condition_view();
    if (!condition_view && columns_wanted() && (int8_t)node_slot(10) == 6) {
        cols.pending = 1;
        cols.columns = 9;
        cols.move = tactics_move;
        cols.weights = tactics_weights;
        cols.new_columns = 11;
    }
    tactics_building = 1;
    o_00899ea0();
    tactics_building = 0;
    int area = cols.area;
    columns_off();
    if (!rt_unwinding && area >= 0) tactics_add_labels(area, !condition_view);
}

/* --- The Player Ratings view. --- */

static struct { int area, widget[2][20]; uint32_t summary; } ratings = {-1, {{0}}, 0};

/* The condition of the player in a row of the view, in per cent, or -1. The view reads its rows from the
 * match summary (0x722ed0: summary + 0x190 + team * 0x625 + row * 0x49, person number at +0, 0xfb at
 * +0x1d for an empty row). The match keeps the condition of each player at +0x4d (a float, times 0.01
 * for per cent, 0x8aafd0) of its records at match + 0x4796, 0x1be bytes each, 20 per team, with
 * the person at +0x69 and 0xfb at +0x19 for an empty record. The runtime finds the player's record by
 * the person, so the rows of both teams line up whatever the order. */
static int ratings_condition(int team, int row)
{
    uint32_t rec = ratings.summary + 0x190u + (uint32_t)team * 0x625u + (uint32_t)row * 0x49u;
    if (rd8(g_mem, rec + 0x1d) == 0xfb) return -1;
    int32_t id = (int32_t)rd32(g_mem, rec);
    if (id < 0) return -1;      /* players of clubs outside the chosen leagues have a number past the
                                 * person count (0x7231bb), so there is no upper limit here */
    uint32_t person = rd32(g_mem, PERSONS) + (uint32_t)id * 0x6eu;
    uint32_t match = rd32(g_mem, MATCH);
    if (!match) return -1;
    for (uint32_t k = 0; k < 40; k++) {
        uint32_t m = match + 0x4796u + k * 0x1beu;
        if (rd8(g_mem, m + 0x19) == 0xfb || rd32(g_mem, m + 0x69) != person) continue;
        uint32_t bits = rd32(g_mem, m + 0x4d);
        float f;
        memcpy(&f, &bits, 4);
        double v = f * 0.01;
        return v < 0 ? 0 : (int)v;   /* the original truncates (__ftol) */
    }
    return -1;
}

static int widget_ok(int w, int column, int row)
{
    if (w < 0 || w >= MAX_WIDGETS) return 0;
    uint32_t a = WIDGETS + WIDGET_SIZE * (uint32_t)w;
    return (int16_t)rd16(g_mem, a + 0x7a) == ratings.area && (int32_t)rd32(g_mem, a + 0x20) == column &&
           (int32_t)rd32(g_mem, a + 0x24) == row;
}

static void ratings_refresh(void)
{
    if (ratings.area < 0) return;
    scratch_keep();
    for (int team = 0; team < 2; team++)
        for (int row = 0; row < 20; row++) {
            int w = ratings.widget[team][row];
            if (!widget_ok(w, team * 9 + RATINGS_CONDITION, row)) {
                ratings.area = -1;
                scratch_restore();
                return;
            }
            int c = ratings_condition(team, row);
            char text[16] = "";
            if (c >= 0) snprintf(text, sizeof text, "%d%%", c);
            if (c >= 0 && options_bool(OPT_CONDITION_COLOURS))
                wr16(g_mem, WIDGETS + WIDGET_SIZE * (uint32_t)w + 0x78, (uint16_t)condition_colour(c));
            win_put_str(SCRATCH, text, SCRATCH_SIZE);
            uint32_t a[2] = {(uint32_t)w, SCRATCH};
            call_this(SET_TEXT, SCREENS, 2, a);
        }
    scratch_restore();
}

void o_00722ed0(void);
void o_00723350(void);

/* The Player Ratings view build, cdecl (two pointers to the area rect). */
RT_WRAP(00722ed0, ratings_build)
{
    columns_off();
    ratings.area = -1;
    if (columns_wanted()) {
        cols.pending = 1;
        cols.columns = 16;
        cols.move = ratings_move;
        cols.weights = ratings_weights;
        cols.new_columns = 18;
    }
    o_00722ed0();
    int area = cols.area;
    columns_off();
    if (rt_unwinding || area < 0) return;
    uint32_t slot0 = node_slot(0);
    ratings.summary = slot0 ? rd32(g_mem, slot0 + 0x5f) : node_slot(0x20);
    scratch_keep();
    for (int team = 0; team < 2; team++)
        for (int row = 0; row < 20; row++)
            ratings.widget[team][row] = label(area, team * 9 + RATINGS_CONDITION, row, 0xc, 2, rd16(g_mem, YELLOW), "");
    scratch_restore();
    ratings.area = area;
    ratings_refresh();
}

/* The Player Ratings update: the runtime updates the conditions after it. */
RT_WRAP(00723350, ratings_update)
{
    o_00723350();
    if (!rt_unwinding) ratings_refresh();
}

/* --- Condition and morale colours. --- */

static struct {
    int condition;           /* the last condition from a getter, or -1 */
    uint32_t morale_colour;  /* the colour of the last morale word, or 0 */
    char morale[32];         /* the last morale word */
} last = {-1, 0, ""};

void o_0062c590(void);
void o_008aafd0(void);
void o_004cc2e0(void);

/* The condition of a player outside a match, thiscall (player, kind): kind 1 gives the percentage, as the
 * profile and the squad list print it. */
RT_WRAP(0062c590, condition_get)
{
    uint32_t kind = rt_arg(1);
    o_0062c590();
    if (!rt_unwinding && kind == 1) last.condition = (int16_t)cpu.eax;
}

/* The condition of a player in a match, thiscall on the tactics object (index, live): the percentage. */
RT_WRAP(008aafd0, match_condition_get)
{
    o_008aafd0();
    if (!rt_unwinding) last.condition = (int8_t)cpu.eax;
}

/* The morale word of a player, thiscall (player, text, size). */
RT_WRAP(004cc2e0, morale_word)
{
    uint32_t player = rt_arg(0), text = rt_arg(1);
    o_004cc2e0();
    if (rt_unwinding || !player || !text) return;
    uint32_t p = rd32(g_mem, player + 0x61);
    int morale = p ? (int8_t)rd8(g_mem, p + 0x45) : 0;
    last.morale_colour = morale < 8 ? COLOUR_LOW : morale < 15 ? COLOUR_MID : COLOUR_TOP;
    snprintf(last.morale, sizeof last.morale, "%s", win_str(text));
}

/* The band colour for a label text that is the last condition or morale word, or 0. */
static uint32_t condition_label_colour(const char *s)
{
    if (last.morale_colour && *s && !strcmp(s, last.morale)) return last.morale_colour;
    if (last.condition < 0) return 0;
    int v = 0, n = 0;
    for (; *s >= '0' && *s <= '9' && n < 4; s++, n++) v = v * 10 + (*s - '0');
    return n && !strcmp(s, "%") && v == last.condition ? condition_colour(v) : 0;
}

void o_00804100(void);
void groups_label_before(void);

/* The label create 0x804100. */
RT_WRAP(00804100, label_create)
{
    groups_label_before();
    uint32_t ret = rd32(g_mem, cpu.esp);
    if ((ret == rt_site_profile_value || ret == rt_site_squad_attribute) && options_bool(OPT_ATTRIBUTE_COLOURS)) {
        uint32_t text = rt_arg(8);
        uint32_t colour = text ? attribute_colour(win_str(text)) : 0;
        if (colour) wr32(g_mem, cpu.esp + 4 + 4 * 7, colour);
    }
    if ((last.condition >= 0 || last.morale_colour) && options_bool(OPT_CONDITION_COLOURS)) {
        uint32_t text = rt_arg(8);
        uint32_t colour = text ? condition_label_colour(win_str(text)) : 0;
        if (colour) wr32(g_mem, cpu.esp + 4 + 4 * 7, colour);
    }
    last.condition = -1;   /* a value colours only the next label */
    last.morale_colour = 0;
    /* RECOMP_COLOUR_LOG=1: the text and the colour of each value label of the profile, for the check that
     * each attribute colour matches its value (tests/smoke/attribute_groups_run.py). */
    if (ret == rt_site_profile_value && getenv("RECOMP_COLOUR_LOG") && rt_arg(8))
        fprintf(stderr, "recomp: profile value \"%s\" colour %#x\n", win_str(rt_arg(8)), rt_arg(7) & 0xffff);
    table_label_seen(ret);
    if (ret == rt_site_table_place_label) {
        uint32_t colour = table_zones_place_colour();
        if (colour) wr32(g_mem, cpu.esp + 4 + 4 * 4, colour);
    }
    o_00804100();
}
