# enable_paging.S - assembly code responsible for enabling paging through control registers
# vim:ts=4 noexpandtab

#define ASM           1
#define SET_PSE_CR4   0x00000010
#define SET_PG_PE_C0  0x80000001

.globl enable_paging
.align 4
enable_paging:
  PUSHL %ebp
  MOVL  %esp, %ebp
  
  MOVL  8(%ebp), %eax    # argument page_directory address 
  MOVL %eax, %cr3        # move page_directory address to CR3

  MOVL %cr4, %eax
  ORL  $SET_PSE_CR4, %eax    # set PSE bit of CR4 for page size extension (4MB)
  MOVL %eax, %cr4

  MOVL %cr0, %eax
  ORL  $SET_PG_PE_C0, %eax  # sets PG and PE bits of C0
  MOVL %eax, %cr0   
  
  MOVL %cr3, %eax     # resest TLB just in case
  MOVL %eax, %cr3
  
  LEAVE
  RET
