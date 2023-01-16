#include "ui.h"


O_Ui 
ui_build(char * glade)
{
    GtkBuilder * builder = gtk_builder_new_from_file(glade);  

    if(builder != NULL)
    {
        return O_Ui_Just(
                Ui(GTK_WIDGET(gtk_builder_get_object(builder, "window"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "page_stack"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "entry_user_name"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "entry_password"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "btn_login"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "lbl_change_password"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "entry_old_password"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "entry_new_password"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "btn_change_password"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "progress_timeout_logout"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "btn_logout"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "combo_table"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "combo_column"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "entry_key"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "btn_filter"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "db_table_view"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "environment_chart"))
                    , GTK_WIDGET(gtk_builder_get_object(builder, "environment_scale"))));
    }
    else
        return O_Ui_Nothing;
}


void
ui_set_visible_login_page(Ui * self)
{
    gtk_stack_set_visible_child_name(GTK_STACK(self->page_stack), "page_login");
}


void
ui_set_visible_passwd_page(Ui * self)
{
    gtk_stack_set_visible_child_name(GTK_STACK(self->page_stack), "page_passwd");
}


void 
ui_set_visible_main_page(Ui * self)
{
    gtk_stack_set_visible_child_name(GTK_STACK(self->page_stack), "page_main");
}
