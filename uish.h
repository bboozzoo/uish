#ifndef __UISH_H__
#define __UISH_H__

#include <sys/queue.h>
#include <histedit.h>
#include <stdio.h>

struct uish_comm_s;

TAILQ_HEAD(uish_commands_head_s, uish_comm_s); /* commands list head */

typedef enum {
    UISH_COMMAND_INVALID = 0,
    UISH_COMMAND_EXIT,
} uish_predef_command_t;

typedef enum {
    UISH_CMDTYPE_USER = 0,
    UISH_CMDTYPE_PREDEFINED
} uish_cmdtype_t;

struct uish_comm_s {
    char * name;  /* command name as input by user */
    uish_cmdtype_t type; /* should be set to 0 if command is among predefined commands list */
    union {
        char * command; /* the actual command to be executed, should be NULL there is nothing to be executed  */
        uish_predef_command_t predef_command;
    } cmd;
    TAILQ_ENTRY(uish_comm_s) list_el; /* commands list member */
    struct uish_commands_head_s commands_head; /* list of further nested commands */
};

struct uish_s {
#define uish_hist(__uish) ((__uish)->hist)
    History * hist;
#define uish_el(__uish) ((__uish)->el)
    EditLine * el;
#define uish_tok(__uish) ((__uish)->tok)
    Tokenizer * tok;
#define uish_commands(__uish) ((__uish)->commands)
    struct uish_commands_head_s commands;
    char * prompt;
};

typedef enum {
    RES_CONTINUE, /* continue receiving input */
    RES_EXIT, /* user wants to quit */
} res_status_t;

int uish_init(struct uish_s * uish, const char * self, const char * prompt, FILE * config);
void uish_end(struct uish_s * uish);
res_status_t uish_handle_input(struct uish_s * uish);
/* command structure manipulation */
struct uish_comm_s * uish_cmd_new(struct uish_s * uish, char * text);
void uish_cmd_free(struct uish_comm_s * comm);
int uish_cmd_set(struct uish_comm_s * comm, char * cmd_text);
int uish_cmd_add_as_child(struct uish_comm_s * parent, struct uish_comm_s * cmd);

#endif /* __UISH_H__ */

