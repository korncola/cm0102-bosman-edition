"""Faults of the original that long games reach (0.6.2 slice 11, Q23, Q24, Q27, open item 102;
src/recomp_rt/win/original_faults.c). The fixes act in every game, the base game too. The test switch
RECOMP_ORIGINAL_FAULTS=keep turns them all off. "keep:managers", "keep:money", "keep:age", "keep:olympics",
"keep:ire_super", "keep:nsl" or "keep:holiday" (or a list: "keep:money,age") turns off one group or more.

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

Olympic squads (open item 102). `NationalTeams::select_squad` 0x76c120 lets the Olympic pick (slot 2) take a player of
his nation's senior squad when `national_teams_can_change_squad` 0x76db00 allows it (0x76ce15, 0x76ce2d). 0x76db00
refuses while the senior squad's window runs, but only for a squad that the human's screens marked selected (+0x44,
0x76de03), never for an AI squad. So an Olympic pick made between an AI senior squad's pick and its match took the
squad's young players, and in a Tier A game Holland played a World Cup qualifier with 10 (box match_eng..cpp 612).
At 0x76ce1c, the jump that skips a candidate, the Olympic pick also skips a player of a senior squad whose window ends
today or later (rt_olympic_keeps_senior; the Olympics: fixture +0 is the key at 0x9cf7a0, as at 0x76ce2d). Other
picks of slot 1 or 2 skip such a player anyway (0x76ce38).

The Irish Super Cup (open item 127). `IreSuperCup::create_table` 0x63b0c0 fills its four places with the Irish clubs
of the Champions League (club +0x1db the key at 0x9cf6e8, 0x63b10a..0x63b17b), the UEFA Cup (0x9cf6f0, 0x63b1c7..
0x63b21a) and the Intertoto Cup (0x9cf6f8, 0x63b228..0x63b27b), then with Premier Division clubs (club +0x57 the
key at 0x9cf80c, 0x63b295..0x63b2df). The last loop takes the first Premier Division clubs in club order and does not
ask whether a club is already in the table. When Ireland has fewer than four European places, a club of the first
loops is often taken again: the run set game a2001offs3 drew "Athlone Town v Athlone Town" in 2057 and a2001offs1
"Bohemian F.C. v Bohemian F.C." in 2041. That match is never played, so the edition never completes, the season pass
never calls `IreSuperCup::start_new_season` 0x63b550, and `update_for_date` 0x63b300 never draws again (it draws only
while +0x45 is 0 and +0x40 is the year, 0x63b311..0x63b331): the cup is gone for the rest of the game. At 0x63b2ae
(the jump that skips a club of another division) the last loop also skips a club that is already in the table
(rt_ire_super_has).

Australian NSL stages (open item 128). The NSL plays three stages: the regular season, the finals series of the top
six (stage 0, `AusNsl::create_finals_series_stage` 0x412010) and the Grand Final of the top two (stage 1,
`AusNsl::create_grand_final_stage` 0x4121c0). The index of the current stage is at +0x30 and the stage count (2) at
+0x2c; the ctor 0x410e90 sets them to -1 and 2 (0x410efe, 0x410f01). `AusNsl::advance_stage` 0x411f70 steps the index and builds
stage 0 or 1, but only while the index is below count - 1 (else the box "aus_nsl.cpp 752", 0x2f0).
`AusNsl::start_new_season` 0x411de0 does not set the index back to -1 and does not free the stages, as the other
classes with stages do at the same point (`ArgPrm::start_new_season` 0x405720, `BraNatFirst::start_new_season`
0x42a640: each stage 0..index freed by its slot 0 with 1, then the index -1; the AusNsl destructor 0x4110e0 frees
the same stages). So from the second season the index
stays at 1: the game builds no finals series and no Grand Final, and `AusNsl::record_season_honours` 0x411d90 writes
the first season's Grand Final pair again from the old stage 1. At 0x411e1c, after the stats reset and before the
step of the year (0x411e1e), each stage 0..index is freed as there, its slot set to 0, and the index set to -1
(rt_aus_nsl_new_season). A save made in season 2 or later keeps the index of 1 until its next start_new_season.

Holiday offers (open item 132). "Go on holiday ?" keeps its settings in the human record (+0x9b23,
`HUMAN_MANAGER::get_holiday_option` 0x5fea50). Option 0 is 1 for "Reject all offers for players" and 2 for "Reject
all offers below asking price" (the build 0x6b2d80 draws the row bright for that value, the input 0x6b35c1 sets it).
A player's asking price is staff +0x52 (`TRANSFER_MANAGER::set_player_squad_status` 0x8ce1e9). Two faults:
- `TRANSFER_MANAGER::ai_respond_to_bid` 0x8c8090 answers a bid for a club on holiday. With option 1 it rejects
  (0x8c8266). With option 2 it rejects only a bid with part-exchange players (0x8c8233) and otherwise gives the
  computer's answer (0x8c8277), which accepts bids down to about 70% of the asking price. So "below asking price" did
  the same as no setting. At 0x8c8264 a bid whose fee (offer +0x1b) is below the asking price now takes the reject
  of option 1 (rt_holiday_below_asking). A player with no asking price (0) keeps the computer's answer.
- `shortlist_manager_predict_selling_club_response` 0x84af00 holds the tests of both options (0x84b05e..0x84b0a8: 1
  answers 2, reject; 2 takes the asking price), but only in its branch for a human club, which it finds with
  0x600a50 and ignore_holiday 0 (0x84b025). That call counts a club on holiday as a computer club, so the tests never
  run. The predictor decides loan requests for a club on holiday (`NewsItem::auto_respond` 0x784100 for news 4000,
  `TRANSFER_MANAGER::process_due_offer` 0x8c63a0 state 0) and the bids that computer clubs plan
  (0x83f3a0, 0x842e10). At 0x84b02f the computer branch now makes the same tests for a club whose manager is on
  holiday (rt_holiday_predict, with the human test of 0x8c81ea: ignore_holiday 1), and at 0x84af77 a loan request
  for such a club with option 1 gets the answer 2 (rt_holiday_refuses_loan). A release clause in force keeps the
  original's answer, as in the dead test (0x84b05e), and the bid reply accepts a bid that meets it before any setting
  (`transfer_manager_selling_club_must_accept` 0x8cd240 at 0x8c81bb), as a real club must. With no human on holiday
  (the count at [0xb63c98] + 0xc) and with the settings off, every answer is the original's.
Three callers (0x8c0527, 0x8c2e3d, 0x8c42fa) call 0x8c8090 with dry_run 1, and the reject at 0x8c8266 acts in a dry
run too. The original does this for "Reject all offers", and the fix does it for a bid below the asking price. In
`process_due_offer` 0x8c63a0, an offer in state 0x11 or 0x12 goes to 0x8c2f50 when offer +0x10 is set (0x8c6983), and
news 0xfa5 confirms an offer through 0x8c3310 (0x784cbc). Both come after a bid was accepted, so with "Reject all
offers" an offer for the club can reach them only when a release clause or the receivers made the club accept it.
Every player of a game has an asking price: none of 6,860 at the start, 6,842 a year later and 10,679 in a game of
2026 has 0 (tests/seasons/holiday_offers_run.py, RECOMP_TEST_LIST=census), so listed and unlisted players meet the
same rule.
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
    0x76CE1C: ("je 0x76cf82", "if (cc_e(F) || rt_olympic_keeps_senior(esi, ebp, ebx, R32(esp + 0x8ecu))) goto L_76cf82;"),
    0x63B2AE: ("jne 0x63b2c6", "if (!cc_e(F) || rt_ire_super_has(esi, edi, ebx)) goto L_63b2c6;"),
    0x5968E8: ("mov edx, dword ptr [0xae23b4]", "rt_max_wage_high(edx, eax); edx = R32(0xae23b4u);"),
    0x597019: ("add ecx, edx", "ecx = rt_max_wage_sum(ecx, edx);"),
    0x411E1C: ("mov eax, dword ptr [esi]", "SYNC(); rt_aus_nsl_new_season(esi); RT_AFTER_CALL(); eax = R32(esi);"),
    0x84B02F: ("je 0x84b0ad", "if (cc_e(F)) { SYNC(); t0 = rt_holiday_predict(ebx, edi, ebp); RT_AFTER_CALL(); if (t0 == 1u) goto L_84b01c; "
                              "if (t0 == 2u) { esi = R32(ebp + 0x52u); goto L_84b0e7; } goto L_84b0ad; }"),
    0x84AF77: ("jne 0x84b174", "if (!cc_e(F)) goto L_84b174; SYNC(); t0 = rt_holiday_refuses_loan(ebx); RT_AFTER_CALL(); "
                               "if (t0) goto L_84b162;"),
    0x8C8264: ("jne 0x8c8277", "if (!cc_e(F) && !rt_holiday_below_asking(eax, esi)) goto L_8c8277;"),
}
