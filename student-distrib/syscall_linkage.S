#define ASM   1
#define SET_IF 0x0200

.GLOBL syscall_generic_handler
.GLOBL sys_halt , sys_execute , sys_read , sys_write , sys_open , sys_close, sys_getargs , sys_vidmap , sys_set_handler , sys_sigreturn

 #
 # syscall_generic_handler: invoked by system calls (0x80 entry of IDT )
 # --- dispatches to the correct system call handler based on 
 # --- EAX value ==> holds system call number
 # registers are saved during process
 # 
 
.ALIGN 4
syscall_generic_handler:
    CLI
    pushl $0
    PUSHFL
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esp
    pushl %ebp
    pushl %esi
    pushl %edi


    CMPL    $1, %eax   # check  0 < syscall num
    JL      invalid_syscall_num
    CMPL    $10 , %eax      # check syscall num <= 10
    JG      invalid_syscall_num

    pushl %edx
    pushl %ecx
    pushl %ebx

    DECL    %eax                        # decrement EAX to match index of jmp table
    CALL    *syscalls_table(, %eax, 4)  # call syscall handler based on syscall number
    addl $12, %esp
    JMP     syscall_end

invalid_syscall_num: 
    MOVL    $-1, %eax      # return -1 to user (failed syscall)

syscall_end:
    popl %edi
    popl %esi
    popl %ebp
    popl %esp
    popl %edx
    popl %ecx
    popl %ebx
    POPFL

    addl $4, %esp
    iret


# jump table of syscall handler functions
.ALIGN 4
syscalls_table:
      .long  sys_halt , sys_execute , sys_read , sys_write , sys_open , sys_close, sys_getargs , sys_vidmap , sys_set_handler , sys_sigreturn
.end
