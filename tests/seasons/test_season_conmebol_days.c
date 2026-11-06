/* The days of the South American section of World Cup qualifying (open item 163 of 0.6.2, season_conmebol_days.c).
 * Every cycle from 2006 to 2302 with 14, 16 and 18 rounds: each round on a FIFA day, at least 3 days after the one
 * before, in the span (or the weeks before it), after the day the section is made; with a finals stand-in that holds
 * every 10 June to 20 July, none in it. The 2014 cycle has the real windows and no round on 1 August 2012 (run set
 * a2002s3), the 2098 cycle none in August 2096 (a2006s3). */
#include "season_conmebol_days.h"
#include "season_uefa_days.h"

#include <stdio.h>
#include <stdlib.h>

static int fails;

#define CHECK(c, ...)                                                                                                 \
    do {                                                                                                              \
        if (!(c)) {                                                                                                   \
            fails++;                                                                                                  \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                               \
            printf(__VA_ARGS__);                                                                                      \
            printf("\n");                                                                                             \
        }                                                                                                             \
    } while (0)

static int is_fifa_day(long z)
{
    int y, m, d, doys[SEASON_FIFA_YEAR_MAX];
    season_from_abs_day(z, &y, &m, &d);
    int n = season_fifa_year_days(y, doys, SEASON_FIFA_YEAR_MAX), doy = season_day_of_year(y, m, d);
    for (int i = 0; i < n; i++)
        if (doys[i] == doy) return 1;
    return 0;
}

static int summer(long z)                       /* a stand-in for the Copa America: 10 June to 20 July */
{
    int y, m, d;
    season_from_abs_day(z, &y, &m, &d);
    return (m == 6 && d >= 10) || (m == 7 && d <= 20);
}

static void cycle(int wc, int rounds, SeasonConmebolBusy busy)
{
    long out[64], first, last;
    long made = season_abs_day(wc - 4, 7, 20);          /* the section is made after the finals before it */
    season_conmebol_span(wc, &first, &last);
    int n = season_conmebol_days(wc, rounds, made + 2, busy, out);
    CHECK(n == rounds, "%d, %d rounds%s: %d placed", wc, rounds, busy ? " (finals)" : "", n);
    for (int i = 0; i < n; i++) {
        int y, m, d;
        season_from_abs_day(out[i], &y, &m, &d);
        CHECK(is_fifa_day(out[i]), "%d round %d on %04d-%02d-%02d: not a FIFA day", wc, i + 1, y, m, d);
        CHECK(out[i] <= last && out[i] >= made + 2, "%d round %d on %04d-%02d-%02d: out of the span", wc, i + 1, y, m,
              d);
        CHECK(!i || out[i] - out[i - 1] >= 3, "%d round %d on %04d-%02d-%02d: %ld days after the one before", wc,
              i + 1, y, m, d, i ? out[i] - out[i - 1] : 0);
        CHECK(!busy || !busy(out[i]), "%d round %d on %04d-%02d-%02d: in a final's days", wc, i + 1, y, m, d);
    }
}

int main(void)
{
    for (int wc = 2006; wc <= 2302; wc += 4)
        for (int r = 14; r <= 18; r += 2) {
            cycle(wc, r, NULL);
            cycle(wc, r, summer);
        }

    /* 2014 (a2002s3): the real windows of October 2011 to October 2013; the list has two single days in their
     * place of June 2012 (Euro 2012: 9 and 10 June are not in it). */
    static const int want[18][3] = {{2011, 10, 7}, {2011, 10, 11}, {2011, 11, 11}, {2011, 11, 15}, {2012, 2, 29},
                                    {2012, 5, 26}, {2012, 9, 7},   {2012, 9, 11},  {2012, 10, 12}, {2012, 10, 16},
                                    {2013, 3, 22}, {2013, 3, 26},  {2013, 6, 7},   {2013, 6, 11},  {2013, 9, 6},
                                    {2013, 9, 10}, {2013, 10, 11}, {2013, 10, 15}};
    long out[64];
    int n = season_conmebol_days(2014, 18, season_abs_day(2010, 7, 27), NULL, out);
    CHECK(n == 18, "2014: %d placed", n);
    for (int i = 0; i < n; i++) {
        int y, m, d;
        season_from_abs_day(out[i], &y, &m, &d);
        CHECK(y == want[i][0] && m == want[i][1] && d == want[i][2], "2014 round %d on %04d-%02d-%02d, want %04d-%02d-%02d",
              i + 1, y, m, d, want[i][0], want[i][1], want[i][2]);
        CHECK(out[i] != season_abs_day(2012, 8, 1), "2014: a round on 1 August 2012");
    }

    /* 2098 (a2006s3): no round in July or August. */
    n = season_conmebol_days(2098, 18, season_abs_day(2094, 7, 20), NULL, out);
    for (int i = 0; i < n; i++) {
        int y, m, d;
        season_from_abs_day(out[i], &y, &m, &d);
        CHECK(m != 7 && m != 8, "2098 round %d on %04d-%02d-%02d", i + 1, y, m, d);
    }

    /* A section made after its span began places nothing before the day it is made. */
    n = season_conmebol_days(2014, 18, season_abs_day(2012, 1, 1), NULL, out);
    CHECK(n == 0 || out[0] >= season_abs_day(2012, 1, 1), "2014 made in 2012: a round before it");

    if (fails) {
        printf("%d failures\n", fails);
        return 1;
    }
    printf("season_conmebol_days: all checks passed\n");
    return 0;
}
