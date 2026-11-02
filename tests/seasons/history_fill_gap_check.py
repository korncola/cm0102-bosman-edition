#!/usr/bin/env python3
"""The gap rows of tools/seasons/history_fill.py for the October 2026 database (0.6.2 open items 97 and 98).

Usage:
    python3 -I tests/seasons/history_fill_gap_check.py [--databases <folder of database folders>]

Runs gap_rows on the October 2026 data alone and checks the plan rows it makes: the 2026 World Cup, the Africa Cup
of Nations 2025 and the Argentine Apertura 2026 (Belgrano, before the start) are added, and the 2025 Argentine rows
that the database holds get no researched row. The databases are in test-fixtures, outside the repo, so this check is
not in ctest. Exit code 1 on a failure.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools", "seasons"))
import history_fill as hf  # noqa: E402
import history_gaps as hg  # noqa: E402

DATABASES = "/Users/max/Projects/cm0102/test-fixtures/databases"


def data_folder(path):
    if os.path.isfile(os.path.join(path, "index.dat")):
        return path
    subs = [os.path.join(path, n) for n in sorted(os.listdir(path)) if os.path.isfile(os.path.join(path, n, "index.dat"))]
    return subs[0] if len(subs) == 1 else path


def main(argv):
    root = argv[argv.index("--databases") + 1] if "--databases" in argv else DATABASES
    folder = data_folder(os.path.join(root, "October 2026"))
    shifts = {r["database"]: r for r in hg.read_tsv(hg.SHIFTS)}
    plan, report = [], []
    hf.gap_rows("October 2026", 2026, folder, hf.research_rows(), hg.Data(os.path.join(root, "Original 3.9.68")),
                hf.aliases(), plan, report, int(shifts["October 2026"]["comp_history"]),
                hf.nation_shifts(hg.SHIFTS).get("October 2026", (0, {})))
    added = {(c[3], c[4]) for c in (p.split("\t") for p in plan) if c[0] == "add"}
    bad = []
    for want in (("FIFA World Cup", "2026"), ("African Cup of Nations", "2025"), ("Argentine Premier Division", "2026")):
        if want not in added:
            bad.append(f"no plan row for {want[0]} {want[1]}")
    if ("Argentine Premier Division", "2025") in added:
        bad.append("a researched row for the Argentine Premier Division 2025, which the database holds")
    print(("FAIL  " + "; ".join(bad)) if bad else f"ok    history fill gap rows of October 2026: {len(added)} added")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
