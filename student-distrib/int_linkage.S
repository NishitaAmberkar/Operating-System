# int_linkage.S - provides assembly linkage interface for calling interrupt handlers
# vim:ts=4 noexpandtab


/* defines the asm linkage wrappers for an interrupt handler -- passed interrupt handler function */
#define INT_LINKAGE(interrupt, interrupt_handler)                 \
    .globl interrupt                                             ;\
    interrupt:                                                   ;\
       cli                                                       ;\
       pushal                                                    ;\
       pushfl                                                    ;\
       call interrupt_handler                                    ;\
       popfl                                                     ;\
       popal                                                     ;\
       sti                                                       ;\
       IRET                                                      

/* declare interrupt handler wrappers through assembly linkage */
INT_LINKAGE (keyboard_handler_link, keyboard_inter_handler) 
INT_LINKAGE (rtc_handler_link, rtc_inter_handler) 
INT_LINKAGE (pit_handler_link, PIT_handler)
