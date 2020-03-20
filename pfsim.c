#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define INVALID 0
#define VALID 1
#define MAX_PROCESSES_COUNT 3;
#define MAX_INFO_LINE 15
#define MAX_FRAME_COUNT 31
#define MAX_PAGE_TABLE_INDEX 127

/* Page table entry */
struct pte {
    bool validity;
    __uint16_t pfn;
};

/* Reverse mapping entry */
struct rme {
    bool unavail;
    bool dirty;
    bool referenced;
    __uint8_t proc;
    __uint16_t vpn;
};

struct pte PTE_list[MAX_PAGE_TABLE_INDEX];
struct rme physical_memory[MAX_FRAME_COUNT];
int total_page_access = 0;
int total_page_faults = 0;
int total_disk_access = 0;

int find_next_availabe_frame(){
    int count_frame_index = 0;
    while(count_frame_index <= MAX_FRAME_COUNT) {
        if (physical_memory[count_frame_index].unavail == VALID){
            return count_frame_index;
        }
        count_frame_index++;
    }
    printf("NO FRAME AVAILABLE\n");
    return -1;           // No empty frame
}

/*first look for an unreferenced page frame.
 *
 * Then look for an unreferenced page frame where the dirty bit is off and replace this page.
 *
 * Then look for an unreferenced page frame where the dirty bit is on and replace that page.
 *
 * If none is found, look for a referenced page frame where the dirty bit is off and replace that page.
 *
 * Finally, you will have to replace a page frame which is both referenced and dirty.*/



int finding_evicted_page(){
   /* for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        printf("index: %d\n", i);
        if (physical_memory[i].referenced == INVALID){
            printf("DONE FROM 1\n");
            return i;
        }
    }*/

   /* for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        printf("vpn: %d   ",physical_memory[i].vpn);
        printf("ref invalid: %d    ", physical_memory[i].referenced);
        printf("dirty invalid: %d\n", physical_memory[i].dirty);
    }*/

    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if ((physical_memory[i].referenced == INVALID) && (physical_memory[i].dirty == INVALID)){
            printf("DONE FROM 2\n");
            return i;
        }
    }

    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if (physical_memory[i].referenced == INVALID && physical_memory[i].dirty == VALID){
            printf("EVICT: total disk access: %d\n", total_disk_access++);
            printf("DONE FROM 3\n");
            return i;
        }
    }

    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if (physical_memory[i].referenced == VALID && physical_memory[i].dirty == INVALID){
            printf("DONE FROM 4\n");
            return i;
        }
    }

    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if (physical_memory[i].referenced == VALID && physical_memory[i].dirty == VALID){
            printf("EVICT: total disk access: %d\n", total_disk_access++);
            printf("DONE FROM 5\n");
            return i;
        }
    }
    return -1;
}

int main(int argc, char **argv) {

    if (argc != 2){
        fprintf(stderr, "Usage: pfsim <file>\n");
        return 1;
    }

    FILE* address_file;
    char info_line[MAX_INFO_LINE];
    char* virtual_address;
    __uint16_t real_virtual_address;
    int VPN;
    int current_process;
    char* action;
    int count_memory_access = 0;

    int count_page_table_index = 0;
    while(count_page_table_index <= MAX_PAGE_TABLE_INDEX) {
        PTE_list[count_page_table_index].validity = INVALID;
        count_page_table_index++;
    }

    int count_frame_index = 0;
    while(count_frame_index <= MAX_FRAME_COUNT) {
        physical_memory[count_frame_index].unavail = VALID;
        physical_memory[count_frame_index].dirty = INVALID;       //The page is not dirty for now
        physical_memory[count_frame_index].referenced = INVALID;  //The page is not referenced yet
        count_frame_index++;
    }

    address_file = fopen(argv[1], "r");

    if (address_file == NULL){
        printf("Cannot open the file\n");
        return 1;
    }

    while (fgets(info_line, MAX_INFO_LINE, address_file) != NULL) {
        printf("original info line: %s\n", info_line);

        /*get the current process*/
        strtok(info_line, " ");
        current_process = strtol(info_line, NULL, 10);
        printf("CURRENT PROCESS: %d\n", current_process);

        /*get the virtual address and extract the VPN*/
        virtual_address = strtok(NULL, " ");
        real_virtual_address = strtol(virtual_address, NULL, 0);
        VPN = real_virtual_address >> 9;
        printf("virtual address: %hu\n", real_virtual_address);
        printf("VPN: %d\n", VPN);

        /*get the action, read or write*/
        action = strtok(NULL, " ");
        printf("THE ACTION: %s", action);

        /*if the pte is invalid, set up the connection */
        if (PTE_list[VPN].validity == INVALID){
            printf ("++++++++++++++++++++++++++++++INVALID FIND & PAGE FAULTS++++++++++++++++++++++++++++++\n");

            for (int i = 0; i <= MAX_FRAME_COUNT; ++i) {
                printf("vpn: %d   ",physical_memory[i].vpn);
                printf("ref invalid: %d    ", physical_memory[i].referenced);
                printf("dirty invalid: %d\n", physical_memory[i].dirty);
            }

            total_page_access++;
            total_page_faults++;
            printf("PAGE FAULT: total disk access: %d\n", total_disk_access++);
            PTE_list[VPN].validity = VALID;

            /*find a available or evict page*/
            if (find_next_availabe_frame() == -1){
                /*evict using the Not Recently Used algo*/
                printf("------------------------------------EVICT----------------------------------------!\n");
                printf("VPN: %d\n", VPN);
                printf("PFN: %d\n", PTE_list[VPN].pfn);

                int index = finding_evicted_page();
                //PTE_list[physical_memory[index].vpn].pfn = 32;
                PTE_list[physical_memory[index].vpn].validity = INVALID;

                printf("FRAME INDEX: %d\n", index);
                //printf("EVICT: total disk access: %d\n", total_disk_access++);
                /*update the rme*/
                PTE_list[VPN].pfn = index;
                printf("NEW PFN: %d\n", PTE_list[VPN].pfn);
                physical_memory[index].proc = current_process;
                physical_memory[index].dirty = VALID;
                physical_memory[index].referenced = VALID;
                physical_memory[index].vpn = VPN;
                physical_memory[index].unavail = INVALID;
            } else {
                PTE_list[VPN].pfn = find_next_availabe_frame();
                printf("PFN: %d\n", PTE_list[VPN].pfn);
                PTE_list[VPN].validity = VALID;

                /*update the rme*/
                physical_memory[PTE_list[VPN].pfn].unavail = INVALID;
                physical_memory[PTE_list[VPN].pfn].proc = current_process;
                physical_memory[PTE_list[VPN].pfn].vpn = VPN;
                physical_memory[PTE_list[VPN].pfn].referenced = VALID;           ////The page is referenced right now
                if (!strncmp(action, "W", 1))
                    physical_memory[PTE_list[VPN].pfn].dirty = VALID;          //The page is dirty right now
            }

        }else{
            /*else the pte is valid, access immediately*/
            total_page_access++;
            physical_memory[PTE_list[VPN].pfn].referenced = VALID;           ////The page is referenced right now
            if (!strncmp(action, "W", 1))
                physical_memory[PTE_list[VPN].pfn].dirty = VALID;          //The page is dirty right now
            printf("vpn=%d => pfn=%d\n", VPN, PTE_list[VPN].pfn);
        }



        int count_frame = 0;
        count_memory_access += 1;
        if (count_memory_access == 200){
            while(count_frame <= MAX_FRAME_COUNT) {
                physical_memory[count_frame].referenced = INVALID;
                count_frame++;
            }
            printf("----------------------------------------- REF RESET --------------------------------------------------\n");
            count_memory_access = 0;
        }

        printf("---------------------------------------\n");
    }
    for (int i = 0; i <= MAX_FRAME_COUNT; ++i) {
        printf("vpn: %d   ",physical_memory[i].vpn);
        printf("ref invalid: %d    ", physical_memory[i].referenced);
        printf("dirty invalid: %d\n", physical_memory[i].dirty);
    }

    printf("Page accesses: %d\n", total_page_access);
    printf("Page faults: %d\n", total_page_faults);
    printf("Disk accesses: %d\n", total_disk_access);
    return 0;
}
