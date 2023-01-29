#include "view.h"
#include "ui.h"
#include "model.h"
#include "login.h"
#include "controller.h"

#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>


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
            , "Unsuccessful attempt to login [%s, %s]"
            , username, password);
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
 
            free(db_table_content->array[i]->array[j]); 
        }

        free(db_table_content->array[i]);
    }

    free(db_table_content);
}


static void
view_combo_table_changed_callback (
  GtkComboBox* widget
  , View * view)
{
    view_init_logout_timer(view);

    ArrayString * column_list = 
        model_get_table_columns(
            view->model
            , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget)));

    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(view->ui.combo_column));
    view_clear_table_list(view->ui.db_table_view);

    GtkListStore * table_view_model = 
        view_prepare_tree_model(column_list->super.size);

    if(column_list != NULL)
    {
        for(size_t i = 0; i < column_list->super.size; i++)
        {
            gtk_combo_box_text_append_text(
                GTK_COMBO_BOX_TEXT(view->ui.combo_column)
                , column_list->array[i]);

            gtk_tree_view_append_column(
                GTK_TREE_VIEW(view->ui.db_table_view)
                , view_new_view_column(column_list->array[i], i));

            free(column_list->array[i]);
        }
        
        gtk_tree_view_set_model(
            GTK_TREE_VIEW(view->ui.db_table_view)
            , GTK_TREE_MODEL(table_view_model));
        gtk_combo_box_set_active(GTK_COMBO_BOX(view->ui.combo_column), 0);

        free(column_list);
    }
    
    TableContent * db_table_content = 
        model_get_table_content(
            view->model
            , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget))
            , "*"
            , NULL
            , NULL);
    view_fill_db_table_view (table_view_model, db_table_content);

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

    ArrayString * column_list = 
        model_get_table_columns(
            view->model
            , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table)));

    view_clear_table_list(view->ui.db_table_view);

    for(size_t i = 0; i < column_list->super.size; i++)
    {
        gtk_tree_view_append_column(
            GTK_TREE_VIEW(view->ui.db_table_view)
            , view_new_view_column(column_list->array[i], i));

        free(column_list->array[i]);
    }

    GtkListStore * table_view_model = 
        view_prepare_tree_model(column_list->super.size);

    gtk_tree_view_set_model(
        GTK_TREE_VIEW(view->ui.db_table_view)
        , GTK_TREE_MODEL(table_view_model));

    TableContent * db_table_content = 
        model_get_table_content(
            view->model 
            , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_table))
            , "*"
            , gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(view->ui.combo_column))
            , (char*) gtk_entry_get_text(GTK_ENTRY(view->ui.entry_key)));

    view_fill_db_table_view (table_view_model, db_table_content);
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

    #ifdef __linux__
        sprintf(
            str_timeout
            , "%02ld:%02ld"
            , self->model->logout_timer / 60, self->model->logout_timer % 60);
    #elif defined(WIN32) || defined(WIN64)
        sprintf(
            str_timeout
            , "%02lld:%02lld"
            , self->model->logout_timer / 60, self->model->logout_timer % 60);
    #endif 

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
    cairo_set_source_rgba(cr, 0, 0, 0, 1);

    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_set_line_width(cr, 1);

    cairo_move_to(cr, 50, 50);

    for (int i = 0; i < 5; i++) 
    {
        cairo_line_to(cr, 50 + i * 50, 50 + data[i].temperature);
    }

    cairo_stroke(cr);

    cairo_set_source_rgba(cr, 0, 0, 1, 1);
    cairo_move_to(cr, 50, 50);

    for (int i = 0; i < 5; i++) 
    {
        cairo_line_to(cr, 50 + i * 50, 50 + data[i].humidity);
    }

  cairo_stroke(cr);
}

static gboolean 
view_environment_chart_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation); 

  draw_graph(cr, allocation);

  return true;
}





static void
signals(View * self)
{
    g_signal_connect(self->ui.btn_login, "clicked", G_CALLBACK(view_btn_login_clicked_callback), self);
    g_signal_connect(self->ui.btn_logout, "clicked", G_CALLBACK(view_btn_logout_clicked_callback), self);
    g_signal_connect(self->ui.combo_table, "changed", G_CALLBACK(view_combo_table_changed_callback), self);
    g_signal_connect(self->ui.btn_filter, "clicked", G_CALLBACK(view_btn_filter_click_callback), self);
    g_signal_connect(self->ui.combo_column, "changed", G_CALLBACK(view_combo_column_changed_callback), self);
    g_signal_connect(self->ui.environment_chart, "draw", G_CALLBACK(view_environment_chart_draw_callback), self);

    g_signal_connect(self->ui.entry_user_name, "activate", G_CALLBACK(view_entry_username_activate_callback), self);
    g_signal_connect(self->ui.entry_password, "activate", G_CALLBACK(view_entry_password_activate_callback), self);
}


void
view_set_table_list(View * self)
{
    ArrayString * table_list = model_get_table_list(self->model);

    if(table_list != NULL)
    {
        for(size_t i = 0; i < table_list->super.size; i++)
        {
            gtk_combo_box_text_append_text(
                GTK_COMBO_BOX_TEXT(self->ui.combo_table)
                , table_list->array[i]);
        }

        array_delete(ARRAY(table_list));  
    }
}


void
view(GtkApplication * app)
{
    O_Ui t  = ui_build("ui/ui.glade");
    Model * model = model_init();      
    Controller * controller = controller_init(model);

    if(t.is_value == true 
        && model != NULL
        && controller != NULL)
    {
        int cyclic_interrupt_id = g_timeout_add(1000, view_cyclic_interupt_callback, &v);

        v = View(t.value, model, controller, cyclic_interrupt_id);
        gtk_window_set_application(GTK_WINDOW(v.ui.window), app);

        gtk_window_maximize(GTK_WINDOW(v.ui.window));
        gtk_window_set_title(GTK_WINDOW(v.ui.window), "Webasto Traceability");

        view_set_table_list(&v);        

        signals(&v);
        
        gtk_widget_show(GTK_WIDGET(v.ui.window));
    }

    /* treat error */
}









