/* The match days of the South American section of World Cup qualifying in a season game (open item 163 of 0.6.2).
 *
 * The cause. WcSouthAmericanLeague::create_schedule 0x920590 gives the original's template of the 2002 cycle (18
 * rounds from March 2000 to November 2001, moved to each cycle), and end_by_october (season_intl_qual.c) only moves
 * its end. Each fixture then moves on its own to the nearest free FIFA day of its year (season_friendly_dates.c
 * fifa_target_by). The template puts 10 rounds in the year two years before the finals, from February, and that
 * year's FIFA days within the stage hold 9: in run set game a2002s3 the whole matchday of 1 August 2012 kept its
 * day (Argentina v Paraguay among them), and the Copa Sudamericana of 31 July (item 95 keeps it off the FIFA days
 * only) met it with three of Huracan's players on duty (get_international_duty_club 0x7757d0: the day before to the
 * day after). The 2098 cycle of a2006s3 did the same on 15 August 2096, and 2080 and 2084 kept November days.
 *
 * Real life. Every cycle from 2006 played its 18 matchdays in the FIFA windows, two a window, from September or
 * October three years before the finals to September or October the year before (the dates in span below). So in a
 * season game the rounds take the FIFA days of the span: in each FIFA window (FIFA days at most WINDOW_GAP apart)
 * its first day and the next at least MIN_GAP days later, the windows with two such days first, then the other
 * days (a third day of a window, a window of one day), earliest first. A day in a South American final's days is
 * left out. When the span holds too few, its first day moves back a week at a time (inference: the 2022 cycle,
 * which the game ends in October 2021, has no FIFA days from March to September 2020 in the list). */
#include "season_conmebol_days.h"
#include "season_uefa_days.h"

#include <stdlib.h>

#define WINDOW_GAP 4                    /* FIFA days of one window: Thursday to Tuesday, at most 4 days apart */
#define MIN_GAP 3                       /* two rounds of a window: Thursday and Sunday, Friday and Tuesday */
#define SLOTS_MAX 256
#define BACK_DAYS 400                   /* how far the first day may move back */

static const struct {
    int wc, y0, m0, d0, y1, m1, d1;
} spans[] = {
    /* matchday 1 and matchday 18 (en.wikipedia.org, "<year> FIFA World Cup qualification (CONMEBOL)") */
    {2006, 2003, 9, 6, 2005, 10, 12},
    {2010, 2007, 10, 13, 2009, 10, 14},
    {2014, 2011, 10, 7, 2013, 10, 15},
    {2018, 2015, 10, 8, 2017, 10, 10},
    /* 2022: matchday 1 on 8 October 2020 (moved from March 2020); the game ends the section by 10 October 2021 for
     * its tie in November 2021 (season_intl_qual.c real_end), the real end was 29 March 2022 */
    {2022, 2020, 10, 8, 2021, 10, 10},
    {2026, 2023, 9, 7, 2025, 9, 9},
};

void season_conmebol_span(int wc, long *first, long *last)
{
    for (size_t i = 0; i < sizeof spans / sizeof spans[0]; i++)
        if (spans[i].wc == wc) {
            *first = season_abs_day(spans[i].y0, spans[i].m0, spans[i].d0);
            *last = season_abs_day(spans[i].y1, spans[i].m1, spans[i].d1);
            return;
        }
    *first = season_abs_day(wc - 3, 9, 1);
    *last = season_abs_day(wc - 1, 10, 15);
}

static int by_day(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;
    return x < y ? -1 : x > y;
}

/* The FIFA days from first to last, in order. */
static int fifa_days(long first, long last, long *out, int max)
{
    int n = 0, y0, y1, m, d;
    season_from_abs_day(first, &y0, &m, &d);
    season_from_abs_day(last, &y1, &m, &d);
    for (int y = y0; y <= y1; y++) {
        int doys[SEASON_FIFA_YEAR_MAX], k = season_fifa_year_days(y, doys, SEASON_FIFA_YEAR_MAX);
        long jan1 = season_abs_day(y, 1, 1);
        for (int i = 0; i < k && n < max; i++)
            if (jan1 + doys[i] >= first && jan1 + doys[i] <= last) out[n++] = jan1 + doys[i];
    }
    qsort(out, (size_t)n, sizeof *out, by_day);
    return n;
}

static int pick(long first, long last, long earliest, SeasonConmebolBusy busy, int rounds, long out[])
{
    long days[SLOTS_MAX], best[SLOTS_MAX], rest[SLOTS_MAX];
    int n = fifa_days(first, last, days, SLOTS_MAX), nb = 0, nr = 0;
    for (int i = 0; i < n;) {
        int j = i + 1;
        while (j < n && days[j] - days[j - 1] <= WINDOW_GAP) j++;          /* days[i..j-1]: one window */
        long taken[8];
        int nt = 0;
        for (int k = i; k < j && nt < 8; k++)
            if (days[k] >= earliest && !(busy && busy(days[k])) && (!nt || days[k] - taken[nt - 1] >= MIN_GAP))
                taken[nt++] = days[k];
        for (int k = 0; k < nt; k++) {
            if (nt >= 2 && k < 2) best[nb++] = taken[k];
            else rest[nr++] = taken[k];
        }
        i = j;
    }
    if (nb + nr < rounds) return 0;
    int m = 0;
    for (int k = 0; k < nb && m < rounds; k++) out[m++] = best[k];
    for (int k = 0; k < nr && m < rounds; k++) out[m++] = rest[k];
    qsort(out, (size_t)m, sizeof *out, by_day);
    return m;
}

int season_conmebol_days(int wc, int rounds, long earliest, SeasonConmebolBusy busy, long out[])
{
    long first, last;
    if (rounds <= 0 || rounds > SLOTS_MAX) return 0;
    season_conmebol_span(wc, &first, &last);
    for (long back = 0; back <= BACK_DAYS; back += 7) {
        if (back && first - back + 7 < earliest) break;                   /* nothing new before the earliest day */
        if (pick(first - back, last, earliest, busy, rounds, out) == rounds) return rounds;
    }
    return 0;
}
