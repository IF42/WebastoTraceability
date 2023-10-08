#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <signal.h>


#include "../src/version.h"
#include "../src/view.h"


#define LOCK_FILE "traceability.lock"


/*
** deleting lock file 
*/
static void
cleanup(void)
{
    remove(LOCK_FILE);
}


static void
handle_cmd_args(
    int argc
    , char ** argv)
{
    if(argc > 1) 
    {
        if(strcmp(argv[1], "--version") == 0)
        {
            printf(
                "%s: %s\nCompiled: %s\n"
                , __progname__
                , __version__
                , __DATE__);
        }
        else if(strcmp(argv[1], "--clean") == 0)
            cleanup();

        exit(EXIT_SUCCESS);
    }
}


/*
** creation of lock file for prevent to run only one 
** instance at one omment
*/
static void
lock_running_instance(void)
{
    /*
    ** check if lock file exists
    */
    FILE * file = fopen(LOCK_FILE, "r");

    if (file != NULL) 
    {
        fclose(file);
        fprintf(stderr, "Another instance is already running.\n");
        exit(EXIT_FAILURE);
    }

    /*
    ** creating of lock file
    */
    file = fopen(LOCK_FILE, "w");
    
    if (file == NULL) 
    {
        fprintf(stderr, "Failed to create config file!");
        exit(EXIT_FAILURE);
    }

    fclose(file);
}


static void 
signal_handler(int signal) 
{
    if (signal == SIGINT || signal == SIGTERM) 
    {
        cleanup();
        exit(EXIT_SUCCESS);
    }
}


int
main(int argc, char ** argv)
{
    Model * model;
    Controller * controller;
    View * view;

    handle_cmd_args(argc, argv);
    lock_running_instance();

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    gtk_init(&argc, &argv);

    /*
    ** initialization of application
    */
    if((model = model_init()) == NULL)
    {
        cleanup();

        return EXIT_FAILURE;
    }
    
    if((controller = controller_new(model)) == NULL)
    {
        model_delete(model);
        cleanup();

        return EXIT_FAILURE;
    }

    if((view = view_new(controller, model)) == NULL)
    {
        model_delete(model);
        controller_delete(controller);
        cleanup();

        return EXIT_FAILURE;
    }

    gtk_main();

    cleanup();
    model_delete(model);
    controller_delete(controller);

    return EXIT_SUCCESS;
}



