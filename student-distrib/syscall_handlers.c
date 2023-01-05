/* syscall_handlers.c - defines system call handlers invoked by the kernel
 */

#include "syscall_handlers.h"
#include "lib.h"
#include "filesystem.h"
#include "rtc.h"
#include "terminal.h"
#include "page.h"
#include "x86_desc.h"
#include "tests.h"
#include "scheduler.h"

// define file operation tables for each type of file
static file_op_table_t op_table_reg_file = {open_file, close_file, read_file, write_file};
static file_op_table_t op_table_dir_file = {open_dir, close_dir, read_dir, write_dir};
static file_op_table_t op_table_rtc_file = {rtc_open, rtc_close, rtc_read, rtc_write};
//static file_op_table_t op_table_terminal = {open_bad_call, close_bad_call, terminal_read, terminal_write};
static file_op_table_t op_table_stdin    = {open_bad_call, close_bad_call, terminal_read, write_bad_call};
static file_op_table_t op_table_stdout   = {open_bad_call, close_bad_call, read_bad_call, terminal_write};

static int pcb_allocated[MAX_PROCESS_CNT]; // keeps track of PIDs allocated
//pcb_t* current_process_pcb = NULL; //keeps track of current process's pcb
//static int first_time_called = 1;

/*
 * sys_halt
 *   DESCRIPTION: halts the currently executing user program
 *   INPUTS: status: dummy argument
 *   OUTPUTS: none
 *   SIDE EFFECTS: flushes tlb
 *   RETURN VALUE: 0 
 */
int32_t sys_halt(uint8_t status){
     pcb_t* parent_process; 
     pcb_t* current_process_pcb;
     
     //printf("\n I am trying to halt in terminal %u \n", active_terminals.current_active_terminal);
     current_process_pcb = active_terminals.terminals[active_terminals.current_active_terminal].active_pcb;
     //printf("\n pid of active terminal pcb: %u", current_process_pcb->pid);

     if (current_process_pcb == NULL){
          return - 1;
     }
     parent_process = (pcb_t*)(current_process_pcb->parent_pcb);
     // close all the fd arrays of current pcb (that were open)
     int i;

     // set terminal read and write files to closed
     for(i = 0; i < START_USER_FILES; i++) {
      if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[i].flags == USED)
     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[i].flags = UNUSED; 
     }
     
     for(i = START_USER_FILES; i < MAX_OPEN_FILES; i++) {
             sys_close(i); //call close on open files
     }
     current_process_pcb->active = UNUSED;
     // remove current process from active tracking
     pcb_allocated[current_process_pcb->pid] = 0;

     if (current_process_pcb->parent_pcb == NULL) {
          // save esp and ebp to prevent overwite
          scheduler_saved_esp = active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->user_esp;
          scheduler_saved_ebp = active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->user_ebp;
          // reset active flags to initalized state
          active_terminals.terminals[active_terminals.current_active_terminal].active_pcb = NULL;
          active_terminals.terminals[active_terminals.current_active_terminal].active = UNUSED;
          printf("Cannot halt base shell\n");
          sys_execute((uint8_t*)"shell");
     }
     //printf("Halt: passed parrent NULL check\n");
   
     // unmap current process + map parent process
     page_vir_phy_map((int)USER_PAGES_VIR_ADDR_START, (int)(_8MB + parent_process->pid * SIZE_4MB_PAGE) , 1); // allocate 4k page at 128MB
     //page_vir_phy_unmap(USER_PAGES_VIR_ADDR_START+SIZE_4MB_PAGE); // unmap user video memory if perviously mapped
     flush_tlb();
     
     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb = parent_process;
     
     //printf("\nrestored user ebp and esp %x and %x: \n", parent_process->user_ebp, parent_process->user_esp);
     //printf("\nrestored tss esp0: %x\n", parent_process->tss_esp0);
     //update relevent TSS info
     //tss.ss0 = KERNEL_DS; 
     tss.esp0 = parent_process->tss_esp0; // - 4; // offset stack pointer by 4 for pcb pointer
     
     asm volatile(
        "movl %0, %%esp    \n"
        "movl %1, %%ebp    \n"  
        :
        :"r"(parent_process->user_esp), "r"(parent_process->user_ebp)
        :"eax"
     );

     asm volatile(
        "movl %0, %%eax    \n"  
        "jmp  execute_end  \n"
        :
        :"r"((uint32_t)status)
        :"eax"
     );

     return 0;
}

/*
 * sys_execute
 *   DESCRIPTION: execute a user program that is executable 
 *   INPUTS: command: a pointer which contains information regarding files and arguments 
 *   OUTPUTS: none
 *   SIDE EFFECTS: assigns pcbs and pages, flushes tlb
 *   RETURN VALUE: 0 (success) or -1 (failure)
 */
int32_t sys_execute(const uint8_t *command){
     pcb_t* my_parent;
     //intercept user return address and user top of stack(esp) pushed by hardware context switch
     // uint32_t user_return_addr =0, user_curr_esp= 0, saved_esp =0, top_ret_addr=0, top_saved_reg;
     // uint32_t edx, ecx, ebx, ebp, edi, esi, eflags, my_flags;
     uint32_t my_flags;
     cli();


     uint8_t filename[MAX_FILENAME_LEN]; // put filename extracted here and set buffer length to match exactly that of file
     uint8_t args[MAX_ARG_LEN];
     uint32_t filename_len, i, free_pid;
     uint32_t arg_start;
     uint8_t eip_buf[_4B]; // buffer to hold the 32bit eip pointer from the file
     uint32_t instructions_start, user_stack;
     pcb_t* new_pcb;
     dentry_t file_dentry;

     if (command == NULL){
          return -1;
     }
     
     arg_start = 0;
     uint32_t command_len =  strlen((int8_t*) command) ; 
     for (filename_len = 0; filename_len < command_len; filename_len ++){
          if (command[filename_len] == ' '){
               arg_start = filename_len + 1;
               break;
          }
     }

     //printf("%u", filename_len);
     if ((filename_len == 0) || (filename_len > MAX_FILENAME_LEN)){
          printf("Execute: --Error-- Invalid file name condition\n");
          return -1;
     }
     strncpy((int8_t*)filename, (int8_t*)command, filename_len);

     //copy arg data into local 
     if(arg_start != 0){
          strncpy((int8_t*)args, (int8_t*)(command+arg_start), command_len-arg_start);
          args[command_len-arg_start] = '\0'; //terminate it with a null character 
     }

     int index;
     for(index= filename_len; index <MAX_FILENAME_LEN; index ++ ) //zero padding filename
          filename[index] = '\0';

     // check if file passed is valid: present and executable
     if(is_file_executable(filename)){ // returns 0 if executable
          printf("file: \"%s\" is not excecutable\n");
          return -1;                   // not executable
     }

     if(read_dentry_by_name((uint8_t*)filename, &file_dentry) == -1){
          printf("Error, file not found in dentries\n");
          return -1;
     }
     
     //read instruction start from file
     if(read_data(file_dentry.inode_num, (uint32_t)EIP_FILE_LOC, eip_buf, (uint32_t)_4B) != _4B){ // 4 = read all 4B of the EIP pointer EIP_FILE_LOC
          printf("Error reading eip from file\n"); 
          return -1;
     }
     instructions_start = *(uint32_t*)eip_buf;
     free_pid = -1;

     // find an empty memory location for the PCB and set it up (don't allow it to be stopped)
     for(i = 0; i < MAX_PROCESS_CNT; i++){
          if(pcb_allocated[i] == 0){
               pcb_allocated[i] = 1;
               free_pid = i;
               break;
          }
     }
     //printf("free_pid = %d\n", free_pid);
     if(free_pid == -1){  // if no free fd is found
          printf("Error, maximum number of processes already running\n");
          return -1;
     }
     // calculate the memroy location of the PCB and cast as pointer
     new_pcb = (pcb_t*)(_8MB - (free_pid + 1) * PCB_MEM_SPACING);

     // setup the page the user program
     page_vir_phy_map((int)USER_PAGES_VIR_ADDR_START, (int)(_8MB + free_pid * SIZE_4MB_PAGE) , 1); // allocate 4k page at 128MB
     flush_tlb();

     // copy file contennts (exe image) to the correct virtual memory location
     read_data(file_dentry.inode_num, 0, (uint8_t*)USER_IMG_ADDR, MAX_FILELENGTH); // Limit the read to the free space in the page for the program

     /* --------- setup pcb --------- */
     // setup stdin and stdout
     new_pcb->fd_arr[STDIN_FD].file_op_table_ptr  = &op_table_stdin;
     new_pcb->fd_arr[STDIN_FD].flags              = USED;
     new_pcb->fd_arr[STDOUT_FD].file_op_table_ptr = &op_table_stdout;
     new_pcb->fd_arr[STDOUT_FD].flags             = USED;

     user_stack = USER_PAGES_VIR_ADDR_START + SIZE_4MB_PAGE; // subtract 4 so we are in the same page (zero counting correction)
        
     new_pcb->pid = free_pid;             // fill in pcb fields
     new_pcb->user_esp = user_stack -4;
     new_pcb->file_inode_nbr = (uint32_t)file_dentry.inode_num; 
     new_pcb->active = USED;
     //new_pcb->user_eip = instructions_start;

     //assign intercepted user return address(eip) and top of user stack pointer (esp)  

     // if(active_terminals.current_active_terminal != active_terminals.current_showing_terminal)
     //      printf("sys_execute: active and showing terminals do not match\n");

     if(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb != NULL){ // if the process has a parent 
          //printf("\nIam inside\n");
          new_pcb->parent_pcb = (void*)active_terminals.terminals[active_terminals.current_active_terminal].active_pcb;
          my_parent = (pcb_t*)(new_pcb->parent_pcb);
          //printf("parent with pid %u in terminal %u had a child with pid %u\n",my_parent->pid, active_terminals.current_active_terminal, new_pcb->pid);
   
          asm volatile(
               "movl %%ebp, %0   \n"
               "movl %%esp, %1   \n"
               :"=r"(my_parent->user_ebp), "=r"(my_parent->user_esp)
               : 
               :"memory"
          );
          //printf("\nsaved user ebp and esp %x and %x: \n", my_parent->user_ebp, my_parent->user_esp);
          // printf("in execute: saved ebp and esp: %u, %u",my_parent->user_ebp )
          my_parent->tss_esp0 = tss.esp0;
          //printf("\nsaved tss esp0: %x\n", my_parent->tss_esp0);

     }
     else{  //if the process has no parent
          new_pcb->parent_pcb = NULL;
          if(active_terminals.terminals[active_terminals.current_active_terminal].active == UNUSED){ // if first shell in a terminal
               //printf("we should see this once per terminal\n");
               active_terminals.terminals[active_terminals.current_active_terminal].active = USED;
               new_pcb->user_esp = scheduler_saved_esp;
               new_pcb->user_ebp = scheduler_saved_ebp;
          }
     }
     
     // prepare tss for context switch (specify kernal data segment and assign esp to the current process's stack)
     tss.ss0 = KERNEL_DS;
     tss.esp0 = (_8MB - new_pcb->pid * PCB_MEM_SPACING) - _4B; // offset stack pointer by 4 for pcb pointer

     // clear args before copy
     for(i =0; i < MAX_ARG_LEN; i++){
          new_pcb->arg[i] = '\0';
     }
     // copy args into the new pcb
     if(arg_start != 0){
          strncpy((int8_t*)(new_pcb->arg), (int8_t*)args, command_len-arg_start +1); // +1 to copy null char
          // printf("in execute: args got: %s\n",new_pcb->arg );
     }
     
     //current_process_pcb = new_pcb; //assign current process to be the new process
     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb = new_pcb; // load the new pcb pointer into the currently active terminal


     //setup stack for CONTEXT SWITCH
     // push USER_DS
     // push ESP
     // push flags
     // push user CS
     // push EIP (from file)  

    asm volatile(
        "pushfl           \n"
        "popl %%eax       \n"
        "orl  $0x200, %%eax \n"
        :"=a" (my_flags)
        :
        :"memory"
     );

     asm volatile(
        "movw %w0, %%ds   \n"
        "pushl %0         \n"
        "pushl %1         \n"
        "pushl %2         \n"
        "pushl %3         \n"
        "pushl %4         \n"
        "iret             \n"
        "execute_end:     \n"    
        :
        :"r" (USER_DS), "r" (user_stack), "r"(my_flags), "r" (USER_CS), "r" (instructions_start)
        :"memory"
     );
    
     return 0;
}

/*
 * sys_open
 *   DESCRIPTION: generic open system call invoked in kernel
 *   INPUTS: filename: name of file
 *   OUTPUTS: none
 *   SIDE EFFECTS: sets up a new fd in currently running process' PCB
 *   RETURN VALUE: 0 (success) or -1 (failure)
 */
int32_t sys_open(const uint8_t *filename)
{
     dentry_t file_dentry;
     if (read_dentry_by_name(filename, &file_dentry))
          return -1; // file is not valid somehow

     uint32_t index, free_fd = MAX_OPEN_FILES;

     for (index = 0; index < MAX_OPEN_FILES; index++)
     {

          if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[index].flags == UNUSED)
          {
               free_fd = index; // opened file will be placed at this spot (fd)
               break;
          }
     }
     if (free_fd == MAX_OPEN_FILES)
     { // in case no free fd was found
          printf("\n NO FREE SPOT TO OPEN ANOTHER FILE... FILLED UP!\n");
          return -1;
     }

     // set up fd arr entry for newly opened file
     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[free_fd].inode_num = file_dentry.inode_num;
     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[free_fd].file_position = 0; // set read cursor to begining of file
     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[free_fd].flags = USED;      // set fd arr entry to used

     switch (file_dentry.filetype)   //assign file operation table according to file type
     {
     case REGULAR_FILE_TYPE:
          active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[free_fd].file_op_table_ptr = &op_table_reg_file;
          break;
     case DIRECTORY_FILE_TYPE:
          active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[free_fd].file_op_table_ptr = &op_table_dir_file;
          break;
     case RTC_FILE_TYPE:
          active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[free_fd].file_op_table_ptr = &op_table_rtc_file;
          break;
     default:
          printf("\n NO FILE TYPE IS MATCHING \n");
     }

     return free_fd; //return the allocated fd
}

/*
 * sys_close
 *   DESCRIPTION: close system call invoked in kernel
 *   INPUTS: fd: file descriptor (index to desc table where open file is stored)
 *   OUTPUTS: none
 *   SIDE EFFECTS: sets specified file desc arr entry to UNUSED
 *   RETURN VALUE: 0 (success) or -1 (failure)
 */
int32_t sys_close(int32_t fd){
     if (fd <= 1 || fd >= MAX_OPEN_FILES) // can only close fd = [2,7]
          return -1;
     if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags == UNUSED)
          return -1; // file is not open

     active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags = UNUSED; //set open file slot to unused 
     return 0;
}

/*
 * sys_read
 *   DESCRIPTION: generic read system call invoked in kernel
 *   INPUTS: fd: file descriptor (index to desc table where open file is stored)
 *           buf: buffer
 *           nbytes: number of bytes to copy
 *   OUTPUTS: none
 *   SIDE EFFECTS: calls specific read function in file operation table correspdonding to fd
 *   RETURN VALUE: number of bytes read (success) or -1 (failure)
 */
int32_t sys_read(int32_t fd, void *buf, int32_t nbytes){
     if (buf == NULL || nbytes < 0 || fd < 0 || fd >= MAX_OPEN_FILES) // can only read fd = [0,7]
          return -1;
     if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags == UNUSED) // if fd entry is unused dont allow operation
          return -1;
     // read data by calling the specific file read function
     // returns number of bytes read
     // function called updates read cursor (file_position)
     return active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].file_op_table_ptr->read(fd, buf, nbytes);
}

/*
 * sys_write
 *   DESCRIPTION: generic write system call invoked in kernel
 *   INPUTS: fd: file descriptor (index to desc table where open file is stored)
 *           buf: buffer
 *           nbytes: number of bytes to copy
 *   OUTPUTS: none
 *   SIDE EFFECTS: calls specific write function in file operation table correspdonding to fd
 *   RETURN VALUE: number of bytes read (success) or -1 (failure)
 */
int32_t sys_write(int32_t fd, const void *buf, int32_t nbytes){
     if (buf == NULL || nbytes < 0 || fd < 0 || fd >= MAX_OPEN_FILES) // can only read fd = [0,7]
          return -1;
     if (active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].flags == UNUSED) // if fd entry is unused dont allow operation
          return -1;
     return active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->fd_arr[fd].file_op_table_ptr->write(fd, buf, nbytes);
}

/*
 * sys_getargs
 *   DESCRIPTION: copies the arguments to current process PCB
 *   INPUTS: buf: pointer to local buffer variable to store arguments
 *           nbytes: number of bytes to copy
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: 0 (success) or -1 (failure)
 */
int32_t sys_getargs(uint8_t *buf, int32_t nbytes){

     if(*(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->arg) == '\0') //if empty args return fail
          return -1;

     uint32_t bytes_to_copy;
     bytes_to_copy = strlen((int8_t*)(active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->arg));

     if(bytes_to_copy > nbytes)
          bytes_to_copy = nbytes;
     
     if (buf == NULL || active_terminals.terminals[active_terminals.current_active_terminal].active_pcb == NULL || nbytes <= 0)
          return -1;
     strncpy((int8_t*)buf, (int8_t*)active_terminals.terminals[active_terminals.current_active_terminal].active_pcb->arg, bytes_to_copy+ 1); // +1 to include a '\0' char

     return 0;
}

/*
 * sys_vidmap
 *   DESCRIPTION: maps the video memory for user program
 *   INPUTS: screen start: a double pointer to start location of screen 
 *   OUTPUTS: none
 *   SIDE EFFECTS: sets the screen to virtual memory that is to be displayed
 *   RETURN VALUE: 0 (success) or -1 (failure)
 */
int32_t sys_vidmap(uint8_t **screen_start){
     if(screen_start==NULL)
          return -1;

     // check to make sure screen_start was within range of the program space (user-level page)
     if(((uint32_t)screen_start < USER_PAGES_VIR_ADDR_START) || ((uint32_t)screen_start > USER_PAGES_VIR_ADDR_START + SIZE_4MB_PAGE))
          return -1;

     // map the next page to the video memory addr (area past the pcb)
     page_vir_phy_map(USER_PAGES_VIR_ADDR_START+SIZE_4MB_PAGE, VIDEO_MEM_ADDR, VIDEO_MEM_SIZE);
     flush_tlb();

     *screen_start = (uint8_t *)(USER_PAGES_VIR_ADDR_START+SIZE_4MB_PAGE);

     return 0;
}

/*
 * sys_set_handler
 *   DESCRIPTION: TBD
 *   INPUTS: TBD
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: -1 (failure)
 */
int32_t sys_set_handler(int32_t signum, void *handler){
     return -1;
}

/*
 * sys_sigreturn
 *   DESCRIPTION: TBD
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: -1 (failure)
 */
int32_t sys_sigreturn(void){
     return -1;
}

/*
 * open_bad_call
 *   DESCRIPTION: bad call for open in operation file table
 *   INPUTS: fname: filename
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: -1 (failure)
 */
int32_t open_bad_call(const uint8_t *fname){
     return -1;
}
/*
 * open_bad_call
 *   DESCRIPTION: bad call for close in operation file table
 *   INPUTS: fd: filedescriptor
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: -1 (failure)
 */
int32_t close_bad_call(int32_t fd){
     return -1;
}
/*
 * open_bad_call
 *   DESCRIPTION: bad call for read in operation file table
 *   INPUTS: fd: filedescriptor , buf: buffer, nbytes: nbr bytes to be read/written
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: -1 (failure)
 */
int32_t read_bad_call(int32_t fd, void* buf, int32_t nbytes){
     return -1;
}
/*
 * write_bad_call
 *   DESCRIPTION: bad call for write in operation file table
 *   INPUTS: fd: filedescriptor , buf: buffer, nbytes: nbr bytes to be read/written
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   RETURN VALUE: -1 (failure)
 */
int32_t write_bad_call(int32_t fd, const void* buf, int32_t nbytes){
     return -1;
}
