#!/usr/bin/env python3
"""Tier A against the normal game (0.6.2 slice 3, decision Q10): two saves of the same start, database, seed and
date, one from a Tier A run (RECOMP_TIER_A=1) and one from the normal game. They count as the same when they agree
on editions, dates, hosts and entrant rules. Winners can differ.

Usage:
    python3 -I tests/seasons/future/tier_a_compare.py <normal.sav> <tier_a.sav> [--out <file>]

The checks, from the history checker (history_check.py) of each save:
    save-date   both saves are of the same day
    editions    the history rows (competition, year) from the start year on, the edition that each finals class
                waits for, and the editions in the fixture list (competition, year)
    dates       the first and the last match day of each edition in the fixture list
    hosts       the host of each history row from the start year on
    entrants    per edition in the fixture list, the number of entrants, and the entrant faults of the checker
                (entrant-confed, -count, -host, -route, -holder, -dead: the year's access rule) are the same
Each difference is one line "DIFF <check> <competition> <year>: normal ... / Tier A ...". The exit code is 1 when
any check differs, or when a save gives no edition or the two saves have other start years.

The route of an entrant lists the honours its club holds, so it depends on the winners. So does the nation of a
title holder. The script lists the editions whose count of entrants per nation differs as NOTE lines, which do not
fail. Compare the saves of every July of the two runs, not only the last, so that a difference that clears again
is still found.
"""
import collections
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import history_check as hc  # noqa: E402


def facts(path):
    game = hc.Game(path)
    rules = hc.Rules(os.path.join(HERE, "competitions.tsv"), os.path.join(HERE, "history_names.tsv"))
    nations = hc.read_nations(os.path.join(HERE, "nations.tsv"))
    access = hc.Access(hc.finals_table(), hc.read_access(os.path.join(HERE, "access.tsv")))
    listing, faults = hc.entrants(game, rules, access, nations)
    name = lambda cid: game.comps[cid]["name"]                                 # noqa: E731
    f = {"date": game.date, "start": game.start}
    f["editions"] = {(name(cid), y) for cid, y, *_ in game.rows if y >= game.start}
    f["editions"] |= {(name(cid), "waits for", y) for cid, y in game.editions.items()}
    f["editions"] |= {(n, y, "fixtures") for _, n, y, *_ in listing}
    f["dates"] = {(n, y): (first, last) for _, n, y, first, last, *_ in listing}
    f["hosts"] = {(name(cid), y): game.club(h)[0] for cid, y, a, b, c, h in game.rows if y >= game.start and h >= 0}
    count, mix = collections.Counter(), collections.defaultdict(collections.Counter)
    for _, n, y, first, last, tid, team, nation, conf, route in listing:
        count[(n, y)] += 1
        mix[(n, y)][nation] += 1
    f["entrants"], f["nations"] = count, mix
    f["entrant faults"] = {(c, n, y) for c, n, y, d in faults}
    return f


def compare(a, b):
    diffs, notes = [], []
    if a["start"] != b["start"]:
        diffs.append(("start", "", "", f"{a['start']} / {b['start']}"))
    for f, who in ((a, "normal"), (b, "Tier A")):
        if not f["editions"]:
            diffs.append(("unread", "", "", f"the {who} save gives no edition"))
    if a["date"] != b["date"]:
        diffs.append(("save-date", "", "", f"{a['date']} / {b['date']}"))
    for e in sorted(a["editions"] ^ b["editions"], key=str):
        where = "normal only" if e in a["editions"] else "Tier A only"
        if len(e) == 3 and e[1] == "waits for":
            diffs.append(("editions", e[0], e[2], where + " (the class waits for this year)"))
        else:
            diffs.append(("editions", e[0], e[1], where + (" (fixture list)" if len(e) == 3 else " (history row)")))
    for key in sorted(set(a["dates"]) & set(b["dates"]), key=str):
        if a["dates"][key] != b["dates"][key]:
            diffs.append(("dates", key[0], key[1], f"{days(a['dates'][key])} / {days(b['dates'][key])}"))
    for key in sorted(set(a["hosts"]) | set(b["hosts"]), key=str):
        if a["hosts"].get(key) != b["hosts"].get(key):
            diffs.append(("hosts", key[0], key[1], f"{a['hosts'].get(key)} / {b['hosts'].get(key)}"))
    for key in sorted(set(a["entrants"]) & set(b["entrants"]), key=str):
        if a["entrants"][key] != b["entrants"][key]:
            diffs.append(("entrants", key[0], key[1], f"{a['entrants'][key]} / {b['entrants'][key]} entrants"))
        elif a["nations"][key] != b["nations"][key]:
            moved = (a["nations"][key] - b["nations"][key]) + (b["nations"][key] - a["nations"][key])
            notes.append(("nations", key[0], key[1], ", ".join(f"{n} {a['nations'][key][n]} / {b['nations'][key][n]}"
                                                            for n in sorted(moved))))
    for e in sorted(a["entrant faults"] ^ b["entrant faults"], key=str):
        diffs.append(("entrant faults", e[1], e[2], f"{e[0]}: " + ("normal only" if e in a["entrant faults"]
                                                                    else "Tier A only")))
    return diffs, notes


def days(pair):
    import datetime
    return "..".join(str(datetime.date.fromordinal(d)) for d in pair)


def main(argv):
    args = argv[1:]
    out = None
    if "--out" in args:
        i = args.index("--out")
        out = args[i + 1]
        del args[i:i + 2]
    if len(args) != 2:
        print(__doc__)
        return 2
    a, b = facts(args[0]), facts(args[1])
    diffs, notes = compare(a, b)
    counts = collections.Counter(d[0] for d in diffs)
    lines = [f"DIFF {c} {n} {y}: {d}" for c, n, y, d in diffs] + [f"NOTE {c} {n} {y}: {d}" for c, n, y, d in notes]
    lines.append(f"normal {args[0]}: start {a['start']}, save {a['date']}; Tier A {args[1]}: start {b['start']}, "
                 f"save {b['date']}; {len(a['editions'])} editions, {len(a['hosts'])} hosts, "
                 f"{len(a['entrants'])} entrant lists")
    lines.append("SAME" if not diffs else "DIFFERENT: " + ", ".join(f"{c} {n}" for c, n in sorted(counts.items())))
    if out:
        with open(out, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
    print("\n".join(lines[-60:]))
    return 1 if diffs else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
