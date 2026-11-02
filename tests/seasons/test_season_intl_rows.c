/* The original's host rows (0.6.2 open item 103, src/recomp_rt/win/season_intl_rows.c).
 *
 * The constructor HostCountry 0x5f97c0 takes the 57 rows from malloc(0x792) (0x5f98b8..0x5f98c3), which does not
 * clear them, and HostCountry::init_default_table 0x5f9970 writes all six host fields of the first 16 rows (World
 * Cup and Euro) but only the first two of rows 16 to 56, which are all fixed (choice 0xfd). find_hosts 0x5fa730
 * reads those two of a fixed row, so the original never sees the other four. season_intl_host_ids reads all six
 * of a fixed row. In the a2001esb game the four held 0, which is Afghanistan, so Afghanistan played the Africa Cup
 * of Nations 2002 as a third host. The clean sets the four fields of each fixed row to -1 (no host) and leaves
 * every other byte. ctest season_intl_rows. */
#include "season_intl.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

#define ROW 0x22

static void put32(uint8_t *p, int32_t v) { memcpy(p, &v, 4); }
static int32_t get32(const uint8_t *p) { int32_t v; memcpy(&v, p, 4); return v; }

/* A row: key, year, six host fields, choice, decision day, cycle. */
static void row(uint8_t *r, int32_t key, int16_t year, const int32_t pairs[6], uint8_t choice, uint16_t day, uint8_t cycle)
{
    put32(r, key);
    memcpy(r + 4, &year, 2);
    for (int k = 0; k < 6; k++) put32(r + 6 + 4 * k, pairs[k]);
    r[0x1e] = choice;
    memcpy(r + 0x1f, &day, 2);
    r[0x21] = cycle;
}

int main(void)
{
    uint8_t rows[4 * ROW], before[sizeof rows];
    /* The rows of the a2001esb save (July 2011): the Africa Cup of Nations 2002 (Egypt, then 0 for Afghanistan),
     * the OFC club row of every year (New Zealand twice), a World Cup row as drawn rows have it, and a fixed row of
     * the a2001s1 save with other leftovers. */
    row(rows, 0x1c3, 2002, (const int32_t[6]){59, -1, 0, 0, 0, 0}, 0xfd, 169, 0x10);
    row(rows + ROW, 0x1b5, 0, (const int32_t[6]){134, 134, 0, 0, 0, 0}, 0xfd, 13, 0);
    row(rows + 2 * ROW, 0x1c1, 2006, (const int32_t[6]){74, -1, 70, -1, 192, -1}, 0xfe, 200, 0x20);
    row(rows + 3 * ROW, 0x147, 0, (const int32_t[6]){74, -1, 28428, -224, 134742271, 28429}, 0xfd, 8, 0);
    memcpy(before, rows, sizeof rows);

    int changed = season_intl_rows_clean(rows, 4);
    CHECK(changed == 3, "%d rows changed, want the 3 fixed rows", changed);
    for (int i = 0; i < 4; i++) {
        const uint8_t *r = rows + i * ROW, *b = before + i * ROW;
        int fixed = b[0x1e] == 0xfd;
        CHECK(!memcmp(r, b, 0xe), "row %d: key, year and the first two host fields stay", i);
        CHECK(!memcmp(r + 0x1e, b + 0x1e, ROW - 0x1e), "row %d: choice, day and cycle stay", i);
        for (int k = 2; k < 6; k++) {
            int32_t v = get32(r + 6 + 4 * k), want = fixed ? -1 : get32(b + 6 + 4 * k);
            CHECK(v == want, "row %d host field %d: %d, want %d", i, k, v, want);
        }
    }
    CHECK(season_intl_rows_clean(rows, 4) == 0, "a second clean changes nothing");
    CHECK(season_intl_rows_clean(rows, 0) == 0, "no rows");

    if (failures) fprintf(stderr, "season_intl_rows: %d failures\n", failures);
    else printf("season_intl_rows: ok\n");
    return failures ? 1 : 0;
}
