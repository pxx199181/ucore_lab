#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_extended_clock.h>
#include <list.h>

/* [wikipedia]The simplest Page Replacement Algorithm(PRA) is a FIFO algorithm. The first-in, first-out
 * page replacement algorithm is a low-overhead algorithm that requires little book-keeping on
 * the part of the operating system. The idea is obvious from the name - the operating system
 * keeps track of all the pages in memory in a queue, with the most recent arrival at the back,
 * and the earliest arrival in front. When a page needs to be replaced, the page at the front
 * of the queue (the oldest page) is selected. While FIFO is cheap and intuitive, it performs
 * poorly in practical application. Thus, it is rarely used in its unmodified form. This
 * algorithm experiences Belady's anomaly.
 *
 * Details of FIFO PRA
 * (1) Prepare: In order to implement FIFO PRA, we should manage all swappable pages, so we can
 *              link these pages into pra_list_head according the time order. At first you should
 *              be familiar to the struct list in list.h. struct list is a simple doubly linked list
 *              implementation. You should know howto USE: list_init, list_add(list_add_after),
 *              list_add_before, list_del, list_next, list_prev. Another tricky method is to transform
 *              a general list struct to a special struct (such as struct page). You can find some MACRO:
 *              le2page (in memlayout.h), (in future labs: le2vma (in vmm.h), le2proc (in proc.h),etc.
 */

list_entry_t pra_list_head;
/*
 * (2) _extended_clock_init_mm: init pra_list_head and let  mm->sm_priv point to the addr of pra_list_head.
 *              Now, From the memory control struct mm_struct, we can access FIFO PRA
 */
static int
_extended_clock_init_mm(struct mm_struct *mm)
{     
     list_init(&pra_list_head);
     mm->sm_priv = &pra_list_head;
     //cprintf(" mm->sm_priv %x in fifo_init_mm\n",mm->sm_priv);
     return 0;
}
/*
 * (3)_extended_clock_map_swappable: According FIFO PRA, we should link the most recent arrival page at the back of pra_list_head qeueue
 */
static int
_extended_clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_entry_t *entry=&(page->pra_page_link);
 
    assert(entry != NULL && head != NULL);
    //record the page access situlation
    /*LAB3 EXERCISE 2: YOUR CODE*/ 
    //(1)link the most recent arrival page at the back of the pra_list_head qeueue.
    
    //pxx-code
    // double link list, so add link before the head means add it to the end of list
    list_add_before(head, entry);
    /*
    //what is the use of swap_in
    if (swap_in == 1) {
        list_add_before(head, entry);
    }
    */
    return 0;
}
/*
 *  (4)_extended_clock_swap_out_victim: According FIFO PRA, we should unlink the  earliest arrival page in front of pra_list_head qeueue,
 *                            then set the addr of addr of this page to ptr_page.
 */
void print_swap_page_info(struct mm_struct* mm) {

         list_entry_t *head=(list_entry_t*) mm->sm_priv;
         list_entry_t *le;
         cprintf("mm_addr:0x%x\n", mm);
         cprintf("pte_addr list:\n");
         le = head;
         while((le = list_next(le)) != head){
            struct Page* page = le2page(le, pra_page_link);
            pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);
            cprintf("0x%x:[%x](%d, %d)->", *ptep, page->pra_vaddr, (*ptep & PTE_A) != 0, (*ptep & PTE_D) != 0);
         }
        cprintf("\n");
         cprintf("pra_addr list:\n");
         le = head;
         while((le = list_next(le)) != head){
            struct Page* page = le2page(le, pra_page_link);
            cprintf("0x%x->",  page->pra_vaddr);
         }
        cprintf("\n");
}
static int
_extended_clock_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
    print_swap_page_info(mm);
    if (0) {

         list_entry_t *head=(list_entry_t*) mm->sm_priv;
         assert(head != NULL);
         assert(in_tick==0);
         /* Select the victim */
         /*LAB3 EXERCISE 2: YOUR CODE*/ 
         //(1)  unlink the  earliest arrival page in front of pra_list_head qeueue
         //(2)  set the addr of addr of this page to ptr_page
         if (list_empty(head)) {
            cprintf("list is empty\n");
            return -1;
         }
         list_entry_t *out_page_le = list_next(head);
         list_del(out_page_le);
         *ptr_page = le2page(out_page_le, pra_page_link);
        cprintf("0x%x\n", get_pte(mm->pgdir, (*ptr_page)->pra_vaddr, 0));
        cprintf("0x%x\n", (*ptr_page)->pra_vaddr);
        print_swap_page_info(mm);

         return 0;
    }


     list_entry_t *head=(list_entry_t*) mm->sm_priv;
     assert(head != NULL);
     assert(in_tick==0);
     /* Select the victim */
     /*LAB3 EXERCISE 2: YOUR CODE*/ 
     //(1)  unlink the  earliest arrival page in front of pra_list_head qeueue
     //(2)  set the addr of addr of this page to ptr_page
     if (list_empty(head)) {
        cprintf("list is empty\n");
        return -1;
     }

    //pxx-code-begin
     list_entry_t *le, *out_page_le;
     int rank_id = 0;
     le = head;
     out_page_le = list_next(le);
     while((le = list_next(le)) != head){

        struct Page* page = le2page(le, pra_page_link);
        pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);

        if (*ptep == NULL) {
            cprintf("error\n");
            return -1;
        }
        //check bit set
        int is_a, is_d;
        is_a = *ptep & PTE_A;
        is_d = *ptep & PTE_D;
        if (is_a == 0) {
            if (is_d == 0) {
                out_page_le = le;
                cprintf("find 3\n");
                break;
            }
            else if (rank_id < 2) {
                rank_id = 2;
                out_page_le = le;
                cprintf("find 2\n");
            }
        }
        else if (rank_id < 1){
            if (is_d == 0) {
                rank_id = 1;
                out_page_le = le;
                cprintf("find 1\n");
            }
        }
     }
     //pxx-code-end
     //out_page_le = list_next(head);
     list_del(out_page_le);
     *ptr_page = le2page(out_page_le, pra_page_link);
     cprintf("%x\n", get_pte(mm->pgdir, (*ptr_page)->pra_vaddr, 0));
     cprintf("%x\n", (*ptr_page)->pra_vaddr);
     print_swap_page_info(mm);

     return 0;
}
static void waitcount(int count) {
    int i = 0;
    while (i < count) {
        i++;
    }
}
static int
_extended_clock_check_swap(void) {
    extern struct mm_struct *check_mm_struct;
    print_swap_page_info(check_mm_struct);
    int wait_time = 200000000;
    cprintf("write Virt Page c in _extended_clock_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    print_swap_page_info(check_mm_struct);
    //assert(pgfault_num==4);
    waitcount(wait_time);
    cprintf("write Virt Page a in _extended_clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==4);
    cprintf("write Virt Page d in _extended_clock_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==4);
    cprintf("write Virt Page b in _extended_clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==4);
    cprintf("write Virt Page e in _extended_clock_check_swap\n");
    *(unsigned char *)0x5000 = 0x0e;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==5);
    cprintf("write Virt Page b in _extended_clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==5);
    cprintf("write Virt Page a in _extended_clock_check_swap\n");
    *(unsigned char *)0x1000 = 0x0a;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==6);
    cprintf("write Virt Page b in _extended_clock_check_swap\n");
    *(unsigned char *)0x2000 = 0x0b;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==7);
    cprintf("write Virt Page c in _extended_clock_check_swap\n");
    *(unsigned char *)0x3000 = 0x0c;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==8);
    cprintf("write Virt Page d in _extended_clock_check_swap\n");
    *(unsigned char *)0x4000 = 0x0d;
    print_swap_page_info(check_mm_struct);
    waitcount(wait_time);
    //assert(pgfault_num==9);
    return 0;
}


static int
_extended_clock_init(void)
{
    return 0;
}

static int
_extended_clock_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
_extended_clock_tick_event(struct mm_struct *mm)
{ 
    //cprintf("swap_clock_tick\n");
    //print_swap_page_info_inswap(mm);
    //pxx-code-begin
    cprintf("--------------before clear--------------\n");
    print_swap_page_info(mm);
    cprintf("--------------------------------------------\n");
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_entry_t *le;
    le = head;
    while((le = list_next(le)) != head){
        struct Page* page = le2page(le, pra_page_link);
        pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);
        if (ptep == NULL) {
            cprintf("error\n");
            return -1;
        }
        *ptep &= ~(PTE_A | PTE_D);
        cprintf("set one\n");
    }
    //pxx-code-end
    cprintf("--------------after clear--------------\n");
    print_swap_page_info(mm);
    cprintf("--------------------------------------------\n");
    return 0; 
}


struct swap_manager swap_manager_extended_clock =
{
     .name            = "extended_clock swap manager",
     .init            = &_extended_clock_init,
     .init_mm         = &_extended_clock_init_mm,
     .tick_event      = &_extended_clock_tick_event,
     .map_swappable   = &_extended_clock_map_swappable,
     .set_unswappable = &_extended_clock_set_unswappable,
     .swap_out_victim = &_extended_clock_swap_out_victim,
     .check_swap      = &_extended_clock_check_swap,
};
