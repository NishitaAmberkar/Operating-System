/* keyboard.h - Defines for keyboard functions */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

// not 100% sure if this port assignment is correct, could be x40 or x60
// (as is maybe hinted in the course notes)
#define KB_DATA_PORT   0x60
#define KB_STAT_CMD_PORT 0x64
#define KB_IRQ_NUM     1
#define KB_BUF_SIZE    128
#define KEY_RELEASED   0x80
#define RELEASED_MASK  0x7F
#define EXTENDED_SC    0xE0
#define KEY_PRESS_SIZE 10
#define ASCII_CAP      0x20
#define MAX_SCANCODE   0x58

// special keys
#define ALT_SC         0x38
#define CTRL_SC        0x1D
#define CAPS_LOCK_SC   0x3A
#define LEFT_SHIFT_SC  0x2A
#define RIGHT_SHIFT_SC 0x36

#define F1_SC          0x3B
#define F2_SC          0x3C
#define F3_SC          0x3D

typedef struct keyboard{
    char caps_lock;
    char shift;
    char ctrl;
    char alt;
    //volatile char input_wait;
    //char input_buf[KB_BUF_SIZE];
    //int  buf_index;
}keyboard_t;

keyboard_t keyboard_state;

// initialize keyboard
void keyboard_init();

// keyboard intrupt handler
extern void keyboard_inter_handler();

// clears n elements from the keybaord buffer
int clear_KB_buffer(int n);

// converts scan codes to ascii
char scan_to_ascii(int scanCode);

#endif /* _KEYBOARD_H */
