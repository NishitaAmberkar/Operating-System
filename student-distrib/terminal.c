/* terminal.c - terminal driver functions */

#include "terminal.h"
#include "keyboard.h"
#include "lib.h"
#include "tests.h"
#include "page.h"

//int32_t current_showing_terminal = 0; //global variable that holds which terminal is currently showing on screen ie. linked to main video memory (0,1 or 2)
//int saved_screen_coord[NBR_TERMINALS][2]= {{0,0}, {0,0}, {0,0}}; //2-D array that saves most recent screen coordinates for each terminal
terminals_t active_terminals;

/* 
 * terminal_open
 *   DESCRIPTION: enables the cursor and resets the input buffer index
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: 0 = success 
 *   SIDE EFFECTS: disables the cursor 
 */
int terminal_open(){
    cursor_enable();
    return 0;
}

/* 
 * terminal_close
 *   DESCRIPTION: disables the cursor
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: -1 = sucesses
 *   SIDE EFFECTS: disables the cursor
 */
int terminal_close(){
    cursor_disable();
    return -1;
}

/* 
 * terminal_read
 *   DESCRIPTION: copies the keybaourd input buffer into the pointed to buffer 
 *   INPUTS: fd  - file descriptor index
 *           buf - char buffer to copy the input buffer into
 *           n   - maximum number of elements to copy
 *   OUTPUTS: none
 *   RETURN VALUE: 0 = sucesses, -1 = fail
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t n){
    if(buf == NULL)
        return -1;
    //printf("INSIDE terminal read: trying to write to current active terminal %u\n", active_terminals.current_active_terminal);
    active_terminals.terminals[active_terminals.current_active_terminal].input_wait = 1; // set wait flag
    clear_KB_buffer(KB_BUF_SIZE);  // clear the Keybaord buffer to prep for input
    sti();
    while(active_terminals.terminals[active_terminals.current_active_terminal].input_wait){} // wait for a new line char to show up (or buf overflow)
    cli();
    
    strncpy((int8_t*)buf,(const int8_t*) active_terminals.terminals[active_terminals.current_active_terminal].KB_buf, n);
    clear_KB_buffer(KB_BUF_SIZE);

    if(buf == NULL)
        return -1; 
    return strlen(buf);
}

/* 
 * terminal_write
 *   DESCRIPTION: prints n characters to the terminal 
 *   INPUTS: fd  - file descriptor index
 *           buf - char buffer containing the chars to display
 *           n   - number of chars to display
 *   OUTPUTS: none
 *   RETURN VALUE: 0 = sucesses, -1 = fail
 *   SIDE EFFECTS: disables the cursor
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t n){
    int i;
    if(buf == NULL)
        return -1;
    // printf("\n terminal trying to terminal write is %u\n", active_terminals.current_active_terminal);
    for(i = 0; i < n; i++){
        //printf("TERMINAL WRITE:current active terminal %u current showing terminal is %u \n",active_terminals.current_active_terminal, active_terminals.current_showing_terminal);
        putc((uint8_t) *((uint8_t*)buf + i), active_terminals.current_active_terminal);}
    return n;
}

/* 
 * switch_terminal
 *   DESCRIPTION: switches terminals
 *   INPUTS: int32_t new_terminal_nbr
 *   OUTPUTS: none
 *   RETURN VALUE: 0 = sucesses, -1 = fail
 *   SIDE EFFECTS: 
 */

int32_t switch_terminal(int32_t new_terminal_nbr){
    if(new_terminal_nbr<TERMINAL_1_NBR || new_terminal_nbr> TERMINAL_3_NBR)
       return -1; 
    //check if terminal requested is same as terminal showing on screen
    if(new_terminal_nbr == active_terminals.current_showing_terminal)
      return -2;

    // remap video memory location to for copy
    k_page_vir_phy_map(VIDEO_MEM_ADDR, VIDEO_MEM_ADDR, VIDEO_MEM_SIZE);
    flush_tlb();
    //step1: save active main video page to corresponding current showing terminal video
           //also save current screen coordinates     
    memcpy((void*)(TERMINAL_1_VIDEO_PAGE_ADDR + active_terminals.current_showing_terminal*PAGE_SIZE), (const void*)VIDEO_MEM_ADDR, PAGE_SIZE); 
    // active_terminals.terminals[active_terminals.current_showing_terminal].saved_screen_x = get_screen_x();
    // active_terminals.terminals[active_terminals.current_showing_terminal].saved_screen_y = get_screen_y();
        //step2: copy requested terminal video page to main video page
            //also retrieve saved cursor location and update showing terminal global variable
    memcpy((void*)VIDEO_MEM_ADDR, (const void*)(TERMINAL_1_VIDEO_PAGE_ADDR + new_terminal_nbr*PAGE_SIZE), PAGE_SIZE); 
    //printf("saved terminal coordinates of terminal nbr %u are %u and %u", new_terminal_nbr,active_terminals.terminals[new_terminal_nbr].saved_screen_x,active_terminals.terminals[new_terminal_nbr].saved_screen_y );
    //update_screen_x_y(active_terminals.terminals[new_terminal_nbr].saved_screen_x, active_terminals.terminals[new_terminal_nbr].saved_screen_y); 
    update_cursor(active_terminals.terminals[new_terminal_nbr].screen_x, active_terminals.terminals[new_terminal_nbr].screen_y); 
    
    active_terminals.current_showing_terminal = new_terminal_nbr;

    //modify paging ie. ??? maybe for scheduling
    return 0; //for success
}

/* 
 * init_terminal
 *   DESCRIPTION: set default values for each terminal  
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: sets 0 or NULL to the fileds in the terminal struct
 */
void init_terminal(void){
    int i,j; 
        active_terminals.current_showing_terminal=0;
        active_terminals.current_active_terminal=0;

    for(i=0; i<NBR_TERMINALS; i++){
        active_terminals.terminals[i].active_pcb = NULL; 
        active_terminals.terminals[i].active = UNUSED;
        active_terminals.terminals[i].screen_x=0;
        active_terminals.terminals[i].screen_y=0;
        active_terminals.terminals[i].buf_index=0;
        active_terminals.terminals[i].input_wait=0;
    for(j=0; j<KB_BUF_SIZE; j++){
        active_terminals.terminals[i].KB_buf[j]= '\0';
    }
    }
}

