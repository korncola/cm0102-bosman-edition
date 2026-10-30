/* Host copies of hot loops of the original that game creation spends its time in
 * (docs/research/2026-10-29-new-game-speed.md). Each copy gives the same memory, stack bytes and
 * registers as the recompiled code, so the game stays the same (rule Q152). RECOMP_HOT=original runs the
 * recompiled code; RECOMP_HOT=check runs both on each call and stops at the first difference. The
 * self-test (RECOMP_QSORT_TEST, crt.c) calls hot_loops_self_test.
 *
 * 0x7ae350, thiscall (int *id), ret 4: is the id in the list of this? The list holds [this+8] rows of
 * 0x1a bytes from [this+4], with the id at +0 of a row. With all nations, the second "Initialising game
 * data" of a 2025/26 start spent most of its 50 s in this loop. It pushes esi and edi, so it leaves them
 * below esp. al is 1 when found, else 0; the upper bytes of eax are those of the found row, of the row
 * after the last, or of the argument; edx is the index of the row, or the count, when the loop runs. */
#include "win.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIST_HAS 0x7ae350u

void o_007ae350(void);

static int hot_mode = -1;          /* 0 host, 1 check, 2 original */

static void list_has_host(void)
{
    uint32_t esp = cpu.esp, arg = rd32(g_mem, esp + 4), eax = arg, edx = cpu.edx;
    wr32(g_mem, esp - 4, cpu.esi);
    wr32(g_mem, esp - 8, cpu.edi);
    int found = 0;
    uint32_t n;
    if (arg && (n = rd32(g_mem, cpu.ecx + 8)) != 0) {
        edx = 0;
        if ((int32_t)n > 0) {
            uint32_t id = rd32(g_mem, arg), row = rd32(g_mem, cpu.ecx + 4);
            for (;;) {
                if (rd32(g_mem, row) == id) { found = 1; break; }
                edx++;
                row += 0x1a;
                if ((int32_t)edx >= (int32_t)n) break;
            }
            eax = row;
        }
    }
    cpu.eax = (eax & 0xffffff00u) | (uint32_t)found;
    cpu.edx = edx;
    cpu.esp = esp + 8;
}

/* Both from the same state; the original's result stays. 0 when all is equal. */
static int list_has_check(void)
{
    uint32_t low = cpu.esp - 8;
    uint8_t before[12], host[12];
    memcpy(before, g_mem + low, 12);
    Cpu in = cpu;
    list_has_host();
    Cpu want = cpu;
    memcpy(host, g_mem + low, 12);
    memcpy(g_mem + low, before, 12);
    cpu = in;
    o_007ae350();
    int bad = want.eax != cpu.eax || want.edx != cpu.edx || want.ecx != cpu.ecx || want.esi != cpu.esi
              || want.edi != cpu.edi || want.esp != cpu.esp || memcmp(host, g_mem + low, 12);
    if (bad)
        fprintf(stderr, "recomp: hot check: 0x7ae350 differs (eax 0x%x host 0x%x, edx 0x%x host 0x%x)\n", cpu.eax,
                want.eax, cpu.edx, want.edx);
    return bad;
}

static unsigned long long hot_checked;
static int hot_test_fails;

static void print_hot_checked(void) { fprintf(stderr, "recomp: hot check: %llu calls equal\n", hot_checked); }

RT_WRAP(007ae350, list_has_id)
{
    if (hot_mode < 0) {
        const char *v = getenv("RECOMP_HOT");
        hot_mode = !v ? 0 : !strcmp(v, "check") ? 1 : !strcmp(v, "original") ? 2 : 0;
    }
    if (hot_mode == 2) { o_007ae350(); return; }
    if (hot_mode == 1) {
        if (list_has_check()) abort();
        if (!hot_checked++) atexit(print_hot_checked);
        return;
    }
    if (hot_mode == 3) { hot_test_fails += list_has_check(); return; }
    list_has_host();
}

/* Lists of 0 to 40 rows with ids from a small range, found and not found, a null argument and a count
 * below 0. Gives the count of calls that differ. */
int hot_loops_self_test(void)
{
    uint32_t list = rt_alloc(16), rows = rt_alloc(41 * 0x1a), idp = rt_alloc(4), seed = 777;
    int calls = 0;
    hot_mode = 3;
    for (int n = -2; n <= 40; n++)
        for (int k = 0; k < 30; k++) {
            for (int i = 0; i < 41 * 0x1a; i++) {
                seed = seed * 1103515245u + 12345u;
                g_mem[rows + (uint32_t)i] = (uint8_t)(seed >> 16);
            }
            for (int i = 0; i < 41; i++) wr32(g_mem, rows + (uint32_t)i * 0x1a, (seed >> 8) % 7 + (uint32_t)i % 5);
            wr32(g_mem, list + 4, rows);
            wr32(g_mem, list + 8, (uint32_t)n);
            wr32(g_mem, idp, (uint32_t)k % 12);
            cpu.ecx = list;
            cpu.edx = 0x1234u + (uint32_t)k;
            cpu.eax = 0xabcdef01u;
            uint32_t args[1] = {k == 29 ? 0 : idp};
            rt_callback(LIST_HAS, 1, args);
            calls++;
        }
    hot_mode = -1;
    fprintf(stderr, "hot loops test: %d calls, %d differ\n", calls, hot_test_fails);
    return hot_test_fails;
}
