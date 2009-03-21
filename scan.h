#ifndef __SCAN_H__
#define __SCAN_H__ 
#include <stdio.h>
#include "uish.h"

typedef enum {
    SCAN_OK,
    SCAN_ERR,
} scan_res_t;

scan_res_t lexscan(FILE * src, struct uish_s * uish);

#endif /* __SCAN_H__ */
