#include "scheduler.h"
#include "i8259.h"
#include "terminal.h"
#include "syscall_handlers.h"
#include "x86_desc.h"
#include "page.h"

uint32_t scheduler_saved_esp, scheduler_saved_ebp;

/* 
 * scheduler
 *   DESCRIPTION: The function the integrates PIT to switch between processes
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */

void scheduler(void){
    int32_t previous_active_terminal;
    
    // check to see if this was the first process to ever run
    if(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb == NULL){
        if(active_terminals.terminals[active_terminals.current_active_terminal].active == UNUSED){ // check if this was the first time this terminal has been opened
            //active_terminals.terminals[active_terminals.current_active_terminal].active = USED;
            //printf("first terminal call, terminal number: %u\n", active_terminals.current_active_terminal);
            
            asm volatile(          
                "movl %%esp, %0   \n"
                "movl %%ebp, %1   \n"
                :"=r"(scheduler_saved_esp), "=r"(scheduler_saved_ebp)
                : 
                :"memory"
            );

            send_eoi(PIT_IRQ_NUM);
            sys_execute((uint8_t *)"shell");
            printf("Scheduler: --Error-- this should not print, ever\n");
            //return;
        }
        else{
            printf("Scheduling - Error: No current active process\n");
            send_eoi(PIT_IRQ_NUM);
            return;
        }
    }

    //save current process' esp/ebp (do we have to save all registers, I think so)
    asm volatile(          
        "movl %%esp, %0   \n"
        "movl %%ebp, %1   \n"
        :"=r"(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->user_esp), "=r"(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->user_ebp)
        : 
        :"memory"
     );
    
    previous_active_terminal = active_terminals.current_active_terminal;

    active_terminals.current_active_terminal = (active_terminals.current_active_terminal + 1) % NBR_TERMINALS;
    
    // save/swap screen x and y values 
    // active_terminals.terminals[previous_active_terminal].saved_screen_x = get_screen_x(); 
    // active_terminals.terminals[previous_active_terminal].saved_screen_y = get_screen_y();
    //update_screen_x_y(active_terminals.terminals[active_terminals.current_active_terminal].saved_screen_x, active_terminals.terminals[active_terminals.current_active_terminal].saved_screen_y); 
    //  printf("\nSCHEDULER was called! new active terminal is %u showing is %u", active_terminals.current_active_terminal, active_terminals.current_showing_terminal);
    //if new active terminal is running in background switch paging of video memory to terminal video page 
    if(active_terminals.current_active_terminal != active_terminals.current_showing_terminal){
        //page_vir_phy_map((int)VIDEO_MEM_ADDR,(int)(TERMINAL_1_VIDEO_PAGE_ADDR + active_terminals.current_active_terminal*PAGE_SIZE) ,(int)VIDEO_MEM_SIZE);
        page_vir_phy_map  (USER_PAGES_VIR_ADDR_START+SIZE_4MB_PAGE, (TERMINAL_1_VIDEO_PAGE_ADDR + active_terminals.current_active_terminal*PAGE_SIZE), PAGE_SIZE);
        flush_tlb();
        k_page_vir_phy_map(VIDEO_MEM_ADDR, (TERMINAL_1_VIDEO_PAGE_ADDR + active_terminals.current_active_terminal*PAGE_SIZE), PAGE_SIZE);
        flush_tlb();
    }
    else{ // map virtual video memory to main video memory (show up on screen)
        page_vir_phy_map  (USER_PAGES_VIR_ADDR_START+SIZE_4MB_PAGE, VIDEO_MEM_ADDR, PAGE_SIZE);
        flush_tlb();
        k_page_vir_phy_map(VIDEO_MEM_ADDR, VIDEO_MEM_ADDR, PAGE_SIZE);
        flush_tlb();
        update_cursor(active_terminals.terminals[active_terminals.current_active_terminal].screen_x, active_terminals.terminals[active_terminals.current_active_terminal].screen_y);
    }  
    
    // check to see if next terminal is runing a process
    if(active_terminals.terminals[active_terminals.current_active_terminal].active == UNUSED){
        //active_terminals.terminals[active_terminals.current_active_terminal].active = USED;
        //printf("remaining terminal calls, terminal number: %u\n", active_terminals.current_active_terminal);

        asm volatile(          
            "movl %%esp, %0   \n"
            "movl %%ebp, %1   \n"
            :"=r"(scheduler_saved_esp), "=r"(scheduler_saved_ebp)
            : 
            :"memory"
        );
        
        send_eoi(PIT_IRQ_NUM);
        sys_execute((uint8_t *)"shell");
        // return;
    }

    // at this point *next_pcb  is active_terminals.terminals[active_terminals.current_active_terminal].active_pcb;*

    // context switch assembly
    // switch esp/ebp to next process' K stack
    // retore next process' tss
    tss.ss0 = KERNEL_DS; 
    tss.esp0 = _8MB - (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->pid * PCB_MEM_SPACING) - _4B; // offset stack pointer by 4 for pcb pointer

    page_vir_phy_map((int)USER_PAGES_VIR_ADDR_START, (int)(_8MB + active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->pid * SIZE_4MB_PAGE) , 1);
    flush_tlb();

    send_eoi(PIT_IRQ_NUM);
    
    asm volatile(          
        "movl %0, %%esp   \n"
        "movl %1, %%ebp  \n"
        :
        :"r"(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->user_esp), "r"(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->user_ebp) 
        :"esp", "ebp"
     );

    return; // shouldn't really get here
}
