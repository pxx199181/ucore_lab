#include <mmu.h>
#include <my_mem_manager.h>
char mem_buff[MAX_MEM_NUMBER][PAGE_SIZE];
bool mem_inuse[MAX_MEM_NUMBER];
int mem_number = MAX_MEM_NUMBER;
