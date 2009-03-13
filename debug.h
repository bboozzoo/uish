#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <stdio.h>

/**
 * @brief initialise debug
 */
void dbg_init(FILE * dest, int lvl, int verbose);
/**
 * @brief
 * log something to debug 
 * not to be used directly
 */
void dbg_log(int lvl, const char * fcn, const char * fil, unsigned int line, const char * format, ...);
/**
 * @brief macro to be used for writing something to debug
 */
#define DBG(lvl, ...) \
        do { \
            dbg_log(lvl, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__); \
        } while(0)

#endif /* __DEBUG_H__ */

