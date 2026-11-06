"""Item 153 (0.6.2): which same-save runs smoke_run.py compares when --custom-db plays a season game.

season-code-off and kinds-base are defined for the base game only (start 2001, changing rules off; BASE_GAME_RUNS
in smoke_run.py). With --custom-db and a normal game in another season, the check must leave them out with a
reason; in every other case it must compare them as before. The saves here are small containers of version 1
(tools/save/savfile.py) with or without the part bosman_season.dat (season.c).

Run: python3 -I tests/smoke/base_game_runs_check.py (ctest same_save_base_game)."""
import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import smoke_run  # noqa: E402

ENTRY_SIZE = 0x10C


def write_save(path, parts):
    """A save of version 1 (parts stored as they are) with the parts [(name, bytes)]."""
    head = struct.pack("<3i", 1, 0, len(parts))
    offset = 12 + ENTRY_SIZE * len(parts)
    rows, body = b"", b""
    for name, data in parts:
        rows += struct.pack("<II", offset + len(body), len(data)) + name.encode("latin-1").ljust(ENTRY_SIZE - 8, b"\0")
        body += data
    with open(path, "wb") as f:
        f.write(head + rows + body)


def season_part(year, rules):
    return struct.pack("<4I", 1, 16, year, rules)


def main():
    failed = 0

    def expect(label, got, want_reason):
        nonlocal failed
        ok = (got is not None) == want_reason
        print(f"{'ok  ' if ok else 'FAIL'}  {label}: {got or 'compared'}")
        failed += not ok

    with tempfile.TemporaryDirectory(prefix="cm0102-base-game-") as work:
        saves = {}
        for key, parts in {
            "base": [("general.dat", b"x" * 40)],
            "base-part": [("general.dat", b"x" * 40), ("bosman_season.dat", season_part(2001, 0))],
            "2025": [("general.dat", b"x" * 40), ("bosman_season.dat", season_part(2025, 0))],
            "rules-on": [("general.dat", b"x" * 40), ("bosman_season.dat", season_part(2001, 1))],
        }.items():
            saves[key] = os.path.join(work, key + ".sav")
            write_save(saves[key], parts)

        expect("season of a save without the part", None if smoke_run.season_of_save(saves["base"]) == (2001, 0)
               else "wrong", False)
        expect("season of a 2025 save", None if smoke_run.season_of_save(saves["2025"]) == (2025, 0) else "wrong",
               False)

        smoke_run.CUSTOM_DB = os.path.join(work, "December 2025")
        for run in smoke_run.BASE_GAME_RUNS:
            expect(f"custom, base game, {run}", smoke_run.base_game_only(run, saves["base"]), False)
            expect(f"custom, base game with a part, {run}", smoke_run.base_game_only(run, saves["base-part"]), False)
            expect(f"custom, 2025/26 game, {run}", smoke_run.base_game_only(run, saves["2025"]), True)
            expect(f"custom, changing rules on, {run}", smoke_run.base_game_only(run, saves["rules-on"]), True)
        for run in ("host-loops-off", "files-off", "again", "ref-build"):
            expect(f"custom, 2025/26 game, {run}", smoke_run.base_game_only(run, saves["2025"]), False)

        smoke_run.CUSTOM_DB = None
        for run in smoke_run.BASE_GAME_RUNS:
            expect(f"no custom data set, 2025/26 game, {run}", smoke_run.base_game_only(run, saves["2025"]), False)

    print("PASS" if not failed else "FAIL")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
