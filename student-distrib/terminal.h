#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"
#include "syscall_handlers.h"
#include "keyboard.h"

#define IN_BUFF_SIZE 128
#define TERMINAL_1_NBR   0
#define TERMINAL_2_NBR   1
#define TERMINAL_3_NBR   2
#define NBR_TERMINALS    3 

// open terminal 
int terminal_open();

// close terminal 
int terminal_close();

// reads characters from the keyboard buffer
int32_t terminal_read(int32_t fd, void* buf, int32_t n);

// writes n chars in buf to the terminal display 
int32_t terminal_write(int32_t fd, const void* buf, int32_t n);

// switches from one terminal to another
int32_t switch_terminal(int32_t new_terminal_nbr);

// initializes terminal struct elements
void init_terminal(void);

typedef struct __attribute__((packed)) terminal_status{
    pcb_t* active_pcb; 
    volatile int root_pid;
    volatile int active;
    volatile int screen_x;
    volatile int screen_y;
    volatile char KB_buf[KB_BUF_SIZE];
    volatile int buf_index;
    volatile int input_wait;
} terminal_status_t;

typedef struct __attribute__((packed)) terminals{
    volatile int32_t current_showing_terminal; 
    volatile int32_t current_active_terminal;
    terminal_status_t terminals[NBR_TERMINALS];
} terminals_t;

extern terminals_t active_terminals; 

#endif /* _TERMINAL_H */
