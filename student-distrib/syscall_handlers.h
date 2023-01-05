#ifndef _SYSCALL_HANDLERS_H
#define _SYSCALL_HANDLERS_H

#include "types.h"

#define MAX_OPEN_FILES    8

// assigned to flags of fd array entries to show if they are used or no
#define USED              1
#define UNUSED            0

#define MAX_PROCESS_CNT   6

#define PCB_MEM_SPACING   0x2000                //8K
#define _8MB              0x800000
#define USER_IMG_ADDR     0x08048000
#define SET_IF            0x0200

#define STDIN_FD          0
#define STDOUT_FD         1
#define _4B               4

#define EIP_FILE_LOC      24
#define MAX_FILELENGTH    100000
#define MAX_FILENAME_LEN  32  
#define MAX_ARG_LEN       128  
#define START_USER_FILES  2

/*  SYSTEM CALL HANDLERS INVOKED BY KERNEL FROM syscall_linkage.S  */
/* halts the currently executing user program */
int32_t sys_halt (uint8_t status);
/* execute a user program that is executable  */
int32_t sys_execute (const uint8_t* command);
/* read from an open file in current process  */
int32_t sys_read (int32_t fd, void* buf, int32_t nbytes);
/* write to an open file in current process  */
int32_t sys_write (int32_t fd, const void* buf, int32_t nbytes);
/*open a file in current process  */
int32_t sys_open (const uint8_t* filename);
/*close an open file in current process  */
int32_t sys_close (int32_t fd);
/* copies the arguments to current process PCB  */
int32_t sys_getargs (uint8_t* buf, int32_t nbytes);
/* maps the video memory for user program  */
int32_t sys_vidmap (uint8_t** screen_start);
/* TDB  */
int32_t sys_set_handler (int32_t signum, void* handler);
/* TBD  */
int32_t sys_sigreturn (void);

/* bad calls for terminal open and close */ 
int32_t open_bad_call (const uint8_t* fname);
int32_t close_bad_call (int32_t fd);
int32_t read_bad_call(int32_t fd, void* buf, int32_t nbytes);
int32_t write_bad_call(int32_t fd, const void* buf, int32_t nbytes);

/* PCB and related structs */ 
/* FILE OPERATIONS JUMP TABLE */
typedef struct __attribute__((packed)) file_op_table {
    int32_t (*open)(const uint8_t* fname);
    int32_t (*close)(int32_t fd);
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
} file_op_table_t;

/* FILE DESCRIPTOR ARRAY ENTRY*/
typedef struct __attribute__((packed)) file_desc_arr_entry{
    file_op_table_t* file_op_table_ptr; 
    uint32_t inode_num;
    uint32_t file_position;
    uint32_t flags;
} fd_arr_entry_t;

/* PCB */
typedef struct __attribute__((packed)) pcb {
    uint32_t pid;

    void* parent_pcb;  //pointer to parent pcb
    fd_arr_entry_t fd_arr[MAX_OPEN_FILES]; // fd array for this process
    uint32_t file_inode_nbr;     //inode of exectuble file
    //saved parent registers
    uint32_t user_ebp;     //to go back to parent stack frame
    uint32_t user_esp;
    
    uint32_t tss_esp0;     //saves tss_esp0 of this process
    uint8_t  active;       //if this an active process
    uint8_t arg[MAX_ARG_LEN]; // argument array
    
} pcb_t;


// global variable holdring address to current process' PCB
extern pcb_t* current_process_pcb; // pointer to pcb of current process


#endif /* _SYSCALL_HANDLERS_H */
