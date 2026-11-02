/* The host fields that the original's fixed host rows leave unset (0.6.2 open item 103).
 *
 * The constructor HostCountry 0x5f97c0 takes the 57 rows of 0x22 bytes from malloc(0x792) (0x5f98b8..0x5f98c3),
 * which does not clear them. HostCountry::init_default_table 0x5f9970 writes all six host fields of the World Cup
 * and Euro rows (rows 0 to 15) and only the first two (+6 and +0xa) of rows 16 to 56, which are all fixed (choice
 * 0xfd from 0x5f9eef). find_hosts 0x5fa730 reads those two of a fixed row (0x5fa7a0..0x5fa7a9), and daily_update
 * 0x5fa850 and post_host_news 0x5faec0 read no other, so the original never sees the other four. They hold what
 * the heap held before: 0 (Afghanistan) in the England, Spain and Brazil game of the run set, other values in the
 * games with all nations. season_intl_host_ids reads all six fields of a fixed row (slice 29), so a 0 there
 * made Afghanistan a third host of the Africa Cup of Nations 2002. ctest season_intl_rows. */
#include "season_intl.h"

#include <string.h>

#define ROW_SIZE 0x22
#define ROW_PAIRS 0x6
#define ROW_CHOICE 0x1e
#define CHOICE_FIXED 0xfd

int season_intl_rows_clean(uint8_t *rows, int count)
{
    int changed = 0;
    for (int i = 0; i < count; i++) {
        uint8_t *row = rows + (size_t)i * ROW_SIZE;
        if (row[ROW_CHOICE] != CHOICE_FIXED) continue;
        uint8_t *rest = row + ROW_PAIRS + 2 * 4, none[4 * 4];
        memset(none, 0xff, sizeof none);
        if (!memcmp(rest, none, sizeof none)) continue;
        memcpy(rest, none, sizeof none);
        changed++;
    }
    return changed;
}
