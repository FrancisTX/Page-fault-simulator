#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define INVALID 0
#define VALID 1
#define MAX_PROCESSES_COUNT 3
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

struct pte PTE_list[MAX_PROCESSES_COUNT][MAX_PAGE_TABLE_INDEX];
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
    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if ((physical_memory[i].referenced == INVALID) && (physical_memory[i].dirty == INVALID)){
            return i;
        }
    }

    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if (physical_memory[i].referenced == INVALID && physical_memory[i].dirty == VALID){
	    total_disk_access++;
            return i;
        }
    }

    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if (physical_memory[i].referenced == VALID && physical_memory[i].dirty == INVALID){
            return i;
        }
    }

    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        if (physical_memory[i].referenced == VALID && physical_memory[i].dirty == VALID){
            total_disk_access++;
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


    for (int count_process = 0; count_process <= MAX_PROCESSES_COUNT; ++count_process) {
        for (int count_page_table_index = 0; count_page_table_index <= MAX_PAGE_TABLE_INDEX; ++count_page_table_index) {
            PTE_list[count_process][count_page_table_index].validity = INVALID;
            count_page_table_index++;
        }
        count_process++;
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

        /*get the current process*/
        strtok(info_line, " ");
        current_process = strtol(info_line, NULL, 10);

        /*get the virtual address and extract the VPN*/
        virtual_address = strtok(NULL, " ");
        real_virtual_address = strtol(virtual_address, NULL, 0);
        VPN = real_virtual_address >> 9;

        /*get the action, read or write*/
        action = strtok(NULL, " ");

        /*if the pte is invalid, set up the connection */
        if (PTE_list[current_process][VPN].validity == INVALID) {
            total_page_access++;
            total_page_faults++;
	    total_disk_access++;
            PTE_list[current_process][VPN].validity = VALID;

            /*find a available or evict page*/
            if (find_next_availabe_frame() == -1) {
                /*evict using the Not Recently Used algo*/
                int index = finding_evicted_page();

                /*update the PTE*/
                //PTE_list[physical_memory[index].vpn].pfn = 32;
                PTE_list[physical_memory[index].proc][physical_memory[index].vpn].validity = INVALID;

                /*update the rme*/
                PTE_list[current_process][VPN].pfn = index;
                physical_memory[index].proc = current_process;
                if (!strncmp(action, "W", 1))
                    physical_memory[PTE_list[current_process][VPN].pfn].dirty = VALID;          //The page is dirty right now
                else
                    physical_memory[PTE_list[current_process][VPN].pfn].dirty = INVALID;
                physical_memory[index].referenced = VALID;
                physical_memory[index].vpn = VPN;
                physical_memory[index].unavail = INVALID;
            } else {
                PTE_list[current_process][VPN].pfn = find_next_availabe_frame();
                PTE_list[current_process][VPN].validity = VALID;

                /*update the rme*/
                physical_memory[PTE_list[current_process][VPN].pfn].unavail = INVALID;
                physical_memory[PTE_list[current_process][VPN].pfn].proc = current_process;
                physical_memory[PTE_list[current_process][VPN].pfn].vpn = VPN;
                physical_memory[PTE_list[current_process][VPN].pfn].referenced = VALID;           ////The page is referenced right now
                if (!strncmp(action, "W", 1))
                    physical_memory[PTE_list[current_process][VPN].pfn].dirty = VALID;          //The page is dirty right now
            }



        }else if (PTE_list[current_process][VPN].validity == VALID){
            /*else the pte is valid, access immediately*/
            total_page_access++;
            physical_memory[PTE_list[current_process][VPN].pfn].referenced = VALID;           ////The page is referenced right now
            if (!strncmp(action, "W", 1))
                physical_memory[PTE_list[current_process][VPN].pfn].dirty = VALID;          //The page is dirty right now
        }



        int count_frame = 0;
        count_memory_access += 1;
        if (count_memory_access == 200){
            while(count_frame <= MAX_FRAME_COUNT) {
                physical_memory[count_frame].referenced = INVALID;
                count_frame++;
            }
            count_memory_access = 0;
        }
    }

    printf("Page accesses: %d\n", total_page_access);
    printf("Page faults: %d\n", total_page_faults);
    printf("Disk accesses: %d\n", total_disk_access);
    return 0;
}
