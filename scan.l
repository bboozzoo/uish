%{
#include <math.h>
#include "scan.h"
#include "uish.h"
#include "debug.h"

static int line_nr = 1;
static scan_res_t scan_res = SCAN_OK;
static struct uish_s * uishp = NULL;
static struct uish_comm_s * last_comm = NULL;
static int in_command = 0;

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
                DBG(0, "starting section\n");
            }

{SECT_END}  { 
                DBG(0,"ending section\n");
                in_command = 0;
            }

{COMMAND}     {
                DBG(0, "command %s\n", yytext);
                in_command = 1;
            }

{COMMAND_NAME} {
                if (1 == in_command) {
                    DBG(0, "text: %s\n", yytext);
                    last_comm = uish_cmd_new(uishp, yytext);
                    CHECK_LAST_COMM;
                } else {
                    DBG(0, "unexpected token\n");
                    FREE_LAST_COMM;
                    SET_SCAN_ERR_RETURN;
                }

                }

{SEMICOLON}   {
                DBG(0, "semicolon\n");
                /* append command */
                last_comm = NULL;
                in_command = 0;
                }

{STRINGDELIM}.*{STRINGDELIM} {
                if (1 == in_command) {
                    int i = 0;
                    char * str = strdup(yytext + 1);
                    if (NULL == str) {
                        FREE_LAST_COMM;
                        SET_SCAN_ERR_RETURN;
                    }
                    for (; i < strlen(str); i++) {
                        if ('\"' == str[i])
                            str[i] = 0;
                    }
                    DBG(0, "string: %s\n", yytext);
                    uish_cmd_set(last_comm, str);
                } else {
                    DBG(0, "error, unexpected token\n");
                    FREE_LAST_COMM;
                    SET_SCAN_ERR_RETURN;
                }
                }

#.*    /* eat up one-line comments */

[ \t]+          /* eat up whitespace */

\n          {
                line_nr++;
            }

.           {
                DBG(1,"error in line: %d\n", line_nr );
                FREE_LAST_COMM;
                SET_SCAN_ERR_RETURN;
            }

%%
int yywrap() {
    return 1;
}

scan_res_t lexscan(FILE * src, struct uish_s * uish) {
    uishp = uish;
    scan_res = SCAN_OK;
    yyin = src;
    yylex();
    yylex_destroy();
    uishp = NULL;
    return scan_res;
}

