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


struct View
{
    Ui ui;
    Model * model;
    Controller * controller;
};


#define View(...)(View){__VA_ARGS__}


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
        gtk_list_store_newv(
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

    GtkTreeModel * model = gtk_tree_view_get_model(GTK_TREE_VIEW(db_table_view));

    if(model != NULL)
    {
    	g_object_unref(model);
    	gtk_tree_view_set_model(GTK_TREE_VIEW(db_table_view), NULL);
    }
}


static int
__fill_table_callback(
	void * data
	, int argc
	, char ** argv
	, char ** colNames)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(((View*) data)->ui.db_table_view);
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkListStore *liststore;
    GtkTreeIter iter;

    if (model == NULL)
    {
    	liststore = view_prepare_tree_model(argc);
        gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(liststore));

        for (int i = 0; i < argc; i++)
        {
        	gtk_tree_view_append_column(
        			GTK_TREE_VIEW(treeview)
        	        , view_new_view_column(colNames[i], i));

        	gtk_combo_box_text_append_text(
        	                GTK_COMBO_BOX_TEXT(((View*) data)->ui.combo_column)
        	                , colNames[i]);
        }

		gtk_widget_set_sensitive (((View* ) data)->ui.btn_export, false);
    }
    else
	{
       	liststore = GTK_LIST_STORE(model);
		gtk_widget_set_sensitive (((View*) data)->ui.btn_export, true);
	}

    gtk_list_store_append(liststore, &iter);

    for(int i = 0; i < argc; i++)
        gtk_list_store_set(liststore, &iter, i, argv[i], -1);

    return 0;
}


static void
view_combo_table_changed_callback (
	GtkComboBox * widget
	, View * view)
{
    view_init_logout_timer(view);

    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(view->ui.combo_column));
    view_clear_table_list(view->ui.db_table_view);
    model_db_callback_read(
		view->model
		, __fill_table_callback
		, view
		, "SELECT * FROM '%s' ORDER BY rowid DESC LIMIT 500;"
		, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));

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
view_btn_login_clicked_callback(
	GtkWidget * widget
	, View * view)
{
    (void) widget;
    view_login(view);
}


static void 
view_btn_logout_clicked_callback(
	GtkWidget * widget
	, View * view)
{
    (void) widget;
    view_logout(view);
}


static int
__fill_filtered_table_callback(
	void * data
	, int argc
	, char ** argv
	, char ** colNames)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(((View*)data)->ui.db_table_view);
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    GtkListStore *liststore;
    GtkTreeIter iter;

    if (model == NULL)
    {
    	liststore = view_prepare_tree_model(argc);
        gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(liststore));

        for (int i = 0; i < argc; i++)
        {
        	gtk_tree_view_append_column(
        			GTK_TREE_VIEW(treeview)
        	        , view_new_view_column(colNames[i], i));
        }

		gtk_widget_set_sensitive (((View* ) data)->ui.btn_export, false);
    }
    else
	{
       	liststore = GTK_LIST_STORE(model);
		gtk_widget_set_sensitive (((View*) data)->ui.btn_export, true);
	}

    gtk_list_store_append(liststore, &iter);

    for(int i = 0; i < argc; i++)
        gtk_list_store_set(liststore, &iter, i, argv[i], -1);

    return 0;
}


static void
view_btn_filter_click_callback(
	GtkWidget * widget
	, View * view)
{
    (void) widget;
    view_init_logout_timer(view);
	view_clear_table_list(view->ui.db_table_view);

	model_db_callback_read(
		view->model
		, __fill_filtered_table_callback
		, view
		, "SELECT * FROM %s WHERE %s LIKE '%s' ORDER BY rowid DESC;"
		, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table))
		, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_column))
		, gtk_entry_get_text(GTK_ENTRY(view->ui.entry_key)));

	gtk_entry_set_text(GTK_ENTRY(view->ui.entry_key), "");
}


#if defined(__linux__)
#include <pwd.h>
#endif


static int
export_to_csv_callback(
    void * data
    , int argc
    , char ** argv
    , char ** azColName)
{
    FILE * f = (FILE *) data;

    if(f == NULL)
        return -1;

    if (ftell(f) == 0) 
	{
        for (int i = 0; i < argc; i++) 
            fprintf(f, i == 0 ? "%s" : ", %s", azColName[i]);

        fprintf(f, "\n");
    }

	for (int i = 0; i < argc; i++)
        fprintf(f, i == 0 ? "%s": ", %s", argv[i]);

    fprintf(f, "\n");
    return 0;
}


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
		char sql[512];

        filename = gtk_file_chooser_get_filename (chooser);
		sprintf(
			sql
			, "SELECT * FROM %s;"
			, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table)));
		
        FILE * f = fopen(filename, "w");

        if(f == NULL
			|| model_db_callback_read(
					view->model
					, export_to_csv_callback
					, f
					, "SELECT * FROM %s;"
					, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table))) == false)
        {
            /*
            ** error dialog if it is not possible to open file for writing
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

		if(f != NULL)
			fclose(f);
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
            , "%02lld:%02lld"
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


static DataPoint data[] =
{
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
        ** delete given record if answer is OK
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
    ** if pressed key is DELETE run delete record action
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
    g_signal_connect(G_OBJECT(self->ui.window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}


int
_set_table_list(
	void *data
	, int argc
	, char **argv
	, char **colNames)
{
	(void) argc;
	(void) colNames;

	gtk_combo_box_text_append_text(
	                GTK_COMBO_BOX_TEXT(data)
	                , argv[0]);

	return 0;
}


void
view_set_table_list(View * self)
{
	model_db_callback_read(
			self->model
			, _set_table_list
			, self->ui.combo_table
			, "SELECT name FROM sqlite_schema WHERE type='table' AND name NOT LIKE \"sqlite_%\";");
}


static View instance;
static View * ptr;

View *
view_new(
    Controller * controller
    , Model * model)
{
    if(ptr == NULL)
    {
        O_Ui ui;
        
        if((ui = ui_build("ui/ui.glade")).is_value == false)
            return NULL;

        instance = View(ui.value, model, controller);
       
        g_timeout_add(1000, view_cyclic_interupt_callback, &instance);

        gtk_window_maximize(GTK_WINDOW(instance.ui.window));
        gtk_window_set_title(GTK_WINDOW(instance.ui.window), "Webasto Traceability");

        view_set_table_list(&instance);        

        signals(&instance);
        gtk_widget_show(GTK_WIDGET(instance.ui.window));

        ptr = &instance;
    }

    return ptr;
}



