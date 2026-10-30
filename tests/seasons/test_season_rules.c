/* Test of the rules table reader (0.6.0 slice 12, src/recomp_rt/win/season_rules.c).
 *
 * Usage: season_rules_test <rules.tsv made by tools/seasons/make_rules.py> <scratch folder>
 * ctest makes the table first (test season_rules_table). The checks: the row count against the count line
 * and the lines of the file, the values of known rows, the lookup by season and file (also for a row with
 * two files), a file with Windows line ends, the limits of a season and a count, and that a damaged or
 * missing file gives an empty table. */
#include "season_rules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static void write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(2); }
    fputs(text, f);
    fclose(f);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: season_rules_test <rules.tsv> <scratch folder>\n");
        return 2;
    }
    /* The row count of the file: line 1 says it, and the file has 2 lines more. */
    int want = -1, lines = 0, c;
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    if (fscanf(f, "#bosman-rules\t1\t%d", &want) != 1) want = -1;
    rewind(f);
    while ((c = fgetc(f)) != EOF) lines += c == '\n';
    fclose(f);
    int n = season_rules_load(argv[1]);
    CHECK(n > 0 && n == want && n == lines - 2, "row count %d, line 1 says %d, %d lines", n, want, lines);
    CHECK(season_rules_count() == n, "count");

    const SeasonRule *r = season_rules_find(2001, "eng_prm", 0);
    CHECK(r && !strcmp(r->id, "eng-28"), "2001 eng_prm is %s", r ? r->id : "none");
    if (r) {
        CHECK(r->season == 2001 && r->tier == 1 && !strcmp(r->tier_code, "1"), "eng-28 season and tier");
        CHECK(r->clubs == 20 && r->up == SEASON_RULE_NA && r->down == 3, "eng-28 clubs %d up %d down %d",
              r->clubs, r->up, r->down);
        CHECK(r->subs_used == 3 && r->bench == 5, "eng-28 subs %d bench %d", r->subs_used, r->bench);
        CHECK(!strcmp(r->name, "FA Premier League") && !strcmp(r->nation, "England"), "eng-28 name");
        CHECK(!strcmp(r->clubs_text, "20") && *r->sources, "eng-28 texts");
    }
    CHECK(!season_rules_find(2001, "eng_prm", 1), "one eng_prm row in 2001");
    r = season_rules_find(2027, "eng_prm", 0);
    CHECK(r && r->season == 2027, "a row of 2027/28");

    /* Slice 24: a row of a new competition (comp_file "new") is found by the file of its def. */
    r = season_rules_find(2021, "new_conference_league", 0);
    CHECK(r && !strcmp(r->comp_file, "new") && !strcmp(r->comp, "Conference League") && r->subs_used == 5,
          "2021 new_conference_league is %s", r ? r->id : "none");
    CHECK(!season_rules_find(2021, "new_conference", 0), "no part match");

    /* A row with two files is found by each of them. */
    int both = 0;
    for (int k = 0; (r = season_rules_find(2001, "wc_oceania_league", k)); k++)
        if (strchr(r->comp_file, ';')) both++;
    CHECK(both == 1, "rows of 2001 with wc_oceania_league and another file: %d", both);

    /* 0.6.1: the lookup by an index gives the rows of the plain scan, for each file of each row (and the
     * file of a new competition), each season and each count, and none for other files. */
    int keys = 0, wrong = 0;
    for (int i = 0; i < season_rules_count(); i++) {
        const SeasonRule *row = season_rules_row(i);
        char files[512], newname[160];
        snprintf(files, sizeof files, "%s", row->comp_file);
        snprintf(newname, sizeof newname, "new_");
        for (const char *c = row->comp ? row->comp : ""; *c && strlen(newname) < sizeof newname - 1; c++) {
            char ch = *c == ' ' ? '_' : (*c >= 'A' && *c <= 'Z') ? (char)(*c - 'A' + 'a') : *c;
            strncat(newname, &ch, 1);
        }
        char *names[64];
        int nn = 0;
        for (char *f = strtok(files, ";"); f && nn < 62; f = strtok(NULL, ";")) names[nn++] = f;
        names[nn++] = newname;
        names[nn++] = "no_such_file";
        for (int k = 0; k < nn; k++)
            for (int season = 1999; season <= 2032; season++)
                for (int n = 0; n < 6; n++) {
                    keys++;
                    if (season_rules_find(season, names[k], n) != season_rules_find_scan(season, names[k], n)) wrong++;
                }
    }
    CHECK(keys > 1000 && !wrong, "index lookups: %d of %d differ from the scan", wrong, keys);

    /* Codes. */
    r = season_rules_find(2001, "uefa_cup", 0);
    CHECK(r && r->clubs == 145 && r->tier == 2, "uefa_cup 2001");
    int not_held = 0, text = 0;
    for (int i = 0; i < season_rules_count(); i++) {
        const SeasonRule *x = season_rules_row(i);
        not_held += x->clubs == SEASON_RULE_NOT_HELD;
        text += x->clubs == SEASON_RULE_TEXT;
    }
    CHECK(not_held > 0 && text > 0, "codes not_held %d text %d", not_held, text);
    CHECK(!season_rules_row(-1) && !season_rules_row(n), "rows out of range");

    /* The same file with Windows line ends reads the same rows, with no '\r' in the last field. */
    char path[1024];
    snprintf(path, sizeof path, "%s/crlf.tsv", argv[2]);
    FILE *in = fopen(argv[1], "rb"), *out = fopen(path, "wb");
    if (!in || !out) { perror(path); return 2; }
    while ((c = fgetc(in)) != EOF) {
        if (c == '\n') fputc('\r', out);
        fputc(c, out);
    }
    fclose(in);
    fclose(out);
    CHECK(season_rules_load(path) == n, "a file with Windows line ends");
    r = season_rules_find(2001, "eng_prm", 0);
    CHECK(r && !strchr(r->sources, '\r'), "no CR in the last field");

    /* A season and a count outside the limits stop the reader. The rows are made from the first row. */
    snprintf(path, sizeof path, "%s/limits.tsv", argv[2]);
    static char cols[8192], row[65536], bad[70000];
    FILE *src = fopen(argv[1], "rb");
    if (!src || !fgets(cols, sizeof cols, src) || !fgets(cols, sizeof cols, src) || !fgets(row, sizeof row, src))
        return 2;
    fclose(src);
    char *tab = strchr(row, '\t');                    /* after the id: the season */
    snprintf(bad, sizeof bad, "#bosman-rules\t1\t1\n%sx-1\t2100%s", cols, strchr(tab + 1, '\t'));
    write_file(path, bad);
    CHECK(season_rules_load(path) == 0, "a season of 2100");
    char *clubs = tab;                                 /* the 8th field: clubs */
    for (int k = 0; k < 6 && clubs; k++) clubs = strchr(clubs + 1, '\t');
    char *after = clubs ? strchr(clubs + 1, '\t') : NULL;
    CHECK(after != NULL, "the first row has a clubs field");
    if (after) {
        snprintf(bad, sizeof bad, "#bosman-rules\t1\t1\n%s%.*s100001%s", cols, (int)(clubs + 1 - row), row, after);
        write_file(path, bad);
        CHECK(season_rules_load(path) == 0, "a count of 100001");
        snprintf(bad, sizeof bad, "#bosman-rules\t1\t1\n%s%.*s100000%s", cols, (int)(clubs + 1 - row), row, after);
        write_file(path, bad);
        CHECK(season_rules_load(path) == 1 && season_rules_row(0)->clubs == 100000, "a count of 100000");
    }

    /* A damaged file and a missing file give an empty table. */
    snprintf(path, sizeof path, "%s/damaged.tsv", argv[2]);
    write_file(path, "#bosman-rules\t1\t1\nid\tseason\n");
    CHECK(season_rules_load(path) == 0 && season_rules_count() == 0, "a damaged header");
    write_file(path, "#bosman-rules\t2\t0\n");
    CHECK(season_rules_load(path) == 0, "a later format");
    snprintf(path, sizeof path, "%s/missing.tsv", argv[2]);
    remove(path);
    CHECK(season_rules_load(path) == 0 && !season_rules_row(0), "a missing file");

    if (failures) fprintf(stderr, "%d failures\n", failures);
    else printf("season_rules_test: all checks passed\n");
    return failures ? 1 : 0;
}
