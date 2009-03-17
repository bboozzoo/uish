#ifndef __UISH_H__
#define __UISH_H__

#include <sys/queue.h>
#include <histedit.h>
#include <stdio.h>

struct uish_s {
#define uish_hist(__uish) ((__uish)->hist)
    History * hist;
#define uish_el(__uish) ((__uish)->el)
    EditLine * el;
#define uish_tok(__uish) ((__uish)->tok)
    Tokenizer * tok;
    void * commands;
    char * prompt;
};

typedef enum {
    RES_CONTINUE, /* continue receiving input */
    RES_EXIT, /* user wants to quit */
} res_status_t;

int uish_init(struct uish_s * uish, const char * self, const char * prompt, FILE * config);
void uish_end(struct uish_s * uish);
res_status_t uish_handle_input(struct uish_s * uish);

#endif /* __UISH_H__ */
