"""Reads of the original that depend on where memory lies (0.6.2 open item 138b; src/recomp_rt/win/layout_reads.c).

A base game (changing rules off) gave other results when only the Setup Game screen of the runtime changed (Setup
Game menu B, 25298081): the screen's blocks moved the database tables to other heap addresses, and the game read
an address as a number, a heap block that nothing had set and stack bytes that nothing had set. None of these is
a value of the game: the original's own layout is not fixed either (its heap is the CRT heap of the computer it runs
on). So each read takes a defined value in its place: the record's id for the address, 0, as fresh memory, for the
unset bytes, and 0x240, the mask of the original's last places, for the place masks of a national squad. The fixes
act in every game, the base game too. The test switch RECOMP_LAYOUT_FIXES=keep, or keep:captain, keep:players,
keep:daily, keep:stats, keep:loans, keep:squads, turns them off (layout_reads.c).

The captain (MATCH_MAN::choose_captain 0x6e1940). For each player of the list it takes the captain score
(match_man_get_captain_score 0x6e1cf0). When the manager's rating (non_player_attribute_rating 0x544410 with the
non-player record [pick + 0x1f] and its byte +0x17) is below the player's staff id modulo 20 (0x6e19a8..0x6e19b7),
the score is scaled by 1.1 - 0.01 x n (constants 0x9689c0 and 0x967898, 0x6e19c5..0x6e19e2), with n from 0 to 19.
The original takes n as the address of the manager's non-player record modulo 20 (0x6e19b9 `mov eax, dword ptr
[ebx + 0x1f]`, then `cdq`, `idiv ecx` with ecx 20). Here n is the record's id modulo 20 (non-player +0x00), as the
test before it takes the player's staff id (0x6e19a8 `mov eax, dword ptr [esi]`). The scale stays one value per
manager from 0.91 to 1.10. The captain goes into the pick (+0x3b, 0x6dc8ad and 0x6dcee0), and the player score of a
pick (match_man_score_player_for_role 0x6e2f20) gives the captain 100 more (0x6e5af9..0x6e5b04), so the eleven
depended on the address too: in a 2001 base game the Cook Islands took another goalkeeper in their first World Cup
qualifier, and the games went apart from there.

The match players (MATCH_PLAYER::MATCH_PLAYER 0x6eada0). The match engine's constructor 0x6b7e50 makes its 40 match
players at +0x4796 (0x1be bytes each) with the vector constructor 0x9463af (0x6b7e69..0x6b7e85). The constructor
sets most fields but not +0x1d to +0x22 (and others), and MATCH_PLAYER::setup 0x6eaea0 fills only the slots that
get a player. After a match, match_eng_update_post_match_reputations 0x6d1cc0 sums the minutes of each slot of a
side (0x6d2090..0x6d25b9): a bench slot counts unless its +0x1f is 0xff (0x6d20b3), and a slot with no player
(+0x69 0, 0x6d20c2) goes to the minutes test (0x6d2557..0x6d25ac), which reads +0x1f, +0x20 and +0x21. A side
with fewer than 20 players has such slots, and the engine is a heap block that held other data before, so the
average (0x6d25e3..0x6d25fd) and with it the club's new reputation (match_eng_compute_club_reputation 0x6d3de0,
0x6d4015; stored at 0x6b7472) depended on what lay there: in a 2001 base game a friendly on day 173 left club 350
at 1000 with one Setup Game screen and at 1001 with the other. Here the constructor first clears the record, as
fresh memory is: an empty slot then has 0 minutes and adds nothing (0x6d2594, `test al, al`).

The daily staff update (game_daily_staff_update 0x5ce2d0). Its loop over all staff (0x5ce32d..) keeps a running
value in the local [esp + 0x70]: for a staff with a club it reads the value (0x5ce830 or 0x5ce84f), adds his byte
+0x59 (or a part of it), stores it back (0x5ce859, 0x5ce864, 0x5ce888) and adds it to the range of a later draw
(0x5ce88c to [esp + 0x20], drawn at 0x5cf116). Nothing sets the local before the loop, so the first staff starts
from what an earlier call left on the stack, and in a 2001 base game that was a heap address (0x73106304 with one
Setup Game screen, 0x73104602 with the other): before the 2001/02 season the draw at 0x5cf116 had the range 31821
or 22541, and the games went apart. Here the local is 0 when the function starts, where it already clears
[esp + 0x8a0] (0x5ce311).

The simulated matches (SimulatedStats::play_daily_matches 0x84be20). For each pair of clubs it plays (0x84c249..)
it builds a fixture (0x4f bytes) on its stack at [esp + 0x40] and gives it to the match engine (init_match 0x6b7f20,
call at 0x84c3e8; the engine keeps it at +0x4792, 0x6b7f4e). It sets the id -2, the clubs, the competition type and
the result bytes (0x84c271..0x84c331, VIRTUAL_STAFF::fill_fixture_squads 0x90d2f0 for the squads), but not the match
flags word +0x38 (2026-10-17-cup-round-record.md, section 2) and some other fields. quick_simulate_match 0x6beeb0
reads the flags at 90 minutes (0x6bfe8e..0x6bfee6: bit 0x80 golden goal, bit 0x02 extra time), so a level match
went to extra time and golden goal by what an earlier call left on the stack. In a 2001 base game the word held the
low half of a heap address (0x17c0 with one Setup Game screen, 0xefd0 with the other), and on day 251 a level match
had golden goal with one and not with the other. Here the fixture is 0 before each match, as fresh memory is (0x84c266,
the first instruction of each match): the flags are 0, and a level simulated match ends level at 90 minutes.

The loans of the database (TRANSFER_MANAGER::add_config_loan 0x8bf400). It builds a contract (0x50 bytes) on its
stack at [esp + 0x28] (STAFF_CONTRACT::STAFF_CONTRACT 0x85e410 at 0x8bf424, init_from_current_contract 0x85e590 at
0x8bf58e) and gives it to CONTRACT_MANAGER::sign_loan_contract 0x4d01e0 (0x8bf5b4), which copies it into the contract
table (STAFF_CONTRACT::operator= 0x4cc810 at 0x4d0786; byte +0x3d also at 0x4d0516 and 0x4d0675). Neither function
sets +0x3d, the player's morale at the club (-100 to 100; the sign-contract code sets it by squad status, 75, 50, 40
or 0, at 0x4cfadf..0x4cfaf1). Media::apply_story_effect_flags_fragment 0x737070 draws with the range +0x3d + 150
(0x737109..0x737114) and changes the player's morale by the result. In a 2001 base game the loan of staff 6943 to
club 657 had 0x5e with one Setup Game screen and 0x40 with the other, and on day 269 the draw ranges were 244 and
214. Here the contract is 0 before the constructor, as fresh memory is (0x8bf420): the loan starts at morale 0.

The national squads (NationalTeams::select_squad 0x76c120). It gives each squad place a position mask in the local
array [esp + 0x70] (one word per place) and passes it to match_man_assign_players_to_positions 0x6e2000 (0x76d035,
0x76d047) and to the place test at 0x76d092. The squad size is the byte +0x42 of the competition's squad record
(0x76c74e..0x76c767). For 26 places it sets 26 masks (0x76c771..0x76c84c), for 24 places 24 (0x76c870..0x76c92c),
and for any other size 22 (0x76c939..0x76c9de, with 0x76c860 and 0x76c865). A squad of 23 places (as in the 2002
World Cup qualifiers of Oceania) has a 23rd mask that nothing set: [esp + 0x9c] held what an earlier call left on the
stack (0xc2b0 with one Setup Game screen, 0xa570 with the other), the place took another player, and on day 26 of
2002 another squad went to the qualifier. The array has 50 words ([esp + 0x70] to [esp + 0xd3], the place loop
runs to the size byte, 0x76d384..0x76d394), so a size from 27 to 50 would leave more masks unset. Here the masks
from the 23rd to the 50th are 0x240 for the other sizes (0x76c939). That is the mask the original gives the last
places of each size it knows: places 19 to 22 of 22, 21 to 24 of 24 and 22 to 26 of 26 (0x76c82c..0x76c84c,
0x76c914..0x76c92c, 0x76c9c6..0x76c9de). So a squad of 23 places names 23 players, as it does in real life, and the
23rd place takes an outfield player as the 24-place squad's 23rd place does. A mask of 0 would leave the place empty
(0x76d092), so a squad of 23 would have 22 players. A mask of 0xffff would make it a fourth goalkeeper: the scorer
match_man_score_player_for_role 0x6e2f20 tests bit 1, the goalkeeper, first (0x6e30a0).
"""

# address: (the instruction in the original, C code)
INSNS = {
    0x6E19B9: ("mov eax, dword ptr [ebx + 0x1f]",
               "eax = rt_layout_fix(0u) ? R32(R32(ebx + 0x1fu)) : R32(ebx + 0x1fu);"),
    0x6EADA0: ("mov eax, ecx", "if (rt_layout_fix(1u)) memset(M + ecx, 0, 0x1beu); eax = ecx;"),
    0x5CE311: ("mov dword ptr [esp + 0x8a0], 0", "W32(esp + 0x8a0u, 0u); if (rt_layout_fix(2u)) W32(esp + 0x70u, 0u);"),
    0x84C266: ("mov eax, dword ptr [edi - 4]",
               "if (rt_layout_fix(3u)) memset(M + esp + 0x40u, 0, 0x4fu); eax = R32(edi - 4u);"),
    0x8BF420: ("lea ecx, [esp + 0x28]",
               "if (rt_layout_fix(4u)) memset(M + esp + 0x28u, 0, 0x50u); ecx = (uint32_t)(esp + 0x28u);"),
    0x76C939: ("mov word ptr [esp + 0x74], ax",
               "if (rt_layout_fix(5u)) for (uint32_t i = 0x9cu; i < 0xd4u; i += 2u) W16(esp + i, 0x240u); "
               "W16(esp + 0x74u, (uint16_t)eax);"),
}
