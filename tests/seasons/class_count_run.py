#!/usr/bin/env python3
"""Fixed class counts at a new game (0.6.2 open item 105): a division whose league class takes a fixed number of
clubs starts with that number after the club moves, and Belgium's leagues move on at the 2026 season change.

Usage:
    python3 tests/seasons/class_count_run.py <cm0102_recomp> <game folder> [--only start,season] [--keep <folder>]

The runs, each a new game of 2025/26 on the game's own data (3.9.68, jumped to 2025), all nations, changing rules
on, Tier A:
    start   ends after the start report (about 3 minutes). No division of a class with a fixed count
            (CLASS_SIZE of src/recomp_rt/win/season_clubs.c) is short after the club moves (the log line
            "club moves: <division> (tier n) holds k clubs, its class takes m"), and the Belgian First and Second
            Division hold 18 clubs each in the report.
            Why: BelSecond takes its clubs by count, and its three period stages expect 10, 12 and 12 matches a club
            (BelSecond::create_period_stage 0x4204d0, 0x4205b4..0x4205cb), the periods of a 34-round season of 18
            clubs (BelSecond::add_result 0x420110: rounds before 10, before 22, then the rest). With 17 clubs a
            club has a free round in a period, its period stage never completes, the promotion play-off (stage 3,
            BelSecond::advance_stage 0x4201a0) is never made, and League::check_complete 0x689ad0 (0x689bdf..
            0x689be8) keeps the division open. BelFirst::all_stages_complete (Comp 0x48ce10) walks the divisions
            below it, so game_season_update_pass 0x5d5ae0 never calls BelFirst::start_new_season (0x5d5ccb), and
            no Belgian division moves on. The club moves left one place of each division open: a listed club that
            the data lacks, and no Belgian club without a division to stand in for it.
    season  plays to the end of 2026/27 (about 30 minutes): no Belgian division stalls at the 2026 season change
            (rules_report_check.league_stalls).
"""
import os
import re
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import rules_report_check as rrc  # noqa: E402

SOURCE = os.path.join(HERE, "..", "..", "src", "recomp_rt", "win", "season_clubs.c")
OPTIONS = {"changing_rules": "on"}
ENV = {"RECOMP_TIER_A": "1"}
BELGIAN = {"bel_first": 18, "bel_second": 18}


def fixed_classes():
    """The division names of CLASS_SIZE in season_clubs.c."""
    text = open(SOURCE, encoding="utf-8").read()
    table = text[text.index("CLASS_SIZE[] = {"):]
    table = table[:table.index("};")]
    return set(re.findall(r'\{"([^"]+)", "[a-z0-9_]+", \d+, \d+\}', table))


def short_fixed(log):
    names = fixed_classes()
    out = []
    for m in re.finditer(r"club moves: (.+) \(tier \d+\) holds (\d+) clubs, its class takes (\d+)", log):
        if m.group(1) in names:
            out.append(f"{m.group(1)} holds {m.group(2)} clubs, its class takes {m.group(3)}")
    return out


def run_start(program, game, work):
    code, report = rrc.run(program, game, 2025, 2025, None, ["all"], [], work, 3600, OPTIONS, ENV, start_only=True)
    fails = [] if code == 0 else [f"start: the game ended with code {code}"]
    log = open(os.path.join(work, "stderr.txt"), encoding="latin-1").read()
    fails += [f"start: message box {b}" for b in re.findall(r"recomp: message box from (.*)", log)]
    fails += [f"start: {line}" for line in short_fixed(log)]
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
    if not os.path.exists(report):
        return fails + [f"season: no report ({work})"]
    _, comps = rrc.read_report(report)
    fails += [f"season: {line}" for line in rrc.league_stalls(comps) if "bel_" in line]
    if not any(c["file"] == "bel_first" and c["season"] == 2026 and c["phase"] == "end" for c in comps):
        fails.append("season: the report has no end of 2026/27")
    return fails


RUNS = {"start": run_start, "season": run_season}


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    program, game = argv[1], argv[2]
    only = rrc.arg(argv, "--only", ",".join(RUNS)).split(",")
    keep = rrc.arg(argv, "--keep", None) or tempfile.mkdtemp(prefix="class-count-")
    fails = []
    for name in only:
        got = RUNS[name](program, game, os.path.join(keep, name))
        print(f"{name}: {'ok' if not got else 'FAIL'} ({os.path.join(keep, name)})")
        fails += got
    for f in fails:
        print("FAIL ", f)
    print("PASS" if not fails else f"FAILED: {len(fails)}")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
