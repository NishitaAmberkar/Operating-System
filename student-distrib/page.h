#ifndef PAGE_H
#define PAGE_H

#ifndef ASM 

#include "types.h"

#define PAGE_DIR_SIZE                 0x400000
#define PAGE_SIZE                     4096
#define PAGE_DIR_NUM_ENTRIES          1024
#define PAGE_TABLE_NUM_ENTRIES        1024
#define SIZE_4MB_PAGE                 0x400000
#define KERNEL_PAGE_DIR_IDX           2

#define BASE_MASK                     0xfffff000
#define BITS_21_12_MASK               0x003FF000

// sets RW flag and zeros out the rest
#define PE_CLEAR_SET_RW               0x00000002
#define PE_CLEAR_SET_RW_USER          0x00000006
// sets present and RW flags
#define PDE_CONTROL_FLAGS_4KB         0x03
#define PDE_CONTROL_FLAGS_4KB_USER    0x07
//sets present, RW, and page size flags
#define PDE_CONTROL_FLAGS_4MB         0x83
#define PDE_CONTROL_FLAGS_4MB_USER    0x87

#define VIDEO_MEM_ADDR                0xB8000
#define TERMINAL_1_VIDEO_PAGE_ADDR    VIDEO_MEM_ADDR  + 2*PAGE_SIZE 
#define TERMINAL_2_VIDEO_PAGE_ADDR    TERMINAL_1_VIDEO_PAGE_ADDR  + PAGE_SIZE 
#define TERMINAL_3_VIDEO_PAGE_ADDR    TERMINAL_2_VIDEO_PAGE_ADDR  + PAGE_SIZE 

#define NUM_COLS                      80
#define NUM_ROWS                      25
#define VIDEO_MEM_SIZE                NUM_COLS * NUM_ROWS

// processses sit in pages starting at 128MB
#define USER_PAGES_VIR_ADDR_START     0x8000000

#define SHIFT_BY_12       12
#define SHIFT_BY_22       22


/* this function initializes paging  */
extern void page_init(void);
/* ASM function that enables paging */
extern void enable_paging(int page_dir_address);
/* helper function that calculates page index given phys addr */
extern int find_page_index(int physical_addr);

/* PDE structure */
typedef struct page_directory_entry {
    union {
        uint32_t val;
        struct {
            uint32_t present       : 1;
            uint32_t read_write    : 1;
            uint32_t user_super    : 1;
            uint32_t write_trough  : 1;
            uint32_t cache_dis     : 1;
            uint32_t accessed      : 1;
            uint32_t reserved      : 1;
            uint32_t page_size     : 1;
            uint32_t global_bit    : 1;
            uint32_t available     : 3;
            uint32_t base_31_12    : 20;
        } __attribute__ ((packed));
    };
} page_directory_entry_t;

/* PTE structure */
typedef struct page_table_entry {
    union {
        uint32_t val;
        struct {
            uint32_t present       : 1;
            uint32_t read_write    : 1;
            uint32_t user_super    : 1;
            uint32_t PWT           : 1;
            uint32_t PCD           : 1;
            uint32_t accessed      : 1;
            uint32_t dirty         : 1;
            uint32_t PAT           : 1;
            uint32_t granularity   : 1;
            uint32_t available     : 3;
            uint32_t base_31_12    : 20;
        } __attribute__ ((packed));
    };
} page_table_entry_t;


/* allocated pages non-user */
extern void setup_pages(page_table_entry_t *page_entry, uint32_t start_addr, int size);
/* allocated pages for user */
extern void setup_pages_user(page_table_entry_t *page_entry, uint32_t start_addr, int size);
/* Maps a virtual addr to a physical addr in a 4MB page or 4kB page in user space page table*/
extern int page_vir_phy_map(int V_ADDR, int P_ADDR, int size);
/* Maps a virtual addr to a physical addr in a 4MB page or 4kB page in kernel space page table*/
extern int k_page_vir_phy_map(int V_ADDR, int P_ADDR, int size);
/* unmaps a page dir (4MB) from the specified virtual mem location */
extern int page_vir_phy_unmap(int V_ADDR);
/* flushes tlb  */
extern void flush_tlb(void);

// page tables allocated (as we go)
page_directory_entry_t page_directory[PAGE_DIR_NUM_ENTRIES] __attribute__((aligned (PAGE_SIZE)));
page_table_entry_t     page_table[PAGE_TABLE_NUM_ENTRIES]   __attribute__((aligned (PAGE_SIZE)));
page_table_entry_t     page_table_user[PAGE_TABLE_NUM_ENTRIES]   __attribute__((aligned (PAGE_SIZE)));

#endif /* ASM */
#endif /* PAGE_H */
