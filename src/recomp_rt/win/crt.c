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
int ftol_self_test(void);
int rt_call_cache_self_test(void);

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
    int ftol = ftol_self_test();                     /* below */
    int calls = rt_call_cache_self_test();           /* rt.c */
    return test_fails || hot || ftol || calls ? 1 : 0;
}

/* _ftol 0x945640, the MSVC 6 float to integer call (0.6.2 item 110). It takes ST(0), pops it and gives the
 * value cut toward zero as a 64-bit integer in edx:eax. The code: push ebp; mov ebp, esp; add esp, -0xc;
 * fnstcw [ebp-2]; mov ax, [ebp-2]; or ah, 0xc; mov [ebp-4], ax; fldcw [ebp-4]; fistp qword [ebp-0xc];
 * fldcw [ebp-2]; mov eax, [ebp-0xc]; mov edx, [ebp-8]; leave; ret. The game calls it millions of times a
 * day (a profile of a game day: 5.7 % of the main thread, most of it the cost of the call itself). The host
 * version below does the same: the same bytes below esp (ebp, the control word, the changed control word,
 * the integer), the same eax, edx, ebp and esp, and the x87 state with ST(0) popped and the control word
 * as before. The game reads stack bytes that earlier calls left (open item 85), so the stack writes stay.
 * RECOMP_FTOL=original runs the recompiled code; RECOMP_FTOL=check runs both on each call and stops the
 * program at the first difference. The self-test (RECOMP_QSORT_TEST) checks a list of values. */
#define FTOL_BELOW 16u              /* the bytes it writes below esp at entry */

void o_00945640(void);

static int ftol_mode = -1;          /* 0 host, 1 check, 2 original, 3 the self-test */
static unsigned long long ftol_checked;
static int ftol_fails;

static void ftol_host(void)
{
    uint32_t e = cpu.esp;
    uint16_t cw = fpu.cw, cut = (uint16_t)(cw | 0x0c00u);
    wr32(g_mem, e - 4, cpu.ebp);                     /* push ebp */
    wr16(g_mem, e - 6, cw);                          /* fnstcw [ebp-2] */
    wr16(g_mem, e - 8, cut);                         /* mov [ebp-4], ax */
    fpu.cw = cut;
    int64_t v = fist64(ST(0));                       /* fistp qword [ebp-0xc], rounded by the control word */
    fpop();
    fpu.cw = cw;                                     /* fldcw [ebp-2] */
    wr64(g_mem, e - 16, (uint64_t)v);
    cpu.eax = (uint32_t)(uint64_t)v;
    cpu.edx = (uint32_t)((uint64_t)v >> 32);
    cpu.esp = e + 4;                                 /* leave; ret */
}

static void print_ftol_checked(void) { fprintf(stderr, "recomp: ftol check: %llu calls equal\n", ftol_checked); }

/* Both versions from the same state; the original's result stays. 0 when all is equal. */
static int ftol_check(void)
{
    Cpu c0 = cpu;
    Fpu f0 = fpu;
    uint32_t low = cpu.esp - FTOL_BELOW;
    uint8_t before[FTOL_BELOW + 4], host[FTOL_BELOW + 4];
    memcpy(before, g_mem + low, sizeof before);
    ftol_host();
    Cpu c1 = cpu;
    Fpu f1 = fpu;
    memcpy(host, g_mem + low, sizeof host);
    cpu = c0;
    fpu = f0;
    memcpy(g_mem + low, before, sizeof before);
    o_00945640();
    int bad = memcmp(&c1, &cpu, sizeof cpu) ? 1 : memcmp(&f1, &fpu, sizeof fpu) ? 2 :
              memcmp(host, g_mem + low, sizeof host) ? 3 : 0;
    if (bad)
        fprintf(stderr, "recomp: ftol check: %s differs (value %.17g, control word 0x%x, eax 0x%x host 0x%x, "
                "edx 0x%x host 0x%x)\n", bad == 1 ? "a register" : bad == 2 ? "the x87 state" : "the stack",
                f0.st[f0.top & 7], f0.cw, cpu.eax, c1.eax, cpu.edx, c1.edx);
    return bad;
}

RT_WRAP(00945640, ftol)
{
    if (ftol_mode < 0) {
        const char *v = getenv("RECOMP_FTOL");
        ftol_mode = !v ? 0 : !strcmp(v, "check") ? 1 : !strcmp(v, "original") ? 2 : 0;
    }
    if (ftol_mode == 0) { ftol_host(); return; }
    if (ftol_mode == 2) { o_00945640(); return; }
    if (ftol_check()) {
        if (ftol_mode == 1) abort();
        ftol_fails++;
    }
    if (ftol_mode == 1 && !ftol_checked++) atexit(print_ftol_checked);
}

/* The self-test: values around the integer steps, halves, the 64-bit limits, infinities and NaN, each with
 * the four rounding modes in the control word before the call. Gives the count of values that differ. */
int ftol_self_test(void)
{
    static const double base[] = {0.0, -0.0, 0.25, 0.5, 0.75, 1.0, 1.5, 2.5, 3.49999, 99.99, 1e9, 2147483647.5,
                                  2147483648.0, 4294967295.9, 4294967296.0, 9.2233720368547748e18,
                                  9.2233720368547758e18, 1e19, 1e300, INFINITY, NAN, 4503599627370495.5,
                                  0.1, 1.0 / 3.0, 65535.5, 1e-310};
    uint32_t stack = rt_alloc(4096) + 2048, n = 0;
    ftol_mode = 3;
    ftol_fails = 0;
    for (size_t i = 0; i < sizeof base / sizeof *base; i++)
        for (int sign = 0; sign < 2; sign++)
            for (uint16_t rc = 0; rc < 4; rc++) {
                Cpu saved = cpu;
                fpu.cw = (uint16_t)(0x027fu | (rc << 10));
                fpush(sign ? -base[i] : base[i]);
                cpu.esp = stack;
                cpu.ebp = 0x12345678u;
                cpu.eax = 0xa5a5a5a5u;
                rt_callback(0x945640u, 0, NULL);
                cpu = saved;
                n++;
            }
    fpu_init();
    ftol_mode = -1;
    fprintf(stderr, "ftol test: %u values, %d differ\n", n, ftol_fails);
    return ftol_fails;
}
