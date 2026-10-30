#!/usr/bin/env python3
"""Tier A against the normal game (0.6.2 slice 3, decision Q10): two saves of the same start, database and seed, one
from a Tier A run (RECOMP_TIER_A=1) and one from the normal game, each near the same July. They count as the same
when they agree on editions, dates, hosts and entrant rules. Winners can differ.

Usage:
    python3 -I tests/seasons/future/tier_a_compare.py <normal.sav> <tier_a.sav> [--out <file>]

The two runs save on days a little apart, and each save keeps the old fixtures of a competition for a while. So
the fixtures are compared in a window that both saves hold: per competition, from the later of the two first
fixture days to the earlier save day. The checks, from the history checker (history_check.py) of each save:
    editions    the history rows (competition, year) from the start year on, the editions in the window
                (history_check.editions_of), and the edition that each finals class waits for
    dates       the fixtures in the window split into parts at each gap of more than PART_GAP days (a season,
                or a half before a long winter break): the same number of parts, the first day of each part
                within FIRST_DAYS days, and its last day within LAST_DAYS days
    hosts       the host of each history row from the start year on
    entrants    the entrant faults of the checker (entrant-confed, -count, -host, -route, -holder, -dead: the
                year's access rule)
Each difference is one line "DIFF <check> <competition> <year>: normal ... / Tier A ...". The exit code is 1 when
any check differs, or when a save gives no edition or the two saves have other start years.

Results move some days, some edition years and some entrants, as they move the winners. These are NOTE lines
and do not fail:
    - match days in one save only: replays of drawn ties, matches moved for a cup or European tie, the
      Saturday or Sunday of a match
    - an edition whose year is one year off in the other save, when its last match moves across 31 December
    - the number of teams in the window (which champions start in an earlier qualifying round follows the club
      coefficients), and the count of entrants per nation (a title holder's nation follows the winner)
    - friendlies, reserve and under-21 matches, which clubs and nations arrange in play
Compare the saves of every July of the two runs, not only the last, so that a difference that clears again is
still found.
"""
import collections
import datetime
import os
import re
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
    y, d = game.date
    f = {"date": game.date, "day": datetime.date(y, 1, 1).toordinal() + d if y else 0, "start": game.start}
    f["rows"] = {(name(cid), yr) for cid, yr, *_ in game.rows if yr >= game.start}
    f["waits"] = {name(cid): yr for cid, yr in game.editions.items()}
    f["fixtures"] = {name(cid): fx for cid, fx in hc.fixtures(game).items() if fx}
    f["hosts"] = {(name(cid), yr): game.club(h)[0] for cid, yr, a, b, c, h in game.rows if yr >= game.start and h >= 0}
    mix = collections.defaultdict(collections.Counter)
    for _, n, yr, first, last, tid, team, nation, conf, route in listing:
        mix[(n, yr)][nation] += 1
    f["nations"] = mix
    f["entrant faults"] = {(c, n, yr) for c, n, yr, detail in faults}
    return f


RECENT = 60          # days: fixtures this close to the save are not yet dropped
PART_GAP = 90        # days with no match that end a part (a season, or the half before a long winter break)
FIRST_DAYS = 3       # days that the first match of a part can move (a Saturday or a Sunday start)
LAST_DAYS = 21       # days that replays or moved matches can shift the last day of a part
FRIENDLY = re.compile(r"Friendly|Reserve|'B' International|Under 21 International")   # arranged in play


def parts(fx):
    """[(first day, last day, teams, Counter of matches per day)]: the fixtures split at each gap of more than
    PART_GAP days."""
    out = []
    for day, home, away in sorted(fx):
        if not out or day - out[-1][1] > PART_GAP:
            out.append([day, day, set(), collections.Counter()])
        out[-1][1] = day
        out[-1][2].update(t for t in (home, away) if t >= 0)
        out[-1][3][day] += 1
    return out


def span(ps):
    return ", ".join(f"{datetime.date.fromordinal(p[0])}..{datetime.date.fromordinal(p[1])}" for p in ps)


def years_apart(only, other):
    """The keys (competition, year) of only that have no key one year off in other: a DIFF; the rest: a NOTE."""
    hard = {k for k in only if (k[0], k[1] - 1) not in other and (k[0], k[1] + 1) not in other}
    return hard, only - hard


def compare(a, b):
    diffs, notes = [], []
    if a["start"] != b["start"]:
        diffs.append(("start", "", "", f"{a['start']} / {b['start']}"))
    for f, who in ((a, "normal"), (b, "Tier A")):
        if not f["rows"] and not f["fixtures"]:
            diffs.append(("unread", "", "", f"the {who} save gives no edition"))
    if a["date"] != b["date"]:
        notes.append(("save-date", "", "", f"{a['date']} / {b['date']}"))
    end = min(a["day"], b["day"])
    for comp in sorted(set(a["fixtures"]) | set(b["fixtures"])):
        fa, fb = a["fixtures"].get(comp, []), b["fixtures"].get(comp, [])
        if not fa or not fb:                    # one save has dropped its old fixtures, or has none
            fx, who = (fa, "normal only") if fa else (fb, "Tier A only")
            recent = [x for x in fx if end - RECENT <= x[0] <= end]
            (diffs if recent else notes).append(("editions", comp, "", f"{len(recent or fx)} fixtures, {who}"))
            continue
        first = max(min(x[0] for x in fa), min(x[0] for x in fb))
        pa = parts([x for x in fa if first <= x[0] <= end])
        pb = parts([x for x in fb if first <= x[0] <= end])
        if len(pa) != len(pb):
            (notes if FRIENDLY.search(comp) else diffs).append(("dates", comp, "", f"{len(pa)} / {len(pb)} parts in the window: "
                                             f"{span(pa)} / {span(pb)}"))
            continue
        for i, (x, y) in enumerate(zip(pa, pb)):
            year = datetime.date.fromordinal(x[1]).year
            soft = FRIENDLY.search(comp)
            if (abs(x[0] - y[0]) > FIRST_DAYS and min(x[0], y[0]) > first) or abs(x[1] - y[1]) > LAST_DAYS:
                (notes if soft else diffs).append(("dates", comp, year, f"{span([x])} / {span([y])}"))
            elif x[:2] != y[:2]:
                notes.append(("dates", comp, year, f"{span([x])} / {span([y])}"))
            if len(x[2]) != len(y[2]):
                notes.append(("entrants", comp, year, f"{len(x[2])} / {len(y[2])} teams in the window"))
            ca, cb = x[3], y[3]
            moved = sorted(set(ca) ^ set(cb))
            if moved:
                notes.append(("dates", comp, year, f"{len(moved)} match days in one save only, for example "
                              + ", ".join(f"{datetime.date.fromordinal(d)} ({ca[d] or cb[d]}, "
                                          f"{'normal' if d in ca else 'Tier A'})" for d in moved[:3])))
    for check, ka, kb in (("editions", a["rows"], b["rows"]),):
        what = " (history row)"
        for only, other, who in ((ka - kb, kb, "normal only"), (kb - ka, ka, "Tier A only")):
            hard, soft = years_apart(only, other)
            diffs += [(check, k[0], k[1], who + what) for k in sorted(hard)]
            notes += [(check, k[0], k[1], who + what + ", one year off in the other save") for k in sorted(soft)]
    for comp in sorted(set(a["waits"]) | set(b["waits"])):
        if a["waits"].get(comp) != b["waits"].get(comp):
            diffs.append(("editions", comp, "", f"the class waits for {a['waits'].get(comp)} / {b['waits'].get(comp)}"))
    for key in sorted(set(a["hosts"]) | set(b["hosts"]), key=str):
        if a["hosts"].get(key) != b["hosts"].get(key):
            diffs.append(("hosts", key[0], key[1], f"{a['hosts'].get(key)} / {b['hosts'].get(key)}"))
    for key in sorted(set(a["nations"]) & set(b["nations"]), key=str):
        if a["nations"][key] != b["nations"][key]:
            ma, mb = a["nations"][key], b["nations"][key]
            moved = (ma - mb) + (mb - ma)
            notes.append(("nations", key[0], key[1], ", ".join(f"{n} {ma[n]} / {mb[n]}" for n in sorted(moved))))
    for e in sorted(a["entrant faults"] ^ b["entrant faults"], key=str):
        diffs.append(("entrant faults", e[1], e[2], f"{e[0]}: " + ("normal only" if e in a["entrant faults"]
                                                                    else "Tier A only")))
    return diffs, notes


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
                 f"save {b['date']}; {len(a['rows'])} history rows, {len(a['hosts'])} hosts, "
                 f"{len(a['fixtures'])} competitions with fixtures")
    lines.append("SAME" if not diffs else "DIFFERENT: " + ", ".join(f"{c} {n}" for c, n in sorted(counts.items())))
    if out:
        with open(out, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
    print("\n".join(lines[-60:]))
    return 1 if diffs else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
