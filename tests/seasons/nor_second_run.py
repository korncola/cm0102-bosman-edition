#!/usr/bin/env python3
"""Norway: a second team does not go up into the First Division (open item 111, 0.7.0; season_nor_second.c).

Usage:
    python3 tests/seasons/nor_second_run.py <cm0102_recomp> <game folder> [--only on,off,old1,old2] [--jobs n]
        [--keep <folder>]

Each run is a season jump of tests/seasons/rules_report_check.py with Norway alone on the October 2024 database
(changing rules off), from the 2024 start to 1 June 2025, so the game plays the Norwegian season change of
January 2025 (NorPrm::start_new_season 0x7927a0, which calls 0x792960). The test switch
RECOMP_TEST_NOR_SECOND_REP=1 gives each second team of the eight groups of the Second Division the highest
reputation, so the original would send four of them up. RECOMP_NOR_SECOND_LOG=1 logs the clubs that stay out and
the clubs that go up. The runs:
    on    the shipped code: no second team goes up, four other clubs go up, and at least one second team stays
          (each one back in its group of the Second Division after the call)
    off   RECOMP_NOR_SECOND=off, the original's list: second teams go up (proves that the test sees the fault)
    old1  the state of an old save, made in memory (no save is written or loaded): RECOMP_TEST_NOR_SECOND_OLD=1
          plays as the original until the season change of January 2025 is done, so second teams go up into the
          First Division, and RECOMP_TEST_NOR_SECOND_NO_MARK=1 lets its 2025 table give them places as the original
          (as a save made after the table was complete). To 1 June 2026: the wrap of the play-off hands a place on,
          at the change of January 2026 every second team goes down, the First Division keeps its size, and no
          second team goes up
    old2  the same (with the marks) and the original until January 2026, to 1 June 2027: the 2026 season plays with the fix, and
          at the change of January 2027 every second team leaves the First Division and those of Eliteserien go down
          (one for each club of Eliteserien that stays up)
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
ENV = {"RECOMP_TEST_NOR_SECOND_REP": "1", "RECOMP_NOR_SECOND_LOG": "1"}


def jump(name, program, game, work, env, last=2024):
    code, _ = rrc.run(program, game, last, 2024, os.path.join(DB, "October 2024"), ["Norway"], [], work, 7200, None,
                      dict(ENV, **env))
    fails = [] if code == 0 else [f"{name}: the jump ended with code {code}"]
    found, log = ufr.boxes(work)
    fails += [f"{name}: message box {b}" for b in found]
    up = re.findall(r"recomp: nor second: (.*) goes up to the First Division( \(a second team\))?", log)
    stays = re.findall(r"recomp: nor second: (.*) stays in (?!Eliteserien one more season)(.*)", log)
    lost = [n for n, d in stays if not d.startswith("Norwegian Second Division Group")]
    if lost:
        fails.append(f"{name}: second teams held back lost their group: {', '.join(lost)}")
    return fails, up, stays, log


CENSUS = re.compile(r"recomp: nor second: census (before|after): Eliteserien (\d+) clubs, (\d+) second teams; "
                    r"First Division (\d+) clubs, (\d+) second teams")


def changes(log):
    """The log of each Norwegian season change: (census before, census after, lines between)."""
    out, cur = [], None
    for line in log.splitlines():
        m = CENSUS.search(line)
        if m and m.group(1) == "before":
            cur = [tuple(map(int, m.groups()[1:])), None, []]
        elif m and cur:
            cur[1] = tuple(map(int, m.groups()[1:]))
            out.append(cur)
            cur = None
        elif cur and "recomp: nor second:" in line:
            cur[2].append(line)
    return out


def run_old(name, program, game, work, n, last, no_mark=False):
    env = {"RECOMP_TEST_NOR_SECOND_OLD": str(n), **({"RECOMP_TEST_NOR_SECOND_NO_MARK": "1"} if no_mark else {})}
    fails, _, _, log = jump(name, program, game, work, env, last)
    seen = changes(log)
    if len(seen) < n + 1:
        return fails + [f"{name}: {len(seen)} Norwegian season changes seen, want {n + 1}"]
    if not any("goes up to the First Division (a second team)" in l for l in seen[0][2]):
        fails.append(f"{name}: no second team went up as the original in the first change (no old save made)")
    before, after, lines = seen[n]
    print(f"{name}: change {n + 1}: before {before}, after {after} (Eliteserien clubs, second teams; First clubs, "
          f"second teams)")
    if before[3] == 0:
        fails.append(f"{name}: no second team in the First Division before change {n + 1} (the test sees no case)")
    if after[3]:
        fails.append(f"{name}: {after[3]} second teams still in the First Division after change {n + 1}")
    if after[2] != before[2] or after[0] != before[0]:
        fails.append(f"{name}: division sizes changed: {before} to {after}")
    if after[1] > before[1] or (before[1] and after[1] == before[1]):
        fails.append(f"{name}: Eliteserien second teams {before[1]} before, {after[1]} after")
    went = [l for l in lines if "(a second team)" in l]
    if went:
        fails.append(f"{name}: second teams went up: {went}")
    if no_mark and not any("in place of a second team before the play-off" in l for l in log.splitlines()):
        fails.append(f"{name}: no place was handed on before the play-off (the old-save path is not seen)")
    if not any("goes down from the First Division" in l for l in lines):
        fails.append(f"{name}: no second team went down from the First Division")
    if n == 2 and not before[1]:
        fails.append(f"{name}: no second team in Eliteserien before change 3 (the Eliteserien case is not seen)")
    return fails


def run_on(program, game, work):
    fails, up, stays, _ = jump("on", program, game, work, {})
    seconds = [n for n, s in up if s]
    if seconds:
        fails.append(f"on: second teams went up to the First Division: {', '.join(seconds)}")
    if len(up) != 4:
        fails.append(f"on: {len(up)} clubs went up to the First Division, want 4")
    if not stays:
        fails.append("on: no second team stayed in the Second Division (no season change seen)")
    return fails


def run_off(program, game, work):
    fails, up, stays, _ = jump("off", program, game, work, {"RECOMP_NOR_SECOND": "off"})
    if stays:
        fails.append(f"off: {len(stays)} second teams were held back with the switch off")
    if not [n for n, s in up if s]:
        fails.append("off: no second team went up with the original's list (the test does not see the fault)")
    return fails


RUNS = {"on": run_on, "off": run_off,
        "old1": lambda p, g, w: run_old("old1", p, g, w, 1, 2025, no_mark=True),
        "old2": lambda p, g, w: run_old("old2", p, g, w, 2, 2026)}


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = argv[1], argv[2]
    only = rrc.arg(argv, "--only", ",".join(RUNS)).split(",")
    jobs = int(rrc.arg(argv, "--jobs", "1"))
    keep = rrc.arg(argv, "--keep", None) or tempfile.mkdtemp(prefix="nor-second-")
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
