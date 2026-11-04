#!/usr/bin/env python3
"""ctest of the database of a rules report run, with no game (0.6.2 open item 129).

Usage:
    python3 -I tests/seasons/rules_report_database_check.py

The expected files of a start year Y (rules_report_expected/Y-...) are written on the season database of Y, the
folder whose name starts with SEASON_DATABASES[Y] (November 2007 for 2007). A run with --year Y and no --database
played the base data (3.9.68) and compared it with those files: the Brazilian state championships of a 2007 start
then held 12, 5 and 5 clubs in place of 13, 6 and 6 (batch P, rr2007-a). run_database() must give the season
database of the year, keep an explicit --database, keep the base data for 2001 and for the years of the own
databases, and stop when a year has no folder.
Exit code 0 when every case passes.
"""
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import rules_report_check as rrc  # noqa: E402


def main():
    fails = []

    def expect(name, got, want):
        if got != want:
            fails.append(f"{name}: got {got!r}, want {want!r}")

    with tempfile.TemporaryDirectory() as root:
        def folder(*parts, index=True):
            path = os.path.join(root, *parts)
            os.makedirs(path)
            if index:
                open(os.path.join(path, "index.dat"), "wb").close()
            return path

        nov2007 = folder("November 2007")
        sep2010 = folder("September 2010 ODB")
        folder("October 2026", index=False)
        oct2026 = folder("October 2026", "October 2026 Data Update")
        mine = folder("Mine")
        try:
            expect("2007", rrc.run_database(2007, None, root), nov2007)
            expect("2010 (name with a suffix)", rrc.run_database(2010, None, root), sep2010)
            expect("2026 (index.dat in a subfolder)", rrc.run_database(2026, None, root), oct2026)
            expect("--database wins", rrc.run_database(2007, mine, root), mine)
            expect("2001 base data", rrc.run_database(2001, None, root), None)
            expect("no --year", rrc.run_database(None, None, root), None)
            for y in sorted(rrc.OWN_DATABASES):
                expect(f"{y} own database year", rrc.run_database(y, None, root), None)
        except AttributeError as e:
            fails.append(f"rules_report_check.py has no {e.name}: a run with --year and no --database plays the "
                         f"base data")
        if not fails:
            try:
                rrc.run_database(2008, None, root)
                fails.append("2008 with no September 2008 folder: no error")
            except SystemExit as e:
                expect("2008 error names the folder", "September 2008" in str(e), True)
        for y in range(2002, 2027):
            if not fails and y not in rrc.SEASON_DATABASES and y not in rrc.OWN_DATABASES:
                fails.append(f"{y}: in neither table")

    for f in fails:
        print("FAIL  " + f)
    print("PASS" if not fails else f"FAILED: {len(fails)}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
