#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "lib.h"
#include "pit.h"

extern uint32_t scheduler_saved_esp, scheduler_saved_ebp;

void scheduler(void);
#endif /* SCHEDULER_H */
