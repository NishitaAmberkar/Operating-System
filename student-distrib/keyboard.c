/* keyboard.c - functions to support keyboard use */

#include "keyboard.h"
#include "terminal.h"
#include "lib.h"
#include "i8259.h"
#include "page.h"

// scan codes to ascii LUT
char scanCode_ascii[MAX_SCANCODE] = 
    {
     0 ,  27, '1', '2', '3', '4', '5', '6', '7', '8',  // first space is blank as scancode[0] doesn't exist
    '9', '0', '-', '=','\b','\t', 'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']','\n',   0, // skip left ctrl
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`',  0,'\\', 'z', 'x', 'c', 'v', 'b', 'n', // skip left shift
    'm', ',', '.', '/',   0, '*',   0, ' ',   0,   0, // skip right shift, left alt, capslock, F1
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // skip F2-10, numlock
      0,   0,   0,   0,   0,   0,   0,   0
    };

char scanCode_ascii_extended[MAX_SCANCODE] = 
    {
     0 ,  27, '!', '@', '#', '$', '%', '^', '&', '*',  // first space is blank as scancode[0] doesn't exist
    '(', ')', '_', '+','\b','\t', 'Q', 'W', 'E', 'R', 
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}','\n',   0, // skip left ctrl
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '\"', '~',  0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', // skip left shift
    'M', '<', '>', '?',   0, '*',   0, ' ',   0,   0, // skip right shift, left alt, capslock, F1
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // skip F2-10, numlock
      0,   0,   0,   0,   0,   0,   0,   0
    };

/* 
 * keyboard_init
 *   DESCRIPTION: Initialize the keyboard and enable its intrupt on the PIC
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: None
 *   SIDE EFFECTS: PIC keybaord irq enabled
 * Notes: call i8259_init before enabling inturupts or mask off PIC INT before calling
 */
void keyboard_init(){
    // enable KB inturupts on PIC
    enable_irq(KB_IRQ_NUM);
    
    keyboard_state.caps_lock = 0;
    // that's all we should have to do here... 
}

/* 
 * keyboard_inter_handler
 *   DESCRIPTION: handles keybaord inturpts 
 *   INPUTS: None
 *   OUTPUTS: none
 *   RETURN VALUE: None
 *   SIDE EFFECTS: 
 */
void keyboard_inter_handler(){ 
    cli();
    // printf("Im in interrupt handler\n");
    char pressed;
	int scanCode;
    int32_t original_terminal = active_terminals.current_active_terminal; 
    int32_t potential_previous_showing  = active_terminals.current_showing_terminal;
    // with irq dirven polling, no need to check KB status reg before reading
	scanCode = inb(KB_DATA_PORT);

    // if the scan code was part of the extended set, do nothing
    // this may cause issues for the next SC read... come back to this
    //if (scanCode == EXTENDED_SC){
    //    send_eoi(KB_IRQ_NUM);
    //    return;
    //}

    // deal with all of the specal characters 
    if(scanCode == CTRL_SC)
        keyboard_state.ctrl = 1;
    else if(scanCode == (CTRL_SC | KEY_RELEASED))
        keyboard_state.ctrl = 0;
    else if (scanCode == ALT_SC)
        keyboard_state.alt = 1;
    else if (scanCode == (ALT_SC | KEY_RELEASED))
        keyboard_state.alt = 0;
    else if (scanCode == CAPS_LOCK_SC)
        keyboard_state.caps_lock = ~keyboard_state.caps_lock;  // flip state if pressed
    else if (scanCode == LEFT_SHIFT_SC)
        keyboard_state.shift = 1;
    else if (scanCode == (LEFT_SHIFT_SC| KEY_RELEASED))
        keyboard_state.shift = 0;
    else if (scanCode == RIGHT_SHIFT_SC)
        keyboard_state.shift = 1;
    else if (scanCode == (RIGHT_SHIFT_SC | KEY_RELEASED))
        keyboard_state.shift = 0;

    pressed = scan_to_ascii(scanCode);    // convert scan code to ascii

    
    if(active_terminals.current_active_terminal != active_terminals.current_showing_terminal){
        k_page_vir_phy_map(VIDEO_MEM_ADDR, VIDEO_MEM_ADDR, VIDEO_MEM_SIZE);
        flush_tlb();
        //update_screen_x_y(active_terminals.terminals[active_terminals.current_showing_terminal].screen_x, active_terminals.terminals[active_terminals.current_showing_terminal].screen_y); 
        update_cursor(active_terminals.terminals[active_terminals.current_showing_terminal].screen_x, active_terminals.terminals[active_terminals.current_showing_terminal].screen_y);
        
        active_terminals.current_active_terminal = active_terminals.current_showing_terminal;
    }
      
   // CTRL + l should clear the screen 
    if((keyboard_state.ctrl == 1)){
        //clear the screen
        if(((pressed|ASCII_CAP) == 'l')){
            clear();
            clear_KB_buffer(KB_BUF_SIZE);
            goto restore;
        }
        else if(scanCode == F1_SC){
            send_eoi(KB_IRQ_NUM);
            active_terminals.current_active_terminal = original_terminal;
            switch_terminal(TERMINAL_1_NBR);
            goto restore;
        }
        else if(scanCode == F2_SC){
            send_eoi(KB_IRQ_NUM);
            active_terminals.current_active_terminal = original_terminal;
            switch_terminal(TERMINAL_2_NBR); 
            goto restore;
        }
        else if(scanCode == F3_SC){
            send_eoi(KB_IRQ_NUM);
            active_terminals.current_active_terminal = original_terminal;
            switch_terminal(TERMINAL_3_NBR);
            goto restore;
        }
        // else if(pressed == 't'){ // change the terminal color
        //     send_eoi(KB_IRQ_NUM);
        //     terminal_color_edit();
        //     goto restore;
        // }
    }
    if((keyboard_state.ctrl == 1) && (pressed == 'p')){ // debug input buff print, please use only after enter has been pressed or buf is full
        printf((int8_t*)active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf);
        send_eoi(KB_IRQ_NUM);
        goto restore;
    }

    // change the value of the pressed key based on if caps lock is pressed
	if(keyboard_state.caps_lock){ 
        if(((pressed >= 'A') && (pressed <= 'Z')) || ((pressed >= 'a') && (pressed <= 'z')))
            pressed ^= ASCII_CAP; // flips the 6th bit 
    }

    // just echo back to the terminal for now, don't print if ctrl is pressed 
    if(pressed != 0){
        if(pressed == '\b'){
            if(active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf[active_terminals.terminals[active_terminals.current_showing_terminal].buf_index-1] == '\t'){ // if clearing tab, delete 4 items form the screen (3 extra times)
                putc('\b', active_terminals.current_showing_terminal);
                putc('\b', active_terminals.current_showing_terminal);
                putc('\b', active_terminals.current_showing_terminal);
            }
            if(clear_KB_buffer(1) == 0) // check for failed removal
                putc('\b', active_terminals.current_showing_terminal); // this should prevent you from clearing the whole screen with backspace
                
            goto restore;
        
        }

        if(pressed == '\n'){
            if(active_terminals.terminals[active_terminals.current_showing_terminal].input_wait){ // if someone was waitnig for the input, let them know data is ready
                active_terminals.terminals[active_terminals.current_showing_terminal].input_wait = 0;
            }
            else{
                clear_KB_buffer(KB_BUF_SIZE); // else throw it away
                putc(pressed, active_terminals.current_showing_terminal);
                goto restore;
        
            }
        }

        if(active_terminals.terminals[active_terminals.current_showing_terminal].buf_index < KB_BUF_SIZE-2){ // only write to buffer if not full, reserve space for a new line
            putc(pressed, active_terminals.current_showing_terminal); // print char to screen
            active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf[active_terminals.terminals[active_terminals.current_showing_terminal].buf_index] = pressed;
            active_terminals.terminals[active_terminals.current_showing_terminal].buf_index++;
        }
        else{ // input buffer is full, do what you must 
            // do nothing 
            if(pressed == '\n'){
                putc(pressed, active_terminals.current_showing_terminal);
                active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf[active_terminals.terminals[active_terminals.current_showing_terminal].buf_index] = pressed;
                active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf[active_terminals.terminals[active_terminals.current_showing_terminal].buf_index+1] = '\0';
                //printf("buf_index after overflow = %u\n", active_terminals.terminals[active_terminals.current_showing_terminal].buf_index);
            }
            goto restore;
            // active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf[active_terminals.terminals[active_terminals.current_showing_terminal].buf_index] = '\n'; // force last char in buffer to be new line
            // putc('\n'); // print new line as well 
            
            // if(active_terminals.terminals[active_terminals.current_showing_terminal].input_wait)
            //     active_terminals.terminals[active_terminals.current_showing_terminal].input_wait = 0;
            // else
            //     clear_KB_buffer(KB_BUF_SIZE);
            // printf("Error: Keyboard input buffer overflow"); // maybe just prevent someone from continueing to type - TA
        }
    }

restore: 
    if(original_terminal != potential_previous_showing){
        k_page_vir_phy_map(VIDEO_MEM_ADDR, (TERMINAL_1_VIDEO_PAGE_ADDR + original_terminal*PAGE_SIZE), VIDEO_MEM_SIZE);
        flush_tlb();
        if(potential_previous_showing == active_terminals.current_showing_terminal){
            //update_screen_x_y(active_terminals.terminals[original_terminal].screen_x, active_terminals.terminals[original_terminal].screen_y); 
        }
    active_terminals.current_active_terminal = original_terminal;
    }
    else if(potential_previous_showing != active_terminals.current_showing_terminal){
    k_page_vir_phy_map(VIDEO_MEM_ADDR, (TERMINAL_1_VIDEO_PAGE_ADDR + potential_previous_showing*PAGE_SIZE), VIDEO_MEM_SIZE);
    flush_tlb();
    }

	// send EOI to PIC
	send_eoi(KB_IRQ_NUM);
}

/* clear_KB_buffer
 *   DESCRIPTION: clears n elements from the keyboard buffer and display 
 *   INPUTS: n - number of elements to remove from the buffer
 *   OUTPUTS: none
 *   RETURN VALUE: the number of characters it failed to remove
 *   NOTES: does not remove things from the display, to clear the whole buffer just pass it's size for n
 */
int clear_KB_buffer(int n){
    int i;
    
    for(i = n; i > 0; i--){
        active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf[active_terminals.terminals[active_terminals.current_showing_terminal].buf_index] = '\0';
        if(active_terminals.terminals[active_terminals.current_showing_terminal].buf_index != 0){
            active_terminals.terminals[active_terminals.current_showing_terminal].buf_index--;
        }
        else{
            active_terminals.terminals[active_terminals.current_showing_terminal].KB_buf[active_terminals.terminals[active_terminals.current_showing_terminal].buf_index] = '\n';
            return i;
        }  
    }
    return i;
}

/* 
 * keyboard_init
 *   DESCRIPTION: Helper function to quickly convert scancode to ascii.
 *                it will retun the ascii value to be displayed on screen
 *                or 0 if the scancode was outside the valid bounds of the
 *                scancode look up table (ignores release scan codes)
 *   INPUTS: scanCode - scan code directly from the keybaord
 *   OUTPUTS: none 
 *   RETURN VALUE: the scan code's ascii equivalent
 */
/* returns the ascii value */
char scan_to_ascii(int scanCode){
    // check bounds of LUT
    if (scanCode > MAX_SCANCODE)
        return 0;
    return keyboard_state.shift ? scanCode_ascii_extended[scanCode] : scanCode_ascii[scanCode];
}
