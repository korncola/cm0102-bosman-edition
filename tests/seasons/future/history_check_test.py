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
import datetime
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
         ("Elbonia Rovers", 4)] + [(f"Aland Club {k}", 0) for k in range(8)]     # 6 to 13: a Champions Cup
SIDE0 = 100                                  # national side ids follow the club ids
# id, name, continent, nation, file
COMPS = [(0, "Aland League", 2, 0, "club"), (1, "Euro Super Cup", 2, -1, "club"),
         (10, "World Finals", -1, -1, "nation"), (11, "Euro Finals", 2, -1, "nation"),
         (12, "Asia Finals", 1, -1, "nation"), (13, "Americas Finals", 3, -1, "nation"),
         (14, "Hatay Cup", 5, -1, "nation"), (15, "Nations Shield", 1, -1, "nation"),
         (16, "Champions Match", 2, -1, "club"), (17, "Qual Zone", 1, -1, "nation"),
         (18, "Durania Cup", 1, 3, "club"), (19, "Pacific Bowl", -1, -1, "club"), (20, "Champions Cup", 2, -1, "club"),
         (21, "Holders Cup", 2, -1, "club"), (22, "Holders Final", 2, -1, "club"),
         (23, "Club Trophy", 2, -1, "club"), (24, "Friendly", -1, -1, "nation"), (25, "Club Shield", 2, -1, "club"),
         (26, "Aland Second", 2, 0, "club"), (27, "Asia Club Cup", 1, -1, "club")]
LEVELS = {0: 20, 26: 10}                     # the level of a division (+0x69), higher is better
# The places of the Champions Cup in the save (euro-place, open item 94): club id: (division, last division, last
# place). Aland City from 1st place passes; Aland Club 0 from the second division and Aland Club 1 from 9th place
# are faults. Every other club has no mark.
EURO_MARKS = {0: (0, 0, 1), 6: (0, 26, 2), 7: (0, 0, 9)}
EURO_FAULTS = {("euro-place", "Champions Cup", 2040)}


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
    rows.append((21, 2039, 6, 7, -1, -1))                             # Holders Cup: Aland Club 0 holds the title
    rows.append((22, 2039, 6, 7, -1, -1))                             # Holders Final: the same holder
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
    for y in range(2015, 2031):                                       # Durania Cup: not played after 2030
        rows.append((18, y, 4, -1, -1, -1))
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
                  "rename\t2040\t07-08\tOsterlich\tNova Osterlich\tdue on the save date, day 189 of 2040\n"
                  "confed\t2040\t01-10\tCarpania\tUEFA\tdue; in the Asia Club Cup of 2039/40, which began before\n"
                  "confed\t2041\t01-10\tElbonia\tUEFA\tnot due; Elbonia Rovers (AFC) in the Champions Match of 2040 "
                  "stays a fault\n")
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
# Pacific Bowl (the Suruga Bank shape, open item 77): the winners of the Durania Cup, a national cup that the game
# does not play after 2030, and of the Aland League. Its place goes to a drawn club of Durania: Elbonia Rovers (AFC,
# not of Durania) in 2039 is a fault, Durania Stars in 2040 is not.
FIXTURES_2039 = [(16, 0, 3, 220), (16, 4, 0, 222), (1, 0, 1, 100), (13, side(6), side(8), 150),
                 (17, side(3), side(4), 120), (17, side(0), side(11), 124), (17, side(12), side(3), 127),
                 (15, side(0), side(1), 130), (19, 5, 0, 140)] + \
    [(20, 6 + k, 7 + k, 250 + 7 * k) for k in range(0, 8, 2)] + \
    [(27, 4, 3, 300)]       # Asia Club Cup 2039/40 (AFC): Carpania FC enters before Carpania moves to UEFA on 10
                            # January 2040; its row year is 2040, no fault (the AFC club cups of 2001/02 and Kazakhstan)
FIXTURES_2040 = [(16, 0, 5, 150), (16, 3, 0, 250), (1, 0, 3, 100),   # the match of day 250 is after the save date
                 (27, 3, 4, 20),
                 (19, 4, 0, 140),
                 (21, 8, 9, 187), (21, 10, 11, 187),   # Holders Cup 2040: a qualifying day 2 days before the save date,
                                                      # still running (open item 70): its holder enters later, no fault
                 (22, 8, 9, 180), (22, 10, 11, 180), (22, 8, 10, 186),   # Holders Final 2040: a final (one match) 3 days
                                                                        # before the save date has ended: its holder fault
                 (23, 0, 6, 200), (24, side(0), side(1), 201),   # duty clash: Aland City (3 of Aland's) the day before
                                                                 # a friendly of Aland's senior side (open item 93)
                 (25, 1, 7, 200),                                # no clash: Aland United has 2 in Aland's squad,
                 (25, 0, 8, 203),                                # Aland City 2 days after the friendly,
                 (25, 0, 9, 188), (24, side(0), side(2), 187)]   # a match before the save date
ENTRANT_FAULTS = {("entrant-count", "World Finals", 2038), ("entrant-host", "World Finals", 2038),
                  ("entrant-route", "Champions Match", 2039), ("entrant-route", "Champions Match", 2040),
                  ("entrant-confed", "Champions Match", 2040), ("entrant-holder", "Euro Super Cup", 2040),
                  ("entrant-host", "Americas Finals", 2039), ("entrant-dead", "Qual Zone", 2039),
                  ("entrant-confed", "Asia Shield", 2039), ("entrant-count", "Champions Match", 2039),
                  ("entrant-limit", "Champions Cup", 2039),
                  ("entrant-route", "Pacific Bowl", 2039), ("entrant-holder", "Holders Final", 2040)}
ENTRANT_FAULTS_PLAIN = {("duty-clash", "Club Trophy", 2040),          # needs no rules file
                        ("entrant-dead", "Qual Zone", 2039),       # found with no rules file too; Ruritania
                        ("entrant-dead", "Americas Finals", 2039)}  # plays under the name its rename replaced
TABLE = '{"world", 2037, 2038, 4, 0, 2038, 6, 1, "Carpania/Karpania", "row-1"},\n'
ACCESS = ("comp\tfrom\tto\tteams\troutes\tsource\nchamp\t2030\tnone\t2\twinner:aland;winner:esc\tmade up\n"
          "pacific\t2030\tnone\t2\twinner:durania_cup;winner:aland\tmade up\n"
          "esc\t2030\tnone\t\tholder\tmade up\n"
          "holders\t2030\tnone\t\tholder\tmade up\n"
          "holders_final\t2030\tnone\t\tholder\tmade up\n")


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
                 ("empty-squad", "Borduria under-21", 2040), ("overflow", "age", 2040), ("overflow", "balance", 2040),
                 ("full-squad", "Carpania FC", 2040), ("full-squad", "Elbonia Rovers", 2040)}
# Full squads (open item 78). Ages by the age field: Aland's 30 players (staff 0 to 29) are 30; of Borduria's 30
# (staff 30 to 59), 21 are 21 or under (YOUNG, staff 59 by its wrong age field 7). Abilities (player.dat): Aland 50,
# Borduria 60 for the older, 100 + id for YOUNG, so YOUNG are the best. Each full club lists YOUNG last, so the
# slot order alone would keep them out of the 14 best.
# - Carpania FC: Aland's players only, no one 21 or under: a fault.
# - Elbonia Rovers: YOUNG and 29 of Aland's. The 14 best are young, 7 young remain: a fault.
# - Borduria Sport: YOUNG with staff 59 twice and 28 of Aland's: 8 young remain, no fault.
# - Durania Stars: 49 of Aland's in 49 slots, not full: no fault.
YOUNG = [30 + k for k in range(30) if 24 - k % 10 <= 21]
FULL_SQUADS = {"Aland City": [1, 3, 5],                           # duty clash: 3 in Aland's squad (odd ids)
               "Aland United": [7, 9],                               # 2 in Aland's squad: no duty clash
               "Carpania FC": list(range(30)) + list(range(20)),
               "Elbonia Rovers": list(range(29)) + YOUNG,
               "Borduria Sport": list(range(28)) + YOUNG + [59],
               "Durania Stars": list(range(30)) + list(range(19))}


def players():
    out = bytearray()
    for i in range(60):
        r = bytearray(0x46)
        struct.pack_into("<ih", r, 0, i, 0)
        struct.pack_into("<h", r, 5, 50 if i < 30 else 100 + i if i in YOUNG else 60)
        out += r
    return bytes(out)


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

    def club(cid, name, nation, manager=-1, squad=()):
        r = bytearray(0x245)
        struct.pack_into("<i", r, 0, cid)
        struct.pack_into("<i", r, 0x5b, -1)                          # no last division
        struct.pack_into("<i", r, 0x1db, -1)                         # no European place
        if cid in EURO_MARKS:
            div, last, place = EURO_MARKS[cid]
            struct.pack_into("<ii", r, 0x57, div, last)
            struct.pack_into("<i", r, 0x1db, 20)
            r[0x5f] = place
        for j in range(50):                                          # squad slots, -1 when empty
            struct.pack_into("<i", r, 0xD7 + 4 * j, squad[j] if j < len(squad) else -1)
        r[4:4 + len(name)] = name.encode()
        struct.pack_into("<i", r, 0x53, nation)
        struct.pack_into("<i", r, 0xCF, manager)
        return bytes(r)

    clubs = b"".join(club(i, n, nat_, squad=FULL_SQUADS.get(n, ())) for i, (n, nat_) in enumerate(CLUBS))
    sides = b"".join(club(side(i), SIDE_NAMES.get(n, n), i, 0 if i < 2 else -1) for i, (n, _) in enumerate(NATIONS))
    sides += b"".join(club(side(len(NATIONS) + i), n + " U21", i, -1) for i, (n, _) in enumerate(NATIONS))
    comp = {"club": bytearray(), "nation": bytearray()}
    for cid, name, c, n, part in COMPS:
        r = bytearray(0x6B)
        struct.pack_into("<i", r, 0, cid)
        r[4:4 + len(name)] = name.encode()
        struct.pack_into("<ii", r, 0x59, c, n)
        struct.pack_into("<h", r, 0x69, LEVELS.get(cid, 0))
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
               ("player.dat", players()), ("finance.dat", finance())]
    # Two files in a merged part (0.6.2 slice 13, savfile.merge): the World Finals class waiting on 2200 and the
    # fixtures of 2039 give planted faults only when the checker reads them from it.
    merged = [m for m in members if m[0] in ("World Finals_10.tmp", "fixtures_2039.tmp")]
    members = [m for m in members if m not in merged] + [("x#.tmp.parts", savfile.merge(merged))]
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
    # entrant-count: in a base game the OFC Nations Cup keeps the original's 6 (open item 45), in the table, after it
    # and in a year between; another class keeps the table's count; the hosts stay
    ofc = history_check.Access({"oceania_nations": [(2002, 8, ["New Zealand"]), (2008, 4, [])],
                                "world_cup": [(2006, 32, ["Borduria"])]})
    for key, year, base, want in (("oceania_nations", 2002, False, (8, ["New Zealand"], [])),
                                  ("oceania_nations", 2002, True, (6, ["New Zealand"], [])),
                                  ("oceania_nations", 2004, False, (None, [], [])),
                                  ("oceania_nations", 2004, True, (6, [], [])),
                                  ("oceania_nations", 2030, False, (4, [], [])),
                                  ("oceania_nations", 2030, True, (6, [], [])),
                                  ("world_cup", 2006, True, (32, ["Borduria"], []))):
        got = ofc.rule(key, year, base)
        if got != want:
            bad.append(f"access rule of {key} {year} (base game {base}): {got}, not {want}")
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
    # open item 76: after the table (no host in access) the hosts come from the save: the host row of HostCountry.tmp
    # (fixed: every nation of its six fields; drawn: its pair; not decided: none) and a UEFA nation that played the
    # finals without the qualifying (an automatic place: a host, as a season game has no holder)
    for year, first, entered, extra, want in ((2037, 2036, {"Borduria", "Kosovo"}, {"Aland"}, []),
                                              (2037, 2036, {"Kosovo"}, {"Aland", "Borduria"}, []),
                                              (2037, 2036, {"Kosovo"}, {"Aland"}, ["Borduria"])):
        got = history_check.uefa_missing(game, access, changes, year, first, entered, extra)
        if got != want:
            bad.append(f"entrant-missing of {year} with the hosts {sorted(extra)}: {got}, not {want}")
    for finals, entered, host, want in (({"Aland", "Borduria", "Kosovo"}, {"Kosovo"}, "Aland", {"Aland", "Borduria"}),
                                        ({"Aland", "Kosovo"}, {"Kosovo"}, "Borduria", set()),   # host not among them
                                        ({"Aland", "Borduria", "Russia"}, set(), "Aland", set()),  # more than a pair
                                        ({"Aland"}, set(), "", set())):                        # no history host
        got = history_check.finals_hosts(finals, entered, host)
        if got != want:
            bad.append(f"finals_hosts of {sorted(finals)} less {sorted(entered)}, host {host}: {sorted(got)}, "
                       f"not {sorted(want)}")
    game.host_rows = [(7, 2038, [0, 1, -1, -1, -1, -1], 0xfd), (7, 2042, [3, -1, 0, 1, 1, -1], 1),
                      (7, 2046, [0, -1, 1, -1, 3, -1], 0xfe), (8, 2038, [4, -1, -1, -1, -1, -1], 0xfd),
                      (7, 2050, [0, 5, -1, -1, -1, -1], 0xfd), (7, 2054, [3, 99, -1, -1, -1, -1], 0xfd),
                      (7, 2054, [0, 1, -1, -1, -1, -1], 0xfd), (7, 2058, [0, 1, 3, -1, -1, -1], 3)]
    # 2054: the first row of a competition and year wins (row_find), an id outside the nation table is left out;
    # 2058: a choice of 3 is neither a pair nor fixed
    for comp, year, want in ((7, 2038, {"Aland", "Borduria"}), (7, 2042, {"Aland", "Borduria"}), (7, 2046, set()),
                             (7, 2050, {"Aland", "Dead Land"}), (7, 2054, {"Kosovo"}), (7, 2058, set()),
                             (7, 2062, set())):
        got = history_check.save_hosts(game, comp, year)
        if got != want:
            bad.append(f"save_hosts of competition {comp} in {year}: {sorted(got)}, not {sorted(want)}")
    return bad


def titles_rule():
    """duplicate: three Argentine titles in 2025 (the AFA's Campeón de Liga beside the Apertura and Clausura) are no
    fault in the database rows of a 2026 start, but are in a game that writes its own 2025 rows."""
    sys.path.insert(0, HERE)
    import history_check
    bad = []
    for name, year, start, most, want in (("Argentine Premier Division", 2025, 2026, 2, 3),
                                          ("Argentine Premier Division", 2026, 2026, 2, 2),
                                          ("Argentine Premier Division", 2025, 2025, 2, 2),
                                          ("Argentine Premier Division", 2025, 2001, 2, 2),
                                          ("Mexican First Division", 2025, 2026, 2, 2)):
        got = history_check.titles_allowed(name, year, start, most, (1, 0))
        if got != want:
            bad.append(f"titles_allowed {name} {year} (start {start}): {got}, not {want}")
    return bad


def start_season_rule():
    """missing and gap (open item 169): an edition of a season before the start season is history of the database.
    The World Cup of 2098 ends the 2097/98 season, so a 2098 game (a date jump with December 2025 data, whose last
    World Cup row is 2094) next plays 2102, and the Euro of 2096 (season 2095/96) is history there too. A year of the
    table without an edition (the Africa Cup of Nations of 2020) is no history, nor a competition outside the
    table."""
    sys.path.insert(0, HERE)
    import history_check as hc
    bad = []
    for key, last, cyc, start, want in (("world_cup", 2094, 4, 2098, 2102), ("world_cup", 2095, 4, 2098, 2099),
                                        ("world_cup", 2018, 4, 2022, 2022), ("world_cup", 2022, 4, 2026, 2030),
                                        ("world_cup", 2018, 4, 2021, 2022), ("african_nations", 2018, 2, 2025, 2020),
                                        ("euro_champ", 2092, 4, 2098, 2100), ("", 2094, 4, 2098, 2098)):
        got = hc.due_edition({"key": key}, last, cyc, start)
        if got != want:
            bad.append(f"due_edition {key or 'a competition outside the table'} after {last} in a {start} start: "
                       f"{got}, not {want}")
    for key, years, start, want in (("world_cup", [2098], 2098, True), ("world_cup", [2098, 2102], 2098, False),
                                    ("african_nations", [2020], 2025, False),
                                    ("world_cup", [], 2098, False)):
        if hc.before_start(key, years, start) != want:
            bad.append(f"before_start {key} {years} in a {start} start: not {want}")
    return bad


def not_held_rule():
    """missing and gap (open item 173): the OFC Nations Cup of 2020 was cancelled (OFC, 21 April 2020); the rules
    table holds 2016 and 2024, so after 2016 the next edition due is 2024, and a gap from 2016 to 2024 is none."""
    sys.path.insert(0, HERE)
    import history_check as hc
    ofc = {"key": "oceania_nations", "years": [2002, 2004, 2008, 2012, 2016, 2024]}
    bad = []
    for rule, last, start, want in ((ofc, 2016, 2002, 2024), (ofc, 2012, 2002, 2016), (ofc, 2024, 2002, 2028),
                                    ({"key": "oceania_nations"}, 2016, 2002, 2020)):
        got = hc.due_edition(rule, last, 4, start)
        if got != want:
            bad.append(f"due_edition {rule} after {last}: {got}, not {want}")
    if not hc.not_due(ofc, [2020], 2002) or hc.not_due(ofc, [2020, 2024], 2002) or hc.not_due(ofc, [2028], 2002):
        bad.append("not_due of the OFC Nations Cup: 2020 must be not due, 2024 and 2028 due")
    return bad


def olympics_rule():
    """duty-clash and the Olympics (open item 107): an Olympic match of the under-21 sides is a fault only when a club
    with a player in it keeps fewer than 11 players it can field (not on duty, at the club, not injured). The save
    is built in memory: clubs 1 to 5, senior sides 100 and 101, under-21 sides 110 and 111, and a match day
    2040-08-01 after the save date. The Olympics are found by the name in the data (bosman_season.dat), so the
    renamed record 30 is the Olympics and record 31, "Olympic Qualifying", is not."""
    sys.path.insert(0, HERE)
    import history_check as hc
    day = datetime.date(2040, 8, 1).toordinal()
    people, squads = [], {}                       # people: (staff id, side, club, injured)

    def add(club, n, side=-1, at=None, hurt=False):
        for _ in range(n):
            people.append((len(people), side, club if at is None else at, hurt))
            squads.setdefault(club, []).append(len(people) - 1)

    # 1: item 93's West Bromwich: 20 of 50 on duty, 11 injured and 10 on loan at club 9, so it fields 9 of 50.
    add(1, 20, side=110), add(1, 11, hurt=True), add(1, 10, at=9), add(1, 9)
    add(2, 2, side=110), add(2, 2, side=111), add(2, 1, side=100), add(2, 5)   # 2: two players in each of two
    # Olympic sides and one in a side that plays Olympic qualifying that day: keeps 5
    add(3, 3, side=110), add(3, 11)                           # 3: three in an Olympic side, keeps 11: no fault
    add(4, 3, side=101), add(4, 11)                           # 4: three in a senior side that plays the Olympics
    add(5, 3, side=100), add(5, 20)                           # 5: three in a side that plays Olympic qualifying
    add(6, 15)                                                # 6: no player in a national side
    staff = bytearray()
    for sid, side, club, _ in people:
        r = bytearray(0x6E)
        struct.pack_into("<i", r, 0, sid)
        struct.pack_into("<i", r, 0x24, side)
        struct.pack_into("<i", r, 0x39, club)
        staff += r
    fitness = 0x1F                                # injury.dat: a fitness record per staff id (0x62ab00)
    injury = bytearray(fitness * len(people))
    for sid, _, _, hurt in people:
        struct.pack_into("<i", injury, fitness * sid + 0xE, 0 if hurt else -1)
    clubs = bytearray()
    for cid in range(1, 7):
        r = bytearray(hc.CLUB)
        struct.pack_into("<i", r, 0, cid)
        ids = squads[cid] + [-1] * (hc.SLOTS - len(squads[cid]))
        struct.pack_into(f"<{hc.SLOTS}i", r, 0xD7, *ids)
        clubs += r
    comps = bytearray()
    for cid in range(32):
        r = bytearray(hc.COMP)
        struct.pack_into("<i", r, 0, cid)
        comps += r
    renamed = b"Olympic Games".ljust(51, b"\0")
    season = struct.pack("<4i", 2, 0, 2025, 1) + struct.pack("<i", 1) + struct.pack("<3i", 2039, 0, 3) + b"abc" + \
        struct.pack("<ii", 1, 30) + renamed + struct.pack("<i", 2040)

    class Sav:
        parts = {"staff.dat": bytes(staff), "injury.dat": bytes(injury), "club.dat": bytes(clubs),
                 "club_comp.dat": bytes(comps), "nation_comp.dat": b"", "bosman_season.dat": season}

        members = parts                           # LazySav: the members of the file

        def has(self, name):
            return name in self.parts

        def read(self, name):
            return self.parts[name]

    class Game:
        pass

    g = Game()
    g.sav, g.start, g.date = Sav(), 2025, (2040, 189)
    g.data_names = hc.data_names(g.sav, season) if hasattr(hc, "data_names") else {}
    names = {20: "Club Plate", 21: "Club Bowl", 22: "Club Vase", 23: "Club Cup", 24: "Friendly", 25: "Club Shield",
             30: "Summer Games", 31: "Olympic Qualifying"}
    g.comps = {cid: {"name": n, "nation": -1} for cid, n in names.items()}
    g.sides = {100, 101, 110, 111}
    g.club = lambda cid: (f"club {cid}", "", "")
    g.fixture_list = {30: [(day, 110, 111), (day, 101, 111)], 24: [(day, 101, 111)], 31: [(day, 100, 111)],
                      20: [(day, 1, 3)], 21: [(day + 1, 2, 3)], 22: [(day, 4, 3)], 23: [(day, 5, 3)],
                      25: [(day, 6, 3)]}                  # club 3 alone: it keeps 11, no fault
    bad = []
    if not hasattr(hc, "olympics") or hc.olympics(g) != {30}:
        bad.append("olympics: the renamed record 30 is not found by its name in the data")
    got = {f[1]: f[3] for f in hc.duty_clashes(g)}
    want = {"Club Plate": "club 1 on 2040-08-01 keeps 9 players",
            "Club Bowl": "club 2 on 2040-08-02 keeps 5 players",
            "Club Vase": "club 4 on 2040-08-01 has 3 players in the squad of club 101",
            "Club Cup": "club 5 on 2040-08-01 has 3 players in the squad of club 100"}
    if set(got) != set(want) or any(not got[k].startswith(v) for k, v in want.items()):
        bad.append(f"duty-clash Olympics: {got}, want {want}")
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
                "uefa_cl\tChampions Cup\tclub_continental\tUEFA\t0\t1\t2041\tnone\n"
                "wc_qual_zone\tQual Zone\tintl_qual\tAFC\t0\t4\t2041\tnone\n"
                "durania_cup\tDurania Cup\tdomestic_cup\tDurania\t0\t1\t2031\t2030\n"
                "pacific\tPacific Bowl\tsuper_cup\tAFC|UEFA\t0\t1\t2041\tnone\n"
                "holders\tHolders Cup\tclub_continental\tUEFA\t0\t1\t2041\tnone\n"
                "holders_final\tHolders Final\tclub_continental\tUEFA\t0\t1\t2041\tnone\n"
                "acc\tAsia Club Cup\tclub_continental\tAFC\t0\t1\t2041\tnone\n")
    names = os.path.join(work, "names.tsv")
    with open(names, "w", encoding="utf-8") as f:
        f.write("# a note line\npattern\tcomp\nAmericas Finals\tamericas\n(The )?World Finals\tworld\n"
                "Euro Super Cup\tesc\nNations Shield\tshield\nAsia Shield\tasia_shield\n"
                "Champions Match\tchamp\nAland League\taland\nQual Zone\twc_qual_zone\n"
                "Durania Cup\tdurania_cup\nPacific Bowl\tpacific\nChampions Cup\tuefa_cl\nHolders Cup\tholders\nHolders Final\tholders_final\n"
                "Asia Club Cup\tacc\n")
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
         ("confederation", "Asia Shield", 2032)} | ENTRANT_FAULTS | EURO_FAULTS
    if set(got) != want2:
        bad.append(f"rules run: missed {sorted(want2 - set(got))}, extra {sorted(set(got) - want2)}")
    with open(os.path.join(work, "rules.faults.tsv"), encoding="utf-8") as f:
        euro = sorted(l.split("\t")[4].split(" (")[0] for l in f if l.startswith("euro-place\t"))
    if euro != ["Aland Club 0", "Aland Club 1"]:
        bad.append(f"rules run: euro-place faults of {euro}, not Aland Club 0 and Aland Club 1")
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
    # --tier-a: the same save as a Tier A game has no same-final fault (a check of real results), and the rest stays.
    run([sav, "--out", work, "--label", "tier-a", "--known", empty, "--nations", nations, "--tier-a"])
    plain_faults, tier_a = set(faults(work, "plain")), set(faults(work, "tier-a"))
    if not any(f[0] == "same-final" for f in plain_faults) or tier_a != {f for f in plain_faults if f[0] != "same-final"}:
        bad.append(f"--tier-a: faults {sorted(tier_a ^ {f for f in plain_faults if f[0] != 'same-final'})} differ")

    off = os.path.join(work, "off.sav")
    write_sav(off, records(changing=0)[0])
    # The association limit of the season rules: a fault with changing rules on, none in the base game (lead, 1 Nov).
    for label, save, want_limit in (("limit-on", sav, True), ("limit-off", off, False)):
        run([save, "--out", work, "--label", label, "--known", empty, "--nations", nations, "--rules", rules,
             "--names", names, "--table", table, "--access", access])
        got = ("entrant-limit", "Champions Cup", 2039) in faults(work, label)
        if got != want_limit:
            bad.append(f"entrant-limit ({label}): {'found' if got else 'not found'}")
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
    got = {k for k in faults(work, "health") if k[0] in ("empty-squad", "no-youth", "overflow", "full-squad")}
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
    # A knockout that pauses (open item 70, the Copa Libertadores of 2041 in a run set game): quarter-finals of 4
    # matches, then single-match days on 5, 9 and 15 June and, after a break of 25 days, on 10, 17 and 24 July. One
    # edition.
    o = datetime.date(2041, 1, 1).toordinal() - 1
    ko = [(o + 142, 0, 1), (o + 142, 2, 3), (o + 142, 4, 5), (o + 142, 6, 7), (o + 149, 1, 0), (o + 149, 3, 2),
          (o + 149, 5, 4), (o + 149, 7, 6), (o + 156, 0, 2), (o + 160, 4, 6), (o + 166, 2, 0), (o + 191, 6, 4),
          (o + 198, 0, 4), (o + 205, 4, 0)]
    got = [(y, len(t)) for y, _, _, t in history_check.editions_of(ko)]
    if got != [(2041, 8)]:
        bad.append(f"editions of a knockout that pauses: {got}, not [(2041, 8)]")

    # Open item 70: a group stage that ends on a December day with one match, then the knockout rounds in February
    # (group clubs only), the final in May, and the next season's first qualifying round in June (clubs that played
    # last in June a year before). One edition a season: the knockout rounds stay in the edition of their group stage.
    days = []
    for season in (2017, 2018):
        o = datetime.date(season, 6, 27).toordinal()
        days += [(o, 10, 11), (o, 12, 13)]
        d = datetime.date(season, 9, 12).toordinal()
        while d < datetime.date(season, 12, 6).toordinal():
            days += [(d, 0, 1), (d, 2, 3), (d, 4, 5), (d, 6, 7)]
            d += 14
        days.append((datetime.date(season, 12, 13).toordinal(), 4, 5))
        k = datetime.date(season + 1, 2, 13).toordinal()
        days += [(k, 0, 1), (k, 2, 3), (k + 63, 0, 2)]
    got = [(y, len(t)) for y, _, _, t in history_check.editions_of(days)]
    if got != [(2018, 12), (2019, 12)]:
        bad.append(f"editions of a cup whose group stage ends on a day with one match: {got}, not "
                   f"[(2018, 12), (2019, 12)]")
    # A cup with a first round in December, a group day of 4 matches in March, a final on its own, then 30 days
    # later a new edition whose teams played last in the first round, 97 days before. Two editions: the teams of
    # the new first day did not play in the 60 days before the final.
    a = datetime.date(2030, 3, 1).toordinal()
    days = [(a - 90, 8, 9), (a - 90, 10, 11), (a, 0, 1), (a, 2, 3), (a, 4, 5), (a, 6, 7), (a + 7, 0, 2),
            (a + 37, 8, 9), (a + 37, 10, 11)]
    got = [(y, len(t)) for y, _, _, t in history_check.editions_of(days)]
    if got != [(2030, 12), (2030, 4)]:
        bad.append(f"editions of a cup with a final after a group day: {got}, not [(2030, 12), (2030, 4)]")
    # Open item 45: the OFC Nations Cup of 2008, one group of 4 home and away on 17 and 21 November 2007, 14 and 21
    # June, 6 and 10 September 2008 (a pause of 206 days), then the next edition of the same 4 teams in 2012, with a
    # pairing of 2008. One edition each.
    g = [((2007, 11, 17), 0, 1), ((2007, 11, 17), 2, 3), ((2007, 11, 21), 0, 2), ((2007, 11, 21), 1, 3),
         ((2008, 6, 14), 0, 3), ((2008, 6, 14), 1, 2), ((2008, 6, 21), 1, 0), ((2008, 6, 21), 3, 2),
         ((2008, 9, 6), 2, 0), ((2008, 9, 6), 3, 1), ((2008, 9, 10), 3, 0), ((2008, 9, 10), 2, 1),
         ((2012, 6, 1), 0, 1), ((2012, 6, 1), 2, 3)]
    days = [(datetime.date(*d).toordinal(), h, a) for d, h, a in g]
    got = [(y, len(t)) for y, _, _, t in history_check.editions_of(days)]
    if got != [(2008, 4), (2012, 4)]:
        bad.append(f"editions of a home-and-away group with a pause: {got}, not [(2008, 4), (2012, 4)]")
    # A pause of the same length before a day whose pairing met at home already: a new edition.
    days = [(datetime.date(*d).toordinal(), h, a) for d, h, a in g[:4] + [((2008, 6, 14), 0, 1), ((2008, 6, 14), 2, 3)]]
    got = [(y, len(t)) for y, _, _, t in history_check.editions_of(days)]
    if got != [(2007, 4), (2008, 4)]:
        bad.append(f"editions of a group that meets at home again after a pause: {got}, not [(2007, 4), (2008, 4)]")
    # A single round robin of 4 that ends on a day of two matches, then 200 days later the next edition of the same
    # teams with the home sides reversed: two editions (the first had played its whole round robin).
    rr = g[:4] + [((2007, 11, 24), 0, 3), ((2007, 11, 24), 1, 2), ((2008, 6, 14), 1, 0), ((2008, 6, 14), 3, 2)]
    days = [(datetime.date(*d).toordinal(), h, a) for d, h, a in rr]
    got = [(y, len(t)) for y, _, _, t in history_check.editions_of(days)]
    if got != [(2007, 4), (2008, 4)]:
        bad.append(f"editions of a single round robin and its next edition: {got}, not [(2007, 4), (2008, 4)]")

    # Open item 70: the association limit of the Champions League by season.
    def lim(season, nations):
        return [f[3] for f in history_check.limit_faults("CL", season + 1, datetime.date(season, 7, 1).toordinal(),
                                                         nations)]
    for season, nations, want_ in ((2012, ["England"] * 4, []), (2012, ["England"] * 5, ["England has 5 entrants, "
                                   "the limit of 2012/13 is 4"]), (2017, ["England"] * 5 + ["Spain"] * 4, []),
                                   (2017, ["England"] * 6, ["England has 6 entrants, the limit of 2017/18 is 5"]),
                                   (2024, ["England"] * 7, []), (2008, ["England"] * 6, [])):
        if lim(season, nations) != want_:
            bad.append(f"association limit {season}, {len(nations)} clubs: {lim(season, nations)}, not {want_}")

    # nations.tsv holds the renames of the season code.
    bad += renames_match()
    bad += uefa_moves_match()
    bad += uefa_missing_rule()
    bad += titles_rule()
    bad += olympics_rule()
    bad += start_season_rule()
    bad += not_held_rule()

    for b in bad:
        print("FAIL  " + b)
    print(f"{'ok  ' if not bad else 'FAIL'}  history checker: {len(want)} planted history and nation faults, "
          f"{len(ENTRANT_FAULTS)} entrant and {len(HEALTH_FAULTS)} health faults found, rules and known list applied "
          f"(work folder {work})")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
