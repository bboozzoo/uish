#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "debug.h"
#include "uish.h"

/* local types */
typedef enum {
    RES_CONTINUE,
    RES_EXIT,
} res_status_t;

/* local functions */
static int uish_init(struct uish_s * uish, const char * argv[], const int argc, const char * prompt);
static void uish_end(struct uish_s * uish);
static int uish_set_prompt(struct uish_s * uish, const char * prompt);
static void sig_handler(int sig);
static char * get_prompt(EditLine * el);
static void setup_signals(void);
static History * setup_history(void);
static void cleanup_history(History * hist);
static EditLine * setup_el(const char * name);
static void cleanup_el(EditLine * el);
static Tokenizer * setup_tok(const char * separator);
static void cleanup_tok(Tokenizer * tok);
static res_status_t handle_input(struct uish_s * uish, const char * input, unsigned int len);
static unsigned char * completion(EditLine * el, int ch);

/* static data */
static const char * compl_strings[] = { 
                                    "logout",
                                    "login",
                                    "logat",
                                    "exit", 
                                    NULL
                                };


static struct uish_s uish;


/* signal handler */
static void sig_handler(int sig) {
}

/* initialise main struct */
static int uish_init(struct uish_s * uish, const char * argv[], const int argc, const char * prompt) {
    FILE * config = NULL;
    char * config_file = strdup("/etc/uish.conf");
    int optchar = -1;

    if (NULL == uish)
        goto return_err;

    memset(uish, 0, sizeof(struct uish_s));

    do {
        optchar = getopt(argc, (char * const *) argv, "f:");
        switch (optchar) {
            case 'f':
                DBG(0, "user defined config file: %s\n", optarg);
                config_file = strdup(optarg);
                break;
            default:
                break;
        }

    } while (optchar != -1);

    uish->prompt = strdup(prompt);
    if (NULL == uish->prompt)
        goto return_err;


    uish->hist = setup_history();
    if (NULL == uish->hist)
        goto return_err;

    uish->el = setup_el(argv[0]);
    if (NULL == uish->el)
        goto return_err;

    uish->tok = setup_tok(NULL);
    if (NULL == uish->tok)
        goto return_err;

    DBG(0, "opening config file: %s\n", config_file);
    config = fopen(config_file, "r");
    if (config != NULL) {
        DBG(0, "parse config\n");
        lexscan(config);
        fclose(config);
    } else {
        DBG(0, "config open failed: %s\n", strerror(errno));
        goto return_err;
    }
    free(config_file);

    return 1;
return_err:
    return 0;
}

/* do neccessary cleanup */
static void uish_end(struct uish_s * uish) {
    cleanup_history(uish->hist);
    cleanup_el(uish->el);
    cleanup_tok(uish->tok);
    free(uish->prompt);
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

static char * get_prompt(EditLine * el) {
    return uish.prompt;
}

static Tokenizer * setup_tok(const char * separator) {
    Tokenizer * tok = tok_init(separator);
    return tok;
}

static void cleanup_tok(Tokenizer * tok) {
    tok_end(tok);
}

static void setup_signals(void) {
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = sig_handler;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGHUP, &sigact, NULL);
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

res_status_t handle_input(struct uish_s * uish, const char * input, unsigned int len) {
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

int main(int argc, const char * argv[]) {
    int run = 0;

    dbg_init(stderr, 1, 1); 
    /* setup signals */
    DBG(0, "setup signals\n");
    setup_signals();
    /* setup libedit */
    DBG(0, "init libedit\n");
    if (uish_init(&uish, argv, argc, "% "))
        run = 1;

    DBG(0, "enter loop\n");
    while (run) {
        const char * input = NULL;
        int count = 0;
        res_status_t res = RES_CONTINUE;
        input = el_gets(uish_el(&uish), &count);
        res = handle_input(&uish, input, count); 
        if (res == RES_EXIT) {
            run = 0;
            continue;
        }
        /*
        if (count - 1 > 0) {
            history(hist, &ev, H_ENTER, input);
        }
        */
    }

    uish_end(&uish);
    return 0;
}

