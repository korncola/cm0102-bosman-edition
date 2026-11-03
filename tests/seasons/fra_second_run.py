#!/usr/bin/env python3
"""France: a reserve side does not go up from the CFA into the National (open item 116, 0.7.0; season_fra_second.c).

Usage:
    python3 tests/seasons/fra_second_run.py <cm0102_recomp> <game folder> [--only on,off] [--jobs n] [--keep <folder>]
        [--last <year>]

Each run is a season jump of tests/seasons/rules_report_check.py with France alone on the November 2020 database
(changing rules off, 12 reserve sides in the CFA), from the 2020 start to the end of the season that starts in
--last (default 2021, to 1 June 2022), so the game plays the French season change (FraFirst::start_new_season 0x5bc4d0, which calls
FraFirst::exchange_cfa_national_clubs 0x5bc780). The 12 reserve sides of the database's French CFA keep the CFA
zone in A Lower Division, so they are in the list. The test switch RECOMP_TEST_FRA_SECOND_REP=1 gives each listed
French reserve side the highest reputation, so the original would send four of them up.
RECOMP_FRA_SECOND_LOG=1 logs the reserve sides held back and the clubs that go up. The runs:
    on    the shipped code: no reserve side goes up, four other clubs go up, and reserve sides stay in the CFA
    off   RECOMP_FRA_SECOND=off, the original's list: reserve sides go up (proves that the test sees the fault)
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
ENV = {"RECOMP_TEST_FRA_SECOND_REP": "1", "RECOMP_FRA_SECOND_LOG": "1"}
LAST = 2021


def jump(name, program, game, work, env):
    code, _ = rrc.run(program, game, LAST, 2020, os.path.join(DB, "November 2020"), ["France"], [], work, 7200, None,
                      dict(ENV, **env))
    fails = [] if code == 0 else [f"{name}: the jump ended with code {code}"]
    found, log = ufr.boxes(work)
    fails += [f"{name}: message box {b}" for b in found]
    up = re.findall(r"recomp: fra second: (.*) goes up to the National( \(a reserve side\))?", log)
    stays = re.findall(r"recomp: fra second: (.*) stays in the CFA", log)
    listed = re.findall(r"recomp: fra second: listed reserve side (.*), reputation", log)
    print(f"{name}: {len(up)} clubs up ({sum(1 for _, s in up if s)} reserve sides), {len(stays)} held back, "
          f"{len(listed)} reserve sides listed")
    if not listed:
        fails.append(f"{name}: no reserve side in the list of 0x5bc780 (no season change seen, or no case)")
    return fails, up, stays


def run_on(program, game, work):
    fails, up, stays = jump("on", program, game, work, {})
    seconds = [n for n, s in up if s]
    if seconds:
        fails.append(f"on: reserve sides went up to the National: {', '.join(seconds)}")
    if len(up) != 4:
        fails.append(f"on: {len(up)} clubs went up to the National, want 4")
    if not stays:
        fails.append("on: no reserve side stayed in the CFA")
    return fails


def run_off(program, game, work):
    fails, up, stays = jump("off", program, game, work, {"RECOMP_FRA_SECOND": "off"})
    if stays:
        fails.append(f"off: {len(stays)} reserve sides were held back with the switch off")
    if not [n for n, s in up if s]:
        fails.append("off: no reserve side went up with the original's list (the test does not see the fault)")
    return fails


RUNS = {"on": run_on, "off": run_off}


def main(argv):
    global LAST
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = argv[1], argv[2]
    only = rrc.arg(argv, "--only", ",".join(RUNS)).split(",")
    jobs = int(rrc.arg(argv, "--jobs", "1"))
    LAST = int(rrc.arg(argv, "--last", str(LAST)))
    keep = rrc.arg(argv, "--keep", None) or tempfile.mkdtemp(prefix="fra-second-")
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
