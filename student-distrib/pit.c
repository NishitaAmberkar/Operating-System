#include "pit.h"
#include "i8259.h"
#include "scheduler.h"
#include "lib.h"

/* Information about ports seen from OSDEV- https://wiki.osdev.org/PIT */

/*
 * PIT_set_div
 *   DESCRIPTION: sends the bits to channel 0 according to the timer
 *   INPUTS: divisor- time delay between interrupts
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void PIT_set_div(uint16_t divisor){
    outb(divisor & 0xFF, PIT_CHANNEL_0_PORT);         // send lower byte
    outb((divisor & 0xFF00 )<< 8, PIT_CHANNEL_0_PORT); // send upper byteest
    return;
}

/*
 * PIT_init
 *   DESCRIPTION: initialise the PIT
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void PIT_init(void){
    outb(PIT_CMD_INIT, PIT_COMMAND_REG_PORT);         // send lower byte
    PIT_set_div(PIT_20MS_DIV); // set to 20ms quanta
    enable_irq(PIT_IRQ_NUM);
    return;
}

void PIT_handler(void){
    scheduler();
    // will schedualling really return? I don't think so
    // // !!call EOI in scheduling!!
    // printf("Im in pit handler\n");
    send_eoi(PIT_IRQ_NUM);
    sti();
    return;
}
