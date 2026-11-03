#!/usr/bin/env python3
"""Italy: a second team never plays in its first team's division or above (open item 117, 0.7.0;
season_ita_second.c).

Usage:
    python3 tests/seasons/ita_second_run.py <cm0102_recomp> <game folder> [--only on,off,old,d2023,d2025] [--jobs n]
        [--keep <folder>]

Each run is a season jump of tests/seasons/rules_report_check.py with Italy (and England) on an Update Team
database, so the game plays the Italian season change in the summer (the chain of the Serie C era,
season_divisions_ita.c). RECOMP_ITA_SECOND_LOG=1 logs a census of every second team and its first team before and
after each change, and every move. Test switches: RECOMP_TEST_ITA_SECOND_TOP=1 puts the first second team of Serie B
and of each Serie C group at the top of its table; RECOMP_TEST_ITA_SECOND_BOTTOM=1 puts it at the bottom of its
group; RECOMP_TEST_ITA_SECOND_DOWN=<clubs> sends those Serie A clubs down; RECOMP_TEST_ITA_SECOND_OLD=<n> plays the
original's moves for the first n changes. The runs:
    on     October 2024, 2024 start, to the end of 2025/26. TOP, and Juventus FC, Atalanta BC and AC Milan go down.
           At the change of 2025 each second team wins its group but may not go up (its first team now plays Serie
           B); the next club of its group goes up. After the change no second team plays its first team's division
           or above
    off    the same with RECOMP_ITA_SECOND=off: second teams go up into their first team's division (proves that
           the test sees the fault)
    old    an old save, made in memory (no save is written or loaded): TOP, Atalanta BC goes down, and the
           original's moves at the change of 2025, so second teams reach Serie B (Atalanta U23 with Atalanta). At
           the change of 2026 the second team at the top of Serie B does not go up to Serie A, and the second team
           in its first team's division goes down
    d2023  October 2022, 2022 start: BOTTOM; Juventus Next Gen goes down at the change of 2023 and, before
           2025/26, plays no Serie D (no division)
    d2025  October 2024, 2024 start: BOTTOM; the second teams go down at the change of 2025 and play the Serie D
All runs end with code 0 and show no message box of the game.
"""
import concurrent.futures
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import rules_report_check as rrc  # noqa: E402
import uefa_formats_run as ufr  # noqa: E402

DB = "/Users/max/Projects/cm0102/test-fixtures/databases"
TIERS = ["Serie A", "Serie B", "Serie C", "the Serie D", "no division"]
CENSUS = re.compile(r"recomp: ita second: census (before|after): (.*) in (Serie A|Serie B|Serie C|the Serie D|"
                    r"no division), first team (.*) in (Serie A|Serie B|Serie C|the Serie D|no division)$")


def jump(name, program, game, work, env, year=2024, last=2025, database="October 2024"):
    code, _ = rrc.run(program, game, last, year, os.path.join(DB, database), ["Italy"], [], work, 7200, None,
                      dict({"RECOMP_ITA_SECOND_LOG": "1"}, **env))
    fails = [] if code == 0 else [f"{name}: the jump ended with code {code}"]
    found, log = ufr.boxes(work)
    fails += [f"{name}: message box {b}" for b in found]
    return fails, log


def changes(log):
    """Each Italian season change: (census before, census after, the lines between). A census is
    {second team: (its tier, first team's tier)}."""
    out, cur, phase = [], None, None
    for line in log.splitlines():
        m = CENSUS.search(line)
        if m:
            when = m.group(1)
            if when == "before" and phase != "before":
                cur = [{}, {}, []]
                out.append(cur)
            phase = when
            cur[0 if when == "before" else 1][m.group(2)] = (TIERS.index(m.group(3)), TIERS.index(m.group(5)))
        elif cur is not None and "recomp: ita second:" in line:
            cur[2].append(line)
    return out


def illegal(census):
    """The second teams in their first team's division or above (not the Serie D or no division)."""
    return [f"{n} ({TIERS[t]}, first team {TIERS[f]})" for n, (t, f) in census.items() if t <= f and t <= 2]


def run_on(program, game, work):
    env = {"RECOMP_TEST_ITA_SECOND_TOP": "1", "RECOMP_TEST_ITA_SECOND_DOWN": "Juventus FC,Atalanta BC,AC Milan"}
    fails, log = jump("on", program, game, work, env)
    seen = changes(log)
    if not seen:
        return fails + ["on: no Italian season change seen"]
    before, after, lines = seen[0]
    print(f"on: change 1: before {before}, after {after}")
    if not any("may not go up from Serie C" in l for l in lines):
        fails.append("on: no second team was held back in Serie C (the test sees no case)")
    if not any("takes the place in Serie C" in l for l in lines):
        fails.append("on: no club took the place of a second team in Serie C")
    for n, (t, f) in after.items():
        if f != 1:
            fails.append(f"on: the first team of {n} is in {TIERS[f]}, want Serie B (the test switch)")
    for c in seen:
        bad = illegal(c[1])
        if bad:
            fails.append(f"on: after a change: {', '.join(bad)}")
    return fails


def run_off(program, game, work):
    env = {"RECOMP_TEST_ITA_SECOND_TOP": "1", "RECOMP_TEST_ITA_SECOND_DOWN": "Juventus FC,Atalanta BC,AC Milan",
           "RECOMP_ITA_SECOND": "off"}
    fails, log = jump("off", program, game, work, env)
    seen = changes(log)
    if not seen:
        return fails + ["off: no Italian season change seen"]
    if any("may not go up" in l or "goes down from" in l and "test:" not in l for l in seen[0][2]):
        fails.append("off: a second team was held back or moved with the switch off")
    if not illegal(seen[0][1]):
        fails.append("off: no second team in its first team's division after the change (the test does not see "
                     "the fault)")
    print(f"off: change 1 after: {seen[0][1]}")
    return fails


def run_old(program, game, work):
    env = {"RECOMP_TEST_ITA_SECOND_TOP": "1", "RECOMP_TEST_ITA_SECOND_DOWN": "Atalanta BC",
           "RECOMP_TEST_ITA_SECOND_OLD": "1"}
    fails, log = jump("old", program, game, work, env, last=2026)
    seen = changes(log)
    if len(seen) < 2:
        return fails + [f"old: {len(seen)} Italian season changes seen, want 2"]
    before, after, lines = seen[1]
    print(f"old: change 2: before {before}, after {after}")
    if not [n for n, (t, f) in before.items() if t == 1]:
        fails.append("old: no second team in Serie B before change 2 (no old save made)")
    if not illegal(before):
        fails.append("old: no second team in its first team's division before change 2 (the case is not seen)")
    if not any("may not go up from Serie B" in l for l in lines):
        fails.append("old: no second team was held back in Serie B")
    if not any("goes down from Serie B" in l and "test:" not in l for l in lines):
        fails.append("old: no second team went down from Serie B")
    if [n for n, (t, f) in after.items() if t == 0]:
        fails.append(f"old: a second team in Serie A after change 2: {after}")
    bad = illegal(after)
    if bad:
        fails.append(f"old: after change 2: {', '.join(bad)}")
    return fails


def run_d(name, year, database, want):
    def run(program, game, work):
        fails, log = jump(name, program, game, work, {"RECOMP_TEST_ITA_SECOND_BOTTOM": "1"}, year=year, last=year + 1,
                          database=database)
        seen = changes(log)
        if not seen:
            return fails + [f"{name}: no Italian season change seen"]
        before, after, lines = seen[0]
        print(f"{name}: change 1: before {before}, after {after}")
        down = [l.split("test: ")[1].split(" ends last")[0] for l in lines if " ends last" in l]
        if not down:
            fails.append(f"{name}: no second team was put last (the test sees no case)")
        for n in down:
            if after.get(n, (None,))[0] != TIERS.index(want):
                fails.append(f"{name}: {n} is in {TIERS[after[n][0]] if n in after else '?'} after the change, "
                             f"want {want}")
        return fails
    return run


RUNS = {"on": run_on, "off": run_off, "old": run_old,
        "d2023": run_d("d2023", 2022, "October 2022", "no division"),
        "d2025": run_d("d2025", 2024, "October 2024", "the Serie D")}


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = argv[1], argv[2]
    only = rrc.arg(argv, "--only", ",".join(RUNS)).split(",")
    jobs = int(rrc.arg(argv, "--jobs", "1"))
    keep = rrc.arg(argv, "--keep", None) or tempfile.mkdtemp(prefix="ita-second-")
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as ex:
        futures = {n: ex.submit(RUNS[n], program, game, os.path.join(keep, n)) for n in only}
        fails = []
        for n in only:
            got = futures[n].result()
            print(f"{n}: {'PASS' if not got else 'FAIL'}")
            fails += got
    for f in fails:
        print("  " + f)
    print("PASS" if not fails else "FAIL")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
