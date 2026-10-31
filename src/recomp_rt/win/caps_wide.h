/* International caps above 255 (0.6.2 slice 12, fault 30, Q25; caps_wide.c, tools/recomp/caps_wide.py).
 *
 * The original keeps a player's caps in the unsigned byte at staff +0x22. NationalTeams::record_international_result
 * 0x76b600 raises it with no check (0x76b8c8 mov al, [esi + 0x22] / 0x76b8ce inc al / 0x76b8d0 mov [esi + 0x22], al),
 * so the 256th cap gives 0, and the next match takes the first-cap path at 0x76b890 (test al, al / jne 0x76b8c8): a
 * second "first cap" history row (call 0x86cc50) and the nation of that match made his first nation again.
 *
 * Here the byte stops at 255, and the runtime keeps the full count of each player above 255, by staff id (staff +0).
 * Every reader of the byte in the game then sees 255, the most it can hold, and the screens that show caps read the
 * full count. Older builds that load the save see 255. The counts go into the season part of the save (version 3,
 * season.c and season_change.c). Below 255 nothing changes: the store is empty and the game is the original's.
 * With the test switch RECOMP_SEASON_CODE=off no season part is written, so a save keeps only the byte 255. A count
 * left by a player whose id a new player takes (the byte then starts below 255) goes when that byte reaches 255. */
#ifndef RECOMP_CAPS_WIDE_H
#define RECOMP_CAPS_WIDE_H

#include <stdint.h>

#define CAPS_BYTE_MAX 255u

/* The new byte after one more cap for the player with this id and this byte: byte + 1 up to 255, then 255 while the
 * store counts on. A byte that reaches 255 from below starts with no stored count (a staff id used again). */
uint32_t caps_wide_add(uint32_t id, uint32_t byte);

/* The full caps of the player with this id and this byte: the stored count when the byte is 255, else the byte. */
uint32_t caps_wide_full(uint32_t id, uint32_t byte);

/* The number of players with a stored count. */
int caps_wide_count(void);

/* Before a new game, a load, and the end of a game: no stored counts. */
void caps_wide_reset(void);

/* The field of the season part (version 3): u32 count, then count times u32 staff id, u32 caps (256 or more).
 * caps_wide_part_size gives 0 for an empty store. The read takes exactly n bytes: 0 for damaged bytes. */
uint32_t caps_wide_part_size(void);
void caps_wide_part_write(uint8_t *out);
int caps_wide_part_read(const uint8_t *in, uint32_t n);

#endif
