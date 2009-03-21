#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "debug.h"
#include "uish.h"
#include "scan.h"

/* local types */
struct predef_command_s {
    const char * name;
    uish_predef_command_t id;
};
/* local functions */
static History * setup_history(void);
static void cleanup_history(History * hist);
static EditLine * setup_el(const char * name);
static void cleanup_el(EditLine * el);
static Tokenizer * setup_tok(const char * separator);
static void cleanup_tok(Tokenizer * tok);
static unsigned char * completion(EditLine * el, int ch);
static char * get_prompt(EditLine * el);
static int uish_set_prompt(struct uish_s * uish, const char * prompt);
/* static data */
static struct predef_command_s __predefined_commands[] = {
                    {"_exit", UISH_COMMAND_EXIT},
                    {NULL, UISH_COMMAND_INVALID}
};
static const char * compl_strings[] = {"exit",
                                       "logout",
                                       NULL };

static struct uish_s * local_uish;

/* functions */

static char * get_prompt(EditLine * el) {
    return local_uish->prompt;
}

static History * setup_history(void) {
    History * ret = history_init();
    if (ret != NULL) {
        HistEvent ev;
        history(ret, &ev, H_SETSIZE, 100);
    }
    return ret;
}

static void cleanup_history(History * hist) {
    history_end(hist);
}

static EditLine * setup_el(const char * name) {
    EditLine * el = NULL;

    el = el_init(name, stdin, stdout, stderr);
    if (el != NULL) {
        el_set(el, EL_SIGNAL, 1);
        el_set(el, EL_PROMPT, get_prompt);
        el_set(el, EL_EDITOR, "emacs");
        el_set(el, EL_BIND, "-e", "^P", "ed-prev-history", NULL);
        el_set(el, EL_BIND, "-e", "^N", "ed-next-history", NULL);  
        el_set(el, EL_ADDFN, "ed-complete", "completion", completion);
        el_set(el, EL_BIND, "^I", "ed-complete", NULL);
    }
    return el;
}

static void cleanup_el(EditLine * el) {
    el_end(el);
}

static Tokenizer * setup_tok(const char * separator) {
    Tokenizer * tok = tok_init(separator);
    return tok;
}

static void cleanup_tok(Tokenizer * tok) {
    tok_end(tok);
}

static int uish_set_prompt(struct uish_s * uish, const char * prompt) {
    char * oldpr = NULL;
    if (NULL == uish || NULL == uish->prompt)
        return 0;

    oldpr = uish->prompt;
    uish->prompt = strdup(prompt);
    if (NULL == uish->prompt) {
        uish->prompt = oldpr;
        printf("setting new prompt failed\n");
        return 0;
    }
    return 1;
}

static unsigned char * completion(EditLine * el, int ch) {
    const LineInfo * li = el_line(el);
    char * word = NULL;
    char * tmp = (char *) li->cursor;
    const char * compl_str = NULL;
    unsigned int word_len = 0;
    unsigned int i;
    int result = CC_ERROR;
    int only_one = 1;

    for (; tmp >= li->buffer; tmp--) {
        if (' ' == *tmp || tmp == li->buffer) {
            word = tmp;
            break;
        }
    }
    word_len = li->cursor - word;
    /*
    printf("\'%s\'\n", word);
    printf("buffer: %p cursor: %p wordstart: %p len: %d\n", li->buffer, li->cursor, word, word_len);
    printf("sizeof compl: %d\n", sizeof(compl_strings));
    */
    for (i = 0; compl_strings[i] != NULL; i++) {
        if (strncmp(word, compl_strings[i], word_len) == 0) {
      /*      printf("got match: %s\n", compl_strings[i]); */
            if (compl_str == NULL) {
                compl_str = compl_strings[i];
            } else { /* one match was already found */
                if (only_one != 0) { /* if this is a second match, show the first */
                    only_one = 0;
                    printf("\n%s\n", compl_str);
                }
            }
            if (only_one == 0) {
                printf("%s\n", compl_strings[i]);
                result = CC_REDISPLAY;
            }
        }
    }

    if (compl_str != NULL && only_one == 1) {
        el_insertstr(el, compl_str + word_len);
        result = CC_REFRESH;
    }

/*    printf("complete\n"); */
    return (unsigned char *) result;
}
/* initialise main struct */
int uish_init(struct uish_s * uish, const char * self, const char * prompt, FILE * config) {
    int optchar = -1;

    if (NULL == uish)
        goto return_err;

    memset(uish, 0, sizeof(struct uish_s));
    local_uish = uish;

    uish->prompt = strdup(prompt);
    if (NULL == uish->prompt)
        goto return_err;


    uish->hist = setup_history();
    if (NULL == uish->hist)
        goto return_err;

    uish->el = setup_el(self);
    if (NULL == uish->el)
        goto return_err;

    uish->tok = setup_tok(NULL);
    if (NULL == uish->tok)
        goto return_err;

    TAILQ_INIT(&uish->commands);
    if (config != NULL) {
        if (SCAN_ERR == lexscan(config, uish)) {
            DBG(0, "error while parsing config\n");
            goto return_err;
        }
    } else
        goto return_err;

    return 1;
return_err:
    return 0;
}

/* do neccessary cleanup */
void uish_end(struct uish_s * uish) {
    cleanup_history(uish->hist);
    cleanup_el(uish->el);
    cleanup_tok(uish->tok);
    free(uish->prompt);
}

/* handle input */
res_status_t uish_handle_input(struct uish_s * uish) {
    int len = 0;
    const char * input = NULL;

    input = el_gets(uish_el(uish), &len);
    if (input == NULL)
        return RES_EXIT;

    if (len > 0) {
        const LineInfo * li = el_line(uish_el(uish));
        int argc = 0;
        const char ** argv = NULL;
        int res = 0;

        res = tok_line(uish_tok(uish), li, &argc, &argv, NULL, NULL);
        if (res == 0) {
            int i = 0;
            printf("arg count: %d\n", argc);
            for (; i < argc; i++) {
                printf("arg: %s\n", argv[i]);
            }
            if (argc == 2) {
                if (strcmp(argv[0], "prompt") == 0) {
                    uish_set_prompt(uish, argv[1]);
                }
            }
        }
        tok_reset(uish_tok(uish));
        /*
        if (strncmp(input, "exit", len) == 0 || strncmp(input, "logout", len) == 0)
            return RES_EXIT;
            */
    }
    return RES_CONTINUE;
}

/* command handling */
struct uish_comm_s * uish_cmd_new(struct uish_s * uish, char * text) {
    struct uish_comm_s * res = calloc(1, sizeof(struct uish_comm_s));
    if (NULL != res) {
        res->name = strdup(text);
        if (NULL == res->name) {
            DBG(0, "allocating command name failed\n");
            free(res);
            res = NULL;
        }
        TAILQ_INIT(&res->commands_head);
    } else {
        DBG(0, "allocating command failed\n");
    }
    return res;
}

/* command is expected to be removed from list already */
void uish_cmd_free(struct uish_comm_s * comm) {
    if (NULL != comm) {
        DBG(0, "removing command, name: %s\n", comm->name);
        if (NULL != comm->name)
            free(comm->name);
        if (UISH_CMDTYPE_USER == comm->type && NULL != comm->cmd.command) {
            DBG(0, "user command %s\n", comm->cmd.command);
            free(comm->cmd.command);
        }
        free(comm);
    }
}

/* check if command is predefined */
uish_predef_command_t check_predef_command(const char * text) {
    uish_predef_command_t res = UISH_COMMAND_INVALID;
    struct predef_command_s * pre;
    for (pre = __predefined_commands; pre->name != NULL && pre->id != UISH_COMMAND_INVALID; pre++) {
        DBG(0, "compare \'%s\' with \'%s\'\n", text, pre->name);
        if (0 == strcmp(text, pre->name)) {
            DBG(0, "found predefined comand for: %s, predef id: %d\n", text, pre->id);
            res = pre->id;
        }
    }
    return res;
}

int uish_cmd_set(struct uish_comm_s * comm, char * cmd_text) {
    int res = 0;
    if (NULL != comm && NULL != cmd_text) {
        DBG(0, "user command: %s\n", cmd_text);
        uish_predef_command_t predef_id = check_predef_command(cmd_text);
        if (UISH_COMMAND_INVALID != predef_id) {
            DBG(0, "command is predefined\n");
            comm->type = UISH_CMDTYPE_PREDEFINED;
            comm->cmd.predef_command = predef_id;
            res = 1;
        } else {
            comm->type = UISH_CMDTYPE_USER;
            comm->cmd.command = strdup(cmd_text);
            if (NULL == comm->cmd.command) {
                DBG(0, "command allocation failed\n");
            } else {
                res = 1;
            }
        }
    }
    return res;
}

int uish_cmd_add_as_child(struct uish_comm_s * parent, struct uish_comm_s * cmd) {
    return 0;
}
