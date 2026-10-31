#ifndef RECOMP_FOLDER_NAMES_H
#define RECOMP_FOLDER_NAMES_H

#include <stddef.h>

/* found = the spelling of part in the folder path, an exact match first, else one that differs only in case;
 * empty when there is none or the folder cannot be read (folder_names.c). */
void folder_lookup(const char *path, const char *part, char *found, size_t room);

/* Forgets every kept folder (tests). */
void folder_names_reset(void);

#endif
