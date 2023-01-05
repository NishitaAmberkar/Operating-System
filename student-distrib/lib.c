/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "keyboard.h"
#include "terminal.h"

#define VIDEO       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25
//#define ATTRIB      0x2 // cool hacker green color
#define SKIP_ATTRIB 1

#define CRTC_CONTROLLER_ADDR_PORT 0x3D4
#define CRTC_CONTROLLER_DATA_PORT 0x3D5
#define CURSOR_START_REG          0x0A
#define CURSOR_END_REG            0x0B
#define CURSOR_START              0
#define CURSOR_END                15
#define CURSOR_DISABLE_BIT        0x20
#define MASK_LOWER_5_LSB          0xC0
#define MASK_LOWER_4_LSB          0xC0
#define LSBYTE_MASK               0xFF
#define CURSOR_LOC_LOW            0x0F
#define CURSOR_LOC_HIGH           0x0E
#define BYTE_SHIFT                8


//static int screen_x;
//static int screen_y;
static char* video_mem = (char *)VIDEO;
static char terminal_colors[NBR_TERMINALS] = {TERM_1_COLOR, TERM_2_COLOR, TERM_3_COLOR};

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory  */
void clear(void) {

    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = terminal_colors[active_terminals.current_showing_terminal];
    }
    //screen_x = 0;
    //screen_y = 0;
    active_terminals.terminals[active_terminals.current_showing_terminal].screen_x = 0;
    active_terminals.terminals[active_terminals.current_showing_terminal].screen_y = 0;
    update_cursor(active_terminals.terminals[active_terminals.current_showing_terminal].screen_x, active_terminals.terminals[active_terminals.current_showing_terminal].screen_y);
}

/* void clear_terminal_video_page;
 * Inputs: video_page_addr - pointer to the memory location to modify 
 *         attrib - color pallete index
 * Return Value: none
 * Function: Clears video memory for terminals  */
void clear_terminal_video_page(char* video_page_addr, uint8_t attrib) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_page_addr + (i << 1)) = ' ';
        *(uint8_t *)(video_page_addr + (i << 1) + 1) = attrib;
    }
}

/* void terminal_color_edit;
* Inputs: video_page_addr - pointer to the memory location to modify 
 *        attrib - color pallete index
 * Return Value: none
 * Function: allows user to set a new terminal color  */
void terminal_color_edit(void){
    char color;
    int ret_val;
    char color_return[MAX_FILENAME_LEN];
    printf("Color options:\n0 - black\n1 - blue\n2 - green\n3 - teal\n4 - red\n5 - purple\n6 - orange\n7 - white\n");
    printf("8 - grey\n9 - blue/purple (idk looks good)\nA - light green\nB - cyan\nC - coral\nD - pink\n");
    printf("E - yellow\nF - white\n");
    printf("select background color: ");
    if(terminal_read(0, (void*)color_return, MAX_FILENAME_LEN) != 1){
        printf("Error - invalid color\n");
        return;
    }

    ret_val = char_to_int(color_return[0]);

    if(ret_val == -1){
        printf("Error - invalid color\n");
        return;
    }
    color = ret_val<<4; // shift by 2 to make space for the text color

    printf("\nselect text color: ");
    if(terminal_read(0, (void*)color_return, MAX_FILENAME_LEN) != 1){
        printf("%c\n",color_return[0]);
        printf("Error - invalid color\n");
        return;
    }
    ret_val = char_to_int(color_return[0]);

    if(ret_val == -1){
        printf("Error - invalid color %u\n", ret_val);
        return;
    }
    color &= ret_val;
    terminal_colors[active_terminals.current_showing_terminal] = color;
    set_terminal_color(video_mem, color);
}

/* int char_to_int;
 * Inputs: c - char to convert to an int
 * Return Value: int value from the hex char
 *               -1 error, input invalid
 * Function: converts char to an int  */
int char_to_int(char c){
    int i;
    char lookup[16] = "023456789ABCDE";
    for(i = 0; i > 16; i++){
        if(lookup[i] == c)
            return i;
    }
    return -1;
}

/* void set_terminal_color;
* Inputs: video_page_addr - pointer to the memory location to modify 
 *        attrib - color pallete index
 * Return Value: none
 * Function: Clears video memory for terminals  */
void set_terminal_color(char* video_page_addr, uint8_t attrib){
    int32_t i;
    // update terminal color
    terminal_colors[active_terminals.current_showing_terminal] = attrib;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_page_addr + (i << 1) + 1) = attrib;
    }
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%' , active_terminals.current_active_terminal);
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp), active_terminals.current_active_terminal);
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf, active_terminals.current_active_terminal);
                break;
        }
        buf++;

    }

    return (buf - format);
}

 /* Inputs: uint_8* c = character to print
  Return Value: void
   Function: Output a character to the console */
// void printf_showing_term(int8_t *format, ...)  {

//     if(active_terminals.current_showing_terminal == active_terminals.current_active_terminal){
//         printf(*format, ...);
//         return;
//     }
// }
/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;

    while (s[index] != '\0') {
        putc(s[index], active_terminals.current_active_terminal);
        
        index++;
    }
    

    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc(uint8_t c, int32_t term_nbr) {
    // printf("INSIDE putc: trying to write to current active terminal %u\n", term_nbr);
    if((term_nbr < 0) || (term_nbr > 2))
        return;

    if(c == '\0'){
        return;
        //goto retrieve_flags;
    }
    
    if(c == '\n' || c == '\r') { // new line or return
        if(active_terminals.terminals[term_nbr].screen_y == NUM_ROWS-1){
            scroll_screen_down(term_nbr);
        }
        else{ 
            active_terminals.terminals[term_nbr].screen_y++;
            active_terminals.terminals[term_nbr].screen_x = 0;
        }
    }
    else if(c == '\b'){ // backspace
        if((active_terminals.terminals[term_nbr].screen_y == 0) && (active_terminals.terminals[term_nbr].screen_x == 0)){ // if at (0,0) do nothing
            return;
            //goto retrieve_flags;
        }
        else if(active_terminals.terminals[term_nbr].screen_x == 0){      // if at x = 0 then decrement y
            active_terminals.terminals[term_nbr].screen_y--;              // decrement y location
            active_terminals.terminals[term_nbr].screen_x = NUM_COLS-1;   // set x to the furtheset to the right     
        }
        else{
            active_terminals.terminals[term_nbr].screen_x--;
        }
        *(uint8_t *)(video_mem + ((NUM_COLS * active_terminals.terminals[term_nbr].screen_y + active_terminals.terminals[term_nbr].screen_x) << SKIP_ATTRIB)) = ' ';
    }
    else if (c == '\t'){ // tab
        putc(' ' , term_nbr);       // print 4 spaces
        putc(' ' , term_nbr);
        putc(' ' , term_nbr);
        putc(' ' , term_nbr);
    }
    else {
        *(uint8_t *)(video_mem + ((NUM_COLS * active_terminals.terminals[term_nbr].screen_y + active_terminals.terminals[term_nbr].screen_x) << SKIP_ATTRIB)) = c;
        if(active_terminals.terminals[term_nbr].screen_x < NUM_COLS)
            active_terminals.terminals[term_nbr].screen_x++;
        else{               // at the end of a line, move to next line
            active_terminals.terminals[term_nbr].screen_x = 0;
            if(active_terminals.terminals[term_nbr].screen_y == NUM_ROWS-1)
                scroll_screen_down(term_nbr);
            else
                active_terminals.terminals[term_nbr].screen_y++;
        }
    } 

//retrieve_flags:
    // update the cursor if showing
    if(term_nbr == active_terminals.current_showing_terminal){
        update_cursor(active_terminals.terminals[term_nbr].screen_x, active_terminals.terminals[term_nbr].screen_y);
    }
////    active_terminals.terminals[active_terminals.current_active_terminalsaved_].saved_screen_x = screen_x;
////    active_terminals.terminalactive_terminals.current_active_terminal_tsaved_erminals.current_active_terminal].saved_screen_y = screen_y;
    //active_terminals.terminalactive_terminals.current_active_terminalald_screen_x = screen_x;
    //active_terminals.terminalterm_nbrald_screen_y = screen_y; 
}

/* scroll_screen_down();
 * Inputs: none
 * Return Value: void
 * Function: Scrolls the screen down by one row */
void scroll_screen_down(int32_t term_nbr){
    int i;
    for(i = 0; i < (NUM_ROWS-1)*NUM_COLS; i++){                                                                   // skip copying to the last row as there is nothing to copy 
        *(uint8_t *)(video_mem + (i << SKIP_ATTRIB)) = *(uint8_t *)(video_mem + ((NUM_COLS + i) << SKIP_ATTRIB)); // copy over values from next row (shifting by one to skip the pallet index part)
    }
    for(i = (NUM_ROWS-1)*NUM_COLS; i < NUM_ROWS*NUM_COLS; i++){ // write spaces to the last row
        *(uint8_t *)(video_mem + (i << SKIP_ATTRIB)) = ' ';
    }
    //screen_x = 0;   //update screen y values
    active_terminals.terminals[term_nbr].screen_x = 0;
    
    if(term_nbr == active_terminals.current_showing_terminal){
        update_cursor(active_terminals.terminals[term_nbr].screen_x, active_terminals.terminals[term_nbr].screen_y);
    }
}

/* cursor functionalities obtained from OSDev - https://wiki.osdev.org/Text_Mode_Cursor and OSDever-http://www.osdever.net/FreeVGA/vga/crtcreg.htm#0E */

/* cursor_enable();
 * Inputs: none
 * Return Value: void
 * Function: Enables the cursor in the logical view window and shows it */
void cursor_enable(){
    outb(CURSOR_START_REG, CRTC_CONTROLLER_ADDR_PORT);
    outb(((inb(CRTC_CONTROLLER_DATA_PORT)&MASK_LOWER_5_LSB) | CURSOR_START), CRTC_CONTROLLER_DATA_PORT);
    outb(CURSOR_END_REG, CRTC_CONTROLLER_ADDR_PORT);
    outb(((inb(CRTC_CONTROLLER_DATA_PORT)&MASK_LOWER_4_LSB) | CURSOR_END), CRTC_CONTROLLER_DATA_PORT);
}

/* update_cursor();
 * Inputs: none
 * Return Value: takes in the x and y coordinates of the screen
 * Function: Updates the cursor according to the given coordinates as input*/
void update_cursor(int x, int y){
    if(active_terminals.current_active_terminal != active_terminals.current_showing_terminal)
       return; 

    uint16_t pos;
    pos= y*NUM_COLS+x;
    outb(CURSOR_LOC_LOW, CRTC_CONTROLLER_ADDR_PORT);
	outb((uint8_t) (pos & LSBYTE_MASK), CRTC_CONTROLLER_DATA_PORT);
	outb(CURSOR_LOC_HIGH, CRTC_CONTROLLER_ADDR_PORT);
	outb((uint8_t) ((pos >> BYTE_SHIFT) & LSBYTE_MASK), CRTC_CONTROLLER_DATA_PORT);
}

/* disable_cursor();
 * Inputs: none
 * Return Value: void
 * Function: Removes the cursor */
void cursor_disable(){
    if(active_terminals.current_active_terminal != active_terminals.current_showing_terminal)
       return; 
       
    outb(CURSOR_START_REG, CRTC_CONTROLLER_ADDR_PORT);
	outb(CURSOR_DISABLE_BIT, CRTC_CONTROLLER_DATA_PORT);
}

/* end of cursor functionalities */

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {


    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';

        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {


    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}

int get_screen_x(void){
    return active_terminals.terminals[active_terminals.current_showing_terminal].screen_x; 
}
int get_screen_y(void){
    return active_terminals.terminals[active_terminals.current_showing_terminal].screen_y; 
}

void update_screen_x_y(int new_screen_x, int new_screen_y){
    active_terminals.terminals[active_terminals.current_showing_terminal].screen_x = new_screen_x; 
    active_terminals.terminals[active_terminals.current_showing_terminal].screen_y = new_screen_y; 
}
