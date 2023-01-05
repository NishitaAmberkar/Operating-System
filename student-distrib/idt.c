/* idt.c - Defines functions to set up the IDT
 * vim:ts=4 noexpandtab
 */
 
#include "idt.h"
#include "lib.h"
#include "x86_desc.h"
#include "keyboard.h"

/*
 * idt_init
 *   DESCRIPTION: initiliaze the IDT by loading entries for exceptions, 
 *                 interrupts and system calls
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void idt_init(void){
	int i;  
    //First: load kernel reserved exceptions (vector nbr 0 to 31) 
   // set up interrupt gate bits
	for(i  = 0; i <EXCEPT_NUM; i++){	
        idt[i].dpl = KERNEL_MODE_PREV;  // set exceptions to have kernel privelege 
        idt[i].present = 1; // validate the descriptor entry
		idt[i].seg_selector = KERNEL_CS; 
		idt[i].size = 1;
		// fill in reserved bits to match interrupt gate bits
        idt[i].reserved0 = 0;
		idt[i].reserved1 = 1;
		idt[i].reserved2 = 1;
		idt[i].reserved3 = 0;
		idt[i].reserved4 = 0;
	}

    // load offset fields for expections IDT entries
	SET_IDT_ENTRY(idt[0], DE_expt_handler);
	SET_IDT_ENTRY(idt[1], DB_expt_handler);
	SET_IDT_ENTRY(idt[2], NMI_inter_handler);
	SET_IDT_ENTRY(idt[3], BP_expt_handler);
	SET_IDT_ENTRY(idt[4], OF_expt_handler);
	SET_IDT_ENTRY(idt[5], BR_expt_handler);
	SET_IDT_ENTRY(idt[6], UD_expt_handler);
	SET_IDT_ENTRY(idt[7], NM_expt_handler);
	SET_IDT_ENTRY(idt[8], DF_expt_handler);
	SET_IDT_ENTRY(idt[9], COP_expt_handler);
	SET_IDT_ENTRY(idt[10], TS_expt_handler);
	SET_IDT_ENTRY(idt[11], NP_expt_handler);
	SET_IDT_ENTRY(idt[12], SS_expt_handler);
	SET_IDT_ENTRY(idt[13], GP_expt_handler);
	SET_IDT_ENTRY(idt[14], PF_expt_handler);
	SET_IDT_ENTRY(idt[16], MF_expt_handler);  //skipped nbr 15 <== intel defined
	SET_IDT_ENTRY(idt[17], AC_expt_handler);
	SET_IDT_ENTRY(idt[18], MC_expt_handler);
	SET_IDT_ENTRY(idt[19], XF_expt_handler);    //skipped  20-31
	

    //SECOND: load user defined interrupts (RTC and Keyboard)
	
/* PIT INTERRUPT DESCRIPTOR */
        idt[PIT_IDT_INDEX].dpl = KERNEL_MODE_PREV;  // set exceptions to have kernel privelege 
        idt[PIT_IDT_INDEX].present = 1; // validate the descriptor entry
		idt[PIT_IDT_INDEX].seg_selector = KERNEL_CS; 
		idt[PIT_IDT_INDEX].size = 1;
		// fill in reserved bits to match interrupt gate bits
        idt[PIT_IDT_INDEX].reserved0 = 0;
		idt[PIT_IDT_INDEX].reserved1 = 1;
		idt[PIT_IDT_INDEX].reserved2 = 1;
		idt[PIT_IDT_INDEX].reserved3 = 0;
		idt[PIT_IDT_INDEX].reserved4 = 0;
        SET_IDT_ENTRY(idt[PIT_IDT_INDEX], pit_handler_link);  //set pointer to handler through assembly linkage

    /* KEYBOARD INTERRUPT DESCRIPTOR */
        idt[KEYBOARD_IDT_INDEX].dpl = KERNEL_MODE_PREV;  // set exceptions to have kernel privelege 
        idt[KEYBOARD_IDT_INDEX].present = 1; // validate the descriptor entry
		idt[KEYBOARD_IDT_INDEX].seg_selector = KERNEL_CS; 
		idt[KEYBOARD_IDT_INDEX].size = 1;
		// fill in reserved bits to match interrupt gate bits
        idt[KEYBOARD_IDT_INDEX].reserved0 = 0;
		idt[KEYBOARD_IDT_INDEX].reserved1 = 1;
		idt[KEYBOARD_IDT_INDEX].reserved2 = 1;
		idt[KEYBOARD_IDT_INDEX].reserved3 = 0;
		idt[KEYBOARD_IDT_INDEX].reserved4 = 0;
        SET_IDT_ENTRY(idt[KEYBOARD_IDT_INDEX], keyboard_handler_link);  //set pointer to handler through assembly linkage

    /* RTC INTERRUPT DESCRIPTOR */
        idt[RTC_IDT_INDEX].dpl = KERNEL_MODE_PREV;  // set user interrupts to have kernel privelege 
        idt[RTC_IDT_INDEX].present = 1; // validate the descriptor entry
		idt[RTC_IDT_INDEX].seg_selector = KERNEL_CS; 
		idt[RTC_IDT_INDEX].size = 1;
		// fill in reserved bits to match interrupt gate bits
        idt[RTC_IDT_INDEX].reserved0 = 0;
		idt[RTC_IDT_INDEX].reserved1 = 1;
		idt[RTC_IDT_INDEX].reserved2 = 1;
		idt[RTC_IDT_INDEX].reserved3 = 0;
		idt[RTC_IDT_INDEX].reserved4 = 0;
        SET_IDT_ENTRY(idt[RTC_IDT_INDEX], rtc_handler_link); //set pointer to handler through assembly linkage


    //Third: Load system calls (x80)
    
        idt[SYSCALL_IDT_INDEX].dpl = USER_MODE_PREV;  // set system calls to have user privelege (so they can be called from used mode) 
        idt[SYSCALL_IDT_INDEX].present = 1; // validate the descriptor entry
		idt[SYSCALL_IDT_INDEX].seg_selector = KERNEL_CS; // setting segment delector field to kernel's code segment description from Appendix D
		idt[SYSCALL_IDT_INDEX].size = 1;
		// fill in reserved bits to match interrupt gate bits
        idt[SYSCALL_IDT_INDEX].reserved0 = 0;
		idt[SYSCALL_IDT_INDEX].reserved1 = 1;
		idt[SYSCALL_IDT_INDEX].reserved2 = 1;
		idt[SYSCALL_IDT_INDEX].reserved3 = 0;
		idt[SYSCALL_IDT_INDEX].reserved4 = 0;
	    SET_IDT_ENTRY(idt[SYSCALL_IDT_INDEX], syscall_generic_handler);//set pointer to sys call function

		

}



/* DUMMY EXCEPTION HANDLER FUNCTIONS */
/*
 * DE_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void DE_expt_handler() {
	printf("\n-- 0- DIVIDE BY ZERO EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * DB_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void DB_expt_handler() {
	printf("\n-- 1- RESERVED EXCEPTION (vect no 01) OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * NMI_inter_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void NMI_inter_handler() {
	printf("\n-- 2- NMI INTERRUPT OCCURED! -- \n");
    while(1){}; //infinite loop 
}
/*
 * BP_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void BP_expt_handler() {
	printf("\n-- 3- BREAKPOINT EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * OF_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void OF_expt_handler() {
	printf("\n-- 4- OVERFLOW EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * BR_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void BR_expt_handler() {
	printf("\n-- 5-BOUND RANGE EXCEEDED EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * UD_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void UD_expt_handler() {
	printf("\n-- 6-UNDEFINED OPCODE EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}/*
 * NM_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void NM_expt_handler() {
	printf("\n-- 7-DEVICE NOT AVAILABLE EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}/*
 * DF_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void DF_expt_handler() {
	printf("\n-- 8-DOUBLE FAULT EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}/*
 * COP_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void COP_expt_handler() {
	printf("\n-- 9-COPROCESSOR OVERRUN EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}/*
 * TS_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void TS_expt_handler() {
	printf("\n-- 10-INVALID TSS EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * NP_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void NP_expt_handler() {
	printf("\n-- 11-SEGMENT NOT PRESENT EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * SS_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void SS_expt_handler() {
	printf("\n-- 12- STACK SEGMENT EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * GP_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void GP_expt_handler() {
	printf("\n-- 13-GENERAL PROTECTION EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * PF_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void PF_expt_handler() {
	printf("\n-- 14-PAGE FAULT EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * MF_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void MF_expt_handler() {
	printf("\n-- 16-FLOATING POINT EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * AC_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void AC_expt_handler() {
	printf("\n-- 17-ALIGNMENT CHECK EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
/*
 * MC_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void MC_expt_handler() {
	printf("\n-- 18-MACHINE CHECK EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}/*
 * XF_expt_handler
 *   DESCRIPTION: Dummy to show the appropriate interrupt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void XF_expt_handler() {
	printf("\n-- 19-SIMD FLOATING POINT EXCEPTION OCCURED! -- \n");
    while(1){}; //infinite loop
}
