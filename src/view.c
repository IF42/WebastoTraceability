#include "view.h"
#include "ui.h"
#include "model.h"
#include "login.h"
#include "controller.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>


#define TREE_VIEW_SIZE_LIMIT ((size_t) 2000)
#define LOGOUT_INTERVAL 60*10 //seconds


typedef struct
{
    Ui ui;
    Model * model;
    Controller * controller;
    int cyclic_interrupt_id;
}View;


#define View(...)(View){__VA_ARGS__}


static View v;


static void
view_init_gui(View * self)
{
    self->model->logout_timer = 0;
}
 

static inline void
view_init_logout_timer(View * self)
{
    self->model->logout_timer = LOGOUT_INTERVAL;
}


static void 
view_login(View * self)
{
    const char * username = gtk_entry_get_text(GTK_ENTRY(self->ui.entry_user_name));
    const char * password = gtk_entry_get_text(GTK_ENTRY(self->ui.entry_password));

    if(system_login(username, password) == true)
    {
        view_init_logout_timer(self);
        ui_set_visible_main_page(&self->ui);

        strcpy(
            self->model->logged_user
            , gtk_entry_get_text(GTK_ENTRY(self->ui.entry_user_name)));

        log_debug(
            self->model->log
            , "User %s logged in"
            , self->model->logged_user);
    }
    else
    {
        log_warning(
            self->model->log
            , "Unsuccessful attempt to login [%s]"
            , username);
    }

    gtk_entry_set_text(GTK_ENTRY(self->ui.entry_password), "");
}


static void
view_logout(View * self)
{
    ui_set_visible_login_page(&self->ui);
    view_init_gui(self);

    log_debug(
        self->model->log
        , "User %s logged out"
        , self->model->logged_user);
}


GtkTreeViewColumn *
view_new_view_column(
    char * label
    , int index)
{
    GtkTreeViewColumn * column = 
        gtk_tree_view_column_new_with_attributes(
            label
            , (GtkCellRenderer*) gtk_cell_renderer_text_new()
            , "text"
            , index
            , NULL);

    gtk_tree_view_column_set_min_width(column, 100);
    gtk_tree_view_column_set_expand(column, true);
    gtk_tree_view_column_set_resizable(column, true);

    return column;
}


static GtkListStore *
view_prepare_tree_model(size_t columns)
{
    GType * column_type_list = malloc(sizeof(GType) * columns);
    
    for(size_t i = 0; i < columns; i ++)
        column_type_list[i] = G_TYPE_STRING;

    GtkListStore * tree_list_store = 
        gtk_list_store_newv (
            columns
            , column_type_list);
    
    free(column_type_list);

    return tree_list_store;
}


static void
view_clear_table_list(GtkWidget * db_table_view)
{
    if(gtk_tree_view_get_n_columns(GTK_TREE_VIEW(db_table_view)) > 0)
    {
        while(gtk_tree_view_remove_column(GTK_TREE_VIEW(db_table_view)
            , GTK_TREE_VIEW_COLUMN(gtk_tree_view_get_column(GTK_TREE_VIEW(db_table_view), 0))) > 0);
    }
}


static void
view_fill_db_table_view(
    GtkListStore * db_table_model
    , TableContent * db_table_content)
{
    if(db_table_content == NULL)
        return;

    GtkTreeIter iter;

    for(size_t i = 0; i < db_table_content->super.size; i++)
    {
        if(db_table_content->array[i] == NULL)
            return;

        gtk_list_store_append (db_table_model, &iter);   

        for(size_t j = 0; j < db_table_content->array[i]->super.size; j++)
        {
            if(db_table_content->array[i]->array[j] == NULL)
                break;

            gtk_list_store_set (
                db_table_model
                , &iter
                , j
                , db_table_content->array[i]->array[j]
                , -1);
        }
    }
}


static void
view_combo_table_changed_callback (
  GtkComboBox * widget
  , View * view)
{
    view_init_logout_timer(view);

    TableContent * column_list = 
        model_get_table_columns(
            view->model
            , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));

    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(view->ui.combo_column));
    view_clear_table_list(view->ui.db_table_view);

    if(column_list != NULL)
    {
        GtkListStore * table_view_model = 
            view_prepare_tree_model(column_list->super.size);

        for(size_t i = 0; i < column_list->super.size; i++)
        {
            gtk_combo_box_text_append_text(
                GTK_COMBO_BOX_TEXT(view->ui.combo_column)
                , column_list->array[i]->array[1]);

            gtk_tree_view_append_column(
                GTK_TREE_VIEW(view->ui.db_table_view)
                , view_new_view_column(column_list->array[i]->array[1], i));
        }
        
        array_delete(ARRAY(column_list));

        gtk_tree_view_set_model(
            GTK_TREE_VIEW(view->ui.db_table_view)
            , GTK_TREE_MODEL(table_view_model));
        gtk_combo_box_set_active(GTK_COMBO_BOX(view->ui.combo_column), 0);
 
        TableContent * db_table_content = 
            model_get_table_content(
                view->model
                , TREE_VIEW_SIZE_LIMIT
                , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget))
                , "*"
                , NULL
                , NULL);

        if(db_table_content == NULL || db_table_content->super.size == 0)
            gtk_widget_set_sensitive (view->ui.btn_export, false);
        else
            gtk_widget_set_sensitive (view->ui.btn_export, true);

        view_fill_db_table_view (table_view_model, db_table_content);
        array_delete(ARRAY(db_table_content));
    }

    gtk_entry_set_text(GTK_ENTRY(view->ui.entry_key), "");
}


static void
view_entry_username_activate_callback(
    GtkWidget * widget
    , View * view)
{
    (void) widget;
    view_login(view);
}


static void
view_entry_password_activate_callback(
    GtkWidget * widget
    , View * view)
{
    (void) widget;
    view_login(view);
}


static void 
view_btn_login_clicked_callback(GtkWidget * widget, View * view)
{
    (void) widget;
    view_login(view);
}


static void 
view_btn_logout_clicked_callback(GtkWidget * widget, View * view)
{
    (void) widget;
    view_logout(view);
}


static void
view_btn_filter_click_callback(GtkWidget * widget, View * view)
{
    (void) widget;
    view_init_logout_timer(view);

    TableContent * column_list = 
        model_get_table_columns(
            view->model
            , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table)));

    if(column_list != NULL)
    {
        view_clear_table_list(view->ui.db_table_view);

        for(size_t i = 0; i < column_list->super.size; i++)
        {
            gtk_tree_view_append_column(
                GTK_TREE_VIEW(view->ui.db_table_view)
                , view_new_view_column(column_list->array[i]->array[1], i));
        }

        GtkListStore * table_view_model = 
            view_prepare_tree_model(column_list->super.size);

        gtk_tree_view_set_model(
            GTK_TREE_VIEW(view->ui.db_table_view)
            , GTK_TREE_MODEL(table_view_model));

        TableContent * db_table_content = 
            model_get_table_content(
                view->model 
                , TREE_VIEW_SIZE_LIMIT
                , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table))
                , "*"
                , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_column))
                , (char*) gtk_entry_get_text(GTK_ENTRY(view->ui.entry_key)));

        array_delete(ARRAY(column_list));

        if(db_table_content == NULL || db_table_content->super.size == 0)
            gtk_widget_set_sensitive (view->ui.btn_export, false);
        else
            gtk_widget_set_sensitive (view->ui.btn_export, true);

        view_fill_db_table_view (table_view_model, db_table_content);
        array_delete(ARRAY(db_table_content));
    }
}


#if defined(__linux__)
#include <pwd.h>
#endif

static void
view_btn_export_click_callback(
    GtkWidget * widget
    , View * view)
{
    (void) widget;

    /*
    ** preparing of file choose dialog for entry required csv name
    */
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;

    GtkWidget * dialog = 
        gtk_file_chooser_dialog_new (
            "Export as csv"
              , GTK_WINDOW(view->ui.window)
              , action
              , "_Cancel"
              , GTK_RESPONSE_CANCEL
              , "_Save"
              , GTK_RESPONSE_ACCEPT
              , NULL);

    /*
    ** setup parametr of file choose dialog to save only as .csv file extension and
    ** with default file name and to default directory path based on running system
    */
    GtkFileChooser * chooser = GTK_FILE_CHOOSER (dialog);
    GtkFileFilter * filter   = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.csv");
    
    gtk_file_chooser_set_filter(chooser, filter);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);
    gtk_file_chooser_set_current_name(chooser, "export.csv");

#if defined(__linux__)
    char * homedir = getenv("HOME");

    if(homedir == NULL)
        homedir = "/";

    gtk_file_chooser_set_current_folder(chooser, homedir);
#else
    gtk_file_chooser_set_current_folder(chooser, "C:/");
#endif

    gtk_widget_show_all(dialog);

    int res = gtk_dialog_run (GTK_DIALOG (dialog));

    /*
    ** generation of csv content 
    */
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char * filename;

        filename = gtk_file_chooser_get_filename (chooser);

        /* TODO: export */

        FILE * f = fopen(filename, "w");

        if(f != NULL)
        {
            TableContent * column_list = 
                model_get_table_columns(
                    view->model
                    , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table)));

            if(column_list != NULL)
            {
                for(size_t i = 0; i < column_list->super.size; i++)
                    fprintf(f, i == 0 ? "%s" : ", %s", column_list->array[i]->array[1]);
            }

            TableContent * db_table_content = 
                model_get_table_content(
                    view->model 
                    , 0
                    , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table))
                    , "*"
                    , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_column))
                    , (char*) gtk_entry_get_text(GTK_ENTRY(view->ui.entry_key)));

            if(db_table_content != NULL)
            {
                for(size_t i = 0; i < db_table_content->super.size; i++)
                {
                    if(db_table_content->array[i] == NULL)
                        continue;

                    for(size_t j = 0; j < db_table_content->array[i]->super.size; j++)
                        fprintf(f, j == 0 ? "\n%s" : ", %s", db_table_content->array[i]->array[j]);
                }
            }

            array_delete(ARRAY(column_list));
            array_delete(ARRAY(db_table_content));

            fclose(f);
        }
        else
        {
            /*
            ** error dialog if it is not possioble to open file for writing
            */
            GtkWidget * error_dialog = 
                gtk_message_dialog_new (
                      GTK_WINDOW(view->ui.window)
                      , 0
                      , GTK_MESSAGE_ERROR
                      , GTK_BUTTONS_OK
                      , "Error save file: %s"
                      , filename);

            gtk_dialog_run (GTK_DIALOG (error_dialog));
            gtk_widget_destroy(error_dialog);
        }

        g_free (filename);
    }

    gtk_widget_destroy (dialog);
}


static int
view_cyclic_interupt_callback(gpointer param)
{
    View * self = param;
    char str_timeout[32];

    if(self->model->logout_timer > 0)
    {
        if((--self->model->logout_timer) == 0)
            view_logout(self);      
         
        gtk_progress_bar_set_fraction (
            GTK_PROGRESS_BAR (self->ui.progress_timeout_logout)
            , ((float)self->model->logout_timer)/((float)LOGOUT_INTERVAL));

        sprintf(
            str_timeout
            , "%02ld:%02ld"
            , self->model->logout_timer / 60, self->model->logout_timer % 60);

        gtk_progress_bar_set_text(
            GTK_PROGRESS_BAR(self->ui.progress_timeout_logout)
            , str_timeout);
    }
    
    return true;
}

void
view_combo_column_changed_callback(
  GtkComboBox* widget
  , View * view)
{
    (void) widget;

    view_init_logout_timer(view);
}

typedef struct 
{
  int temperature;
  int humidity;
} DataPoint;

static DataPoint data[] = {
  { 20, 40 },
  { 22, 45 },
  { 25, 50 },
  { 28, 55 },
  { 30, 60 },
};


static void 
draw_graph(
    cairo_t *cr
    , GtkAllocation allocation)
{
    /* unused parameter */
    (void) allocation;

    cairo_set_source_rgba(cr, 0, 0, 0, 1);

    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_set_line_width(cr, 1);

    cairo_move_to(cr, 50, 50);

    for (int i = 0; i < 5; i++) 
        cairo_line_to(cr, 50 + i * 50, 50 + data[i].temperature);

    cairo_stroke(cr);

    cairo_set_source_rgba(cr, 0, 0, 1, 1);
    cairo_move_to(cr, 50, 50);

    for (int i = 0; i < 5; i++) 
        cairo_line_to(cr, 50 + i * 50, 50 + data[i].humidity);

    cairo_stroke(cr);
}


static gboolean 
view_environment_chart_draw_callback(
    GtkWidget * widget
    , cairo_t *cr
    , gpointer data)
{
    /* unused attribute */
    (void) data;

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation); 

    draw_graph(cr, allocation);

    return true;
}


static void
view_delete_record(View * self)
{
    GtkTreeView * tree_view      = GTK_TREE_VIEW(self->ui.db_table_view);
    GtkTreeSelection * selection = gtk_tree_view_get_selection(tree_view);
    GtkTreeModel * model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        /*
        ** confirmation dialog for record delete  
        */
        GtkWidget * error_dialog = 
            gtk_message_dialog_new (
                  GTK_WINDOW(self->ui.window)
                  , 0
                  , GTK_MESSAGE_QUESTION
                  , GTK_BUTTONS_OK_CANCEL
                  , "Delete record?");

        int res = gtk_dialog_run (GTK_DIALOG (error_dialog));
        
        /*
        ** delete given record if ansvare is OK
        */
        if(res == GTK_RESPONSE_OK)
        {
            //TODO: remove from DB
            GtkTreeViewColumn *column;
            size_t  num_columns  = gtk_tree_view_get_n_columns(tree_view);
            const char ** columns_list = malloc(sizeof(char*) * num_columns);
            const char ** values_list = malloc(sizeof(char *) * num_columns);

            for (size_t i = 0; i < num_columns; i++) 
            {
                gchar * value;
                column = gtk_tree_view_get_column(tree_view, i);
                const gchar *column_name = gtk_tree_view_column_get_title(column);
                columns_list[i] = column_name;
                gtk_tree_model_get(model, &iter, i, &value, -1);
                values_list[i] = value;
            }

            model_delete_record(
                self->model
                , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(self->ui.combo_table))
                , num_columns
                , columns_list
                , values_list);

            for(size_t i = 0; i < num_columns; i++)
                g_free((void*) values_list[i]);

            free(columns_list);
            free(values_list);

            

            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        }

        gtk_widget_destroy(error_dialog);
    }
}


static gboolean 
view_db_table_view_on_key_press_callback(
    GtkWidget * widget
    , GdkEventKey * event
    , View * view) 
{
    (void) widget;

    /*
    ** if pressed key is DELETE run relete record action
    */
    if (event->keyval == GDK_KEY_Delete) 
        view_delete_record(view);

    return false;
}


static void
signals(View * self)
{
    g_signal_connect(self->ui.btn_login, "clicked", G_CALLBACK(view_btn_login_clicked_callback), self);
    g_signal_connect(self->ui.btn_logout, "clicked", G_CALLBACK(view_btn_logout_clicked_callback), self);
    g_signal_connect(self->ui.combo_table, "changed", G_CALLBACK(view_combo_table_changed_callback), self);
    g_signal_connect(self->ui.btn_filter, "clicked", G_CALLBACK(view_btn_filter_click_callback), self);
    g_signal_connect(self->ui.btn_export, "clicked", G_CALLBACK(view_btn_export_click_callback), self);
    g_signal_connect(self->ui.combo_column, "changed", G_CALLBACK(view_combo_column_changed_callback), self);
    g_signal_connect(self->ui.environment_chart, "draw", G_CALLBACK(view_environment_chart_draw_callback), self);

    g_signal_connect(self->ui.entry_user_name, "activate", G_CALLBACK(view_entry_username_activate_callback), self);
    g_signal_connect(self->ui.entry_password, "activate", G_CALLBACK(view_entry_password_activate_callback), self);

    g_signal_connect(self->ui.db_table_view, "key-press-event", G_CALLBACK(view_db_table_view_on_key_press_callback), self);
}


void
view_set_table_list(View * self)
{
    TableContent * table_list = model_get_table_list(self->model);

    if(table_list != NULL)
    {
        for(size_t i = 0; i < table_list->super.size; i++)
        {
            gtk_combo_box_text_append_text(
                GTK_COMBO_BOX_TEXT(self->ui.combo_table)
                , table_list->array[i]->array[0]);
        }

        array_delete(ARRAY(table_list));  
    }
}


void
view(GtkApplication * app)
{
    O_Ui t;
    Model * model;
    Controller * controller;

    if((model = model_init()) == NULL)
	    return;
    
    if((controller = controller_init(model)) == NULL)
	    return;

    if((t = ui_build("ui/ui.glade")).is_value == false)
	    return;

    int cyclic_interrupt_id = g_timeout_add(1000, view_cyclic_interupt_callback, &v);

    v = View(t.value, model, controller, cyclic_interrupt_id);
    gtk_window_set_application(GTK_WINDOW(v.ui.window), app);

    gtk_window_maximize(GTK_WINDOW(v.ui.window));
    gtk_window_set_title(GTK_WINDOW(v.ui.window), "Webasto Traceability");

    view_set_table_list(&v);        

    signals(&v);
    gtk_widget_show(GTK_WIDGET(v.ui.window));
}



