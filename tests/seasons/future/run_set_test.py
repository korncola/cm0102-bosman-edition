#!/usr/bin/env python3
"""ctest of tests/seasons/future/run_set.py without a game: the run set file, the legs of a row, the Tier B limits
and the release check (--verify) on made-up result folders.

Usage:
    python3 -I tests/seasons/future/run_set_test.py
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import history_check_test as hct  # noqa: E402
import run_set  # noqa: E402

GB = 1 << 30


def results(folder, head, rows):
    os.makedirs(folder, exist_ok=True)
    with open(os.path.join(folder, "results.tsv"), "w", encoding="utf-8") as f:
        f.write(head + "\n" + "\t".join(run_set.RESULT_HEAD) + "\n")
        for r in rows:
            f.write("\t".join(r) + "\n")


def main():
    bad = []
    rows = run_set.read_set()
    release = run_set.chosen(rows, "release")
    names = {r["name"] for r in release}
    for want in ("a2001s1", "a2001offs3", "a2025s3", "a2026s1", "a2026s2", "a2026s3", "a2001esb", "a2001nation", "a2001to2300", "jump2298", "b2001"):
        if want not in names:
            bad.append(f"run set: {want} is not in the release set")
    if any(r["set"] == "quick" for r in release):
        bad.append("run set: a quick game is in the release set")
    if len([r for r in release if r["tier"] == "A" and r["last"] == 2100 and r["nations"] == "all"]) != 25:
        bad.append("run set: not 25 Tier A games of all nations to 2100 (8 starts with Q56's 2026, three seeds, and Q15)")
    for r in (r for r in release if r["start"] == 2026):
        db = run_set.database_of(r, {"databases": run_set.DATABASES, "out": "", "own": run_set.OWN_DBS})
        if not (os.path.relpath(db, run_set.DATABASES).startswith("October 2026")
                and os.path.isfile(os.path.join(db, "index.dat"))):
            bad.append(f"run set: {r['name']} does not play the October 2026 data (the folder of its index.dat): {db}")

    # The new game steps of every row (soak_run's leg 0 uses rules_report_check.league_steps). a2001esb picks Spain,
    # below the 16 rows of Select League(s): pass 2 stopped in the runner with a ValueError. Spain is the 13th row
    # after 10 clicks on the list's down arrow (screenshots in run-logs/runset-pass2/esb).
    import rules_report_check as rrc
    for r in rows:
        try:
            rrc.league_steps(r["nations"].split(","), [])
        except ValueError as e:
            bad.append(f"league steps of {r['name']} ({r['nations']}): {e}")
    esb = [s.split(" ", 1)[1] for s in rrc.league_steps(["England", "Spain", "Brazil"], []) if " click " in s]
    esb = esb[esb.index("click 390 254"):] if "click 390 254" in esb else esb      # from the Brazil row
    if esb[:13] != ["click 390 254"] + ["click 768 517"] * 10 + ["click 390 452", "click 705 572"]:
        bad.append(f"league steps of England, Spain and Brazil: {esb[:13]}")

    legs = run_set.legs_of({"start": 2001, "last": 2100, "leg_years": 10})
    if len(legs) != 10 or legs[0] != (2001, 2010) or legs[-1] != (2091, 2100):
        bad.append(f"legs of 2001 to 2100 by 10: {legs[:2]} ... {legs[-1:]}")
    if run_set.legs_of({"start": 2098, "last": 2100, "leg_years": 1}) != [(2098, 2098), (2099, 2099), (2100, 2100)]:
        bad.append("legs of a date jump: not one season each")

    # The day count of RECOMP_SAVE_AFTER_DAYS (testrun.c): 366 a year. From day 196 of 2056 to day 200 of 2066 it
    # gives 3664; the calendar gives 3657, which saved the leg of pass 1 on day 181 of 2066, before July.
    if run_set.game_days(2066, run_set.SAVE_ON) - run_set.game_days(2056, 196) != 3664:
        bad.append("game_days: not the count of testrun.c (year * 366 + day)")

    def leg(k, year, seconds, size, no_manager=frozenset()):
        return {"leg": k, "year": year, "seconds": seconds, "days": 365, "bytes": size, "no_manager": set(no_manager)}

    good = [leg(k, 2002 + k, 1000 + 10 * k, 200e6 + 1e6 * k) for k in range(30)]
    if run_set.tier_b_limits(good):
        bad.append(f"Tier B limits: faults in a good game: {run_set.tier_b_limits(good)}")
    checks = {
        "no-manager": good[:5] + [leg(5, 2007, 1050, 205e6, {"Wales"}), leg(6, 2008, 1060, 206e6, {"Wales"})],
        "slow": good[:20] + [leg(k, 2002 + k, 2500, 220e6) for k in range(20, 30)],
        # leg 0 also makes the game and is slow: it must not set the speed of the first seasons
        "slow ": [leg(0, 2002, 9000, 200e6)] + good[1:20] + [leg(k, 2002 + k, 2500, 220e6) for k in range(20, 30)],
        "size": [leg(k, 2002 + k, 1000, 200e6 + 30e6 * k) for k in range(30)],
    }
    for check, game in checks.items():
        got = {c for c, _ in run_set.tier_b_limits(game)}
        if got != {check.strip()}:
            bad.append(f"Tier B limits: {check} not found alone, got {got}")
    one_season = good[:5] + [leg(5, 2007, 1050, 205e6, {"Wales"}), leg(6, 2008, 1060, 206e6, {"Chile"})]
    if run_set.tier_b_limits(one_season):
        bad.append("Tier B limits: a side with no manager in one save only fails")
    if [c for c, _ in run_set.tier_b_limits(good[:3] + [leg(3, 2005, 1000, 4 * GB)])] != ["size"]:
        bad.append("Tier B limits: a save of 4 GB is not a fault")

    # Open item 68: one group of the Greek Super League 2 plays from 2026/27, the other waits at 2998.
    def gre(season, year1, year2, clubs=14):
        return [{"season": season, "phase": "start", "file": "gre_second", "year": year1, "clubs": clubs,
                 "name": "Super League 2"},
                {"season": season, "phase": "start", "file": "gre_second", "year": year2, "clubs": 10,
                 "name": "Super League 2"}]
    # a loaded leg's end line of the season before already shows the next year: not a group that plays
    late = [dict(c, phase="end", year=2027) if c["year"] == 2026 else dict(c, phase="end") for c in gre(2026, 2026, 2998)]
    one_group = gre(2025, 2025, 2025) + gre(2026, 2026, 2998) + late + gre(2027, 2027, 2998)
    if run_set.greek_second(one_group):
        bad.append(f"Greek Super League 2: faults in a game with one group from 2026: {run_set.greek_second(one_group)}")
    for name, comps, want in (("two groups", gre(2026, 2026, 2998) + gre(2027, 2027, 2027), 2027),
                              ("small group", gre(2026, 2026, 2998, clubs=10), 2026),
                              ("no group", gre(2026, 2998, 2998), 2026)):
        got = run_set.greek_second(comps)
        if [s_ for s_, _ in got] != [want]:
            bad.append(f"Greek Super League 2 ({name}): {got}, not one fault in {want}")
    if run_set.greek_second(gre(2025, 2025, 2025)):
        bad.append("Greek Super League 2: two groups in 2025/26 are a fault")
    # Open item 104: a leg that loads a save writes its first start lines under the game's start season
    # (season_report_turn), with the state at the load. jump2198 has one group, slot t144: leg 0 writes 2198 at the
    # start of 2198/99, legs 1 and 2 write 2199 and 2200 under 2198 again. Only the first line of a slot counts.
    def slot(season, year, clubs=14, name="t144"):
        return {"season": season, "phase": "start", "file": "gre_second", "year": year, "clubs": clubs,
                "slot": name, "name": "Super League 2"}
    jump = [slot(2198, 2198), slot(2198, 2199), slot(2199, 2199), slot(2198, 2200), slot(2200, 2200)]
    if run_set.greek_second(jump):
        bad.append(f"Greek Super League 2: the loaded legs of a date jump give faults: {run_set.greek_second(jump)}")
    loaded = [slot(2026, 2026), slot(2026, 2998, 10, "t145"), slot(2027, 2027), slot(2027, 2998, 10, "t145"),
              slot(2026, 2027), slot(2026, 2998, 10, "t145")]
    if run_set.greek_second(loaded):
        bad.append(f"Greek Super League 2: a leg loaded in 2027 gives faults in 2026: {run_set.greek_second(loaded)}")
    if [s_ for s_, _ in run_set.greek_second([slot(2026, 2026), slot(2026, 2026, 10, "t145"),
                                             slot(2026, 2027), slot(2026, 2998, 10, "t145")])] != [2026]:
        bad.append("Greek Super League 2: a later loaded leg hides two groups at the start of 2026/27")

    # A passed leg: a kept leg keeps its save and the gzipped log, another leg the log only.
    work = tempfile.mkdtemp(prefix="run-set-")
    for keep in (True, False):
        leg_dir = os.path.join(work, f"leg-{keep}")
        for d in ("user/Databases/December 2025", "home", "shots"):
            os.makedirs(os.path.join(leg_dir, d))
        for n in ("user/" + run_set.SAVE_NAME, "user/match_stats_matches.tmp", "stderr.txt", "report.tsv"):
            open(os.path.join(leg_dir, n), "w").write("x")
        run_set.tidy(leg_dir, keep)
        left = sorted(os.path.relpath(os.path.join(d, n), leg_dir) for d, _, fs in os.walk(leg_dir) for n in fs)
        want = sorted(["report.tsv", "stderr.txt.gz"] + (["user/" + run_set.SAVE_NAME] if keep else []))
        if left != want:
            bad.append(f"tidy (keep {keep}): {left}, not {want}")

    # The release check on made-up runs. Each game passes with one leg and no kept save, unless a case says else.
    head = run_set.git("rev-parse", "HEAD")
    runs = [r for r in release if r["status"] == "run"]      # the rows a run can pass

    def result(r, **change):
        values = {"name": r["name"], "tier": r["tier"], "set": r["set"], "result": "pass", "reason": "",
                  "reached": "2101", "legs": "1", "faults": "0", "seconds": "1", "commit": head, "code": "clean",
                  "program": "sha-x", "row": run_set.row_key(r), "accepted": "0"}
        values.update(change)
        return [str(values[c]) for c in run_set.RESULT_HEAD]

    def make(name, rows, checkpoint=None, save=None):
        folder = os.path.join(work, name)
        results(folder, "# a head line", rows)
        for r in rows:
            os.makedirs(os.path.join(folder, r[0]), exist_ok=True)
            with open(os.path.join(folder, r[0], "legs.tsv"), "w", encoding="utf-8") as f:
                f.write("\t".join(run_set.LEG_HEAD) + "\n")
                f.write(f"0\t2001/02 to 2001/02\tok\t1\t2002-201\t1\t0\t{'checkpoint' if r[0] == checkpoint else ''}\n")
        if save:
            os.makedirs(os.path.join(folder, checkpoint, "leg000", "user"))
            hct.write_sav(os.path.join(folder, checkpoint, "leg000", "user", run_set.SAVE_NAME), save)
        return run_set.verify(folder, rows=runs)

    def finds(lines, text):
        return any(text in line for line in lines)

    ok = make("ok", [result(r) for r in runs])
    if ok:
        bad.append(f"verify: faults in a run where every game passed: {ok[:3]}")
    cases = [
        ("fail", [result(r, result="fail", reason="3 checker faults") if r["name"] == "b2001" else result(r)
                  for r in runs], "b2001: fail 3 checker faults"),
        ("short", [result(r) for r in runs[1:]], "not in the run"),
        ("old", [result(r, commit=run_set.git("rev-list", "-1", "HEAD", "--", "src") + "~1") for r in runs],
         "game code changed"),
        ("dirty", [result(r, code="changes") for r in runs], "not a build of the checkout"),
        ("outside", [result(r, code="outside") for r in runs], "not a build of the checkout"),
        ("two", [result(r, program="sha-y" if r["name"] == "b2025" else "sha-x") for r in runs], "different programs"),
        ("row", [result(r, row=run_set.row_key(dict(r, last=2099))) if r["name"] == "a2013s2" else result(r)
                 for r in runs], "another row"),
    ]
    for name, rows_, text in cases:
        if not finds(make(name, rows_), text):
            bad.append(f"verify ({name}): no line with {text!r}")
    if not finds(make("gone", [result(r) for r in runs], checkpoint="b2001"), "checkpoint save of leg 0 is gone"):
        bad.append("verify: a lost checkpoint save passes")
    members, _ = hct.records()
    lines = make("faults", [result(r) for r in runs], checkpoint="b2001", save=members)
    for text in ("samesave.sav: duplicate Euro Finals 2028", "samesave.sav: no-host World Finals 2034",
                 "samesave.sav: same-final Euro Super Cup 2028"):      # history faults that the fixture plants
        if not finds(lines, text):
            bad.append(f"verify: the checker does not find {text!r} on a kept save")
    lines = make("faults-a", [result(r) for r in runs], checkpoint="a2006s1", save=members)
    if not finds(lines, "samesave.sav: duplicate Euro Finals 2028") or finds(lines, "same-final"):
        bad.append("verify: a Tier A save does not skip same-final (a check of real results) or skips more")
    # A row that waits has no result, and it blocks a release (a2001nation waited until Q15 was built).
    make("waits", [result(r) for r in runs if r["name"] != "a2001nation"])
    waiting = run_set.verify(os.path.join(work, "waits"), rows=[dict(r, status="waits") if r["name"] == "a2001nation"
                                                               else r for r in rows])
    if not finds(waiting, "a2001nation: not in the run"):
        bad.append("verify: a waiting row does not block a release")

    # Q15: leg 0 of a NATION_JOB row passes only with the appointment and the lines after the team's next match.
    if run_set.NATION_JOB.get("a2001nation") != "England" or \
            [r for r in run_set.read_set() if r["name"] == "a2001nation"][0]["status"] != "run":
        bad.append("nation job: a2001nation is not a NATION_JOB row with status run")
    made = "recomp: nation job test: the human manages Wales from day 182 of 2001 (appointed)\n"
    still = "recomp: nation job test: on day 245 of 2001 (after the match of day 244) the human still manages Wales\n"
    lost = still.replace("still manages", "no longer manages")
    match = "recomp: nation job test: Wales played the match of day 244 of 2001 against Spain\n"
    none = "recomp: nation job test: Wales did not play the match of day 244 of 2001\n"
    for text, want in ((made + still + match, ""), (made + lost + match, "no line that the human still manages Wales"),
                       (made + still, "no line that Wales played its next match"),
                       (made + still + none, "no line that Wales played its next match"),
                       (made + still + match.replace("Wales", "Scotland"), "no line that Wales played"),
                       (made, "no line that the human still manages Wales"), ("", "no appointment"),
                       (made + still.replace("Wales", "Scotland") + match, "no line that the human still manages Wales"),
                       ("recomp: nation job test: no national team without a manager and a value over 150\n",
                        "no appointment (no national team"),
                       (made.replace("(appointed)", "(the call did not appoint him)") + still + match, "no appointment")):
        got = run_set.nation_job_check(text)
        if (want == "" and got) or (want and want not in got):
            bad.append(f"nation_job_check: {got!r} for {text!r}, wanted {want!r}")
    code = subprocess.run([sys.executable, "-I", os.path.join(HERE, "run_set.py"), "--verify",
                           os.path.join(work, "fail")], capture_output=True).returncode
    if code != 1:
        bad.append(f"verify: exit code {code} for a failed run, not 1")

    # Accepted rows (Q12): a known row that cites a source is no fault and is counted apart; a rules condition holds.
    class FakeGame:
        def __init__(self, changing):
            self.changing = changing
    checker = run_set.Checker.__new__(run_set.Checker)
    checker.known = [{"check": "gap", "competition": "A Cup", "year": "2022", "note": "source", "rules": "",
                      "accepted": "real"},
                     {"check": "gap", "competition": "B Cup", "year": "*", "note": "no source", "rules": "",
                      "accepted": ""},
                     {"check": "entrant-host", "competition": "C Cup", "year": "2002", "note": "design 0x5f9970",
                      "rules": "off", "accepted": "design"}]
    faults = [("gap", "A Cup", 2022, "d"), ("gap", "B Cup", 2001, "d"), ("gap", "D Cup", 2001, "d"),
              ("entrant-host", "C Cup", 2002, "d")]
    for changing, want_kinds in ((0, ["accepted", "known", "new", "accepted"]), (1, ["accepted", "known", "new", "new"])):
        kinds, n = run_set.grade(checker, faults, FakeGame(changing))
        if kinds != want_kinds or n != want_kinds.count("accepted"):
            bad.append(f"grade (changing rules {changing}): {kinds} and {n}, not {want_kinds}")
    old_read = run_set.hc.read_known
    run_set.hc.read_known = lambda path: [{"check": "duplicate", "competition": "Euro Finals", "year": "2028",
                                           "note": "source", "rules": "", "accepted": "real"}]
    try:
        lines = make("accepted", [result(r) for r in runs], checkpoint="b2001", save=members)
    finally:
        run_set.hc.read_known = old_read
    if finds(lines, "duplicate Euro Finals 2028") or not finds(lines, "no-host World Finals 2034"):
        bad.append("verify: an accepted fault still fails the release check, or an unknown one passes")
    os.makedirs(os.path.join(work, "fin"), exist_ok=True)
    row_ = run_set.finish(os.path.join(work, "fin"), runs[0], "pass", "3 accepted", "2101", 1, 0, 0.0,
                          {"commit": "c", "code": "clean", "sha": "s"}, 3)
    if row_[-1] != 3 or len(row_) != len(run_set.RESULT_HEAD):
        bad.append(f"finish: the result row {row_} has no accepted count at the end")

    for b in bad:
        print("FAIL  " + b)
    print(f"{'ok  ' if not bad else 'FAIL'}  run set: {len(release)} games in the release set, legs, Tier B limits and "
          f"the release check (work folder {work})")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
