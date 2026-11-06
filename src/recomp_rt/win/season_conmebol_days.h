/* The match days of the South American section of World Cup qualifying in a season game (open item 163 of 0.6.2).
 * Pure code (season_conmebol_days.c), so ctest season_conmebol_days links it with the FIFA days. */
#ifndef RECOMP_SEASON_CONMEBOL_DAYS_H
#define RECOMP_SEASON_CONMEBOL_DAYS_H

/* 1 when day z (days from 1 January 1970) lies in the days of a South American finals tournament
 * (season_intl_finals_busy for the confederation), so no round takes it. NULL: none does. */
typedef int (*SeasonConmebolBusy)(long z);

/* The first and last day (days from 1 January 1970) in which the section of World Cup year wc plays its rounds:
 * the real first and last matchday of the cycles 2006 to 2026, and after them from 1 September three years before
 * the finals to 15 October the year before. */
void season_conmebol_span(int wc, long *first, long *last);

/* The days of the `rounds` rounds (in order) of the section of World Cup year wc, as days from 1 January 1970, in
 * out[]. Each is a FIFA day (season_fifa_year_days) in the span, two rounds a FIFA window at least 3 days apart,
 * none earlier than `earliest` and none that busy() holds. Returns rounds, or 0 when the days do not hold them
 * (out[] is then not used). */
int season_conmebol_days(int wc, int rounds, long earliest, SeasonConmebolBusy busy, long out[]);

#endif
