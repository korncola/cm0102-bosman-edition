#!/usr/bin/env python3
"""The run set of the future play spec (0.6.2 slice 15): every game of run_set.tsv, several at a time within the
machine limits, a save at the end of each leg, the history checker on each save, the Tier B limits, and one pass or
fail per game. The release script checks the results with --verify.

Usage:
    python3 tests/seasons/future/run_set.py <cm0102_recomp> <game folder> --out <folder> [--set release]
        [--only <name,...>] [--jobs 2] [--machine 12] [--floor-gb 100] [--databases <folder>] [--own-dbs <folder>]
    python3 tests/seasons/future/run_set.py --verify <folder> [--set release]

--set: release (the rows of set release and nightly, Q12), nightly, quick (the short games that test the script) or
all. --only: these rows of the set. A game whose folder holds result.tsv is done and is not played again; delete
its folder to play it again.

A game: legs of leg_years seasons, each played with tests/seasons/soak_run.py's new_game_leg or load_leg (the season
jump, the manager on holiday, all of RECOMP_FIXED_CLOCK, RECOMP_SCREEN_LOG and a stop at the first box). Every leg
saves on about 20 July after its last season (RECOMP_SAVE_AFTER_DAYS, counted from the save it loaded), the last leg
too (the season jump runs to the season after `last`), so the last save holds the end of the season `last`. A save
before July of the year after its last season fails the leg. Tier A sets RECOMP_TIER_A=1 (slice 3); a jump sets
RECOMP_DATE_JUMP=1 and plays the December 2025 database. The databases of the other start years are those of
tests/seasons/gate_run.py; the runner unpacks the own databases once, before the games start.

After each leg: the leg's own checks (soak_run: no box, no line of the season change net, exit code 0, a save), then
the history checker on the save (history_check.py with its rules, names, nations and access files; with --health for
Tier B). Every fault fails the game, a fault of the known list too: 0.6.2 ships only when the checker finds no fault in
the run set (Q12); faults.tsv marks the known ones. The game plays on, so one run lists the faults of every decade. A
leg that is not ok fails the game and ends it. At the end, every division and domestic cup must move on from each
season to the next over the reports of all legs (league-stall: rules_report_check.league_stalls, as soak_run.py).

The Tier B limits, over the legs of a Tier B game:
    no-manager  a national side with no manager in two saves in a row (more than one season)
    slow        the time per game day of a decade is more than twice that of the first seasons (the median of legs
                1 and 2; leg 0 also makes the game)
    size        a save of 4 GB or more, or a save size that, at the growth per year of the last 50 years of saves,
                would pass 4 GB before the end of 2300
The checker's --health faults (empty-squad, no-youth, overflow) are checker faults of Tier B.

Disk (open item 53): a leg starts only with --floor-gb free on the disk of --out. A game keeps the save of one leg in
each decade (the first save of the decade) and the last save, and of these legs only the save; it deletes the user,
home and shots folders of every other leg once the next leg has loaded its save and passed. Each leg keeps
stderr.txt (gzipped) and report.tsv. A failed leg keeps its folder.

Room: a leg starts only while fewer than --jobs games of this run play, fewer than --machine games run on the Mac,
the load of the last minute is below 24 and /Users/max/Projects/cm0102/run-logs/HOLD does not exist.

The program: when it lies in this checkout, the script first builds it (build-slot.sh cmake --build, target
cm0102_recomp), so it is the build of the checkout's game code. Each result records the commit, "clean" (a program
of this checkout with no change in GAME_CODE), "changes" or "outside" (a program from elsewhere, for tests only), the
program's sha256 and the row of run_set.tsv it was played for.

Files in --out: results.tsv (a head line, then one row per game: name, tier, set, result, reason, reached, legs,
faults, seconds, commit, code, program, row), and per game <name>/legs.tsv (one row per leg), <name>/faults.tsv,
<name>/result.tsv, run.log and the leg folders. A game whose runner stops with no result gets the result fail.

--verify <folder>: exit 1 unless every row of the set (a row that waits too) has the result pass, played for the same
row of run_set.tsv, with a clean program, all with one program, at a commit whose game code (GAME_CODE: src,
tools/recomp, tools/seasons and docs/research/rules-0.6.0) is that of the checkout; every checkpoint save that
legs.tsv lists still exists; and the checker of this checkout finds no fault on any save the run kept.
"""
import csv
import datetime
import gzip
import hashlib
import os
import shutil
import statistics
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
SEASONS = os.path.join(ROOT, "tests", "seasons")
for p in (HERE, SEASONS, os.path.join(ROOT, "tests", "smoke"), os.path.join(ROOT, "tests", "fixtures")):
    sys.path.insert(0, p)
import history_check as hc  # noqa: E402

HOLD = "/Users/max/Projects/cm0102/run-logs/HOLD"
DATABASES = "/Users/max/Projects/cm0102/test-fixtures/databases"
OWN_DBS = "/Users/max/Projects/cm0102/own-dbs"
JUMP_DATABASE = "December 2025"
COLUMNS = ["name", "tier", "start", "changing", "nations", "seed", "last", "leg_years", "set", "status", "note"]
TIERS = {"A", "B", "jump", "normal"}
SETS = {"release": {"release", "nightly"}, "nightly": {"nightly"}, "quick": {"quick"},
        "all": {"release", "nightly", "quick"}}
# What makes the game: its code, the recompiler, and the tables and their sources that the build and the release turn
# into the game's season data (tools/seasons, docs/research/rules-0.6.0).
GAME_CODE = ["src", "tools/recomp", "tools/seasons", "docs/research/rules-0.6.0"]
FOUR_GB = 4 << 30
DAYS = 365.25
SAVE_NAME = "samesave.sav"                    # the save of a leg (smoke_run.SAVE_NAME)
SWITCHES = ("RECOMP_TIER_A", "RECOMP_DATE_JUMP", "RECOMP_TEST_EXTRA")
SAVE_ON = (7, 20)                             # every leg saves on about 20 July after its last season
LEG_HEAD = ["leg", "seasons", "result", "seconds", "save_date", "save_mb", "faults", "note"]


def arg(argv, name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


def read_set(path=None):
    """The rows of run_set.tsv; a row that does not fit the columns stops the script."""
    rows = []
    with open(path or os.path.join(HERE, "run_set.tsv"), encoding="utf-8") as f:
        for r in csv.DictReader((l for l in f if not l.startswith("#")), delimiter="\t"):
            bad = [c for c in COLUMNS if r.get(c) in (None, "") and c != "note"]
            if bad or r["tier"] not in TIERS or r["status"] not in ("run", "waits") or \
                    r["set"] not in SETS["all"] or r["changing"] not in ("on", "off"):
                raise SystemExit(f"run_set.tsv: row {r.get('name')} does not fit the columns ({bad or 'a value'})")
            for c in ("start", "seed", "last", "leg_years"):
                r[c] = int(r[c])
            rows.append(r)
    names = [r["name"] for r in rows]
    if len(set(names)) != len(names):
        raise SystemExit("run_set.tsv: a name is used twice")
    return rows


def chosen(rows, which, only=None):
    out = [r for r in rows if r["set"] in SETS[which]]
    if only:
        out = [r for r in out if r["name"] in only]
    return out


def legs_of(row):
    """[(first season, last season)] of each leg: leg_years seasons each, to the season `last`."""
    out, s = [], row["start"]
    while s <= row["last"]:
        out.append((s, min(s + row["leg_years"] - 1, row["last"])))
        s += row["leg_years"]
    return out


def free_gb(path):
    return shutil.disk_usage(path).free / 1e9


def machine_games():
    return len(subprocess.run(["pgrep", "-x", "cm0102_recomp"], capture_output=True, text=True).stdout.split())


def wait_room(o, log=print):
    """Waits for the HOLD file, the disk floor, the machine limit and the load (see the docstring)."""
    said = None
    while True:
        why = ("HOLD" if os.path.exists(HOLD) else
               f"{free_gb(o['out']):.0f} GB free, below {o['floor']} GB" if free_gb(o["out"]) < o["floor"] else
               f"{machine_games()} games run on the Mac" if machine_games() >= o["machine"] else
               "load 24 or more" if os.getloadavg()[0] >= 24 else None)
        if not why:
            return
        if why != said:
            log(f"waiting: {why}")
            said = why
        time.sleep(30)


# ---- The checker on one save ----

class Checker:
    """history_check.py with the files next to it (the files of the checkout that runs this script)."""

    def __init__(self):
        self.rules = hc.Rules(os.path.join(HERE, "competitions.tsv"), os.path.join(HERE, "history_names.tsv"))
        self.known = hc.read_known(os.path.join(HERE, "history_known.tsv"))
        self.nations = hc.read_nations(os.path.join(HERE, "nations.tsv"))
        self.access = hc.Access(hc.finals_table(), hc.read_access(os.path.join(HERE, "access.tsv")))

    def is_known(self, fault):
        return hc.is_known(self.known, fault[0], fault[1], fault[2])

    def run(self, save, health):
        """(game, faults [(check, competition, year, detail)], health summary, health rows). Every fault counts, a
        known one too: 0.6.2 ships only when the checker finds no fault in the run set (Q12). is_known marks the
        faults of history_known.tsv in the lists."""
        game = hc.Game(save)
        faults = hc.check(game, self.rules, self.nations) + hc.check_nations(game, self.nations)
        faults += sorted(set(hc.entrants(game, self.rules, self.access, self.nations)[1]))
        summary, rows = {}, []
        if health:
            summary, rows, sick = hc.health(game)
            faults += sick
        faults += [("save-part", n, game.date[0], "the size does not fit its count, not read") for n in game.unread]
        return game, faults, summary, rows


# ---- The Tier B limits ----

def tier_b_limits(legs, last_year=2300):
    """Faults [(check, detail)] of the Tier B limits over the legs: [{leg, year (of the save), seconds, days, bytes,
    no_manager (a set of nations, or None when not read)}] in leg order."""
    faults = []
    previous = None
    for g in legs:
        if g.get("no_manager") is not None and previous is not None:
            both = sorted(g["no_manager"] & previous)
            if both:
                faults.append(("no-manager", f"{len(both)} national sides with no manager in the saves of "
                                             f"{g['year'] - 1} and {g['year']}: {', '.join(both[:8])}"))
        previous = g.get("no_manager")
    first = [g["seconds"] / g["days"] for g in legs if g.get("days") and g["leg"] in (1, 2)]
    if first:
        base = statistics.median(first)
        decades = {}
        for g in legs:
            if g.get("days") and g["leg"] > 2:
                decades.setdefault(g["year"] // 10 * 10, []).append(g["seconds"] / g["days"])
        for decade, r in sorted(decades.items()):
            if statistics.median(r) > 2 * base:
                faults.append(("slow", f"{statistics.median(r):.2f} s a game day in the {decade}s, the first seasons "
                                       f"{base:.2f} s"))
    sized = [(g["year"], g["bytes"]) for g in legs if g.get("bytes")]
    for year, size in sized:
        if size >= FOUR_GB:
            faults.append(("size", f"the save of {year} has {size / 1e9:.2f} GB"))
            break
    else:
        window = [(y, s) for y, s in sized if y >= sized[-1][0] - 50] if sized else []
        if len(window) >= 2 and window[-1][0] > window[0][0]:
            per_year = (window[-1][1] - window[0][1]) / (window[-1][0] - window[0][0])
            at_end = window[-1][1] + per_year * (last_year + 1 - window[-1][0])
            if at_end >= FOUR_GB:
                faults.append(("size", f"{window[-1][1] / 1e9:.2f} GB in {window[-1][0]}, {per_year / 1e6:.1f} MB a "
                                       f"year: {at_end / 1e9:.2f} GB at the end of {last_year}"))
    return faults


# ---- One game ----

def database_of(row, o):
    """The --database folder of a row (None: the base data 3.9.68), as tests/seasons/gate_run.py picks it."""
    if row["tier"] == "jump":
        names = sorted(n for n in os.listdir(o["databases"]) if n.startswith(JUMP_DATABASE))
        if not names:
            raise SystemExit(f"run_set.py: no {JUMP_DATABASE} folder in {o['databases']}")
        return os.path.join(o["databases"], names[0])
    import gate_run
    args, _ = gate_run.database(row["start"], {"run": o["out"], "own": o["own"], "databases": o["databases"]})
    return args[1] if args else None


def program_has(program, text):
    with open(program, "rb") as f:
        return text.encode() in f.read()


def tsv_row(path, values, head=None):
    new = not os.path.exists(path)
    with open(path, "a", encoding="utf-8", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        if new and head:
            w.writerow(head)
        w.writerow(values)


def tidy(work, keep_save):
    """Deletes a passed leg's game folders: all of them, or all but the save when it is kept. The stderr.txt is
    gzipped."""
    err = os.path.join(work, "stderr.txt")
    if os.path.exists(err):
        with open(err, "rb") as f, gzip.open(err + ".gz", "wb") as g:
            shutil.copyfileobj(f, g)
        os.remove(err)
    for d in ("home", "shots") + (() if keep_save else ("user",)):
        shutil.rmtree(os.path.join(work, d), ignore_errors=True)
    user = os.path.join(work, "user")
    if keep_save and os.path.isdir(user):            # the kept save only, not the database copy or the temp files
        for n in os.listdir(user):
            if n != SAVE_NAME:
                p = os.path.join(user, n)
                shutil.rmtree(p) if os.path.isdir(p) and not os.path.islink(p) else os.remove(p)


def play_game(row, o):
    """Plays one row; writes <out>/<name>/legs.tsv, faults.tsv and result.tsv. Returns the result row."""
    import soak_run
    folder = os.path.join(o["out"], row["name"])
    os.makedirs(folder, exist_ok=True)
    log = lambda text: print(f"{row['name']}: {text}", flush=True)  # noqa: E731
    began = time.time()
    if row["status"] == "waits":
        return finish(folder, row, "waits", row["note"], "", 0, 0, began, o)
    if row["tier"] == "A" and not program_has(o["program"], "RECOMP_TIER_A"):
        return finish(folder, row, "waits", "the program has no Tier A switch (slice 3)", "", 0, 0, began, o)
    for name in SWITCHES:                       # a switch of the shell must not reach a game of another tier
        os.environ.pop(name, None)
    os.environ.update({"A": {"RECOMP_TIER_A": "1"}, "jump": {"RECOMP_DATE_JUMP": "1"}}.get(row["tier"], {}))
    # one game a process: soak_run's leg_env copies os.environ
    so = {"program": o["program"], "game": o["game"], "start": row["start"], "last": row["last"] + 1,
          "seed": row["seed"], "nations": row["nations"].split(","), "database": database_of(row, o),
          "rules": row["changing"], "extra": []}
    checker = Checker()
    health = row["tier"] == "B"
    legs, kept_decades, save, previous_work, new_faults = [], set(), None, None, 0
    plan = legs_of(row)
    since = datetime.date(row["start"], 7, 1)          # the first day of a new game: about 1 July of the start year
    for k, (s0, s1) in enumerate(plan):
        work = os.path.join(folder, f"leg{k:03d}")
        if os.path.exists(work):
            return finish(folder, row, "fail", f"{work} exists: delete the game's folder to play it again", "",
                          k, new_faults, began, o)
        # RECOMP_SAVE_AFTER_DAYS counts game days from the leg's first day: from the save it loads, to 20 July after
        # its last season, so leap years do not move the saves
        days = (datetime.date(s1 + 1, *SAVE_ON) - since).days
        so["timeout"] = 14400 * (s1 - s0 + 1)
        wait_room(o, log)
        if k == 0:
            code, took, box = soak_run.new_game_leg(so, work, days)
        else:
            code, took, box = soak_run.load_leg(so, work, save, days)
        saved = os.path.join(work, "user", SAVE_NAME)
        net = soak_run.first_net(os.path.join(work, "stderr.txt"))
        result = ("box" if box else "net" if net else f"code {code}" if code != 0 else
                  "no save" if not os.path.exists(saved) else "ok")
        seasons = f"{s0}/{(s0 + 1) % 100:02d} to {s1}/{(s1 + 1) % 100:02d}"
        if result != "ok":
            tsv_row(os.path.join(folder, "legs.tsv"), [k, seasons, result, f"{took:.0f}", "", "", "", box or net or ""],
                    LEG_HEAD)
            return finish(folder, row, "fail", f"leg {k} ({seasons}): {result} {box or net or ''}".strip(), "", k,
                          new_faults, began, o)
        loaded = os.path.join(work, "user", "soak.sav")         # load_leg's copy of the save it loaded
        if os.path.exists(loaded):
            os.remove(loaded)
        game, faults, summary, rows = checker.run(saved, health)
        year = game.date[0]
        if (year, game.date[1]) < (s1 + 1, 181):         # the save must come after the season change of s1
            tsv_row(os.path.join(folder, "legs.tsv"), [k, seasons, "short", f"{took:.0f}",
                                                       f"{year}-{game.date[1] + 1:03d}", "", "", ""], LEG_HEAD)
            return finish(folder, row, "fail", f"leg {k} ({seasons}): the save is of day {game.date[1] + 1} of {year}, "
                                               f"before July {s1 + 1}", "", k, new_faults, began, o)
        since = datetime.date(year, 1, 1) + datetime.timedelta(days=game.date[1])
        for c, comp, y, detail in faults:
            tsv_row(os.path.join(folder, "faults.tsv"), [c, comp, y, "known" if checker.is_known((c, comp, y)) else "new",
                                                         detail, os.path.relpath(saved, folder)],
                    ["check", "competition", "year", "known", "detail", "save"])
        new_faults += len(faults)
        no_manager = None
        if health:
            no_manager = {r[0] for r in rows if r[7] == 0}
            hc.write_health(summary, rows, os.path.join(work, "health.tsv"))
        size = os.path.getsize(saved)
        legs.append({"leg": k, "year": year, "seconds": took, "days": days, "bytes": size, "no_manager": no_manager})
        keep = year // 10 not in kept_decades or k == len(plan) - 1
        if keep:
            kept_decades.add(year // 10)
        tsv_row(os.path.join(folder, "legs.tsv"),
                [k, seasons, "ok", f"{took:.0f}", f"{year}-{game.date[1] + 1:03d}", f"{size / 1e6:.0f}", len(faults),
                 "checkpoint" if keep else ""], LEG_HEAD)
        log(f"leg {k} ({seasons}) ok in {took:.0f} s, save of {year} ({size / 1e6:.0f} MB), {len(faults)} faults")
        if previous_work:
            tidy(previous_work[0], previous_work[1])
        previous_work, save = (work, keep), saved
    if previous_work:
        tidy(previous_work[0], previous_work[1])
    limits = tier_b_limits(legs, row["last"]) if health else []
    # every division and domestic cup moves on from one season to the next, over the reports of all legs (O278)
    import rules_report_check as rrc
    comps = [c for k in range(len(plan)) for c in rrc.read_report(os.path.join(folder, f"leg{k:03d}", "report.tsv"))[1]
             if os.path.exists(os.path.join(folder, f"leg{k:03d}", "report.tsv"))]
    limits += [("league-stall", line) for line in rrc.league_stalls(comps)]
    for c, detail in limits:
        tsv_row(os.path.join(folder, "faults.tsv"), [c, "Tier B" if c != "league-stall" else "leagues",
                                                     legs[-1]["year"], "new", detail, ""],
                ["check", "competition", "year", "known", "detail", "save"])
    reached = f"{legs[-1]['year']}" if legs else ""
    if new_faults or limits:
        return finish(folder, row, "fail", f"{new_faults} checker faults, {len(limits)} other (faults.tsv)",
                      reached, len(plan), new_faults + len(limits), began, o)
    return finish(folder, row, "pass", "", reached, len(plan), 0, began, o)


def row_key(row):
    """The row as run_set.tsv holds it, but the note: a result counts only for the row it was played for."""
    return "|".join(str(row[c]) for c in COLUMNS if c != "note")


def finish(folder, row, result, reason, reached, legs, faults, began, o):
    values = [row["name"], row["tier"], row["set"], result, reason, reached, legs, faults, f"{time.time() - began:.0f}",
              o["commit"], o["code"], o["sha"], row_key(row)]
    with open(os.path.join(folder, "result.tsv"), "w", encoding="utf-8", newline="") as f:
        csv.writer(f, delimiter="\t", lineterminator="\n").writerow(values)
    print(f"{row['name']}: {result.upper()} {reason}".rstrip(), flush=True)
    return values


# ---- The run ----

RESULT_HEAD = ["name", "tier", "set", "result", "reason", "reached", "legs", "faults", "seconds", "commit", "code",
               "program", "row"]


def git(*args):
    return subprocess.run(["git", "-C", ROOT] + list(args), capture_output=True, text=True).stdout.strip()


def identity(program):
    """(commit, code, program sha256): code is "clean" for a program of this checkout's build folder whose game code
    has no change, else "changes" or "outside" (a program from elsewhere, for tests only)."""
    with open(program, "rb") as f:
        sha = hashlib.sha256(f.read()).hexdigest()
    inside = os.path.realpath(program).startswith(os.path.realpath(ROOT) + os.sep)
    code = "outside" if not inside else "changes" if git("status", "--porcelain", "--", *GAME_CODE) else "clean"
    return git("rev-parse", "HEAD"), code, sha


def write_results(o, rows):
    out = os.path.join(o["out"], "results.tsv")
    with open(out, "w", encoding="utf-8", newline="") as f:
        f.write(f"# commit {o['commit']} {o['code']} program {o['sha']} {datetime.datetime.now():%Y-%m-%d %H:%M}\n")
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        w.writerow(RESULT_HEAD)
        for r in rows:
            done = os.path.join(o["out"], r["name"], "result.tsv")
            if os.path.exists(done):
                with open(done, encoding="utf-8") as g:
                    w.writerow(next(csv.reader(g, delimiter="\t")))
            else:
                w.writerow([r["name"], r["tier"], r["set"], "not run"] + [""] * (len(RESULT_HEAD) - 4))
    return out


def run(argv):
    o = {"program": os.path.abspath(argv[1]), "game": os.path.abspath(argv[2]), "out": os.path.abspath(arg(argv, "--out")),
         "jobs": int(arg(argv, "--jobs", 2)), "machine": int(arg(argv, "--machine", 12)),
         "floor": float(arg(argv, "--floor-gb", 100)), "databases": arg(argv, "--databases", DATABASES),
         "own": arg(argv, "--own-dbs", OWN_DBS)}
    only = arg(argv, "--only", "").split(",") if "--only" in argv else None
    rows = chosen(read_set(), arg(argv, "--set", "release"), only)
    if only and set(only) - {r["name"] for r in rows}:
        print(f"run_set.py: not in the set: {', '.join(sorted(set(only) - {r['name'] for r in rows}))}")
        return 2
    os.makedirs(o["out"], exist_ok=True)
    build = os.path.dirname(o["program"])
    if os.path.realpath(build).startswith(os.path.realpath(ROOT) + os.sep):     # the program of this checkout's tree
        if subprocess.run(["/Users/max/Projects/cm0102/build-slot.sh", "cmake", "--build", build, "--target",
                           "cm0102_recomp"], capture_output=True).returncode != 0:
            print(f"run_set.py: the build in {build} failed")
            return 2
    o["commit"], o["code"], o["sha"] = identity(o["program"])
    for r in rows:                              # unpack the own databases once, before the games start
        if r["status"] == "run":
            database_of(r, o)
    todo = [r for r in rows if not os.path.exists(os.path.join(o["out"], r["name"], "result.tsv"))]
    running = {}
    while todo or running:
        for name, p in list(running.items()):
            if p.poll() is not None:
                del running[name]
                done = os.path.join(o["out"], name, "result.tsv")
                if not os.path.exists(done):            # the child stopped with no result: a fail, with its last line
                    with open(os.path.join(o["out"], name, "run.log"), encoding="utf-8", errors="replace") as g:
                        last = ([l.strip() for l in g if l.strip()] or ["no output"])[-1]
                    row = next(r for r in rows if r["name"] == name)
                    finish(os.path.join(o["out"], name), row, "fail", f"the runner stopped (exit {p.returncode}): "
                                                                       f"{last}", "", "", "", time.time(), o)
                write_results(o, rows)
        while todo and len(running) < o["jobs"]:
            r = todo.pop(0)
            os.makedirs(os.path.join(o["out"], r["name"]), exist_ok=True)
            with open(os.path.join(o["out"], r["name"], "run.log"), "a") as logf:
                running[r["name"]] = subprocess.Popen(
                    [sys.executable, "-I", os.path.abspath(__file__), "--one", r["name"], o["program"], o["game"],
                     "--out", o["out"], "--machine", str(o["machine"]), "--floor-gb", str(o["floor"]),
                     "--databases", o["databases"], "--own-dbs", o["own"], "--commit", o["commit"], "--code",
                     o["code"], "--sha", o["sha"]],
                    stdout=logf, stderr=subprocess.STDOUT)
            print(f"{r['name']}: started", flush=True)
            time.sleep(30)                  # the games do not all make their worlds at once
        time.sleep(10)
    out = write_results(o, rows)
    with open(out, encoding="utf-8") as f:
        results = list(csv.DictReader((l for l in f if not l.startswith("#")), delimiter="\t"))
    for r in results:
        print(f"{r['result']:8} {r['name']} {r['reason']}")
    return 0 if all(r["result"] == "pass" for r in results) else 1


def one(argv):
    name = argv[argv.index("--one") + 1]
    rest = [a for i, a in enumerate(argv) if i not in (argv.index("--one"), argv.index("--one") + 1)]
    o = {"program": os.path.abspath(rest[1]), "game": os.path.abspath(rest[2]), "out": os.path.abspath(arg(rest, "--out")),
         "machine": int(arg(rest, "--machine", 12)), "floor": float(arg(rest, "--floor-gb", 100)),
         "databases": arg(rest, "--databases", DATABASES), "own": arg(rest, "--own-dbs", OWN_DBS),
         "commit": arg(rest, "--commit"), "code": arg(rest, "--code"), "sha": arg(rest, "--sha")}
    row = next(r for r in read_set() if r["name"] == name)
    return 0 if play_game(row, o)[3] in ("pass", "waits") else 1


# ---- The release check ----

def verify(folder, which="release", rows=None):
    """Lines of the faults of a run for the release check; empty when it passes. rows: the run set (run_set.tsv)."""
    rows = chosen(rows if rows is not None else read_set(), which)
    out = []
    path = os.path.join(folder, "results.tsv")
    if not os.path.exists(path):
        return [f"{path} does not exist"]
    with open(path, encoding="utf-8") as f:
        results = {r["name"]: r for r in csv.DictReader((l for l in f if not l.startswith("#")), delimiter="\t")}
    programs = set()
    for row in rows:
        r = results.get(row["name"])
        if not r:
            out.append(f"{row['name']}: not in the run")
            continue
        if r["result"] != "pass":
            out.append(f"{row['name']}: {r['result']} {r['reason']}".rstrip())
        if r.get("row") != row_key(row):
            out.append(f"{row['name']}: played for another row of run_set.tsv ({r.get('row')})")
        if r.get("code") != "clean":
            out.append(f"{row['name']}: played with a program that is not a build of the checkout's game code "
                       f"({r.get('code')})")
        commit = r.get("commit") or ""
        if not commit or subprocess.run(["git", "-C", ROOT, "diff", "--quiet", commit, "HEAD", "--"] + GAME_CODE,
                                        capture_output=True).returncode != 0:
            out.append(f"{row['name']}: the game code changed after its commit {commit[:8] or '(none)'} "
                       f"({', '.join(GAME_CODE)})")
        programs.add(r.get("program"))
        legs = os.path.join(folder, row["name"], "legs.tsv")
        if r["result"] == "pass" and os.path.exists(legs):
            with open(legs, encoding="utf-8") as f:
                for leg in csv.DictReader(f, delimiter="\t"):
                    save = os.path.join(folder, row["name"], f"leg{int(leg['leg']):03d}", "user", SAVE_NAME)
                    if leg["note"] == "checkpoint" and not os.path.exists(save):
                        out.append(f"{row['name']}: the checkpoint save of leg {leg['leg']} is gone")
        elif r["result"] == "pass":
            out.append(f"{row['name']}: no legs.tsv")
    if len(programs) > 1:
        out.append(f"the games were played with {len(programs)} different programs")
    checker = Checker()
    for row in rows:
        for dirpath, _, files in os.walk(os.path.join(folder, row["name"])):
            if SAVE_NAME in files:
                save = os.path.join(dirpath, SAVE_NAME)
                try:
                    _, faults, _, _ = checker.run(save, row["tier"] == "B")
                except Exception as e:          # a save the checker cannot read is a fault of the run
                    out.append(f"{row['name']}: {os.path.relpath(save, folder)}: not read ({e})")
                    continue
                for c, comp, y, detail in faults:
                    out.append(f"{row['name']}: {os.path.relpath(save, folder)}: {c} {comp} {y}: {detail}")
    return out


def main(argv):
    if "--verify" in argv:
        faults = verify(os.path.abspath(arg(argv, "--verify")), arg(argv, "--set", "release"))
        for line in faults:
            print("FAIL  " + line)
        print("run set: PASS" if not faults else f"run set: FAILED, {len(faults)} faults")
        return 1 if faults else 0
    if "--one" in argv:
        return one(argv)
    if len(argv) < 3 or "--out" not in argv:
        print(__doc__)
        return 2
    return run(argv)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
