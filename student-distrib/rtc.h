/* rtc.h - Defines functions to set up RTC device
 * vim:ts=4 noexpandtab
 */

#include "types.h"

#ifndef RTC_H
#define RTC_H

#define RTC_IRQ_PIN     8
#define RTC_PORT        0x70
#define CMOS_PORT       0x71
#define DISABLE_NMI     0x80
#define RTC_REG_A       0x0A
#define RTC_REG_B       0x0B
#define RTC_REG_C       0x0C
#define RTC_REG_D       0x0D
#define TURN_ON_BIT_6   0x40

/* initializes the RTC */
extern void rtc_init(void);

/* interrupt handler for RTC */
extern void rtc_inter_handler(void);

/* read system for RTC */
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* write system for RTC */
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

/* open system for RTC */
extern int32_t rtc_open(const uint8_t* filename);

/* close system for RTC */
extern int32_t rtc_close(int32_t fd);

/* sets the rate for the RTC from a frequency */
extern int32_t check_freq(int32_t frequency);

#endif /* RTC_H */
