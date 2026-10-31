#!/usr/bin/env python3
"""Faults of the original that long games reach (0.6.2 slice 11): national managers (fault 26, Q23) and money above
2,147,483,647 (faults 27 and 28, Q24) and the human manager's age (fault 33, Q27).

Usage:
    python3 tests/seasons/original_faults_run.py managers <cm0102_recomp> <game folder> --save <in.sav> --keep <folder>
        [--days 400] [--keep-faults]
    python3 tests/seasons/original_faults_run.py money <cm0102_recomp> <game folder> --keep <folder>
        [--balance 2500000000,5000000000,12000000000,-2500000000] [--days 45] [--keep-faults]
    python3 tests/seasons/original_faults_run.py age <cm0102_recomp> <game folder> --keep <folder> [--born 1870]
        [--keep-faults]

managers: loads a save made on holiday (a soak leg of tests/seasons/soak_run.py, changing rules on, the 3.9.68
database), plays --days game days on holiday and saves (soak_run.load_leg). It reads the senior national teams of
both saves (nat_club.dat: the first nation-count rows; the manager at club +0xcf, the players of the nation at nation
+0x88) and checks:
    - at the end, every senior national team has a manager, but for at most 2 (a vacancy waits for its start date;
      the default of 400 days ends in mid August, after the contract ends of 30 June and the month check of 1 August)
    - no nation without a continent (a nation that no longer exists, such as East Germany) gets a new manager; the
      first check counts only the nations with a continent
    - a national manager left in the run and his team got a new manager (a retirement or a move to a club)
    - no message box, and the program ends with code 0
--keep-faults runs with RECOMP_ORIGINAL_FAULTS=keep (the original's rule). The checks must then fail: this is the
proof that they find the fault (the same holds for money). The suggested save: the 2011 leg of the 0.6.0 soak seed 6
(run-logs/release-test-0.6.0/long-games/soak-seed6/leg09/user/samesave.sav), where 161 of 213 teams have no manager.

money: for each balance, sets the balance of Arsenal (the human's club) and of Liverpool (an AI club whose budget follows its balance: club +0x82 is 0, see
docs/research/2026-10-30-money-64bit.md) in start.sav of the
test fixtures (tools/save/edit_money.py), loads it, opens Arsenal's Finances view (a screen dump), goes on holiday and
saves --days game days later (tests/fixtures/fixtures.py play). The runtime logs each 64-bit amount that a screen
formats (RECOMP_MONEY_LOG: "money text") and each transfer budget of a club above 2,147,483,647 ("money budget").
The checks, for each balance (a negative balance is a debt: only the first check, on the amount of the debt):
    - the Finances view formats the full balance and the full transfer funds ("money text" lines; the funds are at
      least 99% of the balance)
    - Liverpool's budget is at least 99% of its balance, as below 2,147,483,647 (the fault gave a tenth: the floor
      of `get_transfer_budget` was cut to 32 bits), and its shortlist (`try_add_player_to_club_shortlist`, return
      0x84301b, 32 bits) sees that budget clamped to 2,147,483,647
    - `TRANSFER_OFFER::build_bid` asks for Liverpool's budget and gets a positive one (return 0x8e6335 or 0x8e74a7).
      It asks only while it builds a bid. This check also passes with the fault kept (the budget is then a tenth):
      the budget check above is the one that finds the fault
    - no message box, and the program ends with code 0

age: sets the birth year of the human manager of start.sav (+0x12) to --born, loads it, goes on holiday and saves 3
game days later. The day loop stores each staff member's age at +0x18 (0x5cd3c4); for an age above 127 the save must
hold 127, where the original's byte wraps to a negative age.
"""
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools", "save"))
import savfile  # noqa: E402
import soak_run  # noqa: E402

sys.path.insert(0, os.path.join(HERE, "..", "fixtures"))
import fixtures  # noqa: E402

CLUB_SIZE, NATION_SIZE, STAFF_SIZE = 0x245, 0x122, 110
VACANCY = "recomp: national manager: a vacancy for "


def national_managers(path):
    """{team name: (manager staff id or -1, players of the nation, 1 when the nation has a continent)} of the senior
    national teams of a save. A nation with no continent (id -1 at +0x71) no longer exists or is not a FIFA member."""
    sav = savfile.Sav(path)
    teams, nations = sav.read_member("nat_club.dat"), sav.read_member("nation.dat")
    staff = len(sav.read_member("staff.dat")) // STAFF_SIZE
    count = len(nations) // NATION_SIZE
    out = {}
    for row in range(count):                       # the senior teams come first, then the under-21 teams
        rec = teams[row * CLUB_SIZE:(row + 1) * CLUB_SIZE]
        name = rec[4:0x37].split(b"\0")[0].decode("latin-1")
        nation, manager = struct.unpack_from("<i", rec, 0x53)[0], struct.unpack_from("<i", rec, 0xcf)[0]
        players = struct.unpack_from("<i", nations, nation * NATION_SIZE + 0x88)[0]
        continent = struct.unpack_from("<i", nations, nation * NATION_SIZE + 0x71)[0]
        out[name] = (manager if 0 <= manager < staff else -1, players, int(continent >= 0))
    return out


def managers(argv):
    program, game = os.path.abspath(argv[0]), os.path.abspath(argv[1])
    save, keep = os.path.abspath(soak_run.arg(argv, "--save")), os.path.abspath(soak_run.arg(argv, "--keep"))
    days = int(soak_run.arg(argv, "--days", 400))
    if "--keep-faults" in argv:
        os.environ["RECOMP_ORIGINAL_FAULTS"] = "keep"
    o = {"program": program, "game": game, "rules": "on", "database": None, "last": 2099, "seed": 1,
         "timeout": 7200}
    work = os.path.join(keep, "managers")
    code, took, box = soak_run.load_leg(o, work, save, days)
    print(f"run: code {code} after {took:.0f} s, log {os.path.join(work, 'stderr.txt')}")
    fails = []
    if code != 0:
        fails.append(f"the program ended with {code}" + (f" ({box})" if box else ""))
    end = os.path.join(work, "user", "samesave.sav")
    if not os.path.exists(end):
        return fails + ["no save at the end"]
    before, after = national_managers(save), national_managers(end)
    log = open(os.path.join(work, "stderr.txt"), encoding="latin-1").read().split("\n")
    vacancies = [x[len(VACANCY):] for x in log if x.startswith(VACANCY)]
    print(f"senior national teams with a manager: {sum(m >= 0 for m, *_ in before.values())} of {len(before)} at the "
          f"start, {sum(m >= 0 for m, *_ in after.values())} at the end; {len(vacancies)} vacancies of the fix "
          f"({sum('after a retirement' in v for v in vacancies)} after a retirement, "
          f"{sum('after a move to another national team' in v for v in vacancies)} after a move, "
          f"{sum('after a departure' in v for v in vacancies)} after a departure, "
          f"{sum('after the month check' in v for v in vacancies)} at a month check)")
    empty = sorted(t for t, (m, _, live) in after.items() if m < 0 and live)
    dead = sorted(t for t, (m, _, live) in after.items() if not live and m >= 0 and m != before[t][0])
    if dead:
        fails.append(f"a nation that no longer exists got a manager: {', '.join(dead)}")
    if len(empty) > 2:
        fails.append(f"{len(empty)} senior national teams have no manager at the end: {', '.join(empty[:12])}"
                     + (" ..." if len(empty) > 12 else ""))
    replaced = sorted(t for t in before if before[t][0] >= 0 and after[t][0] >= 0 and after[t][0] != before[t][0]
                      and before[t][1] <= 150)
    left = sorted(t for t in before if before[t][0] >= 0 and after[t][0] != before[t][0] and before[t][1] <= 150)
    print(f"teams of 150 or fewer players whose manager left: {len(left)}, with a new manager: {len(replaced)}")
    if not replaced:
        fails.append("no team of 150 or fewer players got a new manager after its manager left")
    return fails


FIXTURES = "/Users/max/Projects/cm0102/test-fixtures"
# From the first news screen of start.sav (Arsenal, 14.7.01): the manager menu > Arsenal Squad > Finances & Info >
# View > Finances (dump), then the manager menu > Go on Holiday > Yes (the holiday steps of tests/smoke/smoke_run.py).
MONEY_STEPS = [
    "2500 click 45 165", "4500 click 150 156", "7000 click 720 97", "9500 click 172 135", "11000 click 160 200",
    "13000 dump {d}/finances.bmp",
    "14000 click 45 165", "16000 click 150 427 | 199 474", "18500 click 705 572",
]
BIDDER = "Liverpool"
BUDGET = "recomp: money budget: "
SHORTLIST = "0x84301b"          # Shortlist_Manager::try_add_player_to_club_shortlist 0x842e10, keeps eax
BID_A, BID_B = "0x8e6335", "0x8e74a7"   # TRANSFER_OFFER::build_bid 0x8e5d60 (eax, and 64 bits)


def money(argv):
    program, game = os.path.abspath(argv[0]), os.path.abspath(argv[1])
    keep = os.path.abspath(soak_run.arg(argv, "--keep"))
    balances = [int(b) for b in soak_run.arg(argv, "--balance", "2500000000,5000000000,12000000000,-2500000000").split(",")]
    days = int(soak_run.arg(argv, "--days", 45))
    os.environ["RECOMP_MONEY_LOG"] = "1"
    if "--keep-faults" in argv:
        os.environ["RECOMP_ORIGINAL_FAULTS"] = "keep"
    edit = os.path.join(HERE, "..", "..", "tools", "save", "edit_money.py")
    fails = []
    for balance in balances:
        work = os.path.join(keep, f"money-{balance}")
        os.makedirs(work, exist_ok=True)
        half, save = os.path.join(work, "half.sav"), os.path.join(work, "start.sav")
        for f in (half, save):
            if os.path.exists(f):
                os.remove(f)
        for src, dst, club in ((os.path.join(FIXTURES, "start.sav"), half, "Arsenal"), (half, save, BIDDER)):
            r = subprocess.run([sys.executable, edit, src, dst, "--club", club, "--balance", str(balance)],
                               capture_output=True, text=True)
            if r.returncode:
                return fails + [f"{balance}: edit_money.py: {r.stdout}{r.stderr}"]
        steps = os.path.join(work, "steps.txt")
        with open(steps, "w") as f:
            f.write("\n".join(MONEY_STEPS) + "\n")
        out = os.path.join(work, "out")
        code = fixtures.play(program, game, work, "start.sav", steps, out, "classic", {}, days)
        path = os.path.join(out, "stderr.txt")
        log = open(path, encoding="latin-1").read().split("\n") if os.path.exists(path) else []
        if code != 0:
            fails.append(f"{balance}: the run failed (code {code})")
        fails += [f"{balance}: {x.strip()}" for x in log if "recomp: message box from" in x]
        texts = [int(x.split(": ")[-1]) for x in log if x.startswith("recomp: money text: ")]
        if balance < 0:      # a debt: the club info tab shows the amount of the debt (0x463a0a); no budget check
            if -balance not in texts:
                fails.append(f"{balance}: the Finances view did not format the debt (money text: {texts[:4]})")
            continue
        if balance not in texts:
            fails.append(f"{balance}: the Finances view did not format the balance (money text: {texts[:4]})")
        if not [t for t in texts if t != balance and t > 0.99 * balance]:
            fails.append(f"{balance}: the Finances view did not format the transfer funds in full ({texts[:4]})")
        full, seen = {}, {}
        for x in log:
            if x.startswith(BUDGET + BIDDER + " from "):
                at, rest = x[len(BUDGET + BIDDER + " from "):].split(": ", 1)
                full.setdefault(at, []).append(int(rest.split(",")[0]))
                seen.setdefault(at, []).append(int(rest.split("the caller sees ")[1]))
        shortlist, bids = full.get(SHORTLIST, []), seen.get(BID_A, []) + seen.get(BID_B, [])
        print(f"{balance}: {BIDDER}: budget {min(shortlist) if shortlist else '-'} at {len(shortlist)} shortlist "
              f"checks, which see {min(seen.get(SHORTLIST, [0]))}; {len(bids)} bids")
        if not shortlist or min(shortlist) < 0.99 * balance:
            fails.append(f"{balance}: {BIDDER}'s budget is not near its balance ({min(shortlist) if shortlist else '-'})")
        elif min(seen[SHORTLIST]) != min(min(shortlist), 2**31 - 1):
            fails.append(f"{balance}: the shortlist sees {min(seen[SHORTLIST])}, not the clamped budget")
        if not bids or min(bids) <= 0:
            fails.append(f"{balance}: {BIDDER} made no bid with a positive budget ({len(bids)} bids)")
    return fails


HOLIDAY_STEPS = ["2500 click 45 165", "4500 click 150 427 | 199 474", "7000 click 705 572"]


def human_row(sav):
    """The staff row of the human manager of start.sav: the manager (club job 5, +0x3d) of Arsenal (+0x39)."""
    clubs, staff = sav.read_member("club.dat"), sav.read_member("staff.dat")
    arsenal = [struct.unpack_from("<i", clubs, r * CLUB_SIZE)[0] for r in range(len(clubs) // CLUB_SIZE)
               if clubs[r * CLUB_SIZE + 4:r * CLUB_SIZE + 0x37].split(b"\0")[0] == b"Arsenal"][0]
    rows = [r for r in range(len(staff) // STAFF_SIZE)
            if struct.unpack_from("<i", staff, r * STAFF_SIZE + 0x39)[0] == arsenal and staff[r * STAFF_SIZE + 0x3d] == 5]
    return rows[-1]


def age(argv):
    program, game = os.path.abspath(argv[0]), os.path.abspath(argv[1])
    keep = os.path.abspath(soak_run.arg(argv, "--keep"))
    born = int(soak_run.arg(argv, "--born", 1870))
    if "--keep-faults" in argv:
        os.environ["RECOMP_ORIGINAL_FAULTS"] = "keep"
    work = os.path.join(keep, f"age-{born}")
    os.makedirs(work, exist_ok=True)
    sav = savfile.Sav(os.path.join(FIXTURES, "start.sav"))
    row = human_row(sav)
    staff = bytearray(sav.read_member("staff.dat"))
    struct.pack_into("<h", staff, row * STAFF_SIZE + 0x12, born)
    sav.write_member("staff.dat", staff)
    save = os.path.join(work, "start.sav")
    if os.path.exists(save):
        os.remove(save)
    sav.save(save)
    steps = os.path.join(work, "steps.txt")
    with open(steps, "w") as f:
        f.write("\n".join(HOLIDAY_STEPS) + "\n")
    out = os.path.join(work, "out")
    code = fixtures.play(program, game, work, "start.sav", steps, out, "classic", {}, 3)
    if code != 0:
        return [f"the run failed (code {code})"]
    end = savfile.Sav(os.path.join(out, "samesave.sav")).read_member("staff.dat")
    got = struct.unpack_from("<h", end, row * STAFF_SIZE + 0x18)[0]
    real = 2001 - born
    print(f"the human manager, born {born}: {real} or {real - 1} on 17.7.01, the save holds {got}")
    if real - 1 <= 127:
        return [f"--born {born} gives an age below 128: nothing to check"]
    return [] if got == 127 else [f"the save holds the age {got}, not 127 (an age above 127 must not wrap)"]


def main(argv):
    if len(argv) < 4 or argv[1] not in ("managers", "money", "age") or "--keep" not in argv \
            or (argv[1] == "managers" and "--save" not in argv):
        print(__doc__)
        return 2
    fails = {"managers": managers, "money": money, "age": age}[argv[1]](argv[2:])

    for f in fails:
        print("FAIL", f)
    print("PASS" if not fails else f"FAIL: {len(fails)}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
