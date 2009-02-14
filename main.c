#include <histedit.h>
#include <signal.h>
#include <string.h>

static const char * compl_strings[] = { 
                                    "logout",
                                    "login",
                                    "exit", 
                                    NULL
                                };

typedef enum {
    RES_CONTINUE,
    RES_EXIT,
} res_status_t;

static void sig_handler(int sig) {

}

static char * get_prompt(EditLine * el) {
    return "% ";
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

static void setup_history(void) {
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
            } else {
                only_one = 0;
                printf("\n%s\n", compl_str);
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

res_status_t handle_input(const char * input, unsigned int len) {
    if (strncmp(input, "exit", len) == 0 || strncmp(input, "logout", len) == 0)
        return RES_EXIT;
    return RES_CONTINUE;
}

int main(int argc, char * argv[]) {
/*    History * hist = NULL;
    HistEvent ev; */
    Tokenizer * tok = NULL;
    EditLine * el = NULL;
    int run = 0;

    /* setup signals */
    setup_signals();
    /* init history */
    /*
    hist = history_init();
    */
    /* setup libedit */
    el = el_init(argv[0], stdin, stdout, stderr);
    el_set(el, EL_SIGNAL, 1);
    el_set(el, EL_PROMPT, get_prompt);
    el_set(el, EL_EDITOR, "emacs");
    el_set(el, EL_ADDFN, "ed-complete", "completion", completion);
    el_set(el, EL_BIND, "^I", "ed-complete", NULL);

    run = 1;

    while (run) {
        const char * input = NULL;
        int count = 0;
        res_status_t res = RES_CONTINUE;
        input = el_gets(el, &count);
/*        printf("read: %p count: %d\n", input, count); */
        if (input == NULL && count == 0) {
            run = 0;
            continue;
        } else {
            printf("input: %s count: %d\n", input, count);
            res = handle_input(input, count - 1); 
        }
        if (res == RES_EXIT)
            run = 0;
    }
    if (NULL != el)
        el_end(el);
    if (NULL != tok)
        tok_end(tok);
    /*
    if (NULL != hist)
        history_end(hist);
    */
    return 0;
}

