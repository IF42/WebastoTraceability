#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include "model.h"
#include <stdbool.h>

typedef struct Controller Controller;

Controller *
controller_new(Model * model);


void
controller_delete(Controller * self);


#endif
