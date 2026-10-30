# How the Bosman Edition is tested

This page explains how we test CM 01/02: Bosman Edition before each release. For each test it gives what it checks, when it runs, the pass rule and the script.

Each test runs at one or more of these points:

- each change, while a feature or fix is built
- each merge, when finished work joins the main branch
- each release, before Max decides to publish

## Code tests

Small test programs check single parts of the code, such as the options file, the season rules tables, the cup draws and the heap. They run with `ctest` in the `build/` folder.

They run at each change and at each merge. The pass rule is that every test passes.

## Same-save checks

The original game picks its results partly from where its data sits in memory. A new screen or option can move that data and so change later matches, even when the player makes the same clicks. Our rule is that a feature must not change the game, except for the team that the player picks.

A same-save check plays the same clicks twice, once with the option on and once with it off. Then it compares the two saves. It also plays one run again to show that the result repeats. The checks start from a saved game at the start of a season, from a saved game before a match, and from a new game.

The script is `tests/smoke/smoke_run.py --same-save`, with `--match` for the match check. `tests/smoke/same_save_views.py` checks the screens.

The checks run before each merge of game code and again on the merged build. The pass rule is equal saves. Five files can differ only when the option opens a screen that the off run does not open, because that moves memory. We record each such case and tell Max. Any other difference stops the merge until we find the cause.

## Start-season soaks

For each start season from 2001/02 to 2025/26, a soak plays a new game with all nations loaded and the changing rules on. The manager goes on holiday and the quick result engine plays the matches.

Each soak goes on until the game completes at least one World Cup and one European Championship, with their qualifiers and every season change between them. It stops at about 1 September after the later tournament. A 2001 start runs to September 2004, and a 2016 start runs to September 2021 because Euro 2020 was played in 2021.

Near each 1 July the game saves. A new copy of the program loads that save and plays on.

The scripts are `tests/seasons/soak_run.py` and `tests/seasons/rules_report_check.py`. The soaks run at each release on the final main branch. A fix that touches a season change gets the same run to the same end point. The pass rule is no error box, no stall and a save and load that work at each July.

## Full play

Full play uses the match engine, as a player sees it. It runs five starts across the eras: 2002, 2006, 2013, 2021 and 2025, with all nations. Each goes to the same end point as the soaks, with the same save and load near each 1 July.

The script is `tests/seasons/full_play_run.py`. It runs at each release. The pass rule is the same as for the soaks.

## Long games

Long games run from 2001/02 to 2027/28 with all nations and different random seeds, to find faults that build up over many seasons. They use `tests/seasons/soak_run.py` with `--last 2027`. They run when the machine has free time, and they do not hold back a release. A fault that they find is fixed in the same way as any other.

## Memory and big saves

A game with all nations uses more memory each season. The heap now has room for about 2 GB of game data. A code test (`tests/recomp/test_heap.c`) fills a small heap and checks that it still serves new blocks.

Saves can now be up to 4 GB. A save that would be larger stops with a clear message and keeps the earlier save. `tests/smoke/big_save_run.py` makes a save above 3 GB and loads it again, checks a season part above 2 GB, and checks the clean stop above 4 GB. It needs about 12 GB of free disk for a short time. These checks run at the merge of a memory or save change and in the long games at each release.

## Screen sweeps and smoke screenshots

The screen sweep opens the tables, results, fixtures, schedule and history of every competition, once each season. The data sweep reads each competition from memory at the start and end of each season. It looks for dates outside the season and for empty tables where the rules give clubs. The scripts are `tests/seasons/screen_sweep_run.py` and `tests/seasons/data_sweep_run.py`. They run at each release.

The smoke test plays a fixed set of clicks through the main screens in the classic and widescreen modes. It compares each screenshot with a reference picture. The script is `tests/smoke/smoke_run.py --mode classic` or `--mode widescreen`. It runs at each merge and at each release. The pass rule is no error box and pictures that match. When a change draws something new on purpose, we look at the picture and record a new reference.

## Online play

Network games (0.7.0) have their own tests. The two-game run (`tests/smoke/network_run.py`) starts a host game and joining games on this Mac with timed clicks, and compares the joiners' screens with the host's. It runs at each change of the network code, also under wine with the Windows build. The server tests in `server/` run against a local copy of the lobby and relay.

Before the release, four release tests run (spec Q29). Their scripts are in `tests/online/`. Each script waits for room on the machine, and for any network run of another worktree to end, because two network runs on one Mac collide on port 6667. Each one adds a line to `run-logs/s9-tests/results.tsv`. While several sessions test network play, each run that hosts a network game goes through `the maintainer's notes`, which lets one host at a time use port 6667.

Test 3 starts its workflow with a push of the branch `online-ci-test` (`github_join_run.py --push`), because GitHub starts a manual workflow only from the default branch, and `main` belongs to the 0.6.0 work. It needs no secret: the Mac puts the AppImage and a zip of the Windows program with its game folder on a draft release, which only people with write access see, and deletes it after the run.

| Test | Script | Pass rule |
|---|---|---|
| 1. Two and four games on this Mac through the real lobby and relay | `release_mac_run.py` | Each two-game run with 1 and 3 joiners through the relay passes. Without the relay in the build the script runs the lobby and LAN runs and reports STAGE. |
| 2. Dropped (Q45): the Windows 11 virtual machine joins a game on this Mac | `vm_join_run.py` (kept, not run) | This Mac's Parallels Desktop edition refuses `prlctl resume`, `start` and `exec`, which need the Pro or Business Edition. Test 3's GitHub Windows machine covers a real Windows machine. |
| 3. GitHub's Windows and Linux machines join a game on this Mac | `github_join_run.py --push`, `remote_join.py`, `.github/workflows/online-release-join.yml` | Both joiners pass their checks and their screens equal the host's within 2 colour levels. |
| 4. A four-player game through a World Cup, a Euros and every season change between them | `season_net_run.py` | The host plays a network game from 2001 to 1 September 2004 with the match engine on, one season a leg. Each leg restores the last save, and three joiners take their clubs back with their passwords. Leg 0 also has a drop and rejoin and a game of another version. The last leg ends with a host quit. No box, no stall, every division moves on. |

`tests/online/match_day_run.py` measures the traffic of a match day with two human managers. It passes when both games ran, the joiner made its manager and the host played a match. Its figures are in the network research note, section 16.

Results:

| Date | Test | Build | Result |
|---|---|---|---|
| 2026-10-27 | 4, short check: 1 leg of 3 days, England, LAN, local lobby (`--days 3 --legs 1`) | online-0.7.0 8b7b21bd | Leg 0 passed every check: four human managers with passwords, join3's drop (lobby 3 online) and rejoin with its password (4 online), the box of a game of version 0.0.1 that names the host's version, the host's holiday and its save after 3 game days. The restore leg then restored the save and Player Two took Barnsley back with the password. The quit leg waits for slice 6. |
| 2026-10-29 | 2 | | Dropped (Q45): `prlctl resume` answered "The command is available only in Parallels Desktop for Mac Pro or Business Edition". |
| 2026-10-27 | 3 | | Not run: the workflow is not on GitHub (a push waits for Max), and the build has no relay. |

| 2026-10-28 | Match day, two human managers, LAN | online-0.7.0 8b7b21bd | Measured days 16.7.01 to 19.7.01: 72 KB a minute to the joiner, one 6.2 KB screen for each of its clicks (research note section 16). FAIL as a match day: the joiner's own friendly did not start, and the host waited on Latest Scores. Open. |

| 2026-10-28 | 1, stage before slice 5 | online-0.7.0 8b7b21bd | STAGE: the lobby run (local copy) and the four-game LAN run pass. Against the real server the lobby run failed: the game's lobby client of this build speaks only `http://`, so the script now uses the local copy until the build has slice 5. |
| 2026-10-28 | 4, short check: 2 legs of 10 days, England, LAN, local lobby | online-0.7.0 8b7b21bd | STAGE: every check passes in both legs (the new game with the drop by kill, the rejoin and the version box; then the restore, where all three joiners took their clubs back with their passwords). The quit leg waits for slice 6, the relay for slice 5. |

| 2026-10-29 | 3, GitHub's Linux and Windows machines | online-0.7.0 7d60ee73 with slice 9 (05e55d19) | Both joined the Mac's game `00S9-CI` through the live lobby and relay (https://online.cm0102bosman.co.uk): each ended with code 0, joined through the relay, sent its 4 clicks, and its three screens equal the host's within 2 colour levels (the Mac's picture decode rounds otherwise; Windows and Linux are equal). The packages came from a draft release, deleted after the run; no secret. The run reported FAIL on two faults of its own checks (a tolerance of 0, and two joiners on one relay address counted once); both are fixed, and the same data passes. Run 40927613267. |

| 2026-10-29 | 4, the full run: 2001 to 4 September 2004, England, 10 legs of 120 days and a quit leg | online-0.7.0 6f12558e with slice 9 | PASS. Legs 0 and 1 over the LAN with a local lobby (leg 1 on slice 6's build with the send fix, after leg 1 on the earlier build found the "scrman 2480" box), legs 2 to 10 through the relay and the live lobby. Each leg: the three joiners took their clubs back with their passwords; every division moved on; the run crossed the World Cup 2002, the Euro 2004 and the season changes of 2002, 2003 and 2004. Quit leg: the host saved at its quit, and each joiner showed "The host left the game", then the start screen. Two test faults fixed on the way: Go on Holiday sits higher in the menu of a manager with no club (the board sacked the host in 2004), and the first quit leg's host stayed on Manager Status after Finish for 10 minutes with two joiners "Waiting" (not seen again in the second try; open). |

| 2026-10-30 | 4, the quit leg 10 times through the live relay | online-0.7.0 877cac08 (slice 5's relay close, slice 6's hold of the other joiners' packets) | 10 of 10 PASS: no hang, the host saved at its quit, and every joiner showed "The host left the game", then the start screen. On slice 6's build before these fixes, 1 of 5 passed (2 with "The connection to the host was lost", 2 with the hang after Finish). |

<!-- s9 results -->

## Platform checks

The game ships for Mac, Windows and Linux. At each release the release script builds all three packages. The Windows build runs its smoke test under wine on the Mac. The Linux build is made and checked in a linux/amd64 container. Each platform runs the smoke test in both screen modes and a season change with a save and load.

## Rules source checks

The season rules come from a table with one row per competition and season. `tools/research/rules_lint.py` checks that every filled fact has a source. `tests/seasons/rules_report_check.py` compares the competitions that the game holds each season with that table. Both run at each change to the rules and at each release. The pass rule is no missing source and no difference.

## Database checks

The duplicate check, `tests/seasons/history_duplicates_check.py`, lists the history rows that a season database holds twice. The game clears the extra copies when it loads the database. `tests/seasons/season_history_run.py --duplicates` checks that no copy is left in the game.

Our own season databases have two more checks. `tools/seasons/own_db/duplicates.py check <database>` fails while any player is listed twice (same nation, matching names and birth dates, or the same full name at one club). `tools/seasons/own_db/long_test.py` plays a holiday from July 2001 to 1 September 2004 on the database, with a save each 1 July that the next step loads. `tools/seasons/own_db/save_check.py` then checks each save: big clubs keep full squads, and no player or name appears twice. The pass rule is no error box and every check passes.

We also check nation names in every database, such as Yugoslavia, the Netherlands Antilles and Kosovo. Draws and qualifying pools must pick only nations that exist in that season.

## After-load sweeps

Many faults show only after a saved game loads. An after-load sweep reads each season module for data that the save does not keep, or that a load does not rebuild or clear. Each suspect place gets a test that saves before the risky moment, loads, and plays on. These sweeps run at each release that adds season code. The July save and load in the soaks and full play repeat this check on every run.

## Reviews

A fresh reviewer reads each change before it merges. A small fix gets one fresh review. A new slice of work gets two. Each problem found is fixed before the merge.

## The rules we keep

### Every failure is fixed

Every failure that any test finds is fixed before the release (decision O151). Each fix needs a passing retest and a same-save check. We do not ship a known issue or move a failure to a later version.

### Run length

Every holiday or soak run, and every retest of a fix that touches a season change, goes on to the end point above (decision O166).

### Machine limits

The tests run many games at once on one machine, so we keep these limits:

- at most 16 test games run at one time
- no new game starts while the machine load is 24 or more
- builds go through a queue, with at most three at once

A test that fails only under heavy load runs again alone before we treat it as a fault.

### Release approval

When every test passes, we write a test report for the release. Max reads it and approves every release himself.
