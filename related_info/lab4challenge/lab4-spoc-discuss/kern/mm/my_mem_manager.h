#ifndef __MY_MEM_MANAGER_H__
#define __MY_MEM_MANAGER_H__
#define MAX_MEM_NUMBER 50
#ifndef PAGE_SIZE
#define PAGE_SIZE PGSIZE
#include <stdio.h>

extern char mem_buff[MAX_MEM_NUMBER][PAGE_SIZE];
extern bool mem_inuse[MAX_MEM_NUMBER];
extern int mem_number;

#define false 0
#define true 1
static inline void *
mem_init() {
    int i ;
    for (i = 0; i < MAX_MEM_NUMBER; ++i)
    {
        mem_inuse[i] = false;
    }
}
static inline void *
mem_alloc(int size) {
    //cprintf("--------------------------------------mem_alloc\n");
    size = (size  + PAGE_SIZE - 1) / PAGE_SIZE;
    int i ;
    for (i = 0; i < MAX_MEM_NUMBER; ++i)
    {
        int j;
        if (mem_inuse[i] == false)
        {
            int count = 0;
            for (j = i; count < size && j < MAX_MEM_NUMBER; ++count, ++j)
            {
                if (mem_inuse[j] == true) {
                    	break;
                }
            }

            if (count == size) {
            	   count = 0;
                for (j = i; count < size; ++j, ++count)
                {
                    mem_inuse[j] = true;
                }
                //cprintf("index:%d, size:%d\n", i, size);
                //cprintf("addr:%x\n", (void*)mem_buff[i]);
                return (void*)mem_buff[i];
            }
        }
    }
    return 0;
}
static inline void *
mem_free(void *p, int size) {
    //cprintf("--------------------------------------mem_free\n");
    size = (size  + PAGE_SIZE - 1) / PAGE_SIZE;
    int index = (p - (void*)mem_buff) / PAGE_SIZE;
    //cprintf("addr:%x\n", (void*)p);
    //cprintf("index:%d, size:%d\n", index, size);
    int i;
    int count = 0;
    for (i = index; count < size; ++i, ++count)
    {
        mem_inuse[i] = false;
    }
}

#endif
#endif