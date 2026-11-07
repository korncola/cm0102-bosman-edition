/* The option unprotected_contracts (issue 107).
 *
 * STAFF_CONTRACT::check_fifa_protected_period 0x868210 (thiscall, contract; 3 arguments, ret 0xc) answers 1 for
 * a protected contract and 0 for an unprotected one. It answers 1 while the new FIFA transfer rules are not in
 * force (0xae233c, 0x86824d), for a contract that began before the rule change date 0xae2330 (0x86826f..0x86829f),
 * and in the first three years of a contract, two from the age of 28 (0x8682f6..0x868344). After that it answers 0.
 * Its second argument, when not NULL, gets 0 at the start (0x868246) and 1 on the day the protection ends. The
 * twelve callers (the daily contract pass 0x4c84e5, transfer offers, the profile's contract tab and others) treat
 * an unprotected contract as one that the player may leave in an international transfer window.
 *
 * With the option off, the wrap turns each answer 0 into 1 and clears the second argument, as the original does
 * for a protected contract. The other parts of the rules stay: free transfers (0x544670), compensation
 * (0x8d0780) and the Player Status Committee (0x569e30) read the flag 0xae233c, not this answer. Only the host of
 * a network game runs this code. The test switch RECOMP_CONTRACT_LOG prints a line for each answer 0. */
#include "win.h"
#include "options.h"
#include "../rt.h"

#include <stdio.h>
#include <stdlib.h>

void o_00868210(void);

RT_WRAP(00868210, check_fifa_protected_period)
{
    uint32_t contract = cpu.ecx, ends_today = rt_arg(1);
    o_00868210();
    if (cpu.eax) return;
    int keep = !options_bool(OPT_UNPROTECTED_CONTRACTS);
    if (getenv("RECOMP_CONTRACT_LOG"))
        fprintf(stderr, "recomp: contract of staff %d %s\n", (int)rd32(g_mem, contract),
                keep ? "kept protected" : "unprotected");
    if (!keep) return;
    if (ends_today) wr32(g_mem, ends_today, 0);
    cpu.eax = 1;
}
