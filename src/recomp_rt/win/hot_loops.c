/* Host copies of hot loops of the original that game creation and the game days spend their time in
 * (docs/research/2026-10-29-new-game-speed.md, 0.6.2 slice 14). Each copy gives the same memory, stack bytes and
 * registers as the recompiled code, so the game stays the same (rule Q152). RECOMP_HOT=original runs the
 * recompiled code; RECOMP_HOT=check runs both on each call and stops at the first difference. The
 * self-test (RECOMP_QSORT_TEST, crt.c) calls hot_loops_self_test.
 *
 * 0x7ae350, thiscall (int *id), ret 4: is the id in the list of this? The list holds [this+8] rows of
 * 0x1a bytes from [this+4], with the id at +0 of a row. With all nations, the second "Initialising game
 * data" of a 2025/26 start spent most of its 50 s in this loop. It pushes esi and edi, so it leaves them
 * below esp. al is 1 when found, else 0; the upper bytes of eax are those of the found row, of the row
 * after the last, or of the argument; edx is the index of the row, or the count, when the loop runs.
 *
 * 0x69fc60, thiscall (int *id), ret 4: is the id in the linked list at [this+8]? (0.6.2 slice 14.) The
 * shortlist search fill_shortlist_slot 0x83f3a0 calls it for each candidate (0x83fa8e), and with its calls
 * of the list step 0x68d1b0 it took 13 % of the samples of July to September 2025 in the profile of
 * 30 October 2026. The other callers are 0x4ceca2 and add_job_vacancy 0x69ba50 (0x69bd8f, 0x69bfb3,
 * 0x69c20c, 0x69c5c2); each tests al next, so none reads the flags. The list holds a
 * 16-bit count at +0, the first node at +2 and a cursor at +0xa; a node holds its data at +0 and the next
 * node at +6, and the id is at +6 of the data. The code (0x69fc60..0x69fd4a) saves the cursor in ebx, sets
 * it to the first node, steps it with 0x68d1b0 up to count times and puts the saved cursor back, so the
 * list ends as it was. It pushes ebx, ebp, esi and edi below 0x200 bytes of locals, and each step pushes
 * its return address 0x69fc8e. al is 1 when found, else 0; the upper bytes of eax are those of the list
 * when found or when the count is 0 or less, else those of the count (movsx); edx is the id of the last
 * data read. A node or data of 0 before the count ends gives the original's error box: the host copy finds
 * that before it writes, and then runs the recompiled code.
 *
 * The index of 0x7ae350 (item 151). The only caller of 0x7ae350 is is_staff_suitable_for_job 0x69f810
 * (0x69fa4b, ecx = the PLAYER_REGEN of [0xb7434c]), and add_job_vacancy 0x69ba50 calls that for each
 * member of staff. With all nations a 2025/26 start makes 790,068 searches of a list of many thousand
 * rows. While a call of 0x69ba50 runs, the copy finds the row in a hash index of the ids (the first row
 * of each id) in place of the scan, and gives the same eax, edx and stack. The index lives from the first
 * search in a call of 0x69ba50 to the end of that call, and is made again when the list, its rows or its
 * count change. The list (rows at +4, count at +8, room at +0xc) is written only by the members of
 * PLAYER_REGEN 0x7a4000..0x7ae400. From 0x69ba50 the direct calls reach 995 functions, and of those
 * members only 0x7a56f0 (the destructor, on the path of the game's end 0x5cc7d0), 0x7a7eb0, 0x7a7ed0
 * (the callers 0x696ba5, 0x696bd7 and 0x83d3cb only test the row pointer for 0), 0x7a81a0 (the spare
 * records at +0x1c..+0x44) and 0x7ae350. The destructor drops the index. A found row whose id differs
 * from the index prints "recomp: hot index stale" and the copy scans; RECOMP_HOT=check compares each
 * search with the recompiled code. Outside 0x69ba50 the copy scans.
 *
 * 0x600a50 human_manager_is_human_club, cdecl (club, ignore_holiday, quick, manager, any), item 151. The
 * shortlist search fill_shortlist_slot 0x83f3a0 calls it for each candidate (0x83fab3), and it took 13 % of
 * "Creating Shortlists" with all nations. The copy runs only the common path of a club of the computer:
 * a club, and either no manager at [club+0xcf], or a manager other than the argument, or a manager below
 * the first human id ([0xae2364] - 0x10, jl at 0x600aa5); then club_is_national_team 0x5409b0 (called at
 * 0x600be6) gives 0: the club id [club] is below [0xae235c] - [0xae2350] (setge at 0x540a40). Every other
 * case runs the recompiled code, before the copy writes anything. The path leaves ebx, ebp, esi and edi
 * (pushes at 0x600a56..0x600a59) at esp - 0x204..esp - 0x210, the argument and return address 0x600beb
 * of the call at esp - 0x214 and esp - 0x218, and the esi that 0x5409b0 pushes (0x540a36) at esp - 0x41c.
 * eax and edx are 0, ecx holds [0xae235c] - [0xae2350]. No caller reads the flags (the port keeps no flags
 * across a call). */
#include "win.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIST_HAS 0x7ae350u
#define LLIST_HAS 0x69fc60u
#define HUMAN_CLUB 0x600a50u

void o_007ae350(void);
void o_0069fc60(void);
void o_0069ba50(void);
void o_00600a50(void);
void o_007a56f0(void);

static int hot_mode = -1;          /* 0 host, 1 check, 2 original, 3 the self-test */

/* The index of 0x7ae350: open addressing on the id, the first row of each id. */
static struct {
    uint32_t list, rows, n;        /* the list it was made for: this, [this+4], [this+8] */
    uint32_t mask;
    uint32_t *ids;
    int32_t *at;                   /* the row, or -1 for an empty place */
    int valid;
} ridx;
static int vacancy_depth;          /* calls of add_job_vacancy 0x69ba50 that run */
static uint64_t ridx_builds;

static uint32_t ridx_hash(uint32_t id) { return (id * 0x9e3779b1u) >> 7; }

static void ridx_build(uint32_t list, uint32_t rows, uint32_t n)
{
    uint32_t want = 16;
    while (want < n * 2) want <<= 1;
    if (want - 1 != ridx.mask || !ridx.ids) {
        free(ridx.ids);
        free(ridx.at);
        ridx.ids = malloc(want * sizeof *ridx.ids);
        ridx.at = malloc(want * sizeof *ridx.at);
        if (!ridx.ids || !ridx.at) { fprintf(stderr, "recomp: out of memory for the hot index\n"); abort(); }
        ridx.mask = want - 1;
    }
    memset(ridx.at, 0xff, want * sizeof *ridx.at);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t id = rd32(g_mem, rows + i * 0x1a), h = ridx_hash(id) & ridx.mask;
        while (ridx.at[h] >= 0 && ridx.ids[h] != id) h = (h + 1) & ridx.mask;
        if (ridx.at[h] < 0) { ridx.ids[h] = id; ridx.at[h] = (int32_t)i; }   /* the first row of the id stays */
    }
    ridx.list = list, ridx.rows = rows, ridx.n = n, ridx.valid = 1;
    ridx_builds++;
}

/* The row of the id, or n when no row has it; -1 when the index cannot answer. */
static int64_t ridx_find(uint32_t list, uint32_t rows, uint32_t n, uint32_t id)
{
    if (!vacancy_depth || (int32_t)n <= 0) return -1;
    if (!ridx.valid || ridx.list != list || ridx.rows != rows || ridx.n != n) ridx_build(list, rows, n);
    uint32_t h = ridx_hash(id) & ridx.mask;
    while (ridx.at[h] >= 0) {
        if (ridx.ids[h] == id) {
            uint32_t i = (uint32_t)ridx.at[h];
            if (rd32(g_mem, rows + i * 0x1a) == id) return i;
            fprintf(stderr, "recomp: hot index stale: row %u of 0x%x\n", i, rows);
            ridx.valid = 0;
            return -1;
        }
        h = (h + 1) & ridx.mask;
    }
    return n;
}

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
            int64_t at = ridx_find(cpu.ecx, row, n, id);
            if (at >= 0) {
                found = (uint32_t)at < n;
                edx = (uint32_t)at;
                row += (uint32_t)at * 0x1a;
            } else for (;;) {
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

/* 0 when the walk would reach the error box, else 1 with the result in the registers and the stack. */
static int llist_has_host(void)
{
    uint32_t esp = cpu.esp, list = rd32(g_mem, cpu.ecx + 8), arg = rd32(g_mem, esp + 4);
    int32_t n = (int16_t)rd16(g_mem, list);
    uint32_t eax = list, edx = cpu.edx, node = rd32(g_mem, list + 2);
    int found = 0;
    if (n > 0) {
        for (int32_t i = 0; i < n; i++) {
            if (!node) return 0;
            uint32_t data = rd32(g_mem, node);
            if (!data) return 0;
            node = rd32(g_mem, node + 6);
            edx = rd32(g_mem, data + 6);
            if (edx == rd32(g_mem, arg)) { found = 1; break; }
        }
        if (!found) eax = (uint32_t)n;
        wr32(g_mem, esp - 0x214, 0x69fc8eu);
    }
    wr32(g_mem, esp - 0x204, cpu.ebx);
    wr32(g_mem, esp - 0x208, cpu.ebp);
    wr32(g_mem, esp - 0x20c, cpu.esi);
    wr32(g_mem, esp - 0x210, cpu.edi);
    cpu.eax = (eax & 0xffffff00u) | (uint32_t)found;
    cpu.ecx = list;
    cpu.edx = edx;
    cpu.esp = esp + 8;
    return 1;
}

static int llist_has_check(void)
{
    uint32_t low = cpu.esp - 0x214;
    uint8_t before[0x14], host[0x14];
    memcpy(before, g_mem + low, 0x14);
    Cpu in = cpu;
    if (!llist_has_host()) { o_0069fc60(); return 0; }
    Cpu want = cpu;
    memcpy(host, g_mem + low, 0x14);
    memcpy(g_mem + low, before, 0x14);
    cpu = in;
    o_0069fc60();
    int bad = want.eax != cpu.eax || want.ecx != cpu.ecx || want.edx != cpu.edx || want.ebx != cpu.ebx
              || want.ebp != cpu.ebp || want.esi != cpu.esi || want.edi != cpu.edi || want.esp != cpu.esp
              || memcmp(host, g_mem + low, 0x14);
    if (bad)
        fprintf(stderr, "recomp: hot check: 0x69fc60 differs (eax 0x%x host 0x%x, edx 0x%x host 0x%x)\n", cpu.eax,
                want.eax, cpu.edx, want.edx);
    return bad;
}

static unsigned long long hot_checked;
static int hot_test_fails;

static void print_hot_checked(void) { fprintf(stderr, "recomp: hot check: %llu calls equal\n", hot_checked); }

static void hot_mode_read(void)
{
    if (hot_mode < 0) {
        const char *v = getenv("RECOMP_HOT");
        hot_mode = !v ? 0 : !strcmp(v, "check") ? 1 : !strcmp(v, "original") ? 2 : 0;
    }
}

RT_WRAP(007ae350, list_has_id)
{
    hot_mode_read();
    if (hot_mode == 2) { o_007ae350(); return; }
    if (hot_mode == 1) {
        if (list_has_check()) abort();
        if (!hot_checked++) atexit(print_hot_checked);
        return;
    }
    if (hot_mode == 3) { hot_test_fails += list_has_check(); return; }
    list_has_host();
}

/* 1 with the result in the registers and the stack; 0 when the case is not the common one (nothing written). */
static int human_club_host(void)
{
    uint32_t esp = cpu.esp, club = rd32(g_mem, esp + 4), manager = rd32(g_mem, esp + 0x10);
    if (!club) return 0;
    uint32_t mgr = rd32(g_mem, club + 0xcf), esi = manager ? manager : mgr;
    if (mgr && esi == mgr && (int32_t)rd32(g_mem, esi) >= (int32_t)(rd32(g_mem, 0xae2364u) - 0x10u)) return 0;
    uint32_t limit = rd32(g_mem, 0xae235cu) - rd32(g_mem, 0xae2350u);
    if ((int32_t)rd32(g_mem, club) >= (int32_t)limit) return 0;              /* a national side */
    wr32(g_mem, esp - 0x204, cpu.ebx);
    wr32(g_mem, esp - 0x208, cpu.ebp);
    wr32(g_mem, esp - 0x20c, cpu.esi);
    wr32(g_mem, esp - 0x210, cpu.edi);
    wr32(g_mem, esp - 0x214, club);
    wr32(g_mem, esp - 0x218, 0x600bebu);
    wr32(g_mem, esp - 0x41c, esi);
    cpu.eax = 0;
    cpu.ecx = limit;
    cpu.edx = 0;
    cpu.esp = esp + 4;
    return 1;
}

static int human_club_check(void)
{
    uint32_t low = cpu.esp - 0x420;
    static uint8_t before[0x424], host[0x424];
    memcpy(before, g_mem + low, sizeof before);
    Cpu in = cpu;
    int took = human_club_host();
    Cpu want = cpu;
    memcpy(host, g_mem + low, sizeof host);
    memcpy(g_mem + low, before, sizeof before);
    cpu = in;
    o_00600a50();
    if (!took) return 0;
    int bad = memcmp(&want, &cpu, sizeof cpu) || memcmp(host, g_mem + low, sizeof host);
    if (bad)
        fprintf(stderr, "recomp: hot check: 0x600a50 differs (eax 0x%x host 0x%x, ecx 0x%x host 0x%x)\n", cpu.eax,
                want.eax, cpu.ecx, want.ecx);
    return bad;
}

RT_WRAP(00600a50, human_club)
{
    hot_mode_read();
    if (hot_mode == 2) { o_00600a50(); return; }
    if (hot_mode == 1) {
        if (human_club_check()) abort();
        if (!hot_checked++) atexit(print_hot_checked);
        return;
    }
    if (hot_mode == 3) { hot_test_fails += human_club_check(); return; }
    if (!human_club_host()) o_00600a50();
}

/* MANAGER_MANAGER::add_job_vacancy 0x69ba50: the index of 0x7ae350 lives while it runs. A nested call
 * or an unwind drops it too. */
RT_WRAP(0069ba50, job_vacancy_index)
{
    vacancy_depth++;
    ridx.valid = 0;
    o_0069ba50();
    ridx.valid = 0;
    if (--vacancy_depth < 0 || rt_unwinding) vacancy_depth = 0;
}

/* PLAYER_REGEN::~PLAYER_REGEN 0x7a56f0 frees the list. */
RT_WRAP(007a56f0, regen_free_index)
{
    ridx.valid = 0;
    o_007a56f0();
}

RT_WRAP(0069fc60, llist_has_id)
{
    hot_mode_read();
    if (hot_mode == 2) { o_0069fc60(); return; }
    if (hot_mode == 1) {
        if (llist_has_check()) abort();
        if (!hot_checked++) atexit(print_hot_checked);
        return;
    }
    if (hot_mode == 3) { hot_test_fails += llist_has_check(); return; }
    if (!llist_has_host()) o_0069fc60();
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
    /* The index (item 151): as inside add_job_vacancy, one list and many searches. The rows change only
     * with the index dropped, as the destructor and a new call of 0x69ba50 drop it. Lists of up to 3000
     * rows hold repeated ids, so the first row of an id must win. */
    uint32_t big = rt_alloc(3001 * 0x1a);
    vacancy_depth = 1;
    for (int n = -2; n <= 3000; n += n < 40 ? 1 : 397) {
        ridx.valid = 0;
        for (int i = 0; i < 3001 * 0x1a; i++) {
            seed = seed * 1103515245u + 12345u;
            g_mem[big + (uint32_t)i] = (uint8_t)(seed >> 16);
        }
        for (int i = 0; i < 3001; i++) {
            seed = seed * 1103515245u + 12345u;
            wr32(g_mem, big + (uint32_t)i * 0x1a, (seed >> 8) % (uint32_t)(n > 4 ? n : 4) + (i % 3 ? 0 : 0x7fffffffu));
        }
        wr32(g_mem, list + 4, big);
        wr32(g_mem, list + 8, (uint32_t)n);
        for (int k = 0; k < 60; k++) {
            wr32(g_mem, idp, (uint32_t)k * 37u + (k % 4 ? 0 : 0x7fffffffu));
            cpu.ecx = list;
            cpu.edx = 0x5678u + (uint32_t)k;
            cpu.eax = 0x12345678u;
            uint32_t args[1] = {k == 59 ? 0 : idp};
            rt_callback(LIST_HAS, 1, args);
            calls++;
        }
    }
    if (!ridx_builds) hot_test_fails++;            /* the index must have run */
    vacancy_depth = 0;
    ridx.valid = 0;
    /* 0x600a50 (item 151): clubs with ids around the national-side limit, with no manager, the argument's
     * manager or another, and manager ids around the first human id. The three globals are put back. */
    uint32_t g64 = rd32(g_mem, 0xae2364u), g5c = rd32(g_mem, 0xae235cu), g50 = rd32(g_mem, 0xae2350u);
    uint32_t club = rt_alloc(0x100), man = rt_alloc(8), other = rt_alloc(8);
    wr32(g_mem, 0xae2364u, 5000);
    wr32(g_mem, 0xae235cu, 9000);
    wr32(g_mem, 0xae2350u, 1000);
    for (int cid = 7990; cid <= 8010; cid += 5)
        for (int m = 0; m < 3; m++)
            for (int arg = 0; arg < 3; arg++)
                for (int mid = 4980; mid <= 4995; mid += 5) {
                    wr32(g_mem, club, (uint32_t)cid);
                    wr32(g_mem, club + 0xcf, m == 0 ? 0 : m == 1 ? man : other);
                    wr32(g_mem, man, (uint32_t)mid);
                    wr32(g_mem, other, (uint32_t)mid + 3);
                    cpu.eax = 0x11111111u, cpu.ecx = 0x22222222u, cpu.edx = 0x33333333u;
                    cpu.ebx = 0x1111u, cpu.ebp = 0x2222u, cpu.esi = 0x3333u, cpu.edi = 0x4444u;
                    uint32_t args[5] = {club, 1, 1, arg == 0 ? 0 : arg == 1 ? man : other, 0};
                    uint32_t esi = args[3] ? args[3] : rd32(g_mem, club + 0xcf), mgr = rd32(g_mem, club + 0xcf);
                    int common = cid < 8000 && !(mgr && esi == mgr && (int32_t)rd32(g_mem, esi) >= 4984);
                    if (!common) {
                        /* The original's other paths need a game (0x5fe040, 0x53b3d0): the copy must only decline. */
                        uint32_t sp = cpu.esp - 0x40;
                        for (int i = 0; i < 5; i++) wr32(g_mem, sp + 4 + 4u * (uint32_t)i, args[i]);
                        Cpu in = cpu;
                        in.esp = sp;
                        cpu = in;
                        if (human_club_host() || memcmp(&in, &cpu, sizeof cpu)) hot_test_fails++;
                        cpu.esp = in.esp + 0x40;
                        calls++;
                        continue;
                    }
                    rt_callback(HUMAN_CLUB, 5, args);
                    calls++;
                }
    wr32(g_mem, 0xae2364u, g64);
    wr32(g_mem, 0xae235cu, g5c);
    wr32(g_mem, 0xae2350u, g50);
    /* Linked lists of 0 to 30 nodes, a count from -2 to the length (a count below the length stops early),
     * ids from a small range, found and not found. */
    uint32_t head = rt_alloc(16), nodes = rt_alloc(31 * 10), datas = rt_alloc(31 * 10), this_ = rt_alloc(12);
    wr32(g_mem, this_ + 8, head);
    for (int len = 0; len <= 30; len++)
        for (int n = -2; n <= len; n++)
            for (int k = 0; k < 8; k++) {
                for (int i = 0; i < len; i++) {
                    seed = seed * 1103515245u + 12345u;
                    uint32_t node = nodes + (uint32_t)i * 10, data = datas + (uint32_t)i * 10;
                    wr32(g_mem, node, data);
                    wr32(g_mem, node + 6, i + 1 < len ? node + 10 : 0);
                    wr32(g_mem, data + 6, (seed >> 16) % 9);
                }
                wr16(g_mem, head, (uint32_t)n);
                wr32(g_mem, head + 2, len ? nodes : 0);
                wr32(g_mem, head + 0xa, 0x5150u + (uint32_t)k);
                wr32(g_mem, idp, (uint32_t)k);
                cpu.ecx = this_;
                cpu.edx = 0x4321u + (uint32_t)k;
                cpu.eax = 0xfedcba98u;
                cpu.ebx = 0x1111u, cpu.ebp = 0x2222u, cpu.esi = 0x3333u, cpu.edi = 0x4444u;
                uint32_t args[1] = {idp};
                rt_callback(LLIST_HAS, 1, args);
                if (rd32(g_mem, head + 0xa) != 0x5150u + (uint32_t)k) hot_test_fails++;
                calls++;
            }
    hot_mode = -1;
    fprintf(stderr, "hot loops test: %d calls, %d differ\n", calls, hot_test_fails);
    return hot_test_fails;
}
