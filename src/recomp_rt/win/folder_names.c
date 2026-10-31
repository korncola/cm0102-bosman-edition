#include "folder_names.h"
#include "../host.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/* The spelling of a name in a folder, ignoring case, for the path lookup of files.c (ci_lookup).
 *
 * Each file the game opens looks up every part of its path, and a read of the whole folder took 19 % of a
 * Tier A year (the profile of 0.6.2 slice 3: the user folder holds hundreds of part files). So the names of
 * a folder are kept with the folder's modification time, and a lookup reads the folder again when that time
 * changes. Creating, deleting or renaming an entry changes it, by the game or by the player outside the
 * game. A folder whose time lies within RECENT seconds of now is read again at each lookup, so a change in
 * the same tick of a coarse clock (1 s on some file systems) is not missed. The result is the result of
 * the read. RECOMP_FOLDER_CACHE=0 reads the folder at each lookup, as before. */
#define CACHE_FOLDERS 64
#define RECENT 2

typedef struct {
    char *dir;
    char **names;
    int count;
    int64_t mtime, mtime_ns;
    unsigned long used;
} FolderNames;

static FolderNames folders[CACHE_FOLDERS];
static unsigned long folder_clock;

static int cache_on(void)
{
    static int on = -1;
    if (on < 0) {
        const char *v = getenv("RECOMP_FOLDER_CACHE");
        on = !(v && !strcmp(v, "0"));
    }
    return on;
}

static void folder_free(FolderNames *f)
{
    for (int i = 0; i < f->count; i++) free(f->names[i]);
    free(f->names);
    free(f->dir);
    memset(f, 0, sizeof *f);
}

/* Reads the folder into f; 0 when it cannot be read. */
static int folder_read(FolderNames *f, const char *dir, const HostStat *st)
{
    HostDir *d = host_opendir(dir);
    if (!d) return 0;
    int room = 0, ok = 1;
    const char *e;
    folder_free(f);
    f->dir = strdup(dir);
    while (ok && (e = host_readdir(d))) {
        if (f->count == room) {
            room = room ? 2 * room : 64;
            char **p = realloc(f->names, (size_t)room * sizeof *p);
            if (!p) { ok = 0; break; }
            f->names = p;
        }
        if (!(f->names[f->count] = strdup(e))) ok = 0;
        else f->count++;
    }
    host_closedir(d);
    f->mtime = st->mtime;
    f->mtime_ns = st->mtime_ns;
    return ok && f->dir != NULL;            /* a list cut short is not kept: the caller frees it */
}

static void ci_scan(const char *dir, const char *part, char *found, size_t room)
{
    HostDir *d = host_opendir(dir);
    if (!d) return;
    const char *e;
    while ((e = host_readdir(d))) {
        if (!strcmp(e, part)) { snprintf(found, room, "%s", e); break; }
        if (!found[0] && !strcasecmp(e, part)) snprintf(found, room, "%s", e);
    }
    host_closedir(d);
}

void folder_lookup(const char *path, const char *part, char *found, size_t room)
{
    found[0] = 0;
    HostStat st;
    if (!cache_on() || host_stat(path, &st) || !st.is_dir || st.mtime >= (int64_t)time(NULL) - RECENT) {
        ci_scan(path, part, found, room);
        return;
    }
    FolderNames *f = NULL, *oldest = &folders[0];
    for (int i = 0; i < CACHE_FOLDERS; i++) {
        if (folders[i].dir && !strcmp(folders[i].dir, path)) { f = &folders[i]; break; }
        if (folders[i].used < oldest->used) oldest = &folders[i];
    }
    if (!f || f->mtime != st.mtime || f->mtime_ns != st.mtime_ns) {
        if (!f) f = oldest;
        if (!folder_read(f, path, &st)) {
            folder_free(f);
            ci_scan(path, part, found, room);
            return;
        }
    }
    f->used = ++folder_clock;
    for (int i = 0; i < f->count; i++) {
        if (!strcmp(f->names[i], part)) { snprintf(found, room, "%s", f->names[i]); return; }
        if (!found[0] && !strcasecmp(f->names[i], part)) snprintf(found, room, "%s", f->names[i]);
    }
}

void folder_names_reset(void)
{
    for (int i = 0; i < CACHE_FOLDERS; i++) folder_free(&folders[i]);
    folder_clock = 0;
}
