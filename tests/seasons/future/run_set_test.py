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
    for want in ("a2001s1", "a2001offs3", "a2025s3", "a2001esb", "a2001nation", "a2001to2300", "jump2298", "b2001"):
        if want not in names:
            bad.append(f"run set: {want} is not in the release set")
    if any(r["set"] == "quick" for r in release):
        bad.append("run set: a quick game is in the release set")
    if len([r for r in release if r["tier"] == "A" and r["last"] == 2100 and r["nations"] == "all"]) != 22:
        bad.append("run set: not 22 Tier A games of all nations to 2100 (7 starts, three seeds, and Q15)")

    legs = run_set.legs_of({"start": 2001, "last": 2100, "leg_years": 10})
    if len(legs) != 10 or legs[0] != (2001, 2010) or legs[-1] != (2091, 2100):
        bad.append(f"legs of 2001 to 2100 by 10: {legs[:2]} ... {legs[-1:]}")
    if run_set.legs_of({"start": 2098, "last": 2100, "leg_years": 1}) != [(2098, 2098), (2099, 2099), (2100, 2100)]:
        bad.append("legs of a date jump: not one season each")

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
    runs = [r for r in release if r["status"] == "run"]      # the rows a run can pass; the waiting row comes below

    def result(r, **change):
        values = {"name": r["name"], "tier": r["tier"], "set": r["set"], "result": "pass", "reason": "",
                  "reached": "2101", "legs": "1", "faults": "0", "seconds": "1", "commit": head, "code": "clean",
                  "program": "sha-x", "row": run_set.row_key(r)}
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
    lines = make("faults", [result(r) for r in runs], checkpoint="a2006s1", save=members)
    for text in ("samesave.sav: duplicate Euro Finals 2028", "samesave.sav: no-host World Finals 2034",
                 "samesave.sav: same-final Euro Super Cup 2028"):      # history faults that the fixture plants
        if not finds(lines, text):
            bad.append(f"verify: the checker does not find {text!r} on a kept save")
    waiting = run_set.verify(os.path.join(work, "ok"))                 # the real set: a2001nation waits
    if not finds(waiting, "a2001nation: not in the run"):
        bad.append("verify: the waiting row a2001nation does not block a release")
    code = subprocess.run([sys.executable, "-I", os.path.join(HERE, "run_set.py"), "--verify",
                           os.path.join(work, "fail")], capture_output=True).returncode
    if code != 1:
        bad.append(f"verify: exit code {code} for a failed run, not 1")

    for b in bad:
        print("FAIL  " + b)
    print(f"{'ok  ' if not bad else 'FAIL'}  run set: {len(release)} games in the release set, legs, Tier B limits and "
          f"the release check (work folder {work})")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
