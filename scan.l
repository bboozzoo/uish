%{
#include <math.h>
#include <assert.h>
#include "scan.h"
#include "uish.h"
#include "debug.h"
#include "list.h"

static int line_nr = 1;
static int nesting_level = 0;
static scan_res_t scan_res = SCAN_OK;
static struct uish_comm_s * curr_cmd = NULL;
static struct uish_comm_s * curr_parent = NULL;
static struct list_head_s * curr_head = NULL;
static struct list_head_s top_head;
static void __display_tree(struct list_head_s * head, int indent);

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
    if (curr_cmd != NULL) {
        curr_head = &curr_cmd->commands_head;
        curr_parent = curr_cmd;
    } else {
        /* obviously error */
        abort();
   }
}

{SECT_END} { 
    nesting_level--;
    assert(nesting_level >= 0);
    DBG(0,"ending section, nesting: %u\n", nesting_level);
    curr_cmd = uish_cmd_get_parent(curr_cmd);
    DBG(0, "parent: %p\n", curr_cmd);
    if (curr_cmd != NULL) {
        DBG(0, "parent: %s\n", curr_cmd->name);
        curr_head = &curr_cmd->commands_head;
    } else {
        curr_head = &top_head;
    }
}

{COMMAND} {
    DBG(0, "command token\n");
}

{COMMAND_NAME} {
    DBG(0, "\tname: %s\n", yytext);
    curr_cmd = uish_cmd_new(yytext);
    DBG(0, "\tallocated cmd: %s\n", curr_cmd->name);
    list_add_tail(curr_head, &curr_cmd->list_el);
    uish_cmd_set_parent(curr_cmd, curr_parent);
    if (curr_parent != NULL) {
        DBG(0, "\tparent: %s\n", curr_parent->name);
    }

}

{SEMICOLON} {
    DBG(0, "semicolon\n");
    curr_cmd = uish_cmd_get_parent(curr_cmd);
    DBG(0, "parent: %p\n", curr_cmd);
    if (curr_cmd != NULL) {
        DBG(0, "parent: %s\n", curr_cmd->name);
        curr_head = &curr_cmd->commands_head;
    } else {
        curr_head = &top_head;
    }
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
    /* set current command's text */
    uish_cmd_set(curr_cmd, str);
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

void __display_tree(struct list_head_s * head, int indent) {
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

void display_tree(struct uish_s * uish) {
    __display_tree(&uish->commands, 0);
}

scan_res_t lexscan(FILE * src, struct uish_s * uish) {
    scan_res = SCAN_OK;
    curr_cmd = NULL;
    curr_parent = NULL;
    curr_head = &top_head;
    list_init(&top_head, NULL);
    yyin = src;
    yylex();
    yylex_destroy();
    if (curr_cmd == NULL) {
        DBG(0, "nothing found?\n");
    } else {
        __display_tree(&top_head, 0);
        DBG(0, "setting commands\n");
        uish_set_commands(uish, &top_head);
        display_tree(uish);
    }
    return scan_res;
}

