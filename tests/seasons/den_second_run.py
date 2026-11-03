#!/usr/bin/env python3
"""Denmark: no second team in the national tournament (open item 119, 0.7.0; season_den_second.c).

Usage:
    python3 tests/seasons/den_second_run.py <cm0102_recomp> <game folder> [--only on,off,y2007,old] [--jobs n]
        [--keep <folder>]

Each run is a season jump of tests/seasons/rules_report_check.py with Denmark alone on the October 2024 database
(changing rules on: the rule acts only there), from the 2024 start, so the game plays the Danish season change of
summer 2025 (DenPrm::start_new_season 0x553aa0, which calls the chain and then 0x553cb0). The test switch
RECOMP_TEST_DEN_SECOND_REP=1 gives each second team of A Lower Division (Aalborg BK II, Aarhus Fremad II, Hobro IK II,
"Naestved IF (Naestved BK II)") the highest reputation, so the original would send some of them up into the 2nd
Division. RECOMP_DEN_SECOND_LOG=1 logs the clubs held, moved and a census of each change. The runs:
    on     the shipped code (rule of 2025/26, Herre-DM § 7.1): no second team goes up into the 2nd Division, four
           other clubs go up, and all four second teams stay in A Lower Division
    off    RECOMP_DEN_SECOND=off, the original's list: a second team goes up (proves that the test sees the fault)
    y2007  RECOMP_TEST_DEN_SECOND_YEAR=2007, the rule of 2005/06 to 2009/10: Aalborg BK II (Aalborg BK plays the
           Superliga) may go up into the 2nd Division and is not held; the three others are held
    old    the state of a long game of an older build, made in memory (no save is written or loaded):
           RECOMP_TEST_DEN_SECOND_OLD=1 plays the change of 2025 as the original (which sends some second teams up
           into the 2nd Division), then RECOMP_TEST_DEN_SECOND_PLACE=1 puts the second teams left in A Lower Division
           into the Superliga, the 1st and the 2nd Division in turn in exchange for the weakest club of each, and RECOMP_TEST_DEN_SECOND_NO_MARK=1 lets the 2025/26 tables give them places as the original
           (as a save made after the table was complete). At the change of 2026 every second team leaves the three
           divisions, no division changes its size, and no second team goes up
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
ENV = {"RECOMP_TEST_DEN_SECOND_REP": "1", "RECOMP_DEN_SECOND_LOG": "1"}
SECONDS = {"Aalborg BK II", "Aarhus Fremad II", "Hobro IK II", "Naestved IF (Naestved BK II)"}
CENSUS = re.compile(r"recomp: den second: census (before|after): Superliga (\d+) clubs, (\d+) second teams; "
                    r"1st Division (\d+) clubs, (\d+) second teams; 2nd Division (\d+) clubs, (\d+) second teams")


def jump(name, program, game, work, env, last=2025):
    code, _ = rrc.run(program, game, last, 2024, os.path.join(DB, "October 2024"), ["Denmark"], [], work, 7200,
                      {"changing_rules": "on"}, dict(ENV, **env))
    fails = [] if code == 0 else [f"{name}: the jump ended with code {code}"]
    found, log = ufr.boxes(work)
    fails += [f"{name}: message box {b}" for b in found]
    return fails, log


def changes(log):
    """The log of each Danish season change: (census before, census after, lines between)."""
    out, cur = [], None
    for line in log.splitlines():
        m = CENSUS.search(line)
        if m and m.group(1) == "before":
            cur = [tuple(map(int, m.groups()[1:])), None, []]
        elif m and cur:
            cur[1] = tuple(map(int, m.groups()[1:]))
            out.append(cur)
            cur = None
        elif cur and "recomp: den second:" in line:
            cur[2].append(line)
    return out


def ups(lines):
    return re.findall(r"recomp: den second: (.*) goes up to the 2nd Division( \(a second team\))?$", "\n".join(lines),
                      re.M)


def held(lines):
    return {m for m in re.findall(r"recomp: den second: (.*) stays in A Lower Division$", "\n".join(lines), re.M)}


def first_change(name, log, fails):
    seen = changes(log)
    if not seen:
        fails.append(f"{name}: no Danish season change seen")
        return None
    before, after, lines = seen[0]
    print(f"{name}: change 1: before {before}, after {after}")
    if after[0::2] != before[0::2]:
        fails.append(f"{name}: division sizes changed: {before} to {after}")
    return lines


def run_on(program, game, work):
    fails, log = jump("on", program, game, work, {})
    lines = first_change("on", log, fails)
    if lines is None:
        return fails
    up = ups(lines)
    if [n for n, s in up if s]:
        fails.append(f"on: second teams went up: {[n for n, s in up if s]}")
    if len(up) != 4:
        fails.append(f"on: {len(up)} clubs went up to the 2nd Division, want 4")
    if held(lines) != SECONDS:
        fails.append(f"on: held {sorted(held(lines))}, want {sorted(SECONDS)}")
    return fails


def run_off(program, game, work):
    fails, log = jump("off", program, game, work, {"RECOMP_DEN_SECOND": "off"})
    lines = first_change("off", log, fails)
    if lines is None:
        return fails
    if held(lines):
        fails.append(f"off: {len(held(lines))} second teams held with the switch off")
    if not [n for n, s in ups(lines) if s]:
        fails.append("off: no second team went up with the original's list (the test does not see the fault)")
    return fails


def run_y2007(program, game, work):
    fails, log = jump("y2007", program, game, work, {"RECOMP_TEST_DEN_SECOND_YEAR": "2007"})
    lines = first_change("y2007", log, fails)
    if lines is None:
        return fails
    want = SECONDS - {"Aalborg BK II"}
    if held(lines) != want:
        fails.append(f"y2007: held {sorted(held(lines))}, want {sorted(want)}")
    bad = [n for n, s in ups(lines) if s and n != "Aalborg BK II"]
    if bad:
        fails.append(f"y2007: barred second teams went up: {bad}")
    if len(ups(lines)) != 4:
        fails.append(f"y2007: {len(ups(lines))} clubs went up to the 2nd Division, want 4")
    return fails


def run_old(program, game, work):
    env = {"RECOMP_TEST_DEN_SECOND_OLD": "1", "RECOMP_TEST_DEN_SECOND_PLACE": "1",
           "RECOMP_TEST_DEN_SECOND_NO_MARK": "1"}
    fails, log = jump("old", program, game, work, env, last=2026)
    # The original's change of 2025 sends some second teams up into the 2nd Division by itself, so the switch may
    # find fewer than three in A Lower Division to place. The Superliga and the 1st Division need one each.
    placed = re.findall(r"recomp: den second: test: (.*) placed in (.*) for", log)
    if not {"the Superliga", "the 1st Division"} <= {w for _, w in placed}:
        fails.append(f"old: placed {placed}, want one in the Superliga and one in the 1st Division (no old save made)")
    seen = changes(log)
    if len(seen) < 2:
        return fails + [f"old: {len(seen)} Danish season changes seen, want 2"]
    before, after, lines = seen[1]
    print(f"old: change 2: before {before}, after {after} (clubs, second teams of the Superliga, 1st, 2nd Division)")
    if before[1] + before[3] + before[5] == 0:
        fails.append("old: no second team in the three divisions before change 2 (the test sees no case)")
    for k, what in ((1, "the Superliga"), (3, "the 1st Division"), (5, "the 2nd Division")):
        if before[k] == 0:
            fails.append(f"old: no second team in {what} before change 2 (that case is not seen)")
        if after[k]:
            fails.append(f"old: {after[k]} second teams still in {what} after change 2")
    if after[0::2] != before[0::2]:
        fails.append(f"old: division sizes changed: {before} to {after}")
    if [n for n, s in ups(lines) if s]:
        fails.append(f"old: second teams went up: {[n for n, s in ups(lines) if s]}")
    return fails


RUNS = {"on": run_on, "off": run_off, "y2007": run_y2007, "old": run_old}


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = argv[1], argv[2]
    only = rrc.arg(argv, "--only", ",".join(RUNS)).split(",")
    jobs = int(rrc.arg(argv, "--jobs", "1"))
    keep = rrc.arg(argv, "--keep", None) or tempfile.mkdtemp(prefix="den-second-")
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
