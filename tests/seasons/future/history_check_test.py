#!/usr/bin/env python3
"""ctest of tests/seasons/future/history_check.py on a small fixture save.

Usage:
    python3 -I tests/seasons/future/history_check_test.py

The test writes a saved game of a few kilobytes into a temporary folder: the container of tools/save/savfile.py
(compressed members) with six continents, twelve nations, their national sides, six clubs, nine competitions and
their history rows, a news list, the .tmp parts of finals classes, three fixtures parts, 60 players and the
finance records. The game starts in 2025 and the save is on day 189 of 2040. The data holds one fault for each
check of the checker, and some competitions hold none. The test runs the checker and compares its faults with the
planted list, then runs it again with a rules file, a finals table, an access file and a known-fault list, then
with changing rules off, then with --health. It also compares the renames and UEFA moves of nations.tsv with the
season code.
All data is made up, so the fixture holds no data of a database.
"""
import csv
import os
import re
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "save"))
import savfile  # noqa: E402

CHECKER = os.path.join(HERE, "history_check.py")
CONTINENTS = ["Africa", "Asia", "Europe", "North America", "Oceania", "South America"]
NATIONS = [("Aland", 2), ("Borduria", 2), ("Carpania", 2), ("Durania", 1), ("Elbonia", 1), ("Freedonia", 3),
           ("Genovia", 3), ("Hatay", 5),
           ("Ruritania", 3), ("Kurlandia", 5), ("Osterlich", 5), ("Old Genovia", -1), ("Bonairia", -1)]
CLUBS = [("Aland City", 0), ("Aland United", 0), ("Borduria Sport", 1), ("Carpania FC", 2), ("Durania Stars", 3),
         ("Elbonia Rovers", 4)]
SIDE0 = 100                                  # national side ids follow the club ids
# id, name, continent, nation, file
COMPS = [(0, "Aland League", 2, 0, "club"), (1, "Euro Super Cup", 2, -1, "club"),
         (10, "World Finals", -1, -1, "nation"), (11, "Euro Finals", 2, -1, "nation"),
         (12, "Asia Finals", 1, -1, "nation"), (13, "Americas Finals", 3, -1, "nation"),
         (14, "Hatay Cup", 5, -1, "nation"), (15, "Nations Shield", 1, -1, "nation"),
         (16, "Champions Match", 2, -1, "club"), (17, "Qual Zone", 1, -1, "nation")]


def side(n):
    return SIDE0 + n


def history():
    """[(competition, year, first, second, third, host)] and the planted faults {(check, competition, year)}."""
    rows, want = [], set()
    for y in range(2010, 2041):                                       # a clean league
        rows.append((0, y, 0, 1, -1, -1))
    for y in range(2015, 2025):                                       # a club cup, then 3 finals of one pair
        rows.append((1, y, 2 + y % 2, 0 + y % 2, -1, -1))
    for y in (2026, 2027, 2028):
        rows.append((1, y, 0, 3, -1, -1))
    want.add(("same-final", "Euro Super Cup", 2028))
    for y in range(2029, 2041):
        rows.append((1, y, y % 3, 3, -1, -1))
    for y in range(1990, 2035, 4):                                    # World Finals: no host in 2034, then none
        rows.append((10, y, side(y % 8), side((y + 3) % 8), side((y + 5) % 8), side(y % 7) if y < 2034 else -1))
    want |= {("no-host", "World Finals", 2034), ("missing", "World Finals", 2038),
             ("next-edition", "World Finals", 2200)}
    for y in range(1996, 2041, 4):                                    # Euro Finals: 2028 twice
        rows.append((11, y, side(y % 3), side((y + 1) % 3), side((y + 2) % 3), side(y % 3)))
    rows.append((11, 2028, side(2), side(0), side(1), side(1)))
    want.add(("duplicate", "Euro Finals", 2028))
    for y in range(2003, 2040, 4):                                    # Asia Finals: a European runner-up in 2031
        rows.append((12, y, side(3 + y % 2), side(1) if y == 2031 else side(10) if y == 2027 else side(4 - y % 2),
                     -1, side(3)))
    want.add(("confederation", "Asia Finals", 2031))                 # 2027: Osterlich moves after it, no fault
    for y in list(range(2015, 2027, 2)) + [2031, 2033, 2035, 2037, 2039]:   # Americas Finals: 2027 and 2029 lost
        rows.append((13, y, side(5), side(6 if y // 2 % 2 else 8), -1, side(5)))
    want.add(("gap", "Americas Finals", 2031))
    for y in range(2015, 2040):                                       # Hatay Cup: clean, its class ended
        a, b = ((7, 9), (9, 10), (10, 7))[y % 3]
        rows.append((14, y, side(a), side(b), -1, -1))
    for y in (2020, 2024):                                            # Nations Shield: a European cup on
        rows.append((15, y, side(1), side(2), -1, -1))                # an Asian record with the short name
    rows.append((15, 2032, side(2), side(0), -1, -1))                 # of an Asian cup
    want.add(("confederation", "Nations Shield", 2032))               # plain run: its continent is Asia
    return rows, want


# Nation changes (nations.tsv): the save is on day 189 of 2040 with changing rules on.
NATION_CHANGES = ("kind\tseason\tfrom\tnation\tto\tsource\n"
                  "rename\t2024\t09-01\tHatay\tHatay Republic\tdue in the start season\n"
                  "rename\t2030\t09-01\tRuritania\tNeo-Ruritania\tdue, a live record\n"
                  "rename\t2030\t09-01\tOld Genovia\tGenovia\tdue, a dead record (no continent)\n"
                  "rename\t2030\t09-01\tOld Freedonia\tFreedonia\tdue, the side keeps the old name\n"
                  "rename\t2040\t09-01\tKurlandia\tCourland\tnot due before 1 September 2040\n"
                  "confed\t2035\t09-01\tDurania\tUEFA\tdue, still in AFC\n"
                  "confed\t2020\t02-01\tElbonia\tAFC\tdue, in AFC\n"
                  "confed\t2030\t09-01\tOsterlich\tCONMEBOL\tdue; an Asia Finals runner-up in 2027, before the move\n"
                  "rename\t2040\t07-08\tOsterlich\tNova Osterlich\tdue on the save date, day 189 of 2040\n")
NATION_FAULTS = {("nation-name", "Hatay", 2024), ("nation-name", "Ruritania", 2030), ("nation-name", "Osterlich", 2040),
                 ("nation-confed", "Durania", 2035), ("side-name", "Old Freedonia", 2030)}
SHORT_NAMES = {15: "Asia Shield"}                               # the short name (+0x38) of a competition
SIDE_NAMES = {"Freedonia": "Old Freedonia"}                      # the national side's name, when not the nation's
NATION_FAULTS_OFF = {("nation-name", "Hatay", 2024)}             # changing rules off: only to the start season


# Entrants (fixtures_<year>.tmp): (competition, home, away, day of the year).
# World Finals 2038: 3 sides, the table has 4 and the host Carpania (not an entrant). Champions Match: the winners
# of the Aland League and of the Euro Super Cup meet; Carpania FC won neither in 2039, Elbonia Rovers (AFC) in 2040.
FIXTURES_2038 = [(10, side(0), side(1), 160), (10, side(4), side(0), 164), (10, side(1), side(4), 168),
                 (10, side(0), side(1), 175)]
# Champions Match 2039 also has Durania Stars (AFC, due to move to UEFA: only its nation fault). Euro Super Cup 2040:
# the holder of 2039 (Borduria Sport) does not play. Americas Finals 2039, after the table: its host Freedonia does
# not play. Qual Zone (a World Cup qualifying, AFC): 2039 has a European side among 4 teams (a play-off, passes)
# and the dead Old Genovia; Bonairia (no continent, never renamed) passes. Nations Shield 2039: two European sides
# under the Asian short name.
FIXTURES_2039 = [(16, 0, 3, 220), (16, 4, 0, 222), (1, 0, 1, 100), (13, side(6), side(8), 150),
                 (17, side(3), side(4), 120), (17, side(0), side(11), 124), (17, side(12), side(3), 127),
                 (15, side(0), side(1), 130)]
FIXTURES_2040 = [(16, 0, 5, 150), (16, 3, 0, 250), (1, 0, 3, 100)]   # the match of day 250 is after the save date
ENTRANT_FAULTS = {("entrant-count", "World Finals", 2038), ("entrant-host", "World Finals", 2038),
                  ("entrant-route", "Champions Match", 2039), ("entrant-route", "Champions Match", 2040),
                  ("entrant-confed", "Champions Match", 2040), ("entrant-holder", "Euro Super Cup", 2040),
                  ("entrant-host", "Americas Finals", 2039), ("entrant-dead", "Qual Zone", 2039),
                  ("entrant-confed", "Asia Shield", 2039), ("entrant-count", "Champions Match", 2039)}
ENTRANT_FAULTS_PLAIN = {("entrant-dead", "Qual Zone", 2039),       # found with no rules file too; Ruritania
                        ("entrant-dead", "Americas Finals", 2039)}  # plays under the name its rename replaced
TABLE = '{"world", 2037, 2038, 4, 0, 2038, 6, 1, "Carpania/Karpania", "row-1"},\n'
ACCESS = ("comp\tfrom\tto\tteams\troutes\tsource\nchamp\t2030\tnone\t2\twinner:aland;winner:esc\tmade up\n"
          "esc\t2030\tnone\t\tholder\tmade up\n")


def fixture_part(fixtures):
    b = bytearray(struct.pack("<i", len(fixtures)))
    for cid, home, away, day in fixtures:
        r = bytearray(0x4F)
        struct.pack_into("<i", r, 0, cid)
        struct.pack_into("<ii", r, 0xC, home, away)
        struct.pack_into("<hh", r, 0x28, 2038 + (fixtures is FIXTURES_2039) + 2 * (fixtures is FIXTURES_2040), day)
        b += r
    return bytes(b)


# World health: Aland has 30 players born in 2010 (none under 23 in 2040), Borduria 30 of all ages; one Borduria
# player has a wrong age field; Carpania FC's balance has wrapped.
# Aland's players are in its senior squad; Borduria's in no squad, so its senior and under-21 squads are empty.
HEALTH_FAULTS = {("no-youth", "Aland", 2040), ("empty-squad", "Borduria", 2040),
                 ("empty-squad", "Borduria under-21", 2040), ("overflow", "age", 2040), ("overflow", "balance", 2040)}


def staff():
    out = bytearray()
    people = [(0, 2010, 30)] * 30 + [(1, 2015 + k % 10, 2040 - 2015 - k % 10 - 1) for k in range(30)]
    people[-1] = (1, 2000, 7)                                       # the age field is 7, the birth date gives 39
    for i, (nation, born, age) in enumerate(people):
        r = bytearray(0x6E)
        struct.pack_into("<i", r, 0, i)
        struct.pack_into("<hh", r, 0x10, 300, born)                 # born on day 300: the birthday is after day 189
        struct.pack_into("<h", r, 0x18, age)
        struct.pack_into("<ii", r, 0x1A, nation, -1)
        struct.pack_into("<i", r, 0x24, side(nation) if nation == 0 and i % 2 else -1)
        r[0x28] = 11
        struct.pack_into("<ii", r, 0x39, -1, 0)
        struct.pack_into("<i", r, 0x61, i)
        out += r
    return bytes(out)


def finance():
    out = bytearray()
    for k, _ in enumerate(CLUBS):
        r = bytearray(0x167)
        struct.pack_into("<q", r, 0, (1 << 62) + 5 if k == 3 else 1000000 * k)
        out += r
    return bytes(out)


def records(changing=1):
    def pad(b, n):
        return b + bytes(n - len(b))

    cont = b"".join(pad(struct.pack("<i", i) + n.encode(), 0xC6) for i, n in enumerate(CONTINENTS))
    nat = bytearray()
    for i, (n, c) in enumerate(NATIONS):
        r = bytearray(0x122)
        struct.pack_into("<i", r, 0, i)
        r[4:4 + len(n)] = n.encode()
        struct.pack_into("<i", r, 0x71, c)
        nat += r

    def club(cid, name, nation, manager=-1):
        r = bytearray(0x245)
        struct.pack_into("<i", r, 0, cid)
        r[4:4 + len(name)] = name.encode()
        struct.pack_into("<i", r, 0x53, nation)
        struct.pack_into("<i", r, 0xCF, manager)
        return bytes(r)

    clubs = b"".join(club(i, n, nat_) for i, (n, nat_) in enumerate(CLUBS))
    sides = b"".join(club(side(i), SIDE_NAMES.get(n, n), i, 0 if i < 2 else -1) for i, (n, _) in enumerate(NATIONS))
    sides += b"".join(club(side(len(NATIONS) + i), n + " U21", i, -1) for i, (n, _) in enumerate(NATIONS))
    comp = {"club": bytearray(), "nation": bytearray()}
    for cid, name, c, n, part in COMPS:
        r = bytearray(0x6B)
        struct.pack_into("<i", r, 0, cid)
        r[4:4 + len(name)] = name.encode()
        struct.pack_into("<ii", r, 0x59, c, n)
        short = SHORT_NAMES.get(cid, "").encode()
        r[0x38:0x38 + len(short)] = short
        comp[part] += r
    rows, want = history()
    want |= NATION_FAULTS | ENTRANT_FAULTS_PLAIN
    hist = {"club": bytearray(), "nation": bytearray()}
    files = {cid: part for cid, _, _, _, part in COMPS}
    for i, (cid, y, a, b, c, h) in enumerate(rows):
        hist[files[cid]] += struct.pack("<iiHiiii", 1000 + i, cid, y, a, b, c, h)
    news = bytearray(8 + 100 * 223)
    struct.pack_into("<ii", news, 0, 100, 3)
    for k, (d, y) in enumerate([(180, 2040), (189, 2040), (20, 2040)]):
        struct.pack_into("<hh", news, 8 + k * 223 + 0xC9, d, y)

    def part(year):                                                   # a finals class waiting on its edition year
        return (b"x.tmp\0" + bytes(40) + b"\xff\xff\xff\xff\x07\x03\x50\x00" + bytes(7) + b"\x01" + bytes(3)
                + struct.pack("<H", year) + bytes(40))

    members = [("continent.dat", cont), ("nation.dat", bytes(nat)), ("club.dat", clubs), ("nat_club.dat", sides),
               ("club_comp.dat", bytes(comp["club"])), ("nation_comp.dat", bytes(comp["nation"])),
               ("club_comp_history.dat", bytes(hist["club"])), ("nation_comp_history.dat", bytes(hist["nation"])),
               ("news.dat", bytes(news)), ("bosman_season.dat", struct.pack("<4i", 1, 16, 2025, changing)),
               ("World Finals_10.tmp", part(2200)), ("Asia Finals_12.tmp", part(2043)),
               ("Euro Finals_11.tmp", b"\0" * 64), ("Americas Finals_13.tmp", b"\0" * 64),
               ("Euro Super Cup_1.tmp", b"\0" * 64), ("Hatay Cup_14.tmp", part(2999)),
               ("fixtures_2038.tmp", fixture_part(FIXTURES_2038)), ("fixtures_2039.tmp", fixture_part(FIXTURES_2039)),
               ("fixtures_2040.tmp", fixture_part(FIXTURES_2040)), ("staff.dat", staff()),
               ("finance.dat", finance())]
    return members, want


def write_sav(path, members):
    header = bytearray(savfile.HEADER_SIZE)
    struct.pack_into("<3i", header, 0, 2, 0, len(members))
    body = bytearray()
    for k, (name, data) in enumerate(members):
        o = 12 + k * savfile.ENTRY_SIZE
        struct.pack_into("<2I", header, o, savfile.HEADER_SIZE + len(body), len(data))
        header[o + 8:o + 8 + len(name)] = name.encode("latin-1")
        body += savfile.encode(data)
    with open(path, "wb") as f:
        f.write(header + body)


def run(args):
    r = subprocess.run([sys.executable, "-I", CHECKER] + args, capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


def faults(folder, label):
    with open(os.path.join(folder, label + ".faults.tsv"), newline="", encoding="utf-8") as f:
        return {(r["check"], r["competition"], int(r["year"])): r["known"] for r in csv.DictReader(f, delimiter="\t")}


def renames_match():
    """The rename rows of nations.tsv against season_intl_renames (season_intl_rules.c)."""
    src = open(os.path.join(ROOT, "src", "recomp_rt", "win", "season_intl_rules.c"), encoding="latin-1").read()
    code = set()
    for season, old, new in re.findall(r'\{(\d{4}), "([^"]*)", ((?:"[^"]*"\s*)+),', src):
        name = "".join(re.findall(r'"([^"]*)"', new))
        code.add((int(season), old, bytes(name, "latin-1").decode("unicode_escape").encode("latin-1")
                  .decode("latin-1")))
    with open(os.path.join(HERE, "nations.tsv"), encoding="utf-8") as f:
        rows = {(int(r["season"]), r["nation"], r["to"])
                for r in csv.DictReader((l for l in f if not l.startswith("#")), delimiter="\t")
                if r["kind"] == "rename"}
    if not code or rows != code:
        return [f"nations.tsv renames differ from season_intl_renames: only in the file {sorted(rows - code)}, "
                f"only in the code {sorted(code - rows)}"]
    return []


def uefa_moves_match():
    """The UEFA rows of nations.tsv against season_intl_uefa_from (season_intl_rules.c): each nation that the season
    code moves to UEFA, and its season (open item 43 of 0.6.2: Kazakhstan from 2002)."""
    src = open(os.path.join(ROOT, "src", "recomp_rt", "win", "season_intl_rules.c"), encoding="latin-1").read()
    body = re.search(r"int season_intl_uefa_from\(const char \*name\)\s*\{(.*?)\n\}", src, re.S)
    # both forms: `if (!strcasecmp(name, "X")) return 2002;` and `!strcasecmp(name, "X") ? 2016 : 0`
    code = {(int(y), n) for n, y in re.findall(r'strcasecmp\(name, "([^"]+)"\)\)?\s*(?:return|\?)\s*(\d{4})',
                                               body.group(1) if body else "")}
    with open(os.path.join(HERE, "nations.tsv"), encoding="utf-8") as f:
        rows = {(int(r["season"]), r["nation"])
                for r in csv.DictReader((l for l in f if not l.startswith("#")), delimiter="\t")
                if r["kind"] == "confed" and r["to"] == "UEFA"}
    if not code or rows != code:
        return [f"nations.tsv UEFA moves differ from season_intl_uefa_from: only in the file {sorted(rows - code)}, "
                f"only in the code {sorted(code - rows)}"]
    return []


def uefa_missing_rule():
    """entrant-missing (open item 57): the European host of the World Cup, Russia from the 2026 cycle and a nation
    that joins UEFA after the edition's first year are no fault; a member that does not enter is."""
    import types
    sys.path.insert(0, HERE)
    import history_check
    game = types.SimpleNamespace(
        nations={0: ("Aland", "UEFA"), 1: ("Borduria", "UEFA"), 2: ("Russia", "UEFA"), 3: ("Kosovo", "UEFA"),
                 4: ("Carpania", "AFC"), 5: ("Dead Land", "")},
        side_names=[("Aland", 0), ("Borduria", 1), ("Russia", 2), ("Kosovo", 3), ("Carpania", 4), ("Dead Land", 5)])
    access = history_check.Access({"world_cup": [(2006, 32, ["Borduria"]), (2026, 48, [])]})
    changes = [{"kind": "confed", "season": 2016, "nation": "Kosovo", "to": "UEFA"}]
    bad = []
    for start, changing, year, want in ((2001, 0, 2005, 0), (2001, 1, 2001, 0), (2001, 1, 2005, 1),
                                        (2013, 0, 2017, 1), (2013, 0, 2025, 0), (2013, 1, 2025, 1),
                                        (2025, 0, 2029, 1)):
        game.start, game.changing = start, changing
        if history_check.uefa_all(game, year) != want:
            bad.append(f"uefa_all of a {start} start (changing rules {changing}) in {year}: not {want}")
    for year, first, entered, want in ((2005, 2004, {"Aland", "Russia"}, []),          # Borduria hosts 2006
                                       (2005, 2004, {"Russia"}, ["Aland"]),
                                       (2021, 2021, {"Aland", "Borduria", "Kosovo"}, ["Russia"]),  # in up to 2022
                                       (2025, 2025, {"Aland", "Borduria", "Kosovo"}, []),  # Russia out from 2026
                                       (2013, 2012, {"Aland", "Borduria", "Russia"}, []),  # Kosovo joins in 2016
                                       (2017, 2016, {"Aland", "Borduria", "Russia"}, ["Kosovo"])):
        got = history_check.uefa_missing(game, access, changes, year, first, entered)
        if got != want:
            bad.append(f"entrant-missing of {year}: {got}, not {want}")
    return bad


def main():
    work = tempfile.mkdtemp(prefix="history-check-")
    members, want = records()
    sav = os.path.join(work, "fixture.sav")
    write_sav(sav, members)
    bad = []
    empty = os.path.join(work, "none.tsv")
    open(empty, "w").write("check\tcompetition\tyear\tnote\n")
    nations = os.path.join(work, "nations.tsv")
    with open(nations, "w", encoding="utf-8") as f:
        f.write("# a note line\n" + NATION_CHANGES)

    code, out = run([sav, "--out", work, "--label", "plain", "--known", empty, "--nations", nations])
    got = faults(work, "plain")
    if set(got) != want:
        bad.append(f"plain run: missed {sorted(want - set(got))}, extra {sorted(set(got) - want)}")
    if code != 1:
        bad.append(f"plain run: exit code {code}, not 1 for new faults")
    if "start 2025, save day 189 of 2040" not in out:
        bad.append("plain run: start year or save date not read: " + out.splitlines()[0])
    with open(os.path.join(work, "plain.history.tsv"), encoding="utf-8") as f:
        lines = f.read().splitlines()
    rows, _ = history()
    if len(lines) != len(rows) + 1:
        bad.append(f"history TSV: {len(lines) - 1} rows, the save holds {len(rows)}")
    if not any(l.startswith("10\tnation\tWorld Finals\t2002\tCarpania (Carpania)\t") and
               l.endswith("\tUEFA\tCONCACAF") for l in lines):
        bad.append("history TSV: the World Finals 2002 row is not as written")

    # A rules file: World Finals ends in 2034 (no missing fault), Euro Super Cup is a league (no same-final
    # fault), Americas Finals keeps its gap. Known list: the Euro Finals duplicate.
    rules = os.path.join(work, "rules.tsv")
    with open(rules, "w", encoding="utf-8") as f:
        f.write("# a note line\ncomp\tname\tkind\tconfed\thas_host\tcycle\tnext_year\tend_year\n"
                "americas\tAmericas Finals\tintl_finals\tCONCACAF\t1\t2\t2041\tnone\n"
                "world\tWorld Finals\tintl_finals\tFIFA\t1\t4\t2038\t2034\n"
                "esc\tEuro Super Cup\tleague\tUEFA\t0\t1\t2041\tnone\n"
                "shield\tNations Shield\tnations_league\tUEFA\t0\t4\t2028\tnone\n"
                "asia_shield\tAsia Shield\tintl_qual\tAFC\t0\t4\t2028\tnone\n"
                "champ\tChampions Match\tsuper_cup\tUEFA\t0\t1\t2041\tnone\n"
                "aland\tAland League\tleague\tnation\t0\t1\t2041\tnone\n"
                "wc_qual_zone\tQual Zone\tintl_qual\tAFC\t0\t4\t2041\tnone\n")
    names = os.path.join(work, "names.tsv")
    with open(names, "w", encoding="utf-8") as f:
        f.write("# a note line\npattern\tcomp\nAmericas Finals\tamericas\n(The )?World Finals\tworld\n"
                "Euro Super Cup\tesc\nNations Shield\tshield\nAsia Shield\tasia_shield\n"
                "Champions Match\tchamp\nAland League\taland\nQual Zone\twc_qual_zone\n")
    known = os.path.join(work, "known.tsv")
    with open(known, "w", encoding="utf-8") as f:
        f.write("check\tcompetition\tyear\tnote\nduplicate\tEuro Finals\t*\tplanted\n")
    table = os.path.join(work, "table.c")
    with open(table, "w", encoding="utf-8") as f:
        f.write(TABLE)
    access = os.path.join(work, "access.tsv")
    with open(access, "w", encoding="utf-8") as f:
        f.write(ACCESS)
    code, out = run([sav, "--out", work, "--label", "rules", "--rules", rules, "--names", names, "--known", known,
                     "--nations", nations, "--table", table, "--access", access])
    got = faults(work, "rules")
    want2 = (want - {("missing", "World Finals", 2038), ("same-final", "Euro Super Cup", 2028),
                     ("confederation", "Nations Shield", 2032)}) | \
        {("comp-continent", "Nations Shield", 2040), ("comp-name", "Nations Shield", 2040),
         ("confederation", "Asia Shield", 2032)} | ENTRANT_FAULTS
    if set(got) != want2:
        bad.append(f"rules run: missed {sorted(want2 - set(got))}, extra {sorted(set(got) - want2)}")
    if got.get(("duplicate", "Euro Finals", 2028)) != "known":
        bad.append("rules run: the known Euro Finals duplicate is not marked known")

    # Only known faults: exit code 0.
    with open(known, "a", encoding="utf-8") as f:
        for c, comp, y in sorted(want2):
            f.write(f"{c}\t{comp}\t{y}\tplanted\n")
    code, out = run([sav, "--rules", rules, "--names", names, "--known", known, "--nations", nations,
                     "--table", table, "--access", access])
    if code != 0:
        bad.append(f"known run: exit code {code} with only known faults\n{out}")

    # Changing rules off: only the nation changes up to the start season are due.
    off = os.path.join(work, "off.sav")
    write_sav(off, records(changing=0)[0])
    run([off, "--out", work, "--label", "off", "--known", empty, "--nations", nations])
    got = {k for k in faults(work, "off") if k[0].startswith("nation-")}
    if got != NATION_FAULTS_OFF:
        bad.append(f"changing rules off: nation faults {sorted(got)}, not {sorted(NATION_FAULTS_OFF)}")

    # The entrant list: the routes in of the Champions Match of 2039.
    with open(os.path.join(work, "rules.entrants.tsv"), encoding="utf-8") as f:
        lines = f.read().splitlines()
    for want_line in ("16\tChampions Match\t2039\t2039-08-09\t2039-08-11\t0\tAland City\tAland\tUEFA\twinner aland; "
                      "winner Aland League 2038; winner Aland League 2039",
                      "16\tChampions Match\t2039\t2039-08-09\t2039-08-11\t3\tCarpania FC\tCarpania\tUEFA\t"
                      "place 2 Euro Super Cup 2038; place 2 Euro Super Cup 2039",
                      "10\tWorld Finals\t2038\t2038-06-10\t2038-06-25\t104\tElbonia\tElbonia\tAFC\tqualifier",
                      "1\tEuro Super Cup\t2039\t2039-04-11\t2039-04-11\t1\tAland United\tAland\tUEFA\ttitle holder; "
                      "place 2 Aland League 2038; place 2 Aland League 2039"):
        if want_line not in lines:
            bad.append(f"entrant list: no line {want_line!r}")

    # World health: the report and its faults.
    run([sav, "--out", work, "--label", "health", "--known", empty, "--nations", nations, "--health"])
    got = {k for k in faults(work, "health") if k[0] in ("empty-squad", "no-youth", "overflow")}
    if got != HEALTH_FAULTS:
        bad.append(f"health run: faults {sorted(got)}, not {sorted(HEALTH_FAULTS)}")
    # no-youth alone fails the run (Q35): every other fault of the save is known.
    only = os.path.join(work, "only-youth.tsv")
    with open(only, "w", encoding="utf-8") as f:
        f.write("check\tcompetition\tyear\tnote\n")
        for c, comp, y in faults(work, "health"):
            if c != "no-youth":
                f.write(f"{c}\t{comp}\t{y}\tplanted\n")
    code, out = run([sav, "--known", only, "--nations", nations, "--health"])
    if code != 1 or "FAULT  no-youth  Aland" not in out:
        bad.append(f"health run: no-youth alone gives exit code {code}, not 1")
    with open(os.path.join(work, "health.health.tsv"), encoding="utf-8") as f:
        report = f.read().splitlines()
    for want_line in ("#\tplayers\t60", "#\tunder_21\t17", "#\tunder_23\t23", "#\tbalances_over_32_bits\t1",
                      "Aland\tUEFA\t30\t0\t0\t15\t0\t1", "Borduria\tUEFA\t30\t17\t23\t0\t0\t1",
                      "Carpania\tUEFA\t0\t0\t0\t0\t0\t0"):
        if want_line not in report:
            bad.append(f"health report: no line {want_line!r}")

    # A club cup from July to May: two matches a day every 3 weeks, a winter break of 105 days, a final on its own,
    # then 40 days to the next first round. One edition a season.
    sys.path.insert(0, HERE)
    import history_check
    import datetime
    days = []
    for season in (2038, 2039):
        d = datetime.date(season, 7, 8).toordinal()
        while d < datetime.date(season + 1, 5, 20).toordinal():
            days += [(d, 0, 1), (d, 2, 3)]
            d += 105 if datetime.date.fromordinal(d).month == 11 else 21
        days.append((datetime.date(season + 1, 5, 29).toordinal(), 0, 2))
    got = [(y, len(t)) for y, _, _, t in history_check.editions_of(days)]
    if got != [(2039, 4), (2040, 4)]:
        bad.append(f"editions of a July to May cup: {got}, not [(2039, 4), (2040, 4)]")

    # nations.tsv holds the renames of the season code.
    bad += renames_match()
    bad += uefa_moves_match()
    bad += uefa_missing_rule()

    for b in bad:
        print("FAIL  " + b)
    print(f"{'ok  ' if not bad else 'FAIL'}  history checker: {len(want)} planted history and nation faults, "
          f"{len(ENTRANT_FAULTS)} entrant and {len(HEALTH_FAULTS)} health faults found, rules and known list applied "
          f"(work folder {work})")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
