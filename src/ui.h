#ifndef _UI_H_
#define _UI_H_

#include <stdbool.h>
#include <gtk/gtk.h>

typedef struct
{
    GtkWidget * window;
    
    GtkWidget * page_stack;

    GtkWidget * entry_user_name;
    GtkWidget * entry_password;
    GtkWidget * btn_login;

    GtkWidget * progress_timeout_logout;
    GtkWidget * btn_logout;
    GtkWidget * btn_export;   

    GtkWidget * combo_table;
    GtkWidget * combo_column;
    GtkWidget * entry_key;
    GtkWidget * btn_filter;
    GtkWidget * db_table_view;    

    GtkWidget * environment_chart;
    GtkWidget * environment_scale;
}Ui;


#define Ui(...)(Ui){__VA_ARGS__}


typedef struct
{
    bool is_value;
    Ui value;
}O_Ui;



O_Ui 
ui_build(char * glade);


void
ui_set_visible_login_page(Ui * self);


void
ui_set_visible_passwd_page(Ui * self);


void 
ui_set_visible_main_page(Ui * self);


#endif
