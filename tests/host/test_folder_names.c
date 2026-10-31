/* The kept folder names of the path lookup (src/recomp_rt/win/folder_names.c): each lookup gives what a read of
 * the folder gives, after the game or the player creates, renames or deletes a file, with names matched as
 * Windows does (case ignored, an exact match first). The folder's time is set into the past (utimes) so that
 * the kept names are used, and set again to show a change, as a later change of the folder does. */
#include "../../src/recomp_rt/win/folder_names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static int fails;
static char dir[512];

static void touch(const char *name)
{
    char p[1024];
    snprintf(p, sizeof p, "%s/%s", dir, name);
    FILE *f = fopen(p, "w");
    if (f) fclose(f);
}

/* The folder's time: when seconds ago, with ns nanoseconds. */
static void folder_time(long ago, long ns)
{
    struct timeval tv[2];
    tv[0].tv_sec = tv[1].tv_sec = time(NULL) - ago;
    tv[0].tv_usec = tv[1].tv_usec = (int)(ns / 1000);
    utimes(dir, tv);
}

static void expect(const char *what, const char *part, const char *want)
{
    char found[256];
    folder_lookup(dir, part, found, sizeof found);
    if (strcmp(found, want)) {
        printf("FAIL %s: %s gives \"%s\", expected \"%s\"\n", what, part, found, want);
        fails++;
    }
}

static void path(char *out, size_t room, const char *name) { snprintf(out, room, "%s/%s", dir, name); }

int main(void)
{
    snprintf(dir, sizeof dir, "%s/folder_names_test_XXXXXX", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
    if (!mkdtemp(dir)) { printf("FAIL mkdtemp\n"); return 1; }
    touch("Fixtures_2013.tmp");
    touch("samesave.sav");
    folder_time(100, 0);

    /* Case is ignored; the kept names serve the next lookups. */
    expect("first read", "FIXTURES_2013.TMP", "Fixtures_2013.tmp");
    expect("kept names", "fixtures_2013.tmp", "Fixtures_2013.tmp");
    expect("kept names, exact", "samesave.sav", "samesave.sav");
    expect("kept names, none", "missing.tmp", "");

    /* A file the player adds outside the game: the folder's time changes. */
    touch("Player.SAV");
    folder_time(50, 0);
    expect("created", "player.sav", "Player.SAV");

    /* Renamed and deleted. */
    char a[1024], b[1024];
    path(a, sizeof a, "Player.SAV");
    path(b, sizeof b, "Renamed.sav");
    rename(a, b);
    folder_time(40, 0);
    expect("renamed, old name", "player.sav", "");
    expect("renamed, new name", "RENAMED.SAV", "Renamed.sav");
    path(a, sizeof a, "Fixtures_2013.tmp");
    unlink(a);
    folder_time(30, 0);
    expect("deleted", "fixtures_2013.tmp", "");

    /* The same second, another nanosecond (a clock finer than a second). */
    folder_time(20, 0);
    expect("time set", "renamed.sav", "Renamed.sav");
    touch("Fine.tmp");
    folder_time(20, 5000);
    expect("same second, other ns", "fine.tmp", "Fine.tmp");

    /* A change in the same tick of a coarse clock: a recent folder is read at each lookup. */
    folder_time(0, 0);
    expect("recent", "fine.tmp", "Fine.tmp");
    touch("Coarse.tmp");
    folder_time(0, 0);
    expect("recent, same tick", "coarse.tmp", "Coarse.tmp");

    /* Two names that differ only in case, on a host folder that keeps both: the exact one wins. */
    struct stat st;
    path(a, sizeof a, "COARSE.TMP");
    if (stat(a, &st)) {                     /* the folder tells case apart */
        touch("coarse.TMP");
        folder_time(10, 0);
        expect("exact first", "coarse.TMP", "coarse.TMP");
        expect("exact first, other", "Coarse.tmp", "Coarse.tmp");
    } else
        printf("note: the folder ignores case, so two names that differ only in case are not tested\n");

    /* After a reset the folder is read again. */
    folder_time(5, 0);
    folder_names_reset();
    expect("after reset", "RENAMED.SAV", "Renamed.sav");

    char cmd[1100];
    snprintf(cmd, sizeof cmd, "rm -rf '%s'", dir);
    if (system(cmd)) printf("note: %s not removed\n", dir);
    printf(fails ? "FAILED: %d\n" : "PASS\n", fails);
    return fails != 0;
}
