#ifndef PIT_H
#define PIT_H

#include "types.h"

#define PIT_CHANNEL_0_PORT   0x40
#define PIT_CHANNEL_1_PORT   0x41
#define PIT_CHANNEL_2_PORT   0x42
#define PIT_COMMAND_REG_PORT 0x43
#define PIT_FREQUENCY        1193182
#define PIT_20MS_DIV         23863 // period*PIT_FREQUENCY

// command reg:
// { 2'b channel , 2'b Access Mode , 3'b Operating Mode , 1'b BDC mode}

#define PIT_CMD_INIT  0x34 // {00, 11, 010, 0} = channel 0, low then high byte access, rate gen mode, 16-bit mode
#define PIT_IRQ_NUM   0

#define MAX_F_DIV            65535

#define PIT_OSC_FREQ         1193182 //=Hz

void PIT_set_div(uint16_t divisor);

void PIT_init(void);

extern void PIT_handler(void);

#endif /* PIT_H */
