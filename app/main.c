#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "../src/view.h"


static char * __version__  = "1.0.0";
static char * __progname__ = "WebastoTraceability";


int
main(int argc, char ** argv)
{
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
    
    return result;
}



