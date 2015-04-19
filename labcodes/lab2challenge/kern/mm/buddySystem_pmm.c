#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddySystem_pmm.h>

/* In the first fit algorithm, the allocator keeps a list of free blocks (known as the free list) and,
   on receiving a request for memory, scans along the list for the first block that is large enough to
   satisfy the request. If the chosen block is significantly larger than that requested, then it is 
   usually split, and the remainder added to the list as another free block.
   Please see Page 196~198, Section 8.2 of Yan Wei Ming's chinese book "Data Structure -- C programming language"
*/
// LAB2 EXERCISE 1: YOUR CODE
// you should rewrite functions: default_init,default_init_memmap,default_alloc_pages, default_free_pages.
/*
 * Details of FFMA
 * (1) Prepare: In order to implement the First-Fit Mem Alloc (FFMA), we should manage the free mem block use some list.
 *              The struct free_area_t is used for the management of free mem blocks. At first you should
 *              be familiar to the struct list in list.h. struct list is a simple doubly linked list implementation.
 *              You should know howto USE: list_init, list_add(list_add_after), list_add_before, list_del, list_next, list_prev
 *              Another tricky method is to transform a general list struct to a special struct (such as struct page):
 *              you can find some MACRO: le2page (in memlayout.h), (in future labs: le2vma (in vmm.h), le2proc (in proc.h),etc.)
 * (2) default_init: you can reuse the  demo default_init fun to init the free_list and set nr_free to 0.
 *              free_list is used to record the free mem blocks. nr_free is the total number for free mem blocks.
 * (3) default_init_memmap:  CALL GRAPH: kern_init --> pmm_init-->page_init-->init_memmap--> pmm_manager->init_memmap
 *              This fun is used to init a free block (with parameter: addr_base, page_number).
 *              First you should init each page (in memlayout.h) in this free block, include:
 *                  p->flags should be set bit PG_property (means this page is valid. In pmm_init fun (in pmm.c),
 *                  the bit PG_reserved is setted in p->flags)
 *                  if this page  is free and is not the first page of free block, p->property should be set to 0.
 *                  if this page  is free and is the first page of free block, p->property should be set to total num of block.
 *                  p->ref should be 0, because now p is free and no reference.
 *                  We can use p->page_link to link this page to free_list, (such as: list_add_before(&free_list, &(p->page_link)); )
 *              Finally, we should sum the number of free mem block: nr_free+=n
 * (4) default_alloc_pages: search find a first free block (block size >=n) in free list and reszie the free block, return the addr
 *              of malloced block.
 *              (4.1) So you should search freelist like this:
 *                       list_entry_t le = &free_list;
 *                       while((le=list_next(le)) != &free_list) {
 *                       ....
 *                 (4.1.1) In while loop, get the struct page and check the p->property (record the num of free block) >=n?
 *                       struct Page *p = le2page(le, page_link);
 *                       if(p->property >= n){ ...
 *                 (4.1.2) If we find this p, then it' means we find a free block(block size >=n), and the first n pages can be malloced.
 *                     Some flag bits of this page should be setted: PG_reserved =1, PG_property =0
 *                     unlink the pages from free_list
 *                     (4.1.2.1) If (p->property >n), we should re-caluclate number of the the rest of this free block,
 *                           (such as: le2page(le,page_link))->property = p->property - n;)
 *                 (4.1.3)  re-caluclate nr_free (number of the the rest of all free block)
 *                 (4.1.4)  return p
 *               (4.2) If we can not find a free block (block size >=n), then return NULL
 * (5) default_free_pages: relink the pages into  free list, maybe merge small free blocks into big free blocks.
 *               (5.1) according the base addr of withdrawed blocks, search free list, find the correct position
 *                     (from low to high addr), and insert the pages. (may use list_next, le2page, list_add_before)
 *               (5.2) reset the fields of pages, such as p->ref, p->flags (PageProperty)
 *               (5.3) try to merge low addr or high addr blocks. Notice: should change some pages's p->property correctly.
 */

#define BUDDY_SYSTEM_SIZE 15
free_area_t free_area[BUDDY_SYSTEM_SIZE];

#define free_list(i) (free_area[i].free_list)
#define nr_free(i) (free_area[i].nr_free)

static inline struct Page *
ppn2page(int index) {
    return pages + index;
}

#define IsRealIndex 0
int mem_begin, mem_end;
int mem_size;
int GetIndexFromPage(struct Page* p) {
    if (IsRealIndex)
        return page2ppn(p);
    else 
        return page2ppn(p) - mem_begin;
}
struct Page* GetPageFromIndex(int index) {
    if (IsRealIndex)
        return ppn2page(index);
    else 
        return ppn2page(index + mem_begin);
}
static void
buddySystem_init(void) {
    int i;
    for (i = 0; i < BUDDY_SYSTEM_SIZE; i++){
        list_init(&free_list(i));
        nr_free(i) = 0;
    }
    //nr_free = 0;
}

static int 
get_buddy(int pageIndex, int layer){
    int tmp_layer = layer;
        while ((pageIndex & 1) == 0 && tmp_layer > 0){
        pageIndex >>= 1;
        tmp_layer--;
    }
    int buddy_index = 0;
    if (tmp_layer == 0) {
        if ((pageIndex & 1) == 0)
            buddy_index = (pageIndex + 1)<<layer;
        else 
            buddy_index = (pageIndex - 1)<<layer;
        if (IsRealIndex ) {
            if (buddy_index < mem_begin || buddy_index > mem_end)
            //cprintf("because of this(buddy_index:%d)->mem_begin%d, mem_end:%d\n", buddy_index, mem_begin, mem_end);
                return -1;
        }
        else {
            if (buddy_index < 0 || buddy_index >= mem_size)
                return -1;
        }
        return buddy_index;
    }
    else
        return -1;
}
int get_size_index(int n) {
    int laye_index = 0;
    int size = 1;
    if (n >= (1<<(BUDDY_SYSTEM_SIZE - 1))) {
        return BUDDY_SYSTEM_SIZE - 1;
    }
    while (size <= n) {
        size <<= 1;
        laye_index++;
    }
    return laye_index - 1;
}
static void 
buddySystem_add_to_list(struct Page *t_base, size_t t_n) {
    struct Page *base = t_base;
    size_t n = t_n;
    while (n > 0) {

        int max_index = get_size_index(n);
        int pageIndex = GetIndexFromPage(base);
        int buddy_index = -1;
        int i;
        for (i = 0; i <= max_index; ++i) {
            buddy_index = get_buddy(pageIndex, i);
            //cprintf("pageIndex:%d, layer:%d, buddy_index:%d\n", pageIndex, i, buddy_index);
            if (buddy_index == -1)
                break;
            struct Page* p = GetPageFromIndex(buddy_index);
            if (p > base && p < base + n)
                continue;
            break;
        }
        if (i > max_index) { 
                    i = max_index;
                    buddy_index = -1;
        }
        int j;
        n -= 1 << i;
        for (j = 0; j < (1 << i); ++j) {
            (base + j)->buddy_layer = -1;
        }

        if (i <= max_index && buddy_index != -1) {
            while (i < BUDDY_SYSTEM_SIZE - 1 && buddy_index != -1) {
                struct Page* p = GetPageFromIndex(buddy_index);
                if (p->buddy_layer != i)
                    break;
                list_del(&(p->page_link));
                nr_free(i) -= p->property;
                p->property = 0;
                ClearPageProperty(p);
                i++;
                base = p > base ? base : p;
                pageIndex = GetIndexFromPage(base);
                buddy_index = get_buddy(pageIndex, i);
            }
        }

        list_add(&free_list(i), &(base->page_link));
        base->buddy_layer = i;
        base->property = 1 << i;
        SetPageProperty(base);
        nr_free(i) += base->property;
        
        base += base->property;
    }
}
static void
buddySystem_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    mem_begin = page2ppn(base);
    
    mem_end = mem_begin + n - 1;
    mem_size = n;
    struct Page *p = base;
    for (; p != base + n; p ++) {
        //cprintf("%d\n", p - base);
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
        p->buddy_layer = -1;
    }
    buddySystem_add_to_list(base, n);
    //base->property = n;
    //SetPageProperty(base);
    //nr_free += n;
    //before code
    //list_add(&free_list, &(base->page_link));
    cprintf("beginPos(ppn):%d, size:%d\n", page2ppn(base), n);
    //modified by pxx --------------begin
    //list_add_before(&free_list, &(base->page_link));
    //modified by pxx --------------end
}

static struct Page *
buddySystem_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > (1<<(BUDDY_SYSTEM_SIZE - 1))) {
        return NULL;
    } 
    int index = get_size_index(n);
    if (n != (1 << index))
        index++;
    //cprintf("alloc index:%d\n", index);
    struct  Page* p = NULL;
    int i;
    for (i = index; i < BUDDY_SYSTEM_SIZE; i++) {

        if (free_list(i).next != &free_list(i)) {

            //get one
            list_entry_t *le = free_list(i).next;
            list_del(le);
            p = le2page(le, page_link);
            p->property = n;
            ClearPageProperty(p);
            nr_free(i) -= (1 << i);

            p->buddy_layer = -1;

            if (n != (1 << i)) {
                //cprintf("i:%d, free more mem:%d, ---> %d\n", i, page2ppn(p + n), (1 << i) - n);
                buddySystem_add_to_list(p + n, (1 << i) - n);
            }
            break;
        }
    }
    return p;
}

static void
buddySystem_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
        p->buddy_layer = -1;
    }
    buddySystem_add_to_list(base, n);
}
//function added by pxx
void print_buddySystem_free_link(){

    cprintf("------------------------mem-map------------------------\n");
    int i;
    for (i = 0; i < BUDDY_SYSTEM_SIZE; ++i)
    {
        cprintf("freelist(%d)[size:%d]:-> ", i, nr_free(i));
        list_entry_t *le = &free_list(i);
        while ((le = list_next(le)) != &free_list(i)) {
            struct Page *p = le2page(le, page_link);
            //cprintf("[addr:%08x,size:%d]-> ", p, p->property);
            cprintf("[offset:%d,size:%d, buddy_layer:%d]-> ", GetIndexFromPage(p), p->property, p->buddy_layer);
        }
        cprintf("(HEAD)\n");
    }
}

static size_t
buddySystem_nr_free_pages(void) {
    return nr_free(0);
}

static void
basic_check(void) {
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    print_buddySystem_free_link();
    assert((p1 = alloc_page()) != NULL);
    print_buddySystem_free_link();
    assert((p2 = alloc_page()) != NULL);
    print_buddySystem_free_link();

    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    cprintf("pageINdex:%d, %08x, %d\n", GetIndexFromPage(p0), page2pa(p0) ,  npage);
    cprintf("pageINdex:%d, %08x, %08x\n", GetIndexFromPage(p0), page2pa(p0) ,  npage * PGSIZE);
    assert(page2pa(p0) < npage * PGSIZE);
    print_buddySystem_free_link();
    cprintf("pageINdex:%d, %08x, %08x\n", GetIndexFromPage(p1), page2pa(p1) ,  npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    print_buddySystem_free_link();
    cprintf("pageINdex:%d, %08x, %08x\n", GetIndexFromPage(p2), page2pa(p2) ,  npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);

    /*
    list_entry_t free_list_store[BUDDY_SYSTEM_SIZE];
    size_t nr_free_store[BUDDY_SYSTEM_SIZE];
    int i;
    for (i = 0; i < BUDDY_SYSTEM_SIZE; ++i) {
        free_list_store[i] = free_list(i);
        nr_free_store[i] = nr_free(i);
    }
    buddySystem_init();
    assert(alloc_page() == NULL);
    assert(alloc_page() == NULL);
    free_page(p0);
    free_page(p1);
    free_page(p2);
    print_buddySystem_free_link();

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    print_buddySystem_free_link();
    for (i = 0; i < BUDDY_SYSTEM_SIZE; ++i) {
        free_list(i) = free_list_store[i];
        nr_free(i) = nr_free_store[i];
    }
*/

    print_buddySystem_free_link();
    free_page(p0);
    print_buddySystem_free_link();
    free_page(p1);
    print_buddySystem_free_link();
    free_page(p2);

    print_buddySystem_free_link();

    p0 = alloc_pages(32);
    print_buddySystem_free_link();
    free_pages(p0, 32);
    print_buddySystem_free_link();



    /*
    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    assert(alloc_page() == NULL);

    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    assert(!list_empty(&free_list));

    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;

    free_page(p);
    free_page(p1);
    free_page(p2);
    */
}

// LAB2: below code is used to check the first fit allocation algorithm (your EXERCISE 1) 
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
buddySystem_check(void) {
    print_buddySystem_free_link();
    basic_check();
    /*
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count ++, total += p->property;
    }
    //assert(total == nr_free_pages());

    basic_check();
    
    struct Page *p0 = alloc_pages(5), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    free_pages(p0 + 2, 3);
    assert(alloc_pages(4) == NULL);
    assert(PageProperty(p0 + 2) && p0[2].property == 3);
    assert((p1 = alloc_pages(3)) != NULL);
    assert(alloc_page() == NULL);
    assert(p0 + 2 == p1);

    p2 = p0 + 1;
    free_page(p0);
    free_pages(p1, 3);
    assert(PageProperty(p0) && p0->property == 1);
    assert(PageProperty(p1) && p1->property == 3);

    assert((p0 = alloc_page()) == p2 - 1);
    free_page(p0);
    assert((p0 = alloc_pages(2)) == p2 + 1);

    free_pages(p0, 2);
    free_page(p2);

    assert((p0 = alloc_pages(5)) != NULL);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    nr_free = nr_free_store;

    free_list = free_list_store;
    free_pages(p0, 5);

    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
    */
}

const struct pmm_manager buddySystem_pmm_manager = {
    .name = "buddySystem_pmm_manager",
    .init = buddySystem_init,
    .init_memmap = buddySystem_init_memmap,
    .alloc_pages = buddySystem_alloc_pages,
    .free_pages = buddySystem_free_pages,
    .nr_free_pages = buddySystem_nr_free_pages,
    .check = buddySystem_check,
};

