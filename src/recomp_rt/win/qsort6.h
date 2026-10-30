/* The qsort of the original's CRT (MSVC 6, 0x945333) in host code. qsort6.c has the details. */
#ifndef QSORT6_H
#define QSORT6_H

#include <stdint.h>

/* The game stack that the original's qsort writes: mem is the game memory, q its ebp (esp at entry
 * minus 4), rows the game address of the rows, then the caller's ebp, ebx, esi and edi and the compare. */
typedef struct {
    uint8_t *mem;
    uint32_t q, rows, ebp, ebx, esi, edi, cmp;
} qsort6_stack;

/* The stack bytes that the original's qsort can write: from esp - QSORT6_BELOW to esp + QSORT6_ABOVE. */
#define QSORT6_BELOW 0x140u
#define QSORT6_ABOVE 0xcu

/* Sorts num rows of width bytes at base with the compare 0x7c6d40, exactly as 0x945333 sorts them:
 * the same compares in the same order and the same swaps, so rows that compare equal end in the same
 * order. It also writes the game stack st as 0x945333 does. Gives back the row that 0x945333 leaves in
 * eax, or NULL when it leaves eax unchanged (fewer than 2 rows). */
uint8_t *qsort6(uint8_t *base, uint32_t num, uint32_t width, const qsort6_stack *st);

/* The compare of the player history lists, 0x7c6d40: the id at +0, then the signed byte at +4, where
 * 0xff in either row counts as equal. */
int qsort6_history_cmp(const uint8_t *a, const uint8_t *b);

/* RECOMP_QSORT_TEST (crt.c): qsort6 against the recompiled qsort; the exit code, 0 when all are equal. */
int qsort6_self_test(void);

#endif
