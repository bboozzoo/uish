%{
#include <math.h>
#include <assert.h>
#include "scan.h"
#include "uish.h"
#include "debug.h"

static int line_nr = 1;
static int nesting_level = 0;
static scan_res_t scan_res = SCAN_OK;

#define FREE_LAST_COMM \
    do { \
        if (NULL != last_comm) \
            uish_cmd_free(last_comm); \
    } while(0)

#define SET_SCAN_ERR_RETURN \
    do { \
        scan_res = SCAN_ERR; \
        return -1; \
    } while(0)

#define CHECK_LAST_COMM \
    do {                            \
        if (NULL ==  (last_comm)) { \
            scan_res = SCAN_ERR;    \
            return -1;              \
        }                           \
    } while (0)

%}

COMMAND  command
SECT_START \{
SECT_END   \}
SEMICOLON ;
STRINGDELIM \"
COMMAND_NAME [a-zA-Z]+

%%

{SECT_START} {
    nesting_level++;
    DBG(0, "starting section, nesting: %u\n", nesting_level);
}

{SECT_END} { 
    nesting_level--;
    assert(nesting_level >= 0);
    DBG(0,"ending section, nesting: %u\n", nesting_level);
}

{COMMAND} {
    DBG(0, "command token\n");
}

{COMMAND_NAME} {
    DBG(0, "\tname: %s\n", yytext);
}

{SEMICOLON} {
    DBG(0, "semicolon\n");
}

{STRINGDELIM}.*{STRINGDELIM} {
    int i = 0;
    char * str = strdup(yytext + 1);
    if (NULL == str) {
        SET_SCAN_ERR_RETURN;
    }
    for (; i < strlen(str); i++) {
        if ('\"' == str[i])
            str[i] = 0;
    }
    DBG(0, "\tstring: %s\n", yytext);
    free(str);
}

#.*    /* eat up one-line comments */

[ \t]+          /* eat up whitespace */

\n {
    line_nr++;
}

. {
    DBG(1,"error in line: %d\n", line_nr );
    SET_SCAN_ERR_RETURN;
}

%%
int yywrap() {
    return 1;
}

void display_tree(struct uish_s * uish) {
    struct uish_comm_s * iter;
    for (iter = uish->commands.tqh_first; iter != NULL; iter = iter->list_el.tqe_next) {
        DBG(0, "%s\n", iter->name);
    }
}

scan_res_t lexscan(FILE * src, struct uish_s * uish) {
    scan_res = SCAN_OK;
    yyin = src;
    yylex();
    yylex_destroy();
    return scan_res;
}

