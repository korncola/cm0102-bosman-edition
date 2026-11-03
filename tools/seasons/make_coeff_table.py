#!/usr/bin/env python3
"""Makes src/recomp_rt/win/season_coeffs_table.c, the missing UEFA season coefficients of some season databases
(0.6.2 open item 106).

Usage:
    python3 -I tools/seasons/make_coeff_table.py --measure <folder of database folders>
        writes docs/research/2026-11-03-database-coefficients.tsv from the databases, and prints for each season
        database how its six coefficient seasons compare with those of the next season database
    python3 -I tools/seasons/make_coeff_table.py            writes the C table from the TSV
    python3 -I tools/seasons/make_coeff_table.py --check    exits 1 when the C table is not the one the TSV gives

A nation record (nation.dat, 0x122 bytes) holds six season coefficients, doubles at +0xe0 to +0x108, oldest first.
At a new game in start year S the game expects the seasons (S-6)/(S-5) to (S-1)/S there: the first season ranks
the nations by +0xe0 to +0x100 (UefaSeeding::roll_coefficients 0x903b50 makes no shift in the first season,
0x903b6f), the European Performance Spots of the first season read +0x108 (season_uefa_seeding.c), and each
later season change shifts the six down one place and writes the game's own points to +0x108 (0x903b50). So
+0x108 is the season before the start, which counts in the five-season totals of the next five seasons.

"2025 Data Update" (December 2025) and "May 2026 Data Update" (May 2026) hold 2019/20 to 2023/24 at +0xe0 to
+0x100 and 0 at +0x108 for every nation: the 2024/25 season is missing. The October 2026 database holds 2024/25
at +0x100 (England 29.464, Spain 23.892). Max's decision (3 November 2026): the 2025/26 databases take these
values. The October 2026 data is used with the Update Team's permission (1 November 2026).

FILLS names each database that gets +0x108 and the database whose +0x100 gives it. A nation is matched by its
name among the UEFA nations (continent id 2) of both databases. The TSV keeps the nation id of the database
that gets the value, its name, the +0x100 value the game must find there (a check that the loaded nation.dat
is the known one), and the new +0x108 value. The values are written with repr(), which a C compiler reads as the
same double.
"""
import csv
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import history_gaps as hg  # noqa: E402

ROOT = hg.ROOT
TSV = os.path.join(ROOT, "docs", "research", "2026-11-03-database-coefficients.tsv")
OUT = os.path.join(ROOT, "src", "recomp_rt", "win", "season_coeffs_table.c")
NATION_TABLE, UEFA = 4, 2
CONTINENT_AT, NAME_AT, COEFFS_AT = 0x71, 0x04, 0xe0
# database that gets +0x108: (database whose +0x100 gives it, the season of the value)
FILLS = {"December 2025": ("October 2026", "2024/25"), "May 2026": ("October 2026", "2024/25")}
COLUMNS = ["database", "nation_id", "nation", "season", "check_0x100", "value_0x108", "source"]


def fail(text):
    raise SystemExit("make_coeff_table.py: " + text)


def known_databases(dbroot):
    """{name: (data season start or None, role, folder)} of the database folders in dbroot."""
    sigs = {(int(r["club_size"]), r["club_sha256"], int(r["staff_size"]), r["staff_sha256"]): r
            for r in hg.read_tsv(hg.SIGS)}
    out = {}
    for sub in sorted(os.listdir(dbroot)):
        p = os.path.join(dbroot, sub)
        if not os.path.isdir(p):
            continue
        r = hg.identify(p, sigs)
        inner = [os.path.join(p, s) for s in os.listdir(p) if os.path.isdir(os.path.join(p, s))]
        if not r and len(inner) == 1:          # a folder that holds the unpacked zip's own folder
            p = inner[0]
            r = hg.identify(p, sigs)
        if r:
            start = int(r["data_season"][:4]) if r["data_season"][:4].isdigit() else None
            out[r["database"]] = (start, r["role"], p)
    return out


def uefa_nations(folder):
    """{name: (nation id, [six doubles])} of the UEFA nations of a database."""
    d = hg.Data(folder)
    out = {}
    for i, r in enumerate(d.rows(NATION_TABLE)):
        if hg.s32(r, CONTINENT_AT) != UEFA:
            continue
        name = r[NAME_AT:NAME_AT + 51].split(b"\0")[0].decode("latin-1")
        if name in out:
            fail(f"{folder}: the UEFA nation {name!r} appears twice")
        if hg.s32(r, 0) != i:
            fail(f"{folder}: nation record {i} has the id {hg.s32(r, 0)}")
        out[name] = (i, list(struct.unpack_from("<6d", r, COEFFS_AT)))
    return out


def measure(dbroot):
    dbs = known_databases(dbroot)
    rows = []
    for target, (donor, season) in FILLS.items():
        for name in (target, donor):
            if name not in dbs:
                fail(f"no folder of the database {name!r} in {dbroot}")
        t, s = uefa_nations(dbs[target][2]), uefa_nations(dbs[donor][2])
        if set(t) != set(s):
            fail(f"{target} and {donor} have other UEFA nations: {sorted(set(t) ^ set(s))}")
        for name in sorted(t, key=lambda n: t[n][0]):
            nid, v = t[name]
            if v[5] != 0:
                fail(f"{target}: {name} has {v[5]!r} at +0x108, not 0")
            if v[4] != s[name][1][3]:
                fail(f"{target}: {name}: +0x100 {v[4]!r} is not the +0xf8 {s[name][1][3]!r} of {donor}: "
                     "the seasons do not line up")
            rows.append({"database": target, "nation_id": str(nid), "nation": name, "season": season,
                         "check_0x100": repr(v[4]), "value_0x108": repr(s[name][1][4]), "source": f"{donor} +0x100"})
    with open(TSV, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, COLUMNS, delimiter="\t", lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    print(f"wrote {TSV} ({len(rows)} rows)")
    report(dbs)


def report(dbs):
    """For each season database, the season of each of its six coefficient slots. The seasons come from a
    timeline of the values of four nations: it starts from October 2026 (+0xe0 holds 2020/21 there: England
    24.357, the real value) and takes each database whose nonzero slots line up with it at one offset (two slots
    or more agree, none disagrees), until no database adds a season. A slot is "-" when it is 0 and "?" when it
    is not in the timeline (the values of no other database)."""
    probe = ("England", "Spain", "Italy", "Germany")
    vals = {}
    for name, (y, role, folder) in dbs.items():
        if role == "season database" and y:
            a = uefa_nations(folder)
            vals[name] = (y, [tuple(a[p][1][k] for p in probe) for k in range(6)])

    def same(u, v):
        return sum(abs(x - y) < 0.05 for x, y in zip(u, v)) >= 3

    timeline = {2021 + k: v for k, v in enumerate(vals["October 2026"][1]) if any(v)}   # season end year: values
    grew = True
    while grew:
        grew = False
        for name, (y, slots) in vals.items():
            for first in range(1990, 2030):
                pairs = [(first + k, v) for k, v in enumerate(slots) if any(v)]
                hit = [e for e, v in pairs if e in timeline and same(v, timeline[e])]
                bad = [e for e, v in pairs if e in timeline and not same(v, timeline[e])]
                if len(hit) >= 2 and not bad:
                    for e, v in pairs:
                        if e not in timeline:
                            timeline[e] = v
                            grew = True
                    break

    def season_of(v):
        if not any(v):
            return "-"
        for e, t in sorted(timeline.items()):
            if same(v, t):
                return f"{e - 1}/{e % 100:02d}"
        return "?"
    for y, name in sorted((v[0], k) for k, v in vals.items()):
        got = [season_of(v) for v in vals[name][1]]
        want = [f"{y - 6 + k}/{(y - 5 + k) % 100:02d}" for k in range(6)]
        # +0xe8..+0x108 go into the totals of the next five seasons, so each must hold its own season
        wrong = [f"+0x{COEFFS_AT + 8 * k:x} holds {got[k]}, not {want[k]}" for k in range(1, 6) if got[k] != want[k]]
        print(f"{name:20s} {y}/{(y + 1) % 100:02d}: {' '.join(got)}  " + ("; ".join(wrong) if wrong else "right"))


def table_text():
    rows = hg.read_tsv(TSV)
    known = {r["database"] for r in hg.read_tsv(hg.SIGS)}
    by_db = {}
    for r in rows:
        if r["database"] not in known:
            fail(f"{r['database']!r} is not in databases-signatures.tsv")
        nid, chk, val = int(r["nation_id"]), float(r["check_0x100"]), float(r["value_0x108"])
        if not 0 <= nid < 1000 or not 0 <= val < 100 or not 0 <= chk < 100:
            fail(f"{r['database']} {r['nation']}: a value out of range")
        if repr(val) != r["value_0x108"] or repr(chk) != r["check_0x100"]:
            fail(f"{r['database']} {r['nation']}: a value is not in the repr() form")
        if any(q in r["nation"] for q in '"\\?') or len(r["nation"].encode("latin-1")) > 50:
            fail(f"{r['database']}: the nation name {r['nation']!r} cannot go into the table")
        by_db.setdefault(r["database"], []).append((nid, r["nation"], r["check_0x100"], r["value_0x108"]))
    lines = [
        "/* The missing UEFA season coefficients of some season databases (season_coeffs.h). Made by",
        " * tools/seasons/make_coeff_table.py from docs/research/2026-11-03-database-coefficients.tsv. Do not edit. */",
        '#include "season_coeffs.h"',
        "",
    ]
    dbs = []
    for db, items in by_db.items():
        ident = "".join(c if c.isalnum() else "_" for c in db)
        lines.append(f"static const SeasonCoeff coeffs_{ident}[] = {{")
        for nid, nation, chk, val in items:
            lines.append(f'    {{{nid}, "{nation}", {chk}, {val}}},')
        lines.append("};")
        dbs.append((db, ident, len(items)))
    lines += ["", "const SeasonCoeffDb season_coeff_dbs[] = {"]
    for db, ident, n in dbs:
        lines.append(f'    {{"{db}", coeffs_{ident}, {n}}},')
    lines += ["};", "", f"const int season_coeff_dbs_count = {len(dbs)};", ""]
    return "\n".join(lines)


def main(argv):
    if argv[:1] == ["--measure"] and len(argv) == 2:
        measure(argv[1])
        return 0
    want = table_text()
    if argv == ["--check"]:
        have = open(OUT, encoding="latin-1").read() if os.path.exists(OUT) else ""
        if have != want:
            print("make_coeff_table.py: season_coeffs_table.c is not the file that the TSV gives. "
                  "Run tools/seasons/make_coeff_table.py.", file=sys.stderr)
            return 1
        print("season_coeffs_table.c matches the coefficient table")
        return 0
    if argv:
        print(__doc__, file=sys.stderr)
        return 2
    with open(OUT, "w", encoding="latin-1") as f:
        f.write(want)
    print(f"wrote {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
