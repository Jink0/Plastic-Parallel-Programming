
#include <stdio.h>
#include <stdlib.h>

#include "app_list.h"

/**
 *
 * Application list held by the controller, giving the
 * current strategy of each registered application.
 *
 */


// Next available application id.
static int next_id = 0;

static int get_next_id(void) {
    return next_id++;
}


/*
 * Application list for use by the controller.
 * Applications are doubly linked lists of application structs, each of which
 * contains application details.
 */


// Head of linked list
app_t applications = {NULL, NULL, -1, NULL, NULL};



/*
 * Return the application with the given id
 * If application does not exist, treat as error.
 */
app_t *get_app(int id) {
    app_t *curr = &applications;
    while (curr != NULL) {
        if (curr->id == id) return curr;
        curr = curr->next;
    }
    // Error if got here
    fprintf(stderr,"Application with id %d not found!\n", id);
    exit(EXIT_FAILURE);
}


/*
 * Remove given app
 */
void remove_app(int thread_id) {

    DBG_MSG("[App list]: Removing app with id %d from list\n", thread_id);

    app_t *curr = &applications;

    while (curr->id != thread_id) {
        curr = curr->next;
        if (curr == NULL) {
            fprintf(stderr,"Cannot find application %d, not in list\n", thread_id);
            exit(EXIT_FAILURE);
        }
    }

    curr->prev->next = curr->next;
    if (curr->next != NULL) curr->next->prev = curr->prev;

    free(curr->strategy);
    free(curr->app_info);
    free(curr);

}


/*
 * Add a new application to the list, returning its id
 */
int add_app(void) {

    int id = get_next_id();
    app_t *app = calloc(1, sizeof(app_t));

    app_t *curr = &applications;


    while (curr->next != NULL) {
        curr = curr->next;
    }

    curr->next = app;
    app->prev = curr;
    app->id = id;

    return id;

}



/*
 * Free memory and delete list
 */
void free_apps(void) {

    DBG_MSG("Freeing application list memory\n");

    app_t *curr;
    app_t *next = applications.next;

    while (next != NULL) {

        curr = next;
        next = next->next;

        free(curr->strategy);
        free(curr->app_info);
        free(curr);
    }

    DBG_MSG("Done freeing application list memory\n");

}




