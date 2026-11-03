/* The missing UEFA season coefficient of some season databases (season_coeffs.h, 0.6.2 open item 106). */
#include "win.h"
#include "season_coeffs.h"

#include <stdio.h>
#include <string.h>

#define NATIONS_PTR 0xae23a8u
#define NATION_COUNT 0xae2350u
#define NATION_SIZE 0x122u

void season_coeffs_fill(const char *name)
{
    const SeasonCoeffDb *db = NULL;
    for (int i = 0; name && i < season_coeff_dbs_count; i++)
        if (!strcmp(season_coeff_dbs[i].db, name)) db = &season_coeff_dbs[i];
    if (!db) return;
    uint32_t base = rd32(g_mem, NATIONS_PTR), count = rd32(g_mem, NATION_COUNT);
    if (!base || !count || count > 10000u) return;
    int written = 0, kept = 0;
    for (int k = 0; k < db->n_rows; k++) {
        const SeasonCoeff *c = &db->rows[k];
        uint32_t rec = base + (uint32_t)c->id * NATION_SIZE;
        double before, now;
        if (c->id >= count || rd32(g_mem, rec) != c->id) {
            kept++;
            continue;
        }
        memcpy(&before, g_mem + rec + 0x100, sizeof before);
        memcpy(&now, g_mem + rec + 0x108, sizeof now);
        if (before != c->check_100 || (now != 0 && now != c->value_108)) {
            fprintf(stderr, "recomp: season coeffs: %s: %s keeps its own values (+0x100 %.3f, +0x108 %.3f)\n",
                    db->db, c->nation, before, now);
            kept++;
            continue;
        }
        memcpy(g_mem + rec + 0x108, &c->value_108, sizeof c->value_108);
        written++;
    }
    fprintf(stderr, "recomp: season coeffs: %s: the season before the start written for %d nations, %d kept\n",
            db->db, written, kept);
}
