#!/usr/bin/env python3
"""Norway: a second team does not go up into the First Division (open item 111, 0.7.0; season_nor_second.c).

Usage:
    python3 tests/seasons/nor_second_run.py <cm0102_recomp> <game folder> [--only on,off] [--jobs n] [--keep <folder>]

Each run is a season jump of tests/seasons/rules_report_check.py with Norway alone on the October 2024 database
(changing rules off), from the 2024 start to 1 June 2025, so the game plays the Norwegian season change of
January 2025 (NorPrm::start_new_season 0x7927a0, which calls 0x792960). The test switch
RECOMP_TEST_NOR_SECOND_REP=1 gives each second team of the eight groups of the Second Division the highest
reputation, so the original would send four of them up. RECOMP_NOR_SECOND_LOG=1 logs the clubs that stay out and
the clubs that go up. The runs:
    on    the shipped code: no second team goes up, four other clubs go up, and at least one second team stays
    off   RECOMP_NOR_SECOND=off, the original's list: second teams go up (proves that the test sees the fault)
Both runs end with code 0 and show no message box of the game.
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


def jump(name, program, game, work, env):
    code, _ = rrc.run(program, game, 2024, 2024, os.path.join(DB, "October 2024"), ["Norway"], [], work, 7200, None,
                      dict(ENV, **env))
    fails = [] if code == 0 else [f"{name}: the jump ended with code {code}"]
    found, log = ufr.boxes(work)
    fails += [f"{name}: message box {b}" for b in found]
    up = re.findall(r"recomp: nor second: (.*) goes up to the First Division( \(a second team\))?", log)
    stays = re.findall(r"recomp: nor second: (.*) stays in the Second Division", log)
    return fails, up, stays


def run_on(program, game, work):
    fails, up, stays = jump("on", program, game, work, {})
    seconds = [n for n, s in up if s]
    if seconds:
        fails.append(f"on: second teams went up to the First Division: {', '.join(seconds)}")
    if len(up) != 4:
        fails.append(f"on: {len(up)} clubs went up to the First Division, want 4")
    if not stays:
        fails.append("on: no second team stayed in the Second Division (no season change seen)")
    return fails


def run_off(program, game, work):
    fails, up, stays = jump("off", program, game, work, {"RECOMP_NOR_SECOND": "off"})
    if stays:
        fails.append(f"off: {len(stays)} second teams were held back with the switch off")
    if not [n for n, s in up if s]:
        fails.append("off: no second team went up with the original's list (the test does not see the fault)")
    return fails


RUNS = {"on": run_on, "off": run_off}


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
