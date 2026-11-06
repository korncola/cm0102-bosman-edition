"""Reads of the original that depend on where memory lies (0.6.2 open item 138b; src/recomp_rt/win/layout_reads.c).

A base game (changing rules off) gave other results when only the Setup Game screen of the runtime changed (Setup
Game menu B, 25298081): the screen's blocks moved the database tables to other heap addresses, and the game read
an address as a number, a heap block that nothing had set and a stack slot that nothing had set. None of these is
a value of the game: the original's own layout is not fixed either (its heap is the CRT heap of the computer it runs
on). So each read takes a defined value in its place: the record's id for the address, and 0, as fresh memory, for
the unset bytes. The fixes act in every game, the base game too. The test switch RECOMP_LAYOUT_FIXES=keep, or
keep:captain, keep:players, keep:daily, turns them off (layout_reads.c).

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
"""

# address: (the instruction in the original, C code)
INSNS = {
    0x6E19B9: ("mov eax, dword ptr [ebx + 0x1f]",
               "eax = rt_layout_fix(0u) ? R32(R32(ebx + 0x1fu)) : R32(ebx + 0x1fu);"),
    0x6EADA0: ("mov eax, ecx", "if (rt_layout_fix(1u)) memset(M + ecx, 0, 0x1beu); eax = ecx;"),
    0x5CE311: ("mov dword ptr [esp + 0x8a0], 0", "W32(esp + 0x8a0u, 0u); if (rt_layout_fix(2u)) W32(esp + 0x70u, 0u);"),
}
