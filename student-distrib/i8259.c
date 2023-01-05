/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* 
 * i8259_init
 *   DESCRIPTION: Initialize the 8259 PIC and setup enabled irq mask
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: None
 *   SIDE EFFECTS: PIC initialized
 * Notes: call i8259_init before enabling inturupts or mask off PIC INT before calling
 */
void i8259_init(void) {

    // initialize the masks 
    master_mask = DISABLE_IRQ_MASK_MASTER;
    slave_mask  = DISABLE_IRQ_MASK_SLAVE;

    outb(DISABLE_IRQ_MASK_SLAVE,  MASTER_8259_PORT | SET_A0_8259);
    outb(DISABLE_IRQ_MASK_SLAVE,  SLAVE_8259_PORT  | SET_A0_8259);

    // send all 4 ICWs to master
    outb(ICW1,        MASTER_8259_PORT);               // Edge trigger mode, call address interval = 8, cascade mode, ICW4 needed
    outb(ICW2_MASTER, MASTER_8259_PORT | SET_A0_8259); // IR0-7 mapped to 0x20-0x27
    outb(ICW3_MASTER, MASTER_8259_PORT | SET_A0_8259); // Secondary PIC on pin IRQ2
    outb(ICW4,        MASTER_8259_PORT | SET_A0_8259); // 8086 mode, normal EOI

    // // send all 4 ICWs to slave 8259
    outb(ICW1,       SLAVE_8259_PORT);                     
    outb(ICW2_SLAVE, SLAVE_8259_PORT  | SET_A0_8259); // IR0-7 mapped to 0x28-0x2F
    outb(ICW3_SLAVE, SLAVE_8259_PORT  | SET_A0_8259); // PIC is on pin 2 of master
    outb(ICW4,       SLAVE_8259_PORT  | SET_A0_8259); 

    // write out the inturput masks (disable all ports)
    outb(master_mask,  MASTER_8259_PORT | SET_A0_8259);
    outb(slave_mask,   SLAVE_8259_PORT  | SET_A0_8259);

}

/* 
 * enable_irq
 *   DESCRIPTION: Writes a new mask to the PICs to enable irqs (one at a time)
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: None
 * Notes: Also updates the correct saved mask in the operation! 
 */
void enable_irq(uint32_t irq_num) {
    unsigned char temp_mask;

    if(irq_num < 8){ // first 7 irq
        temp_mask = 0x01<<irq_num;
        master_mask &= ~temp_mask;
        outb(master_mask, MASTER_8259_PORT | SET_A0_8259);
    }
    else if (irq_num < 16){  // 16: max number of IRQ lines
        temp_mask = 0x01<<(irq_num&7); // &7 to mask upper bits
        slave_mask &= ~temp_mask;
        outb(slave_mask, SLAVE_8259_PORT | SET_A0_8259);
    }

}

/* 
 * disable_irq
 *   DESCRIPTION: Disable (mask) the specified IRQ
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: None
 * Notes: Also updates the correct saved mask in the operation! 
 */
void disable_irq(uint32_t irq_num) {
    unsigned char temp_mask;

    if(irq_num < 8){ // first 7 irq
        temp_mask = 0x01<<irq_num;
        master_mask |= temp_mask;
        outb(master_mask, MASTER_8259_PORT | SET_A0_8259);
    }
    else if (irq_num < 16){ // 16: max number of IRQ lines
        temp_mask = 0x01<<(irq_num&7); // &7 to mask upper bits
        slave_mask |= temp_mask;
        outb(slave_mask, SLAVE_8259_PORT | SET_A0_8259);
    }

}

/* 
 * send_eoi
 *   DESCRIPTION: Send end-of-interrupt signal for the specified IRQ 
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: None
 */
void send_eoi(uint32_t irq_num) {
    
    if (irq_num < 8) // first 7 irq
        outb((EOI | irq_num),  MASTER_8259_PORT);
    else if (irq_num < 16){  // 16: max number of IRQ lines
        outb((EOI | (irq_num & 7)),  SLAVE_8259_PORT); // &7 to mask upper bits
        outb((EOI | 2),  MASTER_8259_PORT);  // 2: where slave PIC is connected
    }
    else
        printf("send_eoi error: eoi irq number is invalid");

}
