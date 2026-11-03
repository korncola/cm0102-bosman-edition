#!/usr/bin/env python3
"""The free Champions League places of the access list in a game with fewer UEFA nations (0.6.2 open item 94).

Usage:
    python3 tests/seasons/uefa_free_places_run.py <cm0102_recomp> <game folder> [--keep <folder>]

The run loads a save of June 2026 (the original data 3.9.68 jumped to 2025/26, England and Spain, changing rules
on, Tier A, seed 1: test-fixtures/uefa-free-places-2026.sav, or RECOMP_FREE_PLACES_SAVE) and plays 20 days in Tier
A, past the UEFA places of day 171, then saves. The original data has 52 UEFA nations and the access list of
2026/27 has 55 ranks, so the Champions Path has free places. The cascade of
src/recomp_rt/win/season_uefa_seeding.c fills each with the next club in league order of the best-ranked nation
under its association limit. The check: the Champions League clubs of each nation, title holders apart, are the
first clubs of its top division of the last season, with no club left out (places 1 to n, a title holder's place
apart). On main ec04eb89 the filler came after the Europa League and Conference League places and took Spain's
7th and 9th, past its 5th and 6th. The log must show each free place. A run takes about 3 to 6 minutes.
"""
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "future"))
import history_check as hc  # noqa: E402
import rules_report_check as rrc  # noqa: E402
import soak_run  # noqa: E402

FIXTURES = "/Users/max/Projects/cm0102/test-fixtures"
DAYS = 20


def gaps(game):
    """[(nation, [(place, club)], missing places)] for each nation whose Champions League clubs skip a club of
    its top division of the last season."""
    rules = hc.Rules(os.path.join(HERE, "future", "competitions.tsv"),
                     os.path.join(HERE, "future", "history_names.tsv"))
    cl = {cid for cid, c in game.comps.items() if c["kind"] == "club" and rules.key(c["name"]) == "uefa_cl"}
    top_year = max((y for cid, y, *_ in game.rows if cid in cl), default=0)
    holders = {a for cid, y, a, *_ in game.rows
               if rules.key(game.comps[cid]["name"]) in ("uefa_cl", "uefa_el") and y >= top_year}
    top = {}                                  # nation: the highest level of a division that holds clubs
    for div, last, mark, place in game.divisions.values():
        c = game.comps.get(last)
        if c and c["kind"] == "club" and c["nation"] >= 0:
            top[c["nation"]] = max(top.get(c["nation"], -1), c["level"])
    by, held = {}, {}
    for club, (div, last, mark, place) in game.divisions.items():
        c = game.comps.get(last)
        if not (c and c["nation"] in top and c["level"] == top[c["nation"]]):
            continue
        if club in holders:
            held.setdefault(c["nation"], set()).add(place)
        elif mark in cl:
            by.setdefault(c["nation"], []).append((place, game.clubs[club][0]))
    bad = []
    for nation, clubs in sorted(by.items()):
        places = sorted(p for p, _ in clubs)
        missing = sorted(set(range(1, places[-1] + 1)) - set(places) - held.get(nation, set()))
        if missing:
            bad.append((game.nations.get(nation, ("?",))[0], sorted(clubs), missing))
    return bad


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game_dir = os.path.abspath(argv[1]), os.path.abspath(argv[2])
    work = rrc.arg(argv, "--keep", None) or tempfile.mkdtemp(prefix="uefa-free-")
    save = os.environ.get("RECOMP_FREE_PLACES_SAVE") or os.path.join(FIXTURES, "uefa-free-places-2026.sav")
    if not os.path.exists(save):
        print(f"FAIL  missing: {save}")
        return 1
    os.environ["RECOMP_TIER_A"] = "1"           # the game of the save was played in Tier A
    o = {"program": program, "game": game_dir, "rules": "on", "database": None, "seed": 1, "last": 2026,
         "timeout": 3600}
    code, took, box = soak_run.load_leg(o, os.path.join(work, "leg"), save, DAYS)
    out = os.path.join(work, "leg", "user", "samesave.sav")
    fails = []
    if code != 0 or box or not os.path.exists(out):
        fails.append(f"the run ended with code {code}{', box ' + box if box else ''} and no save ({work})")
    else:
        game = hc.Game(out)
        if game.date < (2026, 172):
            fails.append(f"the save of day {game.date[1]} of {game.date[0]} is before the UEFA places")
        for nation, clubs, missing in gaps(game):
            fails.append(f"{nation}: Champions League places of {', '.join(f'{p} {n}' for p, n in clubs)}; "
                         f"no place for {', '.join(map(str, missing))} of the same table")
        with open(os.path.join(work, "leg", "stderr.txt"), encoding="latin-1") as f:
            log = f.read()
        lines = re.findall(r"recomp: uefa: Champions League 2026/27: a free place .*", log)
        if not lines:
            fails.append(f"no log line of a free Champions League place ({work}/leg/stderr.txt)")
        for line in lines:
            print(line)
    for f in fails:
        print("FAIL ", f)
    print("PASS" if not fails else f"FAILED: {len(fails)}")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
