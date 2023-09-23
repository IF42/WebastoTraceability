#ifndef _VIEW_H_
#define _VIEW_H_ 

#include "ui.h"
#include "controller.h"
#include "model.h"

#include <gtk/gtk.h>

typedef struct View View;

View * 
view_new(
    Controller * controller
    , Model * model);

#endif
