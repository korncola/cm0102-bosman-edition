"""Faults of the original that long games reach (0.6.2 slice 11, Q23, Q24, Q27; src/recomp_rt/win/original_faults.c).
The fixes act in every game, the base game too. The test switch RECOMP_ORIGINAL_FAULTS=keep turns them off.

National managers (fault 26, Q23). `club_can_be_managed` 0x544820 takes a senior national team (club id from club
count - 2 x nation count to club count - nation count, 0x544835..0x544857) only when its nation had more than 150
players at game creation (nation +0x88, 0x544859). `MANAGER_MANAGER::add_job_vacancy` 0x69ba50 returns at once for a
senior national team of 100 or fewer (0x69bafb). The count is made once, at game creation (0x5262fa..0x52630e). So
when the manager of a smaller nation leaves, nobody replaces him, and in a 2001 game every such team has lost its
manager by about 2019. The places where a national team loses or misses its manager take every senior national
team here:
- 0x7ab158: `player_regen_retire_from_non_playing_staff` 0x7aaeb0 removes a retiring national manager (0x7ab148) and
  opens a vacancy only for a manageable team (0x7ab14e).
- 0x68fe65: `MANAGER_MANAGER::daily_update` 0x68f9c0 appoints the manager of one national team to another and opens a
  vacancy for his old team only when it is manageable (0x68fe11..0x68fe5b).
- 0x69b6d8: `MANAGER_MANAGER::process_pending_departures` 0x69b650 removes a manager whose departure is due, for
  example at the end of his contract (0x69b6c8), and opens a vacancy only for a manageable club (0x69b6ce).
- 0x69d0a5: `MANAGER_MANAGER::fix_missing_jobs` 0x69d060, at each change of month, opens a vacancy for a club with no
  manager and no vacancy only when it is manageable (0x69d09b).
- 0x69bb02: the bound of 100 players in `add_job_vacancy`. Five calls open a vacancy with no test of 0x544820, so this
  bound alone stopped them for a small nation: 0x42b43b, 0x69ec9b, and the new search of `daily_update` when the
  chosen candidate is no longer free (0x68fbf4, 0x68fc51, 0x68fd6a). They now open it too.
The other 17 calls of 0x544820 (board confidence, news, the human job lists) keep the original's rule, so a small
nation's manager is not judged by a board, as before. Under-21 teams stay unmanaged, as in the original (0x544857).
A nation that no longer exists gets no vacancy (Q43): its record has no continent (+0x71), as East and West Germany,
the Soviet Union and Zaire in the 3.9.68 data and the old record of a renamed nation in a season game. While `game_run_to_start_date` 0x5cc850 makes a new game the fix is off,
so a new game starts as the original's (about 95 of 213 national teams of the 3.9.68 data have no manager) and
changes at its first month change in play.

The human manager's age (fault 33, Q27). `database_get_staff_age` 0x53ab90 works out the age from the birth date
(+0x10, +0x12) and a date as a 16-bit value in ax, but its callers keep al: the day loops store `movsx` of al at staff
+0x18 (0x5cca1d, 0x5cd3c4), about 1,400 readers take that byte, and other callers compare al at once (for example
0x4c85c5, 0x4d4dc2, 0x532f0b, 0x8d09a7). The human manager slots are the only staff that never retire, so their age
passes 127 (a manager born 1966, the default of `setup_add_human_manager` 0x820d40, in 2094) and the byte wraps to
-128: every age test then answers as for a child. Here 0x53ab90 itself gives 127 for any age above 127 (the wrap in
original_faults.c), so each test against a bound up to 127 gives the answer of the real age (for example the bound 35
of `MANAGER_MANAGER::is_staff_suitable_for_job` 0x69f931 and `manager_stats_screen_build` 0x6b0070). The byte cannot
hold the real age. The human's age is shown on no screen: the profile tabs that show an age ("Born ... (Age n)",
0x874cef, text 0xa85010) are greyed for a human manager. Below 128 every value is the original's.

Money (faults 27 and 28, Q24). The club balance is signed 64-bit (finance +0x00), but some readers keep 32 bits. The
trace is in docs/research/2026-10-30-money-64bit.md. Each patch gives the original's result whenever the original's
32-bit value holds the right number, so it acts only above 2,147,483,647, but for the top wage:
- `Finance::get_transfer_budget` 0x598800: this year's net spend (0x598f4f), pending fees plus that spend (0x598f8b,
  0x598f92) and the balance less 10m or 15m (0x599135, 0x599185, used at 0x5991d0, 0x5991d6 and 0x599201) keep
  64 bits. Its 23 callers that keep only eax get the budget clamped to 2,147,483,647, so a very rich AI club keeps a
  budget and bids.
- The money text: `Cash::get_display_amount` 0x43f460 reads a 32-bit Cash (`fild dword`). The screens that put a
  64-bit amount into a Cash give the full amount to the runtime (rt_cash_wide, for the reads of that site), and
  0x43f460 reads it from there: the club info balance (0x46399c, read once or twice: a long text is formatted again,
  ended at 0x4639ee), debt (0x463a14, ended at 0x463a6e) and budget (0x463b28, ended at 0x463b7a), the budget of the
  player search (0x808ef3) and of the bid screen (0x8ea2c0: the Cash keeps the clamped budget, because the fee
  colour reads it again at 0x8ea54c, 0x8eaf95 and 0x8eb3b0), and the manager's money spent and received on the
  manager stats screen (0x6b0366, 0x6b0399) and on the staff history tab (0x887fa7, 0x88808f).
- News: a news item keeps 50 32-bit slots (`NewsItem::set_slot` 0x785790 at item + 5 + 4 x slot), saved raw in
  news.dat. Two items carry a transfer budget in slot 2: the job offer to the human (`post_job_offer_to_human_news`
  0x6a4f80, store 0x6a5028) and the finance news (`finance_post_finance_boost_news` 0x59f300, store 0x59f36e). The
  slot keeps the clamped budget, and a budget above 2,147,483,647 also goes into the free slots 8 (low word) and 9
  (high word) of the item (0x6a5021, 0x59f36b). They stay 0 below it, as the item's constructor clears them
  (0x77bc1b..0x77bc29), so news.dat is the same. The text writers (0x6a5120, 0x59f3e0) read slot 2 (0x6a51dd,
  0x59f5ab) and show the full amount from slots 8 and 9 (0x6a533d, 0x59f5c7).
- `Finance::get_max_wage` 0x5968d0 adds 1.3 x the balance (ftol at 0x5968e3, only eax kept at 0x5968ee) to a wage term
  in 32 bits (0x597019) and divides by 52: from a balance of about 1.65 billion the sum wraps and the club's top wage
  goes wrong. The high word of that product is kept at 0x5968e8, and a sum above 2,147,483,647 is clamped to it. This
  fix acts from about 1.65 billion, where the original's sum has already wrapped. A debt keeps the original's result.
"""

# address: (the instruction in the original, C code)
INSNS = {
    0x7AB158: ("je 0x7ab167", "if (cc_e(F) && !rt_national_vacancy(edi, 1u)) goto L_7ab167;"),
    0x68FE65: ("je 0x68fe92", "if (cc_e(F) && !rt_national_vacancy(ebp, 2u)) goto L_68fe92;"),
    0x69D0A5: ("je 0x69d110", "if (cc_e(F) && !rt_national_vacancy(ebx, 3u)) goto L_69d110;"),
    0x69B6D8: ("je 0x69b6e3", "if (cc_e(F) && !rt_national_vacancy(esi, 4u)) goto L_69b6e3;"),
    0x69BB02: ("jle 0x69c525", "if (cc_le(F) && !rt_national_vacancy(esi, 0u)) goto L_69c525;"),
    0x598F4F: ("cdq", "edx = rt_budget_hi(eax, R32(esp + 0x44u));"),
    0x598F8B: ("cdq", "edx = rt_budget_hi(eax, R32(esp + 0x44u));"),
    0x598F92: ("cdq", "edx = rt_budget_hi(eax, ebx);"),
    0x599135: ("mov ecx, dword ptr [eax]", "ecx = rt_budget_floor(eax, 10000000u);"),
    0x599185: ("mov ecx, dword ptr [ecx]", "ecx = rt_budget_floor(ecx, 15000000u);"),
    0x5991D0: ("mov eax, ecx", "eax = rt_budget_floor_low(ecx);"),
    0x5991D6: ("cdq", "edx = rt_budget_floor_high(eax);"),
    0x599201: ("fiadd dword ptr [esp + 0x5c]", "ST(0) = ST(0) + (double)rt_budget_floor_wide(R32(esp + 0x5cu));"),
    0x43F460: ("fild dword ptr [ecx]", "fpush((double)rt_cash_amount(ecx));"),
    0x46399C: ("mov dword ptr [esp + 0x18], eax",
               "W32(esp + 0x18u, eax); rt_cash_wide(esp + 0x18u, eax, R32(ebp + 4u), 2u);"),
    0x4639EE: ("mov dx, word ptr [0xadab48]", "rt_cash_wide_end(); edx = (edx & 0xffff0000u) | R16(0xadab48u);"),
    0x463A14: ("mov dword ptr [esp + 0x18], eax",
               "W32(esp + 0x18u, eax); { uint64_t nb_ = 0u - R64(ebp); "
               "rt_cash_wide(esp + 0x18u, (uint32_t)nb_, (uint32_t)(nb_ >> 32), 2u); }"),
    0x463A6E: ("lea eax, [esp + 0xac]", "rt_cash_wide_end(); eax = (uint32_t)(esp + 0xacu);"),
    0x463B28: ("mov dword ptr [esp + 0x18], eax", "W32(esp + 0x18u, eax); rt_cash_wide(esp + 0x18u, eax, edx, 2u);"),
    0x463B7A: ("mov dx, word ptr [0xadab48]", "rt_cash_wide_end(); edx = (edx & 0xffff0000u) | R16(0xadab48u);"),
    0x808EF3: ("mov dword ptr [esp + 0x64], eax", "W32(esp + 0x64u, eax); rt_cash_wide(esp + 0x64u, eax, edx, 1u);"),
    0x8EA2C0: ("mov dword ptr [esp + 0x48], ebx", "W32(esp + 0x48u, rt_cash_wide_clamped(esp + 0x48u, ebx, edi, 1u));"),
    0x59F36B: ("push edx", "rt_news_wide(ecx, R32(esi), edx); t0 = edx; esp -= 4; W32(esp, t0);"),
    0x6A5021: ("push eax", "rt_news_wide(esp + 0x10u, R32(esi), eax); t0 = eax; esp -= 4; W32(esp, t0);"),
    0x59F5AB: ("push 3", "rt_news_read(edi, eax); t0 = 0x3u; esp -= 4; W32(esp, t0);"),
    0x59F5C7: ("mov dword ptr [esp + 0x18], ebx", "W32(esp + 0x18u, ebx); rt_cash_news(esp + 0x18u, ebx);"),
    0x6A51DD: ("push 3", "rt_news_read(esi, eax); t0 = 0x3u; esp -= 4; W32(esp, t0);"),
    0x6A533D: ("mov dword ptr [esp + 0x10], eax", "W32(esp + 0x10u, eax); rt_cash_news(esp + 0x10u, eax);"),
    0x6B0366: ("mov dword ptr [esp + 0x20], edi", "W32(esp + 0x20u, edi); rt_cash_wide(esp + 0x20u, edi, ecx, 1u);"),
    0x6B0399: ("mov dword ptr [esp + 0x3c], edi", "W32(esp + 0x3cu, edi); rt_cash_wide(esp + 0x3cu, edi, ecx, 1u);"),
    0x887FA7: ("mov dword ptr [esp + 0x34], eax",
               "W32(esp + 0x34u, eax); rt_cash_wide(esp + 0x34u, eax, R32(edi + 0xa8u), 1u);"),
    0x88808F: ("mov dword ptr [esp + 0x34], eax",
               "W32(esp + 0x34u, eax); rt_cash_wide(esp + 0x34u, eax, R32(edi + 0xb0u), 1u);"),
    0x5968E8: ("mov edx, dword ptr [0xae23b4]", "rt_max_wage_high(edx, eax); edx = R32(0xae23b4u);"),
    0x597019: ("add ecx, edx", "ecx = rt_max_wage_sum(ecx, edx);"),
}
