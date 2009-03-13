#include <stdio.h>
#include <stdarg.h>
#include "debug.h"
/* 
 * debug level, msg of lvl < than level will be
 * logged
 */
static int level = 0;
/*
 * verbose - 1, not - 0
 */
static int verbose = 0;
/*
 * destination
 */
static FILE * dest = NULL;

void dbg_init(FILE * dst, int lvl, int vrbose) {
    if (NULL != dst)
        dest = dst;
    else
        dest = stderr;
    if (0 <= lvl)
        level = lvl;
    if (0 != vrbose)
        verbose = 1;
}

void dbg_log(int lvl, const char * fcn, const char * fil, unsigned int line, const char * format, ...) {
    va_list l;
    if (lvl > level)
        return;
    if (1 == verbose &&
        NULL != fcn &&
        NULL != fil &&
        0 != line) {
        fprintf(dest, "%10s:%-4d%%%s: ", fil, line, fcn);
    }

    va_start(l, format);
    vfprintf(dest, format, l);
    va_end(l);
}

