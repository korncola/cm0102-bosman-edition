#!/usr/bin/env python3
"""The 2024/25 UEFA coefficients of the 2025/26 databases (0.6.2 open item 106).

Usage:
    python3 tests/seasons/uefa_coeffs_run.py <cm0102_recomp> <game folder> [--only <name>,...] [--keep <folder>]

"2025 Data Update" (December 2025) and "May 2026 Data Update" (May 2026) hold 0 at nation +0x108, the season
before the start, for every nation. At the database load the game writes the 2024/25 values of the October 2026
database there (src/recomp_rt/win/season_coeffs.c, the table of docs/research/2026-11-03-database-coefficients.tsv).

Each run (dec2025, may2026, reshift) starts a 2025 game on one database with England (rules_report_check.py), saves one day after the start
and ends. The checks: code 0 and no message box; the log line of the fill (56 nations written, none kept); in the
save, +0x108 of every nation of the table holds its value (England 29.464, Spain 23.892); the two European
Performance Spots of 2025/26 go to the nations with the best 2024/25 values, England and Spain (as in the real
2025/26), read at +0x108; Spain ranks 3rd by the five seasons of 2025/26 (+0xe0..+0x100) and of 2026/27
(+0xe8..+0x108, the totals of the first season change, roll_coefficients 0x903b50) and has five Champions League
clubs in 2025/26: four by its rank and one European Performance Spot. A run takes about 3 minutes.
"""
import csv
import os
import re
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "future"))
sys.path.insert(0, os.path.join(HERE, "..", "smoke"))
import rules_report_check as rrc  # noqa: E402
import uefa_formats_run as ufr  # noqa: E402
import history_check as hc  # noqa: E402

DB = "/Users/max/Projects/cm0102/test-fixtures/databases"
TSV = os.path.join(HERE, "..", "..", "docs", "research", "2026-11-03-database-coefficients.tsv")
# run: (database, more environment). reshift: the test switch of season_dates.c makes the shift for 2030 and then
# for 2025 again, so the fill is undone and made again (three fills in the log, the values as after one).
RUNS = {"dec2025": ("December 2025", {}), "may2026": ("May 2026", {}),
        "reshift": ("December 2025", {"RECOMP_DATES_RESHIFT": "2030"})}
EPS_VALUES = {"England": 29.464, "Spain": 23.892}


def ranks(nations, first):
    """[(name)] of the UEFA nations of a save by the total of five doubles from nation +first, best first."""
    rows = sorted(((-sum(struct.unpack_from("<5d", r, first)), name) for name, r in nations.items()))
    return [name for _, name in rows]


def check(name, database, env, work):
    import smoke_run
    fails = []
    code, _ = rrc.run(PROGRAM, GAME, 2025, 2025, os.path.join(DB, database), ["England"], [], work, 3600, None,
                      dict({"RECOMP_SAVE_AFTER_DAYS": "1"}, **env))
    if code != 0:
        fails.append(f"{name}: the game ended with code {code}")
    found, log = ufr.boxes(work)
    fails += [f"{name}: message box {b}" for b in found]
    with open(TSV, encoding="utf-8") as f:
        want = [r for r in csv.DictReader(f, delimiter="\t") if r["database"] == database]
    if not want:
        fails.append(f"{name}: no row of {database} in {TSV}")
    line = f"season coeffs: {database}: the season before the start written for {len(want)} nations, 0 kept"
    fills = 3 if "RECOMP_DATES_RESHIFT" in env else 1
    if log.count(line) != fills or log.count("season coeffs:") != fills:
        fails.append(f"{name}: not {fills} log lines {line!r} and no other")
    eps = re.findall(r"European Performance Spot 2025/26: rank (\d+) \(([0-9.]+) at \+0x([0-9a-f]+)\)", log)
    got = sorted((float(v), at) for _, v, at in eps)
    if got != sorted((v, "108") for v in EPS_VALUES.values()):
        fails.append(f"{name}: the European Performance Spots of 2025/26 are {eps}, not England and Spain at +0x108")
    save = os.path.join(work, "user", smoke_run.SAVE_NAME)
    if not os.path.exists(save):
        return fails + [f"{name}: no save ({save})"]
    g = hc.Game(save)
    nat = g.sav.read("nation.dat")
    recs = {hc.s32(nat, i * hc.NATION): nat[i * hc.NATION:(i + 1) * hc.NATION] for i in range(len(nat) // hc.NATION)}
    for r in want:
        rec = recs.get(int(r["nation_id"]))
        v = struct.unpack_from("<d", rec, 0x108)[0] if rec else None
        if v != float(r["value_0x108"]):
            fails.append(f"{name}: {r['nation']} has {v} at +0x108, not {r['value_0x108']}")
    uefa = {hc.text(rec, 4): rec for rec in recs.values() if g.continents.get(hc.s32(rec, 0x71)) == "UEFA"}
    for first, season in ((0xe0, "2025/26"), (0xe8, "2026/27")):
        order = ranks(uefa, first)
        if order.index("Spain") != 2:
            fails.append(f"{name}: Spain ranks {order.index('Spain') + 1} by the totals of {season}: {order[:6]}")
    spain = [k for k, (n, _) in g.nations.items() if n == "Spain"][0]
    cl = {k for k, c in g.comps.items() if c["name"] in ("European Champions Cup", "UEFA Champions League")}
    clubs = sorted(g.clubs[c][0] for c, (_, _, mark, _) in g.divisions.items()
                   if mark in cl and g.clubs[c][1] == spain)
    if len(clubs) != 5:
        fails.append(f"{name}: Spain has {len(clubs)} Champions League clubs in 2025/26, not 5: {clubs}")
    print(f"{name}: Spain's Champions League clubs: {', '.join(clubs)}")
    return fails


def main(argv):
    global PROGRAM, GAME
    if len(argv) < 2:
        print(__doc__)
        return 2
    PROGRAM, GAME = os.path.abspath(argv[0]), os.path.abspath(argv[1])
    only = rrc.arg(argv, "--only", "").split(",") if "--only" in argv else list(RUNS)
    if any(n not in RUNS for n in only):
        print(f"--only: the runs are {', '.join(RUNS)}")
        return 2
    keep = rrc.arg(argv, "--keep")
    fails = []
    for name in only:
        work = os.path.join(keep, name) if keep else tempfile.mkdtemp(prefix=f"cm0102-coeffs-{name}-")
        f = check(name, *RUNS[name], work)
        print(f"{'PASS' if not f else 'FAIL'}  {name} ({work})")
        fails += f
    for f in fails:
        print(f"FAIL  {f}")
    print("PASS" if not fails else f"FAILED: {len(fails)}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
