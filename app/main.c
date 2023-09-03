#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <signal.h>

#include "../src/view.h"


#define LOCK_FILE "traceability.lock"


static char * __version__  = "1.0.3";
static char * __progname__ = "WebastoTraceability";


static void
cleanup(void)
{
    remove(LOCK_FILE);
}


static void 
signal_handler(int signal) 
{
    if (signal == SIGINT || signal == SIGTERM) 
    {
        cleanup();
        exit(0);
    }
}


int
main(int argc, char ** argv)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /*
    ** check if lock file exists
    */
    FILE * file = fopen(LOCK_FILE, "r");

    if (file != NULL) 
    {
        fclose(file);
        printf("Another instance is already running.\n");
        return 0;
    }

    /*
    ** creating of lock file
    */
    file = fopen(LOCK_FILE, "w");
    
    if (file == NULL) 
    {
        perror("Failed to create config file!");
        return 1;
    }

    fclose(file);

    GtkApplication * app;

    if(argc > 1 && strcmp(argv[1], "--version") == 0)
    {
        printf("%s - %s\n", __progname__, __version__);
        return EXIT_SUCCESS;
    }

    app = gtk_application_new(
                "gtk.example.webasto"
                , G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(
        G_OBJECT(app)
        , "activate"
        , G_CALLBACK(view)
        , NULL);

    int result = 
        g_application_run(
            G_APPLICATION(app)
            , argc
            , argv);

    g_object_unref(app);
    
    cleanup();

    return result;
}



