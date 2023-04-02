#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
//#include <version.h>

#include "../src/view.h"


//static const Version version = Version(0, 6, 0);


int
main(int argc, char ** argv)
{
    GtkApplication * app;

    if(argc > 1 && strcmp(argv[1], "--version") == 0)
    {
        //version_show(stdout, version);
        return EXIT_SUCCESS;
    }

    #if defined(_WIN64) || defined(_WIN32)
    app = gtk_application_new(
                "gtk.example.webasto"
                , G_APPLICATION_DEFAULT_FLAGS);
    #else
     app = gtk_application_new(
                "gtk.example.webasto"
                , G_APPLICATION_FLAGS_NONE);
    #endif

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



