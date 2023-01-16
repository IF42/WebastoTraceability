#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include "model.h"
#include <stdbool.h>

typedef struct Controller Controller;

Controller *
controller_init(Model * model);


void
controller_run(Controller * self);


void
controller_delete(Controller * self);


#endif
