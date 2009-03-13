%{
#include <math.h>
#include "debug.h"
static int in_section = 0;
static int line_nr = 1;
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
            }

{COMMAND}     {
                DBG(0, "command %s\n", yytext);
            }

{COMMAND_NAME} {
                DBG(0, "text: %s\n", yytext);
                }

{SEMICOLON}   {
                DBG(0, "semicolon\n");
                }
{STRINGDELIM}.*{STRINGDELIM} {
                DBG(0, "string: %s\n", yytext);
                }

^#.*    /* eat up one-line comments */

[ \t]+          /* eat up whitespace */

\n          {
                line_nr++;
            }

.           {
                DBG(1,"error in line: %d\n", line_nr );
            }

%%
int yywrap() {
    return 1;
}

int lexscan(FILE * src) {
    yyin = src;
    yylex();
    yylex_destroy();
    return 0;
}

