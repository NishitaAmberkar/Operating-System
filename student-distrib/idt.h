/* lib.h - Defines functions to set up the IDT
 * vim:ts=4 noexpandtab
 */

#ifndef IDT_H
#define IDT_H

#define EXCEPT_NUM          32  //number of reserved expections (first 32 entries in IDT)
#define KEYBOARD_IDT_INDEX  0x21
#define PIT_IDT_INDEX       0x20
#define RTC_IDT_INDEX       0x28  
#define SYSCALL_IDT_INDEX   0x80
#define USER_MODE_PREV      3
#define KERNEL_MODE_PREV    0

/* initiliaze the IDT by loading entries for exceptions, interrupts and system calls */
void idt_init();

/* interrupt handler for pit handler through assembly linkage */
void pit_handler_link(); 

/* interrupt handler for Keyboard through assembly linkage */
void keyboard_handler_link();

/* interrupt handler for RTC through assembly linkage */
void rtc_handler_link();

/* generic system call handler defined through ASM LINKAGE in syscall_linkage.S */ 
extern void syscall_generic_handler(void);

/* dummy Exception handlers : prints exception name and infinite loops*/ 
void DE_expt_handler();
void DB_expt_handler();
void NMI_inter_handler();
void BP_expt_handler();
void OF_expt_handler();
void BR_expt_handler();
void UD_expt_handler();
void NM_expt_handler();
void DF_expt_handler();
void COP_expt_handler();
void TS_expt_handler();
void NP_expt_handler();
void SS_expt_handler();
void GP_expt_handler();
void PF_expt_handler();
void MF_expt_handler();
void AC_expt_handler();
void MC_expt_handler();
void XF_expt_handler();

#endif   /* IDT_H */
