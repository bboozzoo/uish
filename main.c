#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "debug.h"
#include "uish.h"
#include "uish_version.h"

/* local types */
struct help_s {
    const char * sh;
    const char * desc;
};
/* local functions */
static void sig_handler(int sig);
static void setup_signals(void);
static void show_help(void);
static char * find_config_file(void);
/* static data */
static struct uish_s uish;
static struct help_s help_entries[] = { {"-h", "show help"},
                                        {"-f", "config file"},
                                        {NULL, NULL} };
/* functions */

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

static void show_help(void) {
    struct help_s * hlp = help_entries;
    fprintf(stderr,"%s v%s built: %s\n", UISH_NAME, UISH_VER, UISH_DATE);
    fprintf(stderr,"help:\n");
    for (; hlp->sh != NULL; hlp++) 
        fprintf(stderr, "\t%s\t%s\n", hlp->sh, hlp->desc);
}

static char * find_config_file(void) {
#define DEFAULT_CONFIG_FILE "/etc/uish.conf"
    return strdup(DEFAULT_CONFIG_FILE);
}

static FILE * prepare_config(const char * config_file_name) {
    FILE * config = fopen(config_file_name, "r");
    if (NULL == config) {
        fprintf(stderr, "opening config failed: %s\n", strerror(errno));
    }
    return config;
}

static void close_config(FILE * config) {
    if (NULL != config)
        fclose(config);
}

int main(int argc, const char * argv[]) {
    int run = 0;
    char * config_file = NULL;
    FILE * config = NULL;
    int optchar = 0;

    config_file = find_config_file();
    dbg_init(stderr, 1, 1); 
    /* setup signals */
    DBG(0, "setup signals\n");
    setup_signals();
    /* setup libedit */
    do {
        optchar = getopt(argc, (char * const *) argv, "f:h");
        switch (optchar) {
            case 'f':
                DBG(0, "user defined config file: %s\n", optarg);
                if (NULL != config_file)
                    free(config_file);
                config_file = strdup(optarg);
                break;
            case 'h':
            case '?':
                show_help();
                goto do_cleanup;
                break;
            default:
                break;
        }

    } while (optchar != -1);
    /* init */
    DBG(0, "prepare config file: %s\n", config_file);
    config = prepare_config(config_file);
    if (NULL == config)
        goto do_cleanup;
    
    DBG(0, "init uish\n");
    if (uish_init(&uish, argv[0], "% ", config))
        run = 1;
    /* close config, won't be needed anymore */
    close_config(config);

    DBG(0, "enter input loop, run: %d\n", run);
    while (run) {
        res_status_t res = RES_CONTINUE;
        res = uish_handle_input(&uish); 
        if (res == RES_EXIT) {
            run = 0;
            continue;
        }
    }

    uish_end(&uish);
do_cleanup:
    DBG(0, "cleanup\n");
    free(config_file);
    return 0;
}

