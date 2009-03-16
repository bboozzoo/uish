#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "uish.h"

/* local types */
/* local functions */
static void sig_handler(int sig);
static void setup_signals(void);
/* static data */
static struct uish_s uish;


/* signal handler */
static void sig_handler(int sig) {
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
        res_status_t res = RES_CONTINUE;
        res = uish_handle_input(&uish); 
        if (res == RES_EXIT) {
            run = 0;
            continue;
        }
    }

    uish_end(&uish);
    return 0;
}

