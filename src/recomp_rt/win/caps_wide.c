/* International caps above 255 (0.6.2 slice 12, caps_wide.h). */
#include "caps_wide.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t id;
    uint32_t caps;           /* 256 or more */
} CapsWide;

static CapsWide *wide;
static int wide_count, wide_room;

/* Few players pass 255 caps, so a list is enough. */
static int find(uint32_t id)
{
    for (int i = 0; i < wide_count; i++)
        if (wide[i].id == id) return i;
    return -1;
}

static void drop(int i)
{
    wide[i] = wide[--wide_count];
}

static CapsWide *add(uint32_t id)
{
    if (wide_count == wide_room) {
        int room = wide_room ? 2 * wide_room : 64;
        CapsWide *p = realloc(wide, (size_t)room * sizeof *p);
        if (!p) return NULL;
        wide = p;
        wide_room = room;
    }
    CapsWide *w = &wide[wide_count++];
    w->id = id;
    w->caps = CAPS_BYTE_MAX;
    return w;
}

uint32_t caps_wide_add(uint32_t id, uint32_t byte)
{
    byte &= 0xffu;
    int i = find(id);
    if (byte < CAPS_BYTE_MAX) {
        if (i >= 0) drop(i);        /* a count of an earlier player with this id */
        return byte + 1;
    }
    CapsWide *w = i >= 0 ? &wide[i] : add(id);
    if (w && w->caps < 0xffffffffu) w->caps++;
    return CAPS_BYTE_MAX;
}

uint32_t caps_wide_full(uint32_t id, uint32_t byte)
{
    byte &= 0xffu;
    if (byte < CAPS_BYTE_MAX) return byte;
    int i = find(id);
    return i >= 0 ? wide[i].caps : byte;
}

int caps_wide_count(void) { return wide_count; }

void caps_wide_reset(void) { wide_count = 0; }

static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static uint32_t get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }

uint32_t caps_wide_part_size(void) { return wide_count ? 4u + 8u * (uint32_t)wide_count : 0u; }

void caps_wide_part_write(uint8_t *out)
{
    if (!wide_count) return;
    put32(out, (uint32_t)wide_count);
    for (int i = 0; i < wide_count; i++) {
        put32(out + 4 + 8 * (uint32_t)i, wide[i].id);
        put32(out + 8 + 8 * (uint32_t)i, wide[i].caps);
    }
}

int caps_wide_part_read(const uint8_t *in, uint32_t n)
{
    caps_wide_reset();
    if (!n) return 1;
    if (n < 4) return 0;
    uint32_t count = get32(in);
    if (count > (n - 4) / 8 || n != 4 + 8 * count) return 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t id = get32(in + 4 + 8 * i), caps = get32(in + 8 + 8 * i);
        if (caps <= CAPS_BYTE_MAX || find(id) >= 0) {
            caps_wide_reset();
            return 0;
        }
        CapsWide *w = add(id);
        if (!w) {
            caps_wide_reset();
            return 0;
        }
        w->caps = caps;
    }
    return 1;
}
