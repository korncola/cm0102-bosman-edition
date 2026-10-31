#!/usr/bin/env python3
"""History checker for the future runs (step 2 of lead-notes future testing): a saved game in, one TSV of the
history rows of every competition out, then the checks of those rows.

Usage:
    python3 -I tests/seasons/future/history_check.py <save.sav> [<save.sav> ...] [--out <folder>]
        [--rules <competitions.tsv>] [--known <known.tsv>] [--label <name>] [--nations <nations.tsv>]
        [--access <access.tsv>] [--table <season_intl_table.c>] [--health]

--out writes <label>.history.tsv, <label>.faults.tsv, <label>.entrants.tsv and with --health <label>.health.tsv.

The rows are the ones that the winners tab of each competition shows (0x4991f0 draws winner, runner-up, third
and host): the club comp history and the nation comp history of the save (club_comp_history.dat and
nation_comp_history.dat, 26 bytes a row: +0x00 id, +0x04 competition id, +0x08 u16 label year, +0x0a first,
+0x0e second, +0x12 third, +0x16 host, -1 for none; docs/research/2026-10-17-competition-history-fill.md). A row
with a negative competition is a free row. Club and national side ids index club.dat and then nat_club.dat
(581 bytes a record, +0x53 nation). A nation (nation.dat, 290 bytes) has its continent at +0x71. A competition
(club_comp.dat and nation_comp.dat, 107 bytes, the record id is the competition id) has its continent at +0x59
and its nation at +0x5d.

The save date is the latest date of the news list (news.dat: 100 items of 223 bytes after an 8-byte head, the
s16 day of the year from 0 and the s16 year at +0xc9 of each item). Inference: it matches the test logs of the gate runs (date 182/2028 in the
log, last news 189/2028 at the save after it). The start year and changing rules are the words +8 and +0xc of
bosman_season.dat (docs/research/2026-10-17-season-save-part.md, section 4); a save without that part is a 2001
game with changing rules off.

The edition year of a finals class: its .tmp part holds the word after the bytes ff ff ff ff 07 03 xx xx, seven
zero bytes, one byte and three zero bytes (inference from the parts of a 2028 save: FIFA World Cup 2030, the
classes after their table 2200, the ended cups 2999). An edition in progress writes a longer part, where the
pattern is absent; then the check reads nothing.

Checks (one row of <label>.faults.tsv each, check names in the first column):
    duplicate     a competition holds the same label year twice (three times for a split season: two rows a year in
                  most of the 10 years before the start, or an Argentine or Mexican competition with two rows in any
                  year before it: the game plays two titles a year there; the formats of SEVERAL_TITLES)
    no-host       a row with no host in a competition whose rows have hosts (more than half of its rows)
    confederation a winner or runner-up from another confederation, in a row from the start year on. The
                  confederation is the rules file's, else the competition's continent, else the continent of
                  80% or more of the winners before the start year (at least 3); none: no check
    same-final    the same two finalists more than 2 editions in a row (nation competitions, and club
                  competitions with no nation; with --rules every competition whose kind is not a league)
    gap           a gap in the cycle from the start year on: two editions further apart than the cycle and one
                  year. The cycle is the rules file's, else the most common step of the last 6 editions before
                  the start year
    (a competition whose rules cycle is 0 has ended: it gets none of the three checks below)
    missing       no edition after the last one up to the save date (the last edition + cycle + 1 < the save
                  year), for a competition with rows from the start year on or an international one; not for a
                  competition whose class waits on 2999 (ended) or whose rules end year has passed
    next-edition  the class waits for the year 2199 or 2200 (the idle year of rt_intl_next_year), or for a
                  year more than one cycle and one year after the save date
Checks of the competition record (with --rules):
    comp-continent the record's continent is not a confederation of its rules (open item 21)
    comp-name     the short name (+0x38, 26 bytes, the name of the history screen) is another competition than the
                  long name; the rows are then also checked against the confederation of the short name, under
                  that name (open item 4: the UEFA Nations League record named "Asian Cup Quals")
    A winner whose nation moves confederation after the row's year (nations.tsv: Australia, OFC to 2005) passes.

Nations (--nations, default nations.tsv next to this file), for the changes due at the save date:
    nation-name   a live record (with a continent) under the old name of a rename
    side-name     a national side named with the old name while its nation holds the new one
    nation-confed a nation in another confederation than a move gives

Entrants: the fixtures parts give the teams of each edition (an edition: fixtures with no gap of more than 150
days, or after a final: a day of one match and then 25 days or more with none; its year the year of the last
match). The list gives each team's route in: title holder (the winner of
the edition before), host, the winner of a route competition, a winner or place 2 or 3 in the history of the
edition's year or the year before, else "qualifier". Checks, for the editions whose last match is before the
save date (a confederation check for every edition):
    entrant-confed entrants from another confederation than the rules give (also the short name's); in the World
                  Cup qualifying up to 2 teams of each other confederation pass (the play-off between
                  confederations); a nation with a due move to a confederation of the competition passes, and so
                  does the winner of a route competition (a Mexican winner of the Copa Sudamericana in the Recopa)
    entrant-count  a finals edition with another count of teams than the finals table of the season code
                  (season_intl_table.c; after its last row the count of the last row), or than access.tsv
    entrant-host   a host of the finals table that the data holds is not an entrant; after the table, the host of
                  the edition's history row
    entrant-dead   entrants of a nation under the old name of a due rename, in an edition after the rename's
                  season (the dead record beside the live one,
                  or a record the rename missed); a nation with no continent in the data (Bonaire, Guadeloupe)
                  is no fault
    entrant-missing a live UEFA nation with a national side is not an entrant of a European World Cup qualifying
                  edition of at least 30 teams that began in the game: every member enters, less the European
                  hosts of its World Cup, less Russia from 2026 and less a nation that joins UEFA later (open item
                  57). Only where the season code builds the whole pool (uefa_all): not the 2002 campaign, not a
                  2001 game with changing rules off, from 2026 only with changing rules on or a 2025 start
    entrant-holder the title holder is not an entrant (access.tsv "holder": the Champions League from 2006, the
                  Libertadores), when the edition before is at most one cycle and one year before; not for an
                  edition of the start year or before (it began before the game, the holder may be out)
    entrant-route  an entrant of a fixed-route competition (access.tsv: UEFA Super Cup, Recopa, Suruga Bank,
                  Intercontinental Cup, Finalissima, FIFA Intercontinental Cup) won none of its route competitions
                  in the year of the match or the edition before, when that edition is at most one cycle and one
                  year before (checked when every route competition has history rows, for an edition after the
                  start year: an edition of the start year may come from the data); a route competition with no
                  row written in the game (a year after the start) in the year or one cycle before did not play
                  (its nations are not selected), and its place goes to a drawn club, as the original draws one
                  (WorldClubChamp::pick_entrant 0x92ae60, World_Club_Cup 0x92baa8): each such route lets one
                  entrant of its confederation come by no route (slice 9a)
    save-part      a fixtures part whose size does not fit its count (not read)

World health (--health), Tier B: <label>.health.tsv holds the save size and part count, the players, under-21 and
under-23 players (age from the birth date at the save date), the squads of the senior and under-21 sides, the
clubs and national sides with a manager, and the club balances (lowest, highest, outside 32 bits), then one row
per live nation. Time per game day is not in a save. Checks:
    empty-squad   a senior side with no player in its squad while 23 players or more hold the nationality, or an
                  under-21 side with none while 11 or more of them are under 21 (Q17)
    no-youth      a nation with 23 players or more and none under 21 or none under 23 (Q31: the under-21 and
                  Olympic squads then empty). A fault that fails a Tier B run (Q35, open item 52)
    overflow      an age field that is not the age from the birth date (more than 1 year apart), or a balance
                  of 2^62 or more

--rules (default: competitions.tsv next to this file, when it is there; lines with # are notes): per key (column comp) the columns
kind, confed (FIFA or nation: no check, else codes joined by |), has_host (0: no host check, 1: every row from
the start on needs one), cycle (0: the competition ended) and end_year (a year or none). --names (default
history_names.tsv next to this file) gives the key of each competition name of a save; a name with no key keeps
the rules above. --known: a TSV with check, competition (* for any text), year
(or *) and a note; a fault that matches is printed as "known". Exit code 1 when a fault is not known.
"""
import collections
import csv
import datetime
import fnmatch
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "save"))
import savfile  # noqa: E402

CLUB, NATION, COMP, CONT, HIST = 0x245, 0x122, 0x6B, 0xC6, 0x1A
IDLE_YEARS = (2199, 2200)
ENDED_YEAR = 2999
EDITION = re.compile(rb"\xff\xff\xff\xff\x07\x03..\x00{7}.\x00\x00\x00(..)", re.S)
PART = re.compile(r"^(.*)_(\d+)\.tmp$")
SEASON = re.compile(r"^\d{4}(-\d{2})? ")
FIXTURES = re.compile(r"^fixtures_\d+\.tmp$")
FIXTURE = 0x4F
GAP = 150                                     # days between two editions, at the least (the Champions League has
                                              # a winter break of 69 days, 105 in the World Cup year 2022)
FINAL_GAP = 25                                # days with no match after a final, at the least
# continent ids of the data (Africa, Asia, Europe, North America, Oceania, South America in every database)
CONF_IDS = ["CAF", "AFC", "UEFA", "CONCACAF", "OFC", "CONMEBOL"]


class LazySav:
    """The members of a .sav, read one at a time (a long game saves 200 MB and more)."""

    def __init__(self, path):
        self.f = open(path, "rb")
        head = self.f.read(savfile.HEADER_SIZE)
        self.version, _, count = struct.unpack_from("<3i", head, 0)
        if self.version not in (1, 2, 3, 4) or not 0 <= count <= savfile.MAX_ENTRIES:
            raise ValueError(f"{path}: not a saved game")
        self.f.seek(0, 2)
        end = self.f.tell()
        ms = []
        for k in range(count):
            o = 12 + k * savfile.ENTRY_SIZE
            off, size = struct.unpack_from("<2I", head, o)
            ms.append((off, size, head[o + 8:o + savfile.ENTRY_SIZE].split(b"\0")[0].decode("latin-1")))
        order = sorted(ms)
        self.members = {}
        for i, (off, size, name) in enumerate(order):
            nxt = order[i + 1][0] if i + 1 < len(order) else end
            self.members[name] = (off, size, nxt - off)
        # The files of merged parts (0.6.2 slice 13, tools/save/savfile.py): read once, kept in memory.
        self.files = {}
        for name in [n for n in self.members if n.lower().endswith(savfile.MERGED)]:
            self.files.update(savfile.split_merged(self.read(name)))

    def names(self):
        return list(self.members) + list(self.files)

    def read(self, name):
        if name in self.files:
            return self.files[name]
        off, size, raw = self.members[name]
        self.f.seek(off)
        data = self.f.read(raw)
        return savfile.decode(data, size)[0] if self.version in (2, 4) else data[:size]


def s32(b, o):
    return struct.unpack_from("<i", b, o)[0]


def text(b, o, n=51):
    return b[o:o + n].split(b"\0")[0].decode("latin-1")


class Game:
    def __init__(self, path):
        sav = LazySav(path)
        self.path = path
        cont = sav.read("continent.dat")
        self.continents = {s32(cont, i * CONT): CONF_IDS[s32(cont, i * CONT)] if 0 <= s32(cont, i * CONT) < 6
                           else text(cont, i * CONT + 4, 26) for i in range(len(cont) // CONT)}
        nat = sav.read("nation.dat")
        self.nations = {}
        for i in range(len(nat) // NATION):
            r = nat[i * NATION:(i + 1) * NATION]
            self.nations[s32(r, 0)] = (text(r, 4), self.continents.get(s32(r, 0x71), ""))
        self.clubs, self.sides, self.side_names = {}, set(), []
        for part in ("club.dat", "nat_club.dat"):
            data = sav.read(part)
            for i in range(len(data) // CLUB):
                r = data[i * CLUB:(i + 1) * CLUB]
                self.clubs[s32(r, 0)] = (text(r, 4), s32(r, 0x53))
                if part == "nat_club.dat":
                    self.sides.add(s32(r, 0))
                    self.side_names.append((text(r, 4), s32(r, 0x53)))
        self.comps = {}
        for kind, part in (("club", "club_comp.dat"), ("nation", "nation_comp.dat")):
            data = sav.read(part)
            for i in range(len(data) // COMP):
                r = data[i * COMP:(i + 1) * COMP]
                name = text(r, 4)
                if name:
                    self.comps[s32(r, 0)] = {"kind": kind, "name": name, "short": text(r, 0x38, 26),
                                             "continent": s32(r, 0x59), "nation": s32(r, 0x5d)}
        self.rows = []
        for part in ("club_comp_history.dat", "nation_comp_history.dat"):
            data = sav.read(part)
            for i in range(len(data) // HIST):
                rid, cid, year, a, b, c, h = struct.unpack_from("<iiHiiii", data, i * HIST)
                if cid >= 0 and cid in self.comps:
                    self.rows.append((cid, year, a, b, c, h))
        news = sav.read("news.dat")
        dates = [struct.unpack_from("<hh", news, o) for o in range(8 + 0xC9, len(news) - 3, 223)]
        dates = [(y, d) for d, y in dates if 0 <= d < 366 and 1900 < y < 3000]
        self.date = max(dates) if dates else (0, 0)
        self.start, self.changing = 2001, 0
        if "bosman_season.dat" in sav.members:
            part = sav.read("bosman_season.dat")
            if len(part) >= 16:
                self.start, self.changing = struct.unpack_from("<ii", part, 8)
        self.fixture_list, self.unread = None, []
        self.sav, self.fixture_parts = sav, sorted(n for n in sav.names() if FIXTURES.match(n))
        self.editions, self.parts = {}, set()
        for n in sav.names():
            m = PART.match(n)
            if not m or int(m.group(2)) not in self.comps:
                continue
            self.parts.add(int(m.group(2)))
            if sav.members[n][1] > 1 << 20:
                continue
            found = EDITION.search(sav.read(n))
            if found:
                self.editions[int(m.group(2))] = struct.unpack("<H", found.group(1))[0]

    def club(self, cid):
        """(name, nation, confederation) of a club or national side id; empty for -1."""
        if cid < 0 or cid not in self.clubs:
            return ("", "", "")
        name, nat = self.clubs[cid]
        nname, conf = self.nations.get(nat, ("", ""))
        return (name, nname, conf)


def tsv(path):
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader((l for l in f if not l.startswith("#")), delimiter="\t"))


class Rules:
    """The expected rules of competitions.tsv, found by competition name through history_names.tsv."""

    def __init__(self, path=None, names=None):
        self.rows = {r["comp"]: r for r in tsv(path)} if path else {}
        self.names = [(re.compile(r["pattern"], re.I), r["comp"]) for r in tsv(names)] if path and names else []

    def key(self, name):
        """The key of a competition name in the names file, or None (a key needs no row in the rules file)."""
        bare = SEASON.sub("", name)
        return next((k for pat, k in self.names if pat.fullmatch(bare)), None)

    def get(self, name):
        """{} or {key, kind, confs (a set, or None: any), has_host (None, 0 or 1), cycle, end}."""
        key = self.key(name)
        r = self.rows.get(key)
        if not r:
            return {}
        confed = r.get("confed", "")
        cycle = number(r.get("cycle"))
        return {"key": key, "kind": r.get("kind", ""), "has_host": number(r.get("has_host")),
                "confs": set(confed.split("|")) if confed and set(confed.split("|")) <= set(CONF_IDS) else None,
                "cycle": cycle or None, "end": number(r.get("end_year")), "ended": cycle == 0}


def read_known(path):
    if not path or not os.path.exists(path):
        return []
    return [r for r in tsv(path) if r.get("check")]


def is_known(known, check, comp, year):
    """The competition of a known row may hold * for any text (fnmatch)."""
    return any(k["check"] == check and fnmatch.fnmatchcase(comp, k["competition"]) and k["year"] in ("*", str(year))
               for k in known)


def number(v):
    try:
        return int(v)
    except (TypeError, ValueError):
        return None


def table(game):
    """[(competition id, kind, name, year, first, second, third, host)] sorted by competition and year; each
    place is (name, nation, confederation)."""
    out = []
    for cid, year, a, b, c, h in game.rows:
        comp = game.comps[cid]
        out.append((cid, comp["kind"], comp["name"], year, game.club(a), game.club(b), game.club(c), game.club(h)))
    return sorted(out, key=lambda r: (r[2], r[0], r[3]))


def cycle_of(years, start):
    """The most common step of the last 6 editions before the start year, or None."""
    before = sorted(y for y in set(years) if y < start)[-7:]
    steps = [b - a for a, b in zip(before, before[1:]) if b > a]
    if not steps:
        return None
    best = collections.Counter(steps).most_common()
    return max(s for s, n in best if n == best[0][1]) if len(steps) > 1 else steps[0]


# Competitions with several rows of one label by their format (docs/research/2026-10-17-competition-history-fill.md,
# "Two rows per label year"): (name pattern, first label year, rows a year).
SEVERAL_TITLES = ((re.compile(r"(UEFA )?Inter-?Toto Cup", re.I), 0, 3),               # one row per joint winner
                  (re.compile(r"Danish (2nd|Second) Divisions?", re.I), 2006, 2))       # East and West from 2006

FRIENDLY = re.compile(r"^(Friendly|Reserve|'B' International|Under 21 International)")


def check(game, rules=None, changes=()):
    """[(check, competition, year, detail)]. changes: the rows of nations.tsv; a nation that moves to another
    confederation after a row's year (Australia to the AFC in 2006) is not checked in that row."""
    rules = rules or Rules()
    moved = collections.defaultdict(list)
    for c in changes:
        if c["kind"] == "confed":
            moved[c["nation"]].append(c["season"])
    by = collections.defaultdict(list)
    for r in table(game):
        by[r[0]].append(r)
    raw = collections.defaultdict(list)
    for cid, year, a, *_ in game.rows:
        raw[cid].append(a)
    save_year, start = game.date[0], game.start
    faults = []
    for cid, rs in by.items():
        comp = game.comps[cid]
        name = comp["name"]
        if FRIENDLY.match(name):
            continue
        rule = rules.get(name)
        sides = sum(1 for a in raw[cid] if a in game.sides)
        teams = "nation" if sides * 2 > len(raw[cid]) else "club"
        international = comp["nation"] < 0
        years = [r[3] for r in rs]
        game_rows = [r for r in rs if r[3] >= start]
        # the same year twice (from the year before the start: older duplicates are rows of the database); a split
        # season (Apertura and Clausura) holds two rows a year
        per_year = collections.Counter(years)
        before = [per_year[y] for y in range(start - 10, start) if per_year[y]]
        split_nation = game.nations.get(comp["nation"], ("", ""))[0] in ("Argentina", "Mexico")
        most = 2 if before and (sum(1 for k in before if k == 2) * 2 > len(before) or
                                (split_nation and any(per_year[y] == 2 for y in per_year if y < start))) else 1
        several = next((k, f) for pat, f, k in SEVERAL_TITLES if pat.fullmatch(name)) \
            if any(pat.fullmatch(name) for pat, f, k in SEVERAL_TITLES) else (1, 0)
        for y, n in sorted(per_year.items()):
            if n > max(most, several[0] if y >= several[1] else 1) and y >= start - 1:
                pairs = "; ".join(f"{r[4][0]} / {r[5][0]}" for r in rs if r[3] == y)
                faults.append(("duplicate", name, y, f"{n} rows: {pairs}"))
        # a missing host where the competition has hosts
        hosted = [r for r in rs if r[7][0]]
        if rule.get("has_host") == 1 or (rule.get("has_host") is None and len(hosted) * 2 > len(rs)):
            for r in game_rows:
                if not r[7][0]:
                    faults.append(("no-host", name, r[3], f"{r[4][0]} / {r[5][0]}, no host ({len(hosted)} of "
                                                         f"{len(rs)} rows have one)"))
        # a winner or runner-up from the wrong confederation
        if not international:
            conf = game.nations.get(comp["nation"], ("", ""))[1]
        elif "confs" in rule:
            conf = "|".join(sorted(rule["confs"])) if rule["confs"] else ""
        else:
            conf = game.continents.get(comp["continent"], "")
            older = [r[4][2] for r in rs if r[3] < start and r[4][2]]
            if not conf and len(older) >= 3:
                top, n = collections.Counter(older).most_common(1)[0]
                conf = top if n * 5 >= len(older) * 4 else ""
        # the competition record on another continent than its rules (open item 21: the UEFA Nations League built
        # on the record of the Asian Cup qualifiers, continent Asia)
        record = game.continents.get(comp["continent"], "")
        if rule.get("confs") and record in CONF_IDS and record not in rule["confs"]:
            faults.append(("comp-continent", name, save_year, f"the record has continent {record}, the competition "
                                                              f"is {'|'.join(sorted(rule['confs']))}"))
        checks = [(name, conf)] if conf else []
        # the short name (+0x38, the name of the history screen) of another competition than the long name
        # (open item 4: the UEFA Nations League record keeps the short name "Asian Cup Quals"): the rows are
        # checked against the rule of the short name too, under that name
        short = rules.get(comp["short"]) if comp["short"] else {}
        if short and rule and short["key"] != rule["key"]:
            faults.append(("comp-name", name, save_year, f"the short name {comp['short']} is the competition "
                                                         f"{short['key']}, the name is {rule['key']}"))
            if short.get("confs"):
                checks.append((comp["short"], "|".join(sorted(short["confs"]))))
        for shown, conf in checks:
            for r in game_rows:
                for role, p in (("winner", r[4]), ("runner-up", r[5])):
                    if p[0] and p[2] and p[2] not in conf.split("|") and \
                            not any(r[3] < y for y in moved.get(p[1], ())):
                        faults.append(("confederation", shown, r[3], f"{role} {p[0]} ({p[1]}, {p[2]}), "
                                                                     f"the competition is {conf}"))
        # the same two finalists more than 2 editions in a row
        finals = rule["kind"] != "league" if rule.get("kind") else (teams == "nation" or international)
        if finals:
            run, last = 0, None
            for r in sorted(rs, key=lambda r: r[3]):
                pair = frozenset((r[4][0], r[5][0])) if r[4][0] and r[5][0] else None
                run = run + 1 if pair and pair == last else 1
                last = pair
                if run == 3 and r[3] >= start:
                    faults.append(("same-final", name, r[3], f"{r[4][0]} and {r[5][0]} in 3 editions to {r[3]}"))
        # a gap in the cycle and editions missing up to the save date: for a competition that the game runs
        # (it has a .tmp part) and that was held near the start
        cyc = rule.get("cycle") or cycle_of(years, start)
        end = rule.get("end")
        edition = game.editions.get(cid)
        ys = sorted(set(years))
        if rule.get("ended"):
            continue                                  # ended by the rules: no later edition is expected
        if cyc and ys and cid in game.parts and ys[-1] >= start - 2 * cyc:
            for a, b in zip(ys, ys[1:]):
                if b >= start and a >= start - cyc and b - a > cyc + 1 and not (end and a >= end):
                    faults.append(("gap", name, b, f"no edition between {a} and {b} (cycle {cyc})"))
            ended = edition == ENDED_YEAR or (end is not None and end < save_year)
            if not ended and ys[-1] + cyc + 1 < save_year:
                faults.append(("missing", name, ys[-1] + cyc,
                               f"last edition {ys[-1]}, cycle {cyc}, the save is in {save_year}"))
        # the next draw or start more than one cycle after the save date
        if edition in IDLE_YEARS:
            faults.append(("next-edition", name, edition, f"the class waits for {edition} (save in {save_year})"))
        elif edition and edition != ENDED_YEAR and cyc and edition > save_year + cyc + 1:
            faults.append(("next-edition", name, edition, f"next edition {edition}, the save is in {save_year}, "
                                                          f"cycle {cyc}"))
    return sorted(faults, key=lambda f: (f[1], f[0], f[2]))


def read_nations(path):
    """The rows of nations.tsv: [{kind, season, from (month, day), nation, to}]."""
    if not path or not os.path.exists(path):
        return []
    out = []
    for r in tsv(path):
        month, day = (int(x) for x in r["from"].split("-"))
        out.append({"kind": r["kind"], "season": int(r["season"]), "from": (month, day), "nation": r["nation"],
                    "to": r["to"]})
    return out


def due(game, change):
    """1 when a change of nations.tsv holds at the save date: with changing rules on from its date, with changing
    rules off only when its season is not after the start year (the data of a later start holds it)."""
    if not game.changing:
        return change["season"] <= game.start
    month, day = change["from"]
    year = game.date[0]
    start = (datetime.date(change["season"], month, day) - datetime.date(change["season"], 1, 1)).days
    return change["season"] <= game.start or (year, game.date[1]) >= (change["season"], start)


def check_nations(game, changes):
    """[(check, nation, season, detail)]: a live record (with a continent) under a name that a due rename
    replaced, and a nation in another confederation than a due move gives. A dead record (no continent) beside
    the live one of its later name is no fault."""
    faults = []
    live = {}
    for nid, (name, conf) in game.nations.items():
        if conf:
            live.setdefault(name, []).append(conf)
    year = game.date[0]
    for c in changes:
        if not due(game, c):
            continue
        if c["kind"] == "rename" and c["nation"] in live:
            faults.append(("nation-name", c["nation"], c["season"],
                           f"a live record named {c['nation']} in {year}; from {c['season']} the name is {c['to']}"))
        if c["kind"] == "rename":
            for side, nid in game.side_names:
                nation, conf = game.nations.get(nid, ("", ""))
                if side == c["nation"] and conf and nation != side:
                    faults.append(("side-name", side, c["season"], f"the national side of {nation} "
                                                                   f"is named {side} in {year}; from {c['season']} "
                                                                   f"the name is {c['to']}"))
                    break
        if c["kind"] == "confed":
            for conf in live.get(c["nation"], []):
                if conf != c["to"]:
                    faults.append(("nation-confed", c["nation"], c["season"],
                                   f"{c['nation']} is in {conf} in {year}; from {c['season']} it is in {c['to']}"))
    return faults


# ---- Entrants ----

def fixtures(game):
    """{competition id: [(day, home, away)]} from the fixtures parts (fixtures_<year>.tmp: s32 count, then records
    of 0x4f bytes: +0 competition id, +0xc home, +0x10 away, +0x28 s16 year, +0x2a s16 day of the year from 0;
    docs/research/2026-10-15-holiday-next-match-inventory.md). day: a day number (date.toordinal). A part whose
    size is not 4 + count * 0x4f is not read and is named in game.unread."""
    if game.fixture_list is not None:
        return game.fixture_list
    out = collections.defaultdict(list)
    for n in game.fixture_parts:
        b = game.sav.read(n)
        count = s32(b, 0) if len(b) >= 4 else -1
        if count < 0 or 4 + count * FIXTURE != len(b):
            game.unread.append(n)
            continue
        for i in range(count):
            o = 4 + i * FIXTURE
            cid = s32(b, o)
            home, away = struct.unpack_from("<ii", b, o + 0xC)
            y, d = struct.unpack_from("<hh", b, o + 0x28)
            if cid in game.comps and 1900 < y < 3000 and 0 <= d < 366:
                out[cid].append((datetime.date(y, 1, 1).toordinal() + d, home, away))
    game.fixture_list = out
    return out


def editions_of(fx):
    """[(year, first day, last day, teams)]: the fixtures of a competition split after a final (a day with one match
    and then FINAL_GAP days or more with none) and at a gap of more than GAP days, each an edition with the year of
    its last match. A winter break comes after a day of many matches, so a club cup from July to May stays one
    edition."""
    per_day = collections.defaultdict(list)
    for day, home, away in fx:
        per_day[day].append((home, away))
    out = []
    for day in sorted(per_day):
        if not out or day - out[-1][2] > GAP or (day - out[-1][2] >= FINAL_GAP and len(per_day[out[-1][2]]) == 1):
            out.append([0, day, day, set()])
        out[-1][2] = day
        out[-1][3].update(t for pair in per_day[day] for t in pair if t >= 0)
    return [(datetime.date.fromordinal(e[2]).year, e[1], e[2], e[3]) for e in out]


def finals_table(path=None):
    """{class: [(year, teams, [host names, each "a/b" for the names a database can use])]} of the finals table
    of the season code (season_intl_table.c), sorted by year."""
    path = path or os.path.join(ROOT, "src", "recomp_rt", "win", "season_intl_table.c")
    out = collections.defaultdict(list)
    if os.path.exists(path):
        text_ = open(path, encoding="latin-1").read()
        for cls, year, teams, hosts in re.findall(r'\{"(\w+)", \d+, (\d+), (\d+), \d+, \d+, \d+, \d+, "([^"]*)"',
                                                  text_):
            out[cls].append((int(year), int(teams), [h for h in hosts.split(";") if h]))
    for rows in out.values():
        rows.sort()
    if os.path.exists(path) and not out:
        raise ValueError(f"{path}: no row of the finals table read (has its struct changed?)")
    return out


def read_access(path):
    """{key: [(from, to or None, teams or None, [route keys, or "holder"])]} of access.tsv."""
    out = collections.defaultdict(list)
    if path and os.path.exists(path):
        for r in tsv(path):
            routes = [x.split(":", 1)[1] if x.startswith("winner:") else x
                      for x in r.get("routes", "").split(";") if x.startswith("winner:") or x == "holder"]
            out[r["comp"]].append((int(r["from"]), number(r["to"]), number(r["teams"]), routes))
    return out


class Access:
    """The access rules: the finals table of the season code and access.tsv."""

    def __init__(self, table=None, access=None):
        self.table = table if table is not None else {}
        self.access = access if access is not None else {}

    def rule(self, key, year):
        """(teams or None, [host names], [route keys]) of an edition."""
        rows = self.table.get(key)
        if rows:
            row = next((r for r in rows if r[0] == year), None)
            if row:
                return row[1], row[2], []
            if year > rows[-1][0]:
                return rows[-1][1], [], []               # after the table: the count of the last row
            return None, [], []
        for start, end, teams, routes in self.access.get(key, ()):
            if start <= year and (end is None or year <= end):
                return teams, [], routes
        return None, [], []


def entrants(game, rules, access, changes=()):
    """[(competition id, name, year, first day, last day, team id, team, nation, confederation, route)] and the
    faults of the entrant checks."""
    rules = rules or Rules()
    fxs = fixtures(game)
    by_key = collections.defaultdict(list)                    # key: [(year, winner id)]
    held = collections.defaultdict(list)                      # club: [(year, competition name, place)]
    for cid, year, a, b, c, h in game.rows:
        name = game.comps[cid]["name"]
        key = rules.key(name)
        if key:
            by_key[key].append((year, a))
        for place, t in ((1, a), (2, b), (3, c)):
            if t >= 0:
                held[t].append((year, name, place))
    hosts = {(cid, year): h for cid, year, a, b, c, h in game.rows if h >= 0}
    moved, due_to = collections.defaultdict(list), collections.defaultdict(set)
    renamed = {ch["nation"]: (ch["season"], ch["to"]) for ch in changes if ch["kind"] == "rename" and due(game, ch)}
    for ch in changes:
        if ch["kind"] == "confed":
            moved[ch["nation"]].append(ch["season"])
            if due(game, ch):
                due_to[ch["nation"]].add(ch["to"])            # reported once, by check_nations

    def winners(key, year):
        """The winners of a route competition in the year of the match and in its last edition before that year,
        when that edition is at most one cycle and one year before (a super cup in August follows a final in May of
        its year, the Finalissima of 2022 the Euro of 2020; a route competition that stopped gives no winner)."""
        cycle = rules.rows.get(key, {}).get("cycle")
        cycle = number(cycle) or 1
        before = max([y for y, w in by_key.get(key, ()) if y < year], default=0)
        years = {year} | ({before} if year - before <= cycle + 1 else set())
        return {w for y, w in by_key.get(key, ()) if y in years}

    def played(key, year):
        """1 when the game wrote a row of route competition key (a year after the start) in the window of winners():
        else its nations are not selected and a club of its confederation comes by the original's draw (slice 9a).
        A route that stopped by a fault of the game also counts as not played; the check `missing` reports it."""
        cycle = number(rules.rows.get(key, {}).get("cycle")) or 1
        return any(game.start < y <= year and year - y <= cycle + 1 for y, w in by_key.get(key, ()))

    save_day = datetime.date(game.date[0], 1, 1).toordinal() + game.date[1] if game.date[0] > 1900 else 0
    live = {n for n, conf in game.nations.values() if conf}
    listing, faults = [], []
    for cid, fx in sorted(fxs.items()):
        comp = game.comps[cid]
        name = comp["name"]
        if FRIENDLY.match(name):
            continue
        rule = rules.get(name)
        key = rule.get("key")
        short = rules.get(comp["short"]) if comp["short"] else {}
        confs = [(name, rule.get("confs"))]
        if short and short.get("key") != key and short.get("confs"):
            confs.append((comp["short"], short["confs"]))
        eds = editions_of(fx)
        for year, first, last, teams in eds:
            done = last <= save_day
            want, host_names, routes = access.rule(key, year) if key else (None, [], [])
            won = {r: winners(r, year) for r in routes if r != "holder"}
            holder = "holder" in routes
            # the World Cup qualifying holds the play-off between confederations: 2 teams of other confederations
            # in a campaign (a tie such as Bahrain against Trinidad and Tobago in 2005), or an edition of at most 6
            # teams (the play-off tournament of 2026, in the record of the OFC qualifying: 2 of CONCACAF and 1 each of
            # AFC, CAF, CONMEBOL and OFC)
            playoff = (key or "").startswith("wc_qual_")
            foreign, dead, no_route = collections.defaultdict(list), [], []
            prev = [w for y, w in sorted(by_key.get(key, ())) if y < year][-1:] if key else []
            host = hosts.get((cid, year), -1)
            host_nations = [game.clubs.get(host, ("", -1))[1]] if host >= 0 else []
            host_alias = {x for h in host_names for x in h.split("/")}
            for t in sorted(teams, key=lambda t: game.club(t)):
                tname, nation, conf = game.club(t)
                route = []
                if t in prev:
                    route.append("title holder")
                if game.clubs.get(t, ("", -1))[1] in host_nations or nation in host_alias:
                    route.append("host")
                route += [f"winner {r}" for r in won if t in won[r]]
                route += [f"{'winner' if p == 1 else 'place ' + str(p)} {c} {y}" for y, c, p in held.get(t, ())
                          if y in (year - 1, year) and c != name]
                listing.append((cid, name, year, first, last, t, tname, nation, conf, "; ".join(route) or "qualifier"))
                by_route = any(t in won[r] for r in won)              # a route winner enters from any confederation
                if nation in renamed and year > renamed[nation][0]:
                    dead.append(f"{tname} ({nation}, {renamed[nation][1]} from {renamed[nation][0]})")
                for shown, cs in confs:
                    if cs and conf and conf not in cs and not by_route and not due_to[nation] & cs and \
                            not any(year < m for m in moved.get(nation, ())):
                        foreign[(shown, "|".join(sorted(cs)))].append(f"{tname} ({conf})")
                if won and done and year > game.start and not by_route and all(by_key.get(r) for r in won):
                    no_route.append((conf, f"{tname} ({nation}) won none of {', '.join(won)} in {year} or the edition "
                                           f"before"))
            # each route that did not play leaves one place to a drawn club of its confederation
            free = [set(c.split("|")) if (c := rules.rows.get(r, {}).get("confed")) else None
                    for r in won if not played(r, year)]
            for conf, text in no_route:
                take = next((i for i, cs in enumerate(free) if cs is None or conf in cs), None)
                if take is None:
                    faults.append(("entrant-route", name, year, text))
                else:
                    free.pop(take)
            if dead:
                faults.append(("entrant-dead", name, year, f"{len(dead)} entrants of a nation under an old name: "
                                                           f"{', '.join(dead[:6])}"))
            if holder and done and year > game.start and prev and prev[0] not in teams and \
                    year - max(y for y, w in by_key[key] if y < year) <= (rule.get("cycle") or 1) + 1:
                faults.append(("entrant-holder", name, year, f"the title holder {game.club(prev[0])[0]} is not an "
                                                             f"entrant"))
            for (shown, cs), teams_ in sorted(foreign.items()):
                if playoff and (len(teams_) <= 2 or len(teams) <= 6):
                    continue
                faults.append(("entrant-confed", shown, year, f"{len(teams_)} entrants from another confederation "
                                                              f"than {cs}: {', '.join(teams_[:6])}"
                                                              f"{' ...' if len(teams_) > 6 else ''}"))
            if want and done and len(teams) != want:
                faults.append(("entrant-count", name, year, f"{len(teams)} entrants, the access rule has {want}"))
            if done and not host_names and host >= 0 and rule.get("kind") == "intl_finals" and \
                    game.clubs.get(host, ("", -1))[1] not in {game.clubs.get(t, ("", -1))[1] for t in teams}:
                faults.append(("entrant-host", name, year, f"the host {game.club(host)[0]} of the history row is "
                                                           f"not an entrant"))
            if done:                                      # a host of the table that the data holds plays
                names = {game.nations.get(game.clubs.get(t, ("", -1))[1], ("", ""))[0] for t in teams}
                for h in host_names:
                    alias = set(h.split("/"))
                    if not names & alias and alias & live:
                        faults.append(("entrant-host", name, year, f"the host {h} is not an entrant"))
            if key == "wc_qual_uefa" and done and len(teams) >= MISSING_MIN and \
                    first >= datetime.date(game.start, 7, 1).toordinal() and uefa_all(game, year):
                missing = uefa_missing(game, access, changes, year, datetime.date.fromordinal(first).year,
                                       {game.club(t)[1] for t in teams})
                if missing:
                    faults.append(("entrant-missing", name, year, f"{len(missing)} live UEFA nations do not play: "
                                                                  f"{', '.join(missing[:6])}"
                                                                  f"{' ...' if len(missing) > 6 else ''}"))
    return listing, faults


MISSING_MIN = 30                              # the group stage of a European qualifying (a play-off edition is less)


def uefa_all(game, year):
    """1 when the season code puts every UEFA member in the European World Cup qualifying edition of this year: a
    season game (not 2001 with changing rules off, season_intl_on), for the World Cups of 2006 to 2022
    (season_wcq26.c europe_wide) and from 2026 with changing rules on or a start from 2025 (season_wcq26, Q232).
    The 2002 campaign keeps the original's holder and cap."""
    wc = year + (2 - year % 4) % 4
    if game.start == 2001 and not game.changing:
        return 0
    return 2006 <= wc <= 2022 or (wc >= 2026 and (game.changing or game.start >= 2025))


def uefa_missing(game, access, changes, year, first_year, entered):
    """The live UEFA nations with a national side that are not entrants of a European World Cup qualifying edition
    (open item 57 of 0.6.2): every UEFA member enters, less the European hosts of its World Cup (the first World
    Cup year from the edition's year), less Russia from the 2026 cycle (Q29) and less a nation that joins UEFA
    after the edition's first year (nations.tsv: Kosovo from 2016)."""
    wc = year + (2 - year % 4) % 4
    hosts = {x for h in access.rule("world_cup", wc)[1] for x in h.split("/")}
    later = {c["nation"] for c in changes if c["kind"] == "confed" and c["to"] == "UEFA" and c["season"] > first_year}
    sides = {nid for _, nid in game.side_names}
    out = []
    for nid, (nation, conf) in sorted(game.nations.items()):
        if conf != "UEFA" or nid not in sides or nation in entered or nation in hosts or nation in later:
            continue
        if nation == "Russia" and wc >= 2026:
            continue
        out.append(nation)
    return sorted(out)


# ---- World health (Tier B) ----

STAFF = 0x6E
FINANCE = 0x167
MONEY_32 = 1 << 31                            # the 32-bit readers of the balance (open items 27 and 28)
MONEY_WRAP = 1 << 62                          # no club reaches it: a balance beyond it has wrapped
SQUAD = 23                                    # a nation with this many players fills a squad; with no player under
                                              # 21 or under 23 its under-21 or Olympic squad is empty (Q17, Q31)
ELEVEN = 11                                   # under-21 players that fill an under-21 side


def health(game):
    """The world-health numbers of a save: (summary dict, [per nation row], faults). Squads: staff with a national
    side at +0x24 and the job Player (11) at +0x28 (database_get_job_name 0x53ad00).
    staff.dat (0x6e bytes a record): +0x10 s16 day of birth from 0, +0x12 s16 year of birth (both read by the
    research of slice 2 and checked on 83,140 players of a 2028 save against +0x18), +0x18 s16 age (picker.c:70),
    +0x1a nation (season_national_fill.c:13), +0x61 the player data, in a save its row or -1 (picker.c:69).
    Managers: club and national side +0xcf, the manager's staff id or -1 (database_get_club_manager 0x53ca90); the
    senior side of nation n is row n of nat_club.dat. Balance: finance.dat, 0x167 bytes a team, clubs first,
    +0x00 s64 (the finance record, 0x596587; run-logs/future-audit/audit.md)."""
    sav = game.sav
    staff = sav.read("staff.dat") if "staff.dat" in sav.members else b""
    year, day = game.date
    players, u21, u23 = collections.Counter(), collections.Counter(), collections.Counter()
    squad = collections.Counter()
    age_bad = []
    for o in range(0, len(staff) - STAFF + 1, STAFF):
        if s32(staff, o + 0x24) >= 0 and staff[o + 0x28] == 11:    # in the squad of a national side (job Player)
            squad[s32(staff, o + 0x24)] += 1
        if s32(staff, o + 0x61) < 0:
            continue
        bday, byear = struct.unpack_from("<hh", staff, o + 0x10)
        age = struct.unpack_from("<h", staff, o + 0x18)[0]
        if not 1800 < byear < 3000:
            continue
        real = year - byear - (1 if day < bday else 0)        # the age at the save date, from the birth date
        if year and abs(age - real) > 1:
            age_bad.append((s32(staff, o), age, real))
        n = s32(staff, o + 0x1a)
        players[n] += 1
        u21[n] += real < 21
        u23[n] += real < 23
    clubs = sav.read("club.dat")
    sides = sav.read("nat_club.dat")
    club_count = len(clubs) // CLUB
    managed = sum(1 for o in range(0, club_count * CLUB, CLUB) if s32(clubs, o + 0xCF) >= 0)
    side_manager, side_ids = {}, collections.defaultdict(list)
    for o in range(0, len(sides) - CLUB + 1, CLUB):
        n = s32(sides, o + 0x53)
        side_manager.setdefault(n, s32(sides, o + 0xCF) >= 0)    # the first row of a nation is its senior side
        side_ids[n].append(s32(sides, o))                        # then its under-21 side
    finance = sav.read("finance.dat") if "finance.dat" in sav.members else b""
    balances = [struct.unpack_from("<q", finance, k * FINANCE)[0]
                for k in range(min(club_count, len(finance) // FINANCE))]
    live = {nid: (name, conf) for nid, (name, conf) in game.nations.items() if conf}
    rows, faults = [], []
    for nid, (name, conf) in sorted(live.items(), key=lambda x: x[1]):
        has_side = nid in side_manager
        ids = side_ids.get(nid, []) + [None, None]
        rows.append((name, conf, players[nid], u21[nid], u23[nid], squad[ids[0]] if ids[0] is not None else "",
                     squad[ids[1]] if ids[1] is not None else "",
                     int(side_manager.get(nid, False)) if has_side else ""))
        if ids[0] is not None and players[nid] >= SQUAD and squad[ids[0]] == 0:
            faults.append(("empty-squad", name, year, f"the senior side of {name} has no player in its squad, "
                                                      f"{players[nid]} players hold the nationality"))
        if ids[1] is not None and u21[nid] >= ELEVEN and squad[ids[1]] == 0:
            faults.append(("empty-squad", name + " under-21", year, f"the under-21 side of {name} has no player in "
                                                                    f"its squad, {u21[nid]} players are under 21"))
        if players[nid] >= SQUAD and (u21[nid] == 0 or u23[nid] == 0):
            faults.append(("no-youth", name, year, f"{name}: {players[nid]} players, {u21[nid]} under 21, "
                                                   f"{u23[nid]} under 23"))
    if age_bad:
        sid, age, real = age_bad[0]
        faults.append(("overflow", "age", year, f"{len(age_bad)} players with an age field that is not the age "
                                                f"from the birth date (staff {sid}: {age}, not {real})"))
    wrapped = [b for b in balances if abs(b) >= MONEY_WRAP]
    if wrapped:
        faults.append(("overflow", "balance", year, f"{len(wrapped)} clubs with a balance of 2^62 or more, "
                                                    f"for example {wrapped[0]}"))
    summary = {
        "save_year": year, "save_day": day, "save_bytes": os.path.getsize(game.path), "save_parts": len(sav.members),
        "players": sum(players.values()), "under_21": sum(u21.values()), "under_23": sum(u23.values()),
        "clubs": club_count, "clubs_with_manager": managed,
        "national_sides": sum(1 for n in side_manager if n in live),
        "national_sides_with_manager": sum(1 for n, m in side_manager.items() if m and n in live),
        "balance_min": min(balances, default=0), "balance_max": max(balances, default=0),
        "balances_over_32_bits": sum(1 for b in balances if not -MONEY_32 <= b < MONEY_32),
        "time_per_game_day": "not in a save (the Tier B runner measures it)",
    }
    return summary, rows, faults


def write_health(summary, rows, path):
    with open(path, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        for k, v in summary.items():
            w.writerow(["#", k, v])
        w.writerow(["nation", "confederation", "players", "under_21", "under_23", "senior_squad", "under_21_squad",
                    "national_manager"])
        w.writerows(rows)


def place(p):
    return f"{p[0]} ({p[1]})" if p[0] and p[1] else p[0]


def write(game, label, out, faults, known):
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, label + ".history.tsv"), "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        w.writerow(["competition_id", "kind", "competition", "year", "winner", "runner_up", "third", "host",
                    "winner_conf", "runner_up_conf"])
        for cid, kind, name, year, a, b, c, h in table(game):
            w.writerow([cid, kind, name, year, place(a), place(b), place(c), place(h), a[2], b[2]])
    with open(os.path.join(out, label + ".faults.tsv"), "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        w.writerow(["check", "competition", "year", "known", "detail"])
        for c, name, y, d in faults:
            w.writerow([c, name, y, "known" if is_known(known, c, name, y) else "new", d])


def write_entrants(listing, path):
    with open(path, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        w.writerow(["competition_id", "competition", "year", "first", "last", "team_id", "team", "nation", "conf",
                    "route"])
        for cid, name, year, first, last, *rest in listing:
            w.writerow([cid, name, year, datetime.date.fromordinal(first), datetime.date.fromordinal(last), *rest])


def main(argv):
    args = [a for a in argv[1:]]
    opts = {"--health": 1} if "--health" in args else {}
    args = [a for a in args if a != "--health"]
    for k in ("--out", "--rules", "--names", "--known", "--label", "--nations", "--access", "--table"):
        if k in args:
            i = args.index(k)
            opts[k] = args[i + 1]
            del args[i:i + 2]
    if not args:
        print(__doc__)
        return 2
    default = os.path.join(HERE, "competitions.tsv")
    rules = Rules(opts.get("--rules", default if os.path.exists(default) else None),
                  opts.get("--names", os.path.join(HERE, "history_names.tsv")))
    known = read_known(opts.get("--known", os.path.join(HERE, "history_known.tsv")))
    access = Access(finals_table(opts.get("--table")),
                    read_access(opts.get("--access", os.path.join(HERE, "access.tsv"))))
    nations = read_nations(opts.get("--nations", os.path.join(HERE, "nations.tsv")))
    new = 0
    for path in args:
        game = Game(path)
        label = opts.get("--label") if len(args) == 1 and "--label" in opts else \
            re.sub(r"[^A-Za-z0-9.-]+", "_", os.path.relpath(path, os.path.dirname(os.path.dirname(path))))
        listing, entry = entrants(game, rules, access, nations)
        summary, rows, sick = health(game) if "--health" in opts else ({}, [], [])
        sick += [("save-part", n, game.date[0], "the size does not fit its count, not read") for n in game.unread]
        faults = sorted(check(game, rules, nations) + check_nations(game, nations) + sorted(set(entry)) + sick,
                        key=lambda f: (f[1], f[0], f[2]))
        if "--out" in opts:
            write(game, label, opts["--out"], faults, known)
            write_entrants(listing, os.path.join(opts["--out"], label + ".entrants.tsv"))
            if summary:
                write_health(summary, rows, os.path.join(opts["--out"], label + ".health.tsv"))
        fresh = [f for f in faults if not is_known(known, f[0], f[1], f[2])]
        new += len(fresh)
        y, d = game.date
        print(f"{label}: start {game.start}{'' if game.changing else ' (changing rules off)'}, save day {d} of {y}, {len(game.rows)} history rows, "
              f"{len({r[0] for r in game.rows})} competitions, {len(faults)} faults ({len(fresh)} new)")
        if summary:
            print("    health: " + ", ".join(f"{k} {v}" for k, v in summary.items()))
        for c, name, yr, detail in faults:
            print(f"    {'known' if is_known(known, c, name, yr) else 'FAULT'}  {c}  {name} {yr}: {detail}")
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
