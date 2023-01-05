/* rtc.c - Defines functions to set up RTC device
 * vim:ts=4 noexpandtab
 */

#include "rtc.h"
#include "lib.h"
#include "types.h"
#include "i8259.h"

#define RTC_RATE_2  0x0F
#define RTC_RATE_4  0x0E
#define RTC_RATE_8  0x0D
#define RTC_RATE_16  0x0C
#define RTC_RATE_32  0x0B
#define RTC_RATE_64  0x0A
#define RTC_RATE_128  0x09
#define RTC_RATE_256  0x08
#define RTC_RATE_512  0x07
#define RTC_RATE_1024  0x06

#define RTC_FREQ_2  2
#define RTC_FREQ_4  4
#define RTC_FREQ_8  8
#define RTC_FREQ_16  16
#define RTC_FREQ_32  32
#define RTC_FREQ_64  64
#define RTC_FREQ_128  128
#define RTC_FREQ_256  256
#define RTC_FREQ_512  512
#define RTC_FREQ_1024  1024

#define UPPER_4_BITS_MASK  0xF0
#define LOWER_4_BITS_MASK  0x0F

volatile unsigned int interrupt_flag = 0; // keeps track of the interrupts
volatile unsigned int init_flag = 0;  // checks if the rtc is initialized

/*
 * rtc_init
 *   DESCRIPTION: initiliazes the RTC (aknowledgement: wiki.osdev.org/RTC)
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void rtc_init(void){

  outb(RTC_REG_B|DISABLE_NMI, RTC_PORT); // select register RTC's Register B + disable NMI
  char B_val = inb(CMOS_PORT); // read current value of register B
  outb(RTC_REG_B|DISABLE_NMI, RTC_PORT); 
  outb( B_val | TURN_ON_BIT_6, CMOS_PORT);  //write the prev value read
  

  /*The default interrupt rate is 2Hz */
  int rtc_rate = RTC_RATE_2;  
  outb(RTC_REG_A|DISABLE_NMI, RTC_PORT);  // select register RTC's Register A + disable NMI
  char A_val = inb(CMOS_PORT);  // read current value of register A
  outb(RTC_REG_A|DISABLE_NMI, RTC_PORT);
  outb((A_val & UPPER_4_BITS_MASK) | rtc_rate, CMOS_PORT); //write the prev value read

  enable_irq(RTC_IRQ_PIN); //activate irq pin associated with RTC

  init_flag = 1;  // rtc is initialized
  interrupt_flag = 0; // sets interrupt flag to 0
}

/*
 *  rtc_inter_handler
 *   DESCRIPTION: defines the RTC interrupt handler which is called by 
 *                IDT entry associated with RTC interrupt through asm linkage
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void rtc_inter_handler(void){
  interrupt_flag = 1; // sets interrupt flag to 1; handles interrupts
  outb(RTC_REG_C, RTC_PORT); //read register C after interrupt is handle to make sure interrupts happens again
  inb(CMOS_PORT);
  send_eoi(RTC_IRQ_PIN); //sends eoi signal to PIC to unmask lower priority interrupts
  // printf("                                           Im in RTC handler\n");

}

/*
 *  rtc_read
 *   DESCRIPTION: defines the RTC read system which returns 0 only when the interrupt
 *                handler is called
 *   INPUTS: fd  - file descriptor index
 *           buf - buffer for interrupt rate (Hz)
 *           nbytes - number of bytes
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - SUCCESS
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
  /** For the real-time clock (RTC), this call should always return 0, 
   * but only after an interrupt has occurred 
   * (set a flag and wait until the interrupt handler clears it, then return 0) **/
  interrupt_flag = 0;
  sti();
  while (!interrupt_flag) {}  // waits for an interrupt to occur
  cli();
  return 0;
}

/*
 *  check_freq
 *   DESCRIPTION: checks the RTC frequency and sets the required rate
 *   INPUTS: frequency - the current RTC frequency
 *   OUTPUTS: none
 *   RETURN VALUE: rate - the rate of the RTC frequency
 */
int32_t check_freq(int32_t frequency) {
  int32_t rate;

  /* Checks the required frequency and sets it its specific rate;
  the frequencies acceptable by the RTC run in multiples of 2 from 
  2 to 1024 */
  if (frequency == RTC_FREQ_2) {
    rate = RTC_RATE_2;
  } else if (frequency == RTC_FREQ_4) {
    rate = RTC_RATE_4;
  } else if (frequency == RTC_FREQ_8) {
    rate = RTC_RATE_8;
  } else if (frequency == RTC_FREQ_16) {
    rate = RTC_RATE_16;
  } else if (frequency == RTC_FREQ_32) {
    rate = RTC_RATE_32;
  } else if (frequency == RTC_FREQ_64) {
    rate = RTC_RATE_64;
  } else if (frequency == RTC_FREQ_128) {
    rate = RTC_RATE_128;
  } else if (frequency == RTC_FREQ_256) {
    rate = RTC_RATE_256;
  } else if (frequency == RTC_FREQ_512) {
    rate = RTC_RATE_512;
  } else if (frequency == RTC_FREQ_1024) {
    rate = RTC_RATE_1024;
  } else {
    rate = -1;
  }

  return rate;
}

/*
 *  rtc_write
 *   DESCRIPTION: defines the rtc write system, accepts a 4 byte integer
 *                that specifies the interrupt rate to set period interrupts
 *   INPUTS: fd  - file descriptor index
 *           buf - buffer for interrupt rate (Hz)
 *           nbytes - number of bytes
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - SUCCESS/ -1 - FAILURE
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
  int32_t rtc_rate;
  int32_t rtc_freq;

  /* boundary conditions: checks if the buf is null or the number of bytes are not 4 */
  if (nbytes != 4 || buf == NULL) return -1;

  rtc_freq = *(int32_t*)buf;
  rtc_rate = check_freq(rtc_freq);    // obtains the required rate from the frequency passed

  if(rtc_rate < 0){   // checks if the rtc rate is >1024 and returns -1
    return -1;
  }

  rtc_rate &= LOWER_4_BITS_MASK;		// ensures that the rate is above 2 and less than 15

  cli();
  outb(RTC_REG_A|DISABLE_NMI, RTC_PORT);  // select register RTC's Register A + disable NMI
  char A_val = inb(CMOS_PORT); // read current value of register A
  outb(RTC_REG_A|DISABLE_NMI, RTC_PORT);
  outb((A_val & 0xF0) | rtc_rate, CMOS_PORT); // write the prev value
  sti();
  return 0;
}

/*
 *  rtc_open
 *   DESCRIPTION: defines the rtc open system, call rtc_init to initialize the rtc
 *   INPUTS: filename  - name of the file
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - SUCCESS
 */
int32_t rtc_open(const uint8_t* filename) {
  if (!init_flag) rtc_init();   // calls the rtc_init to initialize the rtc if not done
  return 0;
}

/*
 *  rtc_close
 *   DESCRIPTION: defines the rtc close system, returns 0
 *   INPUTS: fd  - file descriptor index
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - SUCCESS
 */
int32_t rtc_close (int32_t fd) {
  return 0;
}
