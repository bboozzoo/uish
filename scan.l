%{
#include <math.h>
#include <assert.h>
#include "scan.h"
#include "uish.h"
#include "debug.h"
#include "list.h"

typedef enum {
    STATE_MIN = 0,
    STATE_NULL,
    STATE_COMMAND,
    STATE_COMMAND_NAME,
    STATE_STRING,
    STATE_SECT_START,
    STATE_SECT_END,
    STATE_SEMICOLON,
    STATE_MAX
} parse_states_t;

struct parser_s {
    int line_nr;
    int nesting_level;
    parse_states_t state;
    struct uish_comm_s * curr_cmd;
    struct uish_comm_s * curr_parent;
    struct list_head_s * curr_head;
    struct list_head_s top_head;
    scan_res_t scan_res;
};

/* allowed state transitions specification */
parse_states_t __states[STATE_MAX][STATE_MAX] = {
/* to: NULL , CMD, CMDNAME, STR, SECTST, SECTEN, SEMICOL */ /* from */
    {      0,   1,       0,   0,      0,      0,      0}, /* NULL */
    {      0,   0,       1,   0,      0,      0,      0}, /* CMD */
    {      0,   0,       0,   1,      1,      0,      0}, /* CMD NAME */
    {      0,   0,       0,   0,      0,      0,      1}, /* STRING */
    {      0,   1,       0,   0,      0,      1,      0}, /* SECTION START */
    {      1,   0,       0,   0,      0,      0,      0}, /* SECTION END */
    {      1,   0,       0,   0,      0,      0,      0}  /* SEMICOLON */
    }; 
static struct parser_s parser; 

#define FREE_LAST_COMM \
    do { \
        if (NULL != last_comm) \
            uish_cmd_free(last_comm); \
    } while(0)

#define SET_SCAN_ERR_RETURN \
    do { \
        parser.scan_res = SCAN_ERR; \
        return -1; \
    } while(0)

#define CHECK_LAST_COMM \
    do {                            \
        if (NULL ==  (last_comm)) { \
            scan_res = SCAN_ERR;    \
            return -1;              \
        }                           \
    } while (0)

static void parser_init(void) {
    struct parser_s * p = &parser;
    p->line_nr = 1;
    p->nesting_level = 0;
    p->state = STATE_NULL;
    list_init(&p->top_head, NULL);
    p->curr_cmd = NULL;
    p->curr_parent = NULL;
    p->curr_head = &parser.top_head;
    p->scan_res = SCAN_OK;
}

static void parser_inc_line(void) {
    parser.line_nr++;
}

static void parser_nest_inc(void) {
    parser.nesting_level++;
    DBG(0, "starting section, nesting: %d\n", parser.nesting_level);
}

static void parser_nest_dec(void) {
    DBG(0,"ending section, nesting: %d\n", parser.nesting_level - 1);
    if (parser.nesting_level == 0)
        abort();
    parser.nesting_level--;
}

static char * parser_get_string(const char * txt) {
    int i = 0;
    char * str = strdup(txt + 1);
    DBG(0, "\tinput string: %s\n", txt);
    if (str != NULL) {
        for (; i < strlen(str); i++) {
            if ('\"' == str[i])
                str[i] = 0;
        }
    }
    DBG(0, "\toutput string: %s\n", str);
    return str;
}

static void __display_tree(struct list_head_s * head, int indent) {
    struct list_head_s * iter;
    list_for(head, iter) {
        struct uish_comm_s * comm = LIST_DATA(iter, struct uish_comm_s);
        if (comm == NULL) {
            DBG(0, "head\n");
        } else {
            int i = 0;
            for(; i < indent; i++)
                DBG(0, " ");
            DBG(0, "%s\n", comm->name);
            if (!list_is_empty(&comm->commands_head)) {
                __display_tree(&comm->commands_head, indent + 1);
            }
        }
    }
}

static void display_tree(struct uish_s * uish) {
    __display_tree(&uish->commands, 0);
}

/*******************/
/* FLEX code below */
/*******************/
%}

COMMAND  command
SECT_START \{
SECT_END   \}
SEMICOLON ;
STRINGDELIM \"
COMMAND_NAME [a-zA-Z]+

%%

{SECT_START} {
    parser_nest_inc();
    if (parser.curr_cmd != NULL) {
        parser.curr_head = &parser.curr_cmd->commands_head;
        parser.curr_parent = parser.curr_cmd;
    } else {
        /* obviously error */
        abort();
   }
}

{SECT_END} { 
    parser_nest_dec();
    parser.curr_cmd = uish_cmd_get_parent(parser.curr_cmd);
    DBG(0, "parent: %p\n", parser.curr_cmd);
    if (parser.curr_cmd != NULL) {
        DBG(0, "parent: %s\n", parser.curr_cmd->name);
        parser.curr_head = &parser.curr_cmd->commands_head;
    } else {
        parser.curr_head = &parser.top_head;
    }
}

{COMMAND} {
    DBG(0, "command token\n");
}

{COMMAND_NAME} {
    DBG(0, "\tname: %s\n", yytext);
    parser.curr_cmd = uish_cmd_new(yytext);
    DBG(0, "\tallocated cmd: %s\n", parser.curr_cmd->name);
    list_add_tail(parser.curr_head, &parser.curr_cmd->list_el);
    uish_cmd_set_parent(parser.curr_cmd, parser.curr_parent);
    if (parser.curr_parent != NULL) {
        DBG(0, "\tparent: %s\n", parser.curr_parent->name);
    }

}

{SEMICOLON} {
    DBG(0, "semicolon\n");
    parser.curr_cmd = uish_cmd_get_parent(parser.curr_cmd);
    DBG(0, "parent: %p\n", parser.curr_cmd);
    if (parser.curr_cmd != NULL) {
        DBG(0, "parent: %s\n", parser.curr_cmd->name);
        parser.curr_head = &parser.curr_cmd->commands_head;
    } else {
        DBG(0, "parent is TOP node\n");
        parser.curr_head = &parser.top_head;
    }
}

{STRINGDELIM}.*{STRINGDELIM} {
    char * str = parser_get_string(yytext);
    if (NULL == str) {
        SET_SCAN_ERR_RETURN;
    }
    /* set current command's text */
    uish_cmd_set(parser.curr_cmd, str);
    free(str);
}

#.*    /* eat up one-line comments */

[ \t]+          /* eat up whitespace */

\n {
    parser_inc_line();
}

. {
    DBG(1,"error in line: %d\n", parser.line_nr );
    SET_SCAN_ERR_RETURN;
}

%%
int yywrap() {
    return 1;
}

scan_res_t lexscan(FILE * src, struct uish_s * uish) {
    parser_init();
    yyin = src;
    yylex();
    yylex_destroy();
    if (list_is_empty(&parser.top_head)) {
        DBG(0, "nothing found?\n");
    } else {
        __display_tree(&parser.top_head, 0);
        DBG(0, "setting commands\n");
        uish_set_commands(uish, &parser.top_head);
        DBG(0, "display tree from uish\n");
        display_tree(uish);
    }
    return parser.scan_res;
}

