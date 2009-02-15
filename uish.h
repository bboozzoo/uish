#ifndef __UISH_H__
#define __UISH_H__

#include <histedit.h>

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


#endif /* __UISH_H__ */
