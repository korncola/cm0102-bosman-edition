#!/usr/bin/env python3
"""Fixed class counts at a new game (0.6.2 open item 105): a division whose league class takes a fixed number of
clubs starts with that number after the club moves, and Belgium's leagues move on at the 2026 season change.

Usage:
    python3 tests/seasons/class_count_run.py <cm0102_recomp> <game folder> [--only start,season,irish] [--keep <folder>]

The runs, each a season game with all nations, changing rules on, as a Tier A run, with no season database. Start
and season use data set 3.9.68 and start in 2025 (the dates of the data move to 2025). Irish starts four games:
    start   ends after the start report (about 3 minutes). After the club moves no division is short (the log
            line "club moves: <division> (tier n) holds k clubs, its class takes m") except the two of SHORT_OK,
            and the Belgian First and Second Division hold 18 clubs each in the report.
            Why: BelSecond takes its clubs by count, and its three period stages expect 10, 12 and 12 matches a club
            (BelSecond::create_period_stage 0x4204d0, 0x4205b4..0x4205cb), the periods of a 34-round season of 18
            clubs (BelSecond::add_result 0x420110: rounds before 10, before 22, then the rest). With 17 clubs a
            club has a free round in a period, its period stage never completes, the promotion play-off (stage 3,
            BelSecond::advance_stage 0x4201a0) is never made, and League::check_complete 0x689ad0 (0x689bdf..
            0x689be8) keeps the division open. BelFirst::all_stages_complete (Comp 0x48ce10) walks the divisions
            below it, so on the update day game_season_update_pass 0x5d5ae0 finds it false (0x5d5ccb) and never
            calls BelFirst::start_new_season (vtable +0x8, 0x5d5d1c): no Belgian division moves on. The club moves left one place of each division open: a listed club that
            the data lacks, and no Belgian club without a division to stand in for it.
    season  plays to the end of 2026/27 (about 30 minutes): no message box, and no Belgian division stalls at
            the 2026 season change (rules_report_check.league_stalls).
    irish   four starts to the start report (0.6.2 open item 123): 2008, 2010 and 2017 on 3.9.68 and 2012 on 3.9.65
            (DATA_3965, skipped when the folder is missing). Each: no message box, no short Irish or Norwegian
            division, and the Irish Premier and First Division at counts their classes take. IrePrm::create_table
            0x63a740 takes only 10 or 12, IreFirst::create_table 0x635910 9 to 12. The 2008 start also holds 12 and
            10, the counts of the data. The list of 2008 names "Bray Wanderers", which 3.9.68 holds as "Bray
            Wanderers A.F.C." (club 1855), so the club moves did not find it and left its place in the Premier
            Division open: 11 clubs, and IrePrm::IrePrm 0x639bb0 stopped at ire_prm..cpp 82 (push 0x52 at
            0x639ca7). The 3.9.65 data hold 9 First Division clubs, so one open place left 8. The club moves now
            fill as many open places as the class needs (season_clubs.c, data_class_count).
Without --keep the runs play in a temporary folder that the script deletes when every run passes.
"""
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import rules_report_check as rrc  # noqa: E402

OPTIONS = {"changing_rules": "on"}
ENV = {"RECOMP_TIER_A": "1"}
BELGIAN = {"bel_first": 18, "bel_second": 18}
# The short divisions of this start that the fix leaves as they are: the French Lower Division (a group class that
# counts the clubs of the data) and the Greek second group, a new division of slice 20 that its own fill takes from
# the Greek Lower Division later in the new game (season_divisions.c).
SHORT_OK = {"French Lower Division", "Greek Second Division Group 2"}


def short_divisions(log):
    out = []
    for m in re.finditer(r"club moves: (.+) \(tier \d+\) holds (\d+) clubs, its class takes (\d+)", log):
        if m.group(1) not in SHORT_OK:
            out.append(f"{m.group(1)} holds {m.group(2)} clubs, its class takes {m.group(3)}")
    return out


def boxes(name, work):
    log = open(os.path.join(work, "stderr.txt"), encoding="latin-1").read()
    return [f"{name}: message box {b}" for b in re.findall(r"recomp: message box from (.*)", log)], log


def run_start(program, game, work):
    code, report = rrc.run(program, game, 2025, 2025, None, ["all"], [], work, 3600, OPTIONS, ENV, start_only=True)
    fails = [] if code == 0 else [f"start: the game ended with code {code}"]
    found, log = boxes("start", work)
    fails += found
    fails += [f"start: {line}" for line in short_divisions(log)]
    if "Greek second group 2025: 10 clubs (10 from the Lower Division)" not in log:
        fails.append("start: the Greek second group does not take its 10 clubs from the Lower Division")
    if not os.path.exists(report):
        return fails + [f"start: no report ({work})"]
    _, comps = rrc.read_report(report)
    for file, want in BELGIAN.items():
        got = [c["clubs"] for c in comps if c["file"] == file and c["phase"] == "start" and c["season"] == 2025]
        if got != [want]:
            fails.append(f"start: {file} holds {got} clubs at the start of 2025/26, not {want}")
    return fails


def run_season(program, game, work):
    code, report = rrc.run(program, game, 2026, 2025, None, ["all"], [], work, 4 * 3600, OPTIONS, ENV)
    fails = [] if code == 0 else [f"season: the game ended with code {code}"]
    fails += boxes("season", work)[0]
    if not os.path.exists(report):
        return fails + [f"season: no report ({work})"]
    _, comps = rrc.read_report(report)
    fails += [f"season: {line}" for line in rrc.league_stalls(comps) if "bel_" in line]
    if not any(c["file"] == "bel_first" and c["season"] == 2026 and c["phase"] == "end" for c in comps):
        fails.append("season: the report has no end of 2026/27")
    return fails


# The irish starts: (year, database folder or None for 3.9.68, exact counts or None). Every start checks that the
# counts are ones the classes take: IrePrm 10 or 12, IreFirst 9 to 12.
DATA_3965 = "/Users/max/Projects/cm0102/data-3965"
IRISH = [(2008, None, {"ire_prm": 12, "ire_first": 10}), (2010, None, None), (2017, None, None),
         (2012, DATA_3965, None)]
TAKES = {"ire_prm": {10, 12}, "ire_first": {9, 10, 11, 12}}


def run_irish(program, game, work):
    env = dict(ENV, RECOMP_CLUB_LOG="1")
    fails = []
    for year, database, exact in IRISH:
        name = f"irish {year}" + (f" {os.path.basename(database)}" if database else "")
        if database and not os.path.isdir(database):
            print(f"note  {name}: no database folder {database}, not run")
            continue
        sub = os.path.join(work, name.replace(" ", "-"))
        code, report = rrc.run(program, game, year, year, database, ["all"], [], sub, 3600, OPTIONS, env,
                               start_only=True, stop_on_box=True)
        if code != 0:
            fails.append(f"{name}: the game ended with code {code}")
        found, log = boxes(name, sub)
        fails += found
        if f"recomp: club moves of {year}/{(year + 1) % 100:02d}:" not in log:
            fails.append(f"{name}: the club moves did not run (no seasons tables beside the program?)")
        fails += [f"{name}: {line}" for line in short_divisions(log) if line.startswith(("Irish", "Norwegian"))]
        if not os.path.exists(report):
            fails.append(f"{name}: no report ({sub})")
            continue
        try:
            _, comps = rrc.read_report(report)
        except SystemExit as e:                 # a box stops the game before the report is whole
            fails.append(f"{name}: {e}")
            continue
        for file, takes in TAKES.items():
            got = [c["clubs"] for c in comps if c["file"] == file and c["phase"] == "start" and c["season"] == year]
            want = exact[file] if exact else None
            if len(got) != 1 or got[0] not in takes or (want is not None and got[0] != want):
                fails.append(f"{name}: {file} holds {got} clubs at the start, the class takes "
                             f"{want if want is not None else sorted(takes)}")
        shown = [f"{c['file']} {c['clubs']}" for c in comps
                 if c["file"] in TAKES and c["phase"] == "start" and c["season"] == year]
        print(f"      {name}: {', '.join(shown)}")
    return fails


RUNS = {"start": run_start, "season": run_season, "irish": run_irish}


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = argv[1], argv[2]
    only = rrc.arg(argv, "--only", ",".join(RUNS)).split(",")
    given = rrc.arg(argv, "--keep", None)
    keep = given or tempfile.mkdtemp(prefix="class-count-")
    fails = []
    for name in only:
        got = RUNS[name](program, game, os.path.join(keep, name))
        print(f"{name}: {'ok' if not got else 'FAIL'} ({os.path.join(keep, name)})")
        fails += got
    for f in fails:
        print("FAIL ", f)
    print("PASS" if not fails else f"FAILED: {len(fails)}")
    if not given and not fails:
        shutil.rmtree(keep, ignore_errors=True)
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
