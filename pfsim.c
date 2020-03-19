#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_INFO_LINE 15
#define MAX_PAGE_TABLE_INDEX 127
#define VALID 1
#define INVALID 0

/* Page table entry */
struct pte {
    bool valid;
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
    struct pte PTE_list[MAX_PAGE_TABLE_INDEX];
    int total_page_access = 0;
    int total_page_faults = 0;
    int total_disk_access = 0;
    int count = 0;

    while(count <= MAX_PAGE_TABLE_INDEX) {
        PTE_list[count].valid = INVALID;
        count++;
    }

    address_file = fopen(argv[1], "r");


    if (address_file == NULL){
        printf("Cannot open the file\n");
        return 1;
    }

    while (fgets(info_line, MAX_INFO_LINE, address_file) != NULL){
        printf("info line before tok: %s\n", info_line);
        strtok(info_line, " ");
        printf("info line after tok: %s\n", info_line);
        virtual_address = strtok(NULL, " ");
        real_virtual_address = strtol(virtual_address, NULL, 0);
        VPN = real_virtual_address >> 9;
        printf("virtual address: %hu\n", real_virtual_address);
        printf("VPN: %d\n", VPN);

        if (PTE_list[VPN].valid == INVALID){
            printf ("done+++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            total_page_access++;
            total_page_faults++;
            total_disk_access++;
            PTE_list[VPN].valid = VALID;
        }else{
            total_page_access++;
        }
        printf("---------------------------------------\n");
    }
    printf("Page accesses: %d\n", total_page_access);
    printf("Page faults: %d\n", total_page_faults);
    printf("Disk accesses: %d\n", total_disk_access);
    return 0;
    
}
