#ifndef APP_LIST_H
#define APP_LIST_H

#include "utils.h"

typedef struct app {
    struct app *prev, *next;
    int id;
    strategy_t *strategy;
    app_info_t *app_info;
} app_t;


app_t *get_app(int id);
int add_app(void);
void remove_app(int id);
void free_apps(void);

#endif
