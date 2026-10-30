/* CRT functions of the original that the runtime replaces by address. The rest of the CRT is
 * recompiled with the game.
 *
 * memcpy (0x94ab50) and memmove (0x94f0f0) are the same MSVC 6 code. It jumps through tables with
 * indexes that the code does not bound (and one table read at negative indexes), which the recompiler
 * cannot follow. Both copy as memmove does and return the target. */
#include "win.h"
#include "qsort6.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RT_REPLACE(0094ab50, memcpy)
{
    uint32_t dst = rt_arg(0), src = rt_arg(1), n = rt_arg(2);
    memmove(g_mem + dst, g_mem + src, n);
    rt_ret_cdecl(dst);
}

RT_REPLACE(0094f0f0, memmove)
{
    uint32_t dst = rt_arg(0), src = rt_arg(1), n = rt_arg(2);
    memmove(g_mem + dst, g_mem + src, n);
    rt_ret_cdecl(dst);
}

/* qsort 0x945333, cdecl (base, count, width, compare). For the compare of the player history lists
 * (0x7c6d40) the runtime sorts in host code with the same steps, the same stack writes and the same eax
 * (qsort6.c); every other compare keeps the recompiled qsort. RECOMP_QSORT=original turns the host sort
 * off; RECOMP_QSORT=check runs both on each call and stops the program at the first row, stack byte or
 * eax that differs. */
#define QSORT 0x945333u
#define HISTORY_CMP 0x7c6d40u

void o_00945333(void);

static int qsort_mode = -1;        /* 0 host, 1 check, 2 original, 3 the self-test below */
static unsigned long long qsort_checked;
static int test_fails;

static void print_qsort_checked(void) { fprintf(stderr, "recomp: qsort check: %llu sorts equal\n", qsort_checked); }

static uint8_t *host_sort(uint8_t *rows, uint32_t base, uint32_t num, uint32_t width)
{
    qsort6_stack st = {g_mem, cpu.esp - 4, base, cpu.ebp, cpu.ebx, cpu.esi, cpu.edi, HISTORY_CMP};
    return qsort6(rows, num, width, &st);
}

/* Both sorts from the same rows and stack; the original's result stays. 0 when all is equal. */
static int qsort_check(uint32_t base, uint32_t num, uint32_t width)
{
    size_t bytes = (size_t)num * width;
    uint32_t low = cpu.esp - QSORT6_BELOW, span = QSORT6_BELOW + QSORT6_ABOVE;
    uint8_t *copy = malloc(bytes + 1), *before = malloc(span), *host = malloc(span);
    if (!copy || !before || !host) rt_fatal("qsort check: out of memory");
    memcpy(copy, g_mem + base, bytes);
    memcpy(before, g_mem + low, span);
    uint32_t eax_in = cpu.eax;
    uint8_t *eax = host_sort(copy, base, num, width);
    uint32_t want = eax ? base + (uint32_t)(eax - copy) : eax_in;
    memcpy(host, g_mem + low, span);
    memcpy(g_mem + low, before, span);
    uint32_t esp_in = cpu.esp;
    o_00945333();
    int bad = memcmp(copy, g_mem + base, bytes) ? 1 : want != cpu.eax ? 2 : memcmp(host, g_mem + low, span) ? 3 : 0;
    if (bad) {
        uint32_t at = 0;
        if (bad == 3) while (host[at] == g_mem[low + at]) at++;
        fprintf(stderr, "recomp: qsort check: %u rows of %u bytes at 0x%x differ: %s (eax 0x%x, host 0x%x, "
                "stack byte esp%+d)\n", num, width, base, bad == 1 ? "rows" : bad == 2 ? "eax" : "stack", cpu.eax,
                want, bad == 3 ? (int)(low + at - esp_in) : 0);
    }
    free(copy);
    free(before);
    free(host);
    return bad;
}

RT_WRAP(00945333, qsort)
{
    uint32_t base = rt_arg(0), num = rt_arg(1), width = rt_arg(2), cmp = rt_arg(3);
    if (qsort_mode < 0) {
        const char *v = getenv("RECOMP_QSORT");
        qsort_mode = !v ? 0 : !strcmp(v, "check") ? 1 : !strcmp(v, "original") ? 2 : 0;
    }
    if (cmp != HISTORY_CMP || qsort_mode == 2) { o_00945333(); return; }
    if (qsort_mode == 1) {
        if (qsort_check(base, num, width)) abort();
        if (!qsort_checked++) atexit(print_qsort_checked);
        return;
    }
    if (qsort_mode == 3) {
        if (qsort_check(base, num, width)) test_fails++;
        return;
    }
    uint8_t *eax = host_sort(g_mem + base, base, num, width);
    rt_ret_cdecl(eax ? (uint32_t)(eax - g_mem) : cpu.eax);
}

/* RECOMP_QSORT_TEST: before the game starts, sorts rows made from a fixed seed with both sorts (the
 * check above, from a call of 0x945333 through the runtime) and compares the rows, the stack and eax.
 * The rows have many equal ids and 0xff bytes, so the order of rows that compare equal is tested. Also
 * lists that grow by one row and are sorted again each time, as 0x7c6a30 does. Gives the exit code: 0
 * when all are equal. */
int hot_loops_self_test(void);

static uint32_t test_seed = 12345;

static uint32_t test_random(void)
{
    test_seed = test_seed * 1103515245u + 12345u;
    return test_seed >> 8;
}

static void test_row(uint8_t *row, uint32_t ids)
{
    uint32_t id = test_random() % ids;
    if (test_random() % 16 == 0) id = 0x80000000u + test_random() % 4;      /* a wrap in the subtraction */
    row[0] = (uint8_t)id; row[1] = (uint8_t)(id >> 8); row[2] = (uint8_t)(id >> 16); row[3] = (uint8_t)(id >> 24);
    uint32_t b = test_random() % 9;
    row[4] = b == 8 ? 0xff : (uint8_t)(b - 4);
    row[5] = (uint8_t)test_random();
}

/* The test's call of 0x945333 lands here through the wrap in check mode. */
static void test_one(uint32_t mem, uint8_t *rows, uint32_t num)
{
    memcpy(g_mem + mem, rows, (size_t)num * 6);
    uint32_t args[4] = {mem, num, 6, HISTORY_CMP};
    cpu.eax = 0x5a5a5a5au;
    rt_callback(QSORT, 4, args);
    memcpy(rows, g_mem + mem, (size_t)num * 6);        /* the original's rows for the next sort */
}

int qsort6_self_test(void)
{
    enum { MAX = 4000 };
    uint8_t *rows = malloc(MAX * 6);
    uint32_t mem = rt_alloc(MAX * 6);
    int sorts = 0;
    qsort_mode = 3;                                  /* check, and count the failures */
    for (uint32_t num = 0; num <= 64; num++)
        for (int k = 0; k < 40; k++) {
            uint32_t ids = 1 + test_random() % (num + 2);
            for (uint32_t i = 0; i < num; i++) test_row(rows + i * 6, ids);
            test_one(mem, rows, num);
            sorts++;
        }
    static const uint32_t big[] = {100, 255, 256, 257, 1000, 2047, 3001, MAX};
    for (size_t b = 0; b < sizeof big / sizeof *big; b++)
        for (int k = 0; k < 6; k++) {
            uint32_t num = big[b], ids = k < 2 ? 3 : k < 4 ? num / 4 + 1 : num * 4;
            for (uint32_t i = 0; i < num; i++) test_row(rows + i * 6, ids);
            if (k == 5) for (uint32_t i = 0; i < num; i++) rows[i * 6 + 4] = 0xff;   /* every row equal by its byte */
            test_one(mem, rows, num);
            test_one(mem, rows, num);                /* sorted rows again */
            sorts += 2;
        }
    for (int list = 0; list < 6; list++) {           /* a list sorted again after each new row (0x7c6a30) */
        uint32_t ids = list < 3 ? 20 : 2000;
        for (uint32_t num = 1; num <= 1500; num++) {
            test_row(rows + (num - 1) * 6, ids);
            test_one(mem, rows, num);
            sorts++;
        }
    }
    free(rows);
    qsort_mode = -1;
    fprintf(stderr, "qsort test: %d sorts, %d differ\n", sorts, test_fails);
    int hot = hot_loops_self_test();                 /* hot_loops.c */
    return test_fails || hot ? 1 : 0;
}
