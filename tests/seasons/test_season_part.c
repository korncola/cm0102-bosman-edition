/* The whole season part of a save (season_change.c): the write puts exactly the bytes that the size gives, and
 * a part read back writes the same bytes. The save (season.c add_part) allocates PART_SIZE + the size with
 * malloc and the write fills it, so a write past the size overruns that heap block. In 0.6.0 the empty row A18
 * list at the end of the part stored a count of 4 bytes that the size left out (0.6.1 items 4 and 10). On glibc
 * (Linux) that overrun hits the header of the next chunk when the block has no slack: a request of 16 * k + 5 to
 * 16 * k + 8 bytes. free() then stops the program ("double free or corruption"), after the original save wrote
 * the file and before the part went in, and the load of that save stops at the box "index.cpp 5809". A 2001/02
 * game first has news and renames in its part at the 2002 season change, so its saves from about June 2002
 * crashed on Linux. The test runs parts with 0 to 15 bytes of news text, so the block size takes every value
 * mod 16. ctest season_part. */
#include "win.h"
#include "season_rules.h"
#include "season_change.h"

#include <stdio.h>
#include <string.h>

/* The game side that the part functions never call. */
Cpu cpu;
uint8_t *g_mem;
int rt_unwinding;
uint32_t rt_callback(uint32_t target, int nargs, const uint32_t *args) { (void)target, (void)nargs, (void)args; return 0; }
uint32_t win_static_alloc(uint32_t size) { (void)size; return 0; }
void o_005cc510(void) {}
void o_005d5ae0(void) {}
void o_0060efd0(void) {}
void o_0077bc80(void) {}
void o_00782b70(void) {}
void o_00784100(void) {}
void o_00784eb0(void) {}
int season_start_year(void) { return 2001; }
int season_game_changes(void) { return 1; }
int season_changing_rules(void) { return 1; }
int season_code_on(void) { return 1; }
int season_day_of_year(int y, int m, int d) { (void)y, (void)m; return d; }
void season_from_day_of_year(int y, int doy, int *m, int *d) { (void)y; *m = 1, *d = doy; }
int season_fifa_day(int y, int m, int want) { (void)y, (void)m; return want; }
const SeasonCompClass season_comp_classes[1];
const int season_comp_class_count = 0;
const SeasonCompClass *season_comps_class(uint32_t vtable) { (void)vtable; return NULL; }
const char *season_comps_file(uint32_t obj) { (void)obj; return NULL; }
const char *season_comps_file_of(uint32_t obj) { (void)obj; return NULL; }
void season_comps_reset(void) {}
void season_cup_groups_reset(void) {}
void season_intl_qual_reset(void) {}
void season_uefa_league_reset(void) {}
void season_playoffs_hold(void) {}
void season_playoffs_release(void) {}
const SeasonRuleKind season_kind_club_cups, season_kind_club_world, season_kind_comps, season_kind_cup_breaks,
    season_kind_cups, season_kind_divisions, season_kind_intl, season_kind_ita_serie_c, season_kind_names,
    season_kind_nation, season_kind_new_cups, season_kind_playoffs, season_kind_sizes, season_kind_subs,
    season_kind_uefa;

uint32_t season_change_part_size(void);
void season_change_part_write(uint8_t *out);
int season_change_part_read(const uint8_t *in, uint32_t n);
void season_change_reset(void);

#define NAME_ROOM 51u
#define ROOM 1024u
#define MARK 0xa5

static int failures;

static void check(int ok, const char *text)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", text);
    if (!ok) failures++;
}

static uint32_t put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); return 4; }

/* A part as a save of a 2001/02 game after the 2002 change holds it: one news field of 2002 with len bytes of
 * text, two renames (the French first and second division), the season point 2002 and an empty row A18 list,
 * which has no bytes. */
static uint32_t make_part(uint8_t *p, uint32_t len)
{
    uint32_t at = put32(p, 1);
    at += put32(p + at, 2002);
    at += put32(p + at, 0);
    at += put32(p + at, len);
    for (uint32_t i = 0; i < len; i++) p[at++] = (uint8_t)('a' + i);
    at += put32(p + at, 2);
    const char *names[2] = {"Division 1", "Division 2"};
    for (uint32_t r = 0; r < 2; r++) {
        at += put32(p + at, 40 + r);
        memset(p + at, 0, NAME_ROOM);
        memcpy(p + at, names[r], strlen(names[r]));
        at += NAME_ROOM;
    }
    at += put32(p + at, 2002);
    return at;
}

int main(void)
{
    char text[200];
    for (uint32_t len = 0; len < 16; len++) {
        uint8_t in[ROOM], out[ROOM];
        uint32_t n = make_part(in, len);
        season_change_reset();
        int read = season_change_part_read(in, n);
        uint32_t size = season_change_part_size();
        memset(out, MARK, sizeof out);
        if (read && size <= ROOM) season_change_part_write(out);
        int past = 0;
        for (uint32_t i = size; i < ROOM; i++)
            if (out[i] != MARK) past++;
        /* The block of add_part: 16 bytes of head, then this part. */
        snprintf(text, sizeof text, "news text of %2u bytes: block of %u bytes (mod 16 = %2u), size %u, "
                 "%d bytes written past it, same bytes back", len, 16 + size, (16 + size) % 16, size, past);
        check(read == 1 && size == n && !past && !memcmp(in, out, n), text);
    }
    season_change_reset();
    check(season_change_part_size() == 0, "after a reset: no part");
    printf("%s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
