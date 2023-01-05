/* page.c - defines paging functionalities 
 */

#include "page.h"
#include "lib.h"
/* 
 * page_init
 *   DESCRIPTION: initializes all 1024 pages to not pressent, write enable
 *                
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void page_init(void){
    //set each entry to not present
    int i;
    for(i = 0; i < PAGE_DIR_NUM_ENTRIES; i++)
    {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
        page_directory[i].val = PE_CLEAR_SET_RW;
        page_table[i].val     = PE_CLEAR_SET_RW;
        // set up base indexes just in case
        page_table[i].base_31_12 = i;

        page_table_user[i].val = PE_CLEAR_SET_RW_USER;
        page_table_user[i].base_31_12 = i; 
    }

    page_directory[0].val = ((uint32_t)page_table & BASE_MASK) | PDE_CONTROL_FLAGS_4KB;

    page_directory[1].val = (1<<SHIFT_BY_22) | PDE_CONTROL_FLAGS_4MB; // set page size, rw, and present page, base addr = 1
    page_directory[1].global_bit = 1;
    //setup video memory page
    setup_pages_user(&page_table[find_page_index(VIDEO_MEM_ADDR)], (uint32_t)VIDEO_MEM_ADDR, (int)PAGE_SIZE);
    //setup terminal specific video pages ==> SAME MAPPPING ie. virtual = physical addresses
    setup_pages_user(&page_table[find_page_index(TERMINAL_1_VIDEO_PAGE_ADDR)], (uint32_t)TERMINAL_1_VIDEO_PAGE_ADDR, (int)PAGE_SIZE);
    setup_pages_user(&page_table[find_page_index(TERMINAL_2_VIDEO_PAGE_ADDR)], (uint32_t)TERMINAL_2_VIDEO_PAGE_ADDR, (int)PAGE_SIZE);
    clear_terminal_video_page((char*)TERMINAL_2_VIDEO_PAGE_ADDR, TERM_2_COLOR); // set the terminal color  
    setup_pages_user(&page_table[find_page_index(TERMINAL_3_VIDEO_PAGE_ADDR)], (uint32_t)TERMINAL_3_VIDEO_PAGE_ADDR, (int)PAGE_SIZE);
    clear_terminal_video_page((char*)TERMINAL_3_VIDEO_PAGE_ADDR, TERM_3_COLOR); // set the terminal color
 
    enable_paging((int) page_directory);
    // clear_terminal_video_page((int32_t*)TERMINAL_2_VIDEO_PAGE_ADDR); //make it green 
    // clear_terminal_video_page((int32_t*)TERMINAL_3_VIDEO_PAGE_ADDR); //make it green 

}

/*
 * find_page_index
 *   DESCRIPTION: Helper function, supports only 4kb page sizes
 *                finds the correct page index for the specifed physical memory addr
 *                by extracting the middle 10 bits that would be the PTE base addr
 *   INPUTS: physical_addr - desired physical memory address 
 *   OUTPUTS: none
 *   RETURN VALUE: The page index to use
 */
int find_page_index(int physical_addr) {
    int page_index;
    // Keep only the middle 21-12 bits of the addr and shift them over to the left 12 times
    page_index = (physical_addr & BITS_21_12_MASK)>>SHIFT_BY_12; 

    return page_index;
}

/* 
 * setup_pages
 *   DESCRIPTION: Loads base addresses into the desired page table based off the 
 *                given start address. Allocates as may pages that are required 
 *                to allocate pages for the specifed size
 *   INPUTS: page_entry - pointer to the first page entry one would like to fill
 *           start_addr - the physical address to start at 
 *           size       - the size of total memory to allocate with pages
 *   OUTPUTS: none
 *   RETURN VALUE: None
 *   SIDE EFFECTS: Page entries modified
 */
// void setup_pages(page_table_entry_t *page_entry, uint32_t start_addr, int size) {
//     start_addr &= BASE_MASK; // mask off the lower 12 bits
//     for(; size>0; start_addr+= PAGE_SIZE, size -= PAGE_SIZE, page_entry++) {
//         page_entry->val = start_addr & BASE_MASK | PDE_CONTROL_FLAGS_4KB; 
//     }
// }
void setup_pages(page_table_entry_t *page_entry, uint32_t start_addr, int size) {
    start_addr &= BASE_MASK; // mask off the lower 12 bits
    for(; size>0; start_addr+= PAGE_SIZE, size -= PAGE_SIZE, page_entry++) {
        page_entry->base_31_12 = start_addr>>SHIFT_BY_12; // must right shift 12 as base_31_12 starts at bit 12
        page_entry->present    = 1;
        page_entry->read_write = 1;
    }
}

/* 
 * setup_pages_user
 *   DESCRIPTION: Loads base addresses into the desired page table based off the 
 *                given start address. Allocates as may pages that are required 
 *                to allocate pages for the specifed size
 *   INPUTS: page_entry - pointer to the first page entry one would like to fill
 *           start_addr - the physical address to start at 
 *           size       - the size of total memory to allocate with pages
 *   OUTPUTS: none
 *   RETURN VALUE: None
 *   SIDE EFFECTS: Page entries modified
 */
void setup_pages_user(page_table_entry_t *page_entry, uint32_t start_addr, int size) {
    start_addr &= BASE_MASK; // mask off the lower 12 bits
    for(; size>0; start_addr+= PAGE_SIZE, size -= PAGE_SIZE, page_entry++) {
        page_entry->base_31_12 = start_addr>>SHIFT_BY_12; // must right shift 12 as base_31_12 starts at bit 12
        page_entry->present    = 1;
        page_entry->read_write = 1;
        page_entry->user_super = 1;
    }
}
/* REMOVE IF STILL COMMENTED AFTER DEBUGGING*/

//int page_index_to_paddr(int page_idx, int offset){
//    int phy_addr;
//    if(page_directory[page_idx].present == 0)
//        return -1;
//    if(page_directory[page_idx].page_size){
//        phy_addr = page_directory[page_idx].base_31_12<<12; // must right shift 12 as base_31_12 starts at bit 12
//        return phy_addr|offset;
//    }
//    else{
//    }
//    return page_directory[page_idx].
//}

/* REMOVE IF STILL COMMENTED AFTER DEBUGGING*/


/* 
 * page_vir_phy_map
 *   DESCRIPTION: Maps a virtual addr to a physical addr in a 4MB page or 4kB page in user space page table
 *   INPUTS: V_ADDR - virtual address to be mapped 
 *           P_ADDR - the physical address to be mapped to
 *           size   - page size bit-> for whether it is 4MB or 4KB page
 *   OUTPUTS: None
 *   RETURN VALUE: -1 = page already present, 0 = success
 *   SIDE EFFECTS: Page entries modified
 */
int page_vir_phy_map(int V_ADDR, int P_ADDR, int size){
    int dir_index = V_ADDR>>22;    // extract the upper 10 bits (this is the dir index                

    if(size == 1){ //4MB page option
        page_directory[dir_index].base_31_12 = P_ADDR>>SHIFT_BY_12; // must right shift 12 as base_31_12 starts at bit 12
        page_directory[dir_index].page_size  = 1;
    }
    else{
        // page_directory[dir_index].base_31_12 = P_ADDR>>22; // shift 22 as only uper 10 bits are used 
        // page_directory[dir_index].base_31_12 = ;
        page_directory[dir_index].val = ((uint32_t)page_table_user & BASE_MASK);
    }
    page_directory[dir_index].present    = 1;
    page_directory[dir_index].user_super = 1;
    page_directory[dir_index].read_write = 1;
    
    if(size != 1){ // may be incomplete
        setup_pages_user(&page_table_user[find_page_index(V_ADDR)], (uint32_t)P_ADDR, size); // setup pages for the requested space
    }

    return 0;
}



/* 
 * k_page_vir_phy_map
 *   DESCRIPTION: Maps a virtual addr to a physical addr in a 4MB page or 4kB page in kernel space page table
 *   INPUTS: V_ADDR - virtual address to be mapped 
 *           P_ADDR - the physical address to be mapped to
 *           size   - page size bit-> for whether it is 4MB or 4KB page
 *   OUTPUTS: None
 *   RETURN VALUE: -1 = page already present, 0 = success
 *   SIDE EFFECTS: Page entries modified
 */
int k_page_vir_phy_map(int V_ADDR, int P_ADDR, int size){
    int dir_index = V_ADDR>>22;    // extract the upper 10 bits (this is the dir index                

    if(size == 1){ //4MB page option
        page_directory[dir_index].base_31_12 = P_ADDR>>SHIFT_BY_12; // must right shift 12 as base_31_12 starts at bit 12
        page_directory[dir_index].page_size  = 1;
    }
    else{
        // page_directory[dir_index].base_31_12 = P_ADDR>>22; // shift 22 as only uper 10 bits are used 
        // page_directory[dir_index].base_31_12 = ;
        page_directory[dir_index].val = ((uint32_t)page_table & BASE_MASK);
    }
    page_directory[dir_index].present    = 1;
    page_directory[dir_index].user_super = 1;
    page_directory[dir_index].read_write = 1;
    
    if(size != 1){ // may be incomplete
        setup_pages_user(&page_table[find_page_index(V_ADDR)], (uint32_t)P_ADDR, size); // setup pages for the requested space
    }

    return 0;
}

/* 
 * page_vir_phy_unmap
 *   DESCRIPTION: unmaps a page dir (4MB) from the specified virtual mem location
 *   INPUTS: V_ADDR - virtual address to be un-mapped 
 *   OUTPUTS: None
 *   RETURN VALUE: 0 = success
 *   SIDE EFFECTS: Page entries modified
 */
int page_vir_phy_unmap(int V_ADDR){
    int i;
    int dir_index = V_ADDR>>SHIFT_BY_22;    // extract the upper 10 bits (this is the dir index                

    if(page_directory[dir_index].present){ // check if present
        if(page_directory[dir_index].page_size){ // check if full of 4kb pages
            for(i = 0; i < PAGE_DIR_NUM_ENTRIES; i++){
                if(page_table_user[i].present)
                    page_table_user[i].present = 0; // may not be a good idea to step through and destroy all of these, but hey this can be a later bug
            }
        }
        page_directory[dir_index].present = 0;
    }

    return 0;
}

/* 
 * flush_tlb
 *   DESCRIPTION: flushes tlb 
 *   INPUTS: void
 *   OUTPUTS: None
 *   RETURN VALUE: none
 *   SIDE EFFECTS: TLB flushed
 */
void flush_tlb(void){
    asm volatile(
        "movl %%cr3, %%eax   \n"
        "movl %%eax, %%cr3   \n"
        :
        :
        :"eax"
    );
    return;
}
