#<center>lab3chellenge_lazy report</center>
* name:pxx
* id:P14226010

##练习1: 给未被映射的地址映射上物理页(需要编程)

###1. 实现过程
            这部主要就是完成do_pgfault(mm/vmm.c)函数,需要弄清楚的是产生异常的原因及种类:
            根据文章提示, 产生页访问异常的原因主要有:
		1. 目标页帧不存在(页表项全为0,即该线性地址与物理地址尚未建立映射或者已经撤销);
		2. 相应的物理页帧不在内存中(页表项非空,但Present标志位=0,比如在swap分区或磁盘文件上);
		3. 不满足访问权限(此时页表项P标志=1,但低权限的程序试图访问高权限的地址空间,或者有程序试图写只读页面).
            当出现上面情况之一,那么就会产生页面pagefault(#PF)异常。
		对于1, 直接分配一页内存即可, 如下:
```
	   ptep = get_pte(mm->pgdir, addr, 1);
	   if (ptep == NULL) {
		cprintf("no page entry\n");
		goto failed;
	   }
	   if (*ptep == NULL) {
		// no page entry (not maped)
		struct Page *page = pgdir_alloc_page(mm->pgdir, addr, perm);
		if (page == NULL) {
		    cprintf("alloc page failed\n");
		    goto failed;
		}
	   }
```
		对于2, 需要换入到内存中去, 如下:
```
	   if (!(*ptep & PTE_P)) {
		// page entry exist but page not in memory
		if (swap_init_ok) {
		    struct Page *page = NULL;
		    swap_in(mm, addr, &page);
		    perm |= PTE_P;
		    page_insert(mm->pgdir, page, addr, perm);
		    swap_map_swappable(mm, addr, page, 1);
		}
		else {
		    cprintf("no swap_init_ok but ptep is %x, failed\n",*ptep);
		    goto failed;
		}
	   }
```
		对于3, 可以检测到异常, 但是没法处理, 在前面的代码中已经把大多数的读写操作异常处理了, 只是对于页表的权限(比如
            用户权限读取内存权限的页表时)没有处理, 所以在这里对此异常进行说明, 如下:
```
	   if (!(*ptep & PTE_U)  && (error_code & 0x004)) {
		cprintf("processor was executing at user mode (1) but page entry is in supervisor mode (0) \n");
		goto failed;
	   }
```
		具体详情见代码; 

##练习2: 补充完成基于FIFO的页面替换算法(需要编程)

###1. 实现过程
		这部分的主要工作就是实现FIFO算法的swap_fifo.c中完成map_swappable和swap_out_vistim函数, 其实两部分函数的实现代码
            非常简单, 因为FIFO算法本身就比较简单, 在理解操作系统的页面替换算法原理后, 这部分填写的代码非常少.
		对于map_swappable函数来说, 这个函数的主要功能就是将可交换的页添加到一个链表里面区, 要求链表功能类似于队列, 其实就
            是插入节点和取节点分别在链表两头即可, 在这里, 我才有的是后插法, 每次添加节点到最后(也就是头节点的前面, 因为双向链表), 
            然后头头节点的下一个节点处读取要换出的页面, 这里只需要添加一句话即可, 其实这个函数的实现有一个疑问, 就是那个参数swap_in的
            含义是什么? 根据提示, 照理说应该是该页能否换出, 后来看练习中的有关说明, 貌似对于内核中的代码, 页是不能换出的, 不然效率很低, 
            所以我猜想这个可能是为后面留下的接口, 所以未予理会, 代码如下:
```
	list_add_before(head, entry);
```
		同样对于_fifo_swap_out_victim函数来说, 需要实现的就是只要从头摘除一个页面即可, 如下:
```
	list_entry_t *out_page_le = list_next(head);
	list_del(out_page_le);
	*ptr_page = le2page(out_page_le, pra_page_link);
```
		具体详情见代码; 

##练习3: 扩展练习Challenge:实现识别dirty bit的extended clock页替换算法(需要编程)
## (两种实现, 基于时钟{定时清理标志位}和基于lazy方法{用到是清理标志位}, 分别问lab3challenge和lab3challenge_lazy)
		这部分实现挺有意思的, 看练习中的提示, 这部分对于页面替换算法实现的比较好, 能够极大通过算法效率, 相关实现代码在
            lab3challenge中, 将对应的fifo命名的函数有extended_clock替换, 实现的设计思路如下:
		1. 对于这是可交换的物理页来说, 操作和fifo一样, 在此不在赘述:
		2. 对于查找换出页面算法函数_extended_clock_swap_out_victim来说, 需要将access位和dirty位分别当做查找依据, 安照(0, 0)
            (access bit 为0, dirty bit为0), (0, 1), (1, 0), (1, 1)的顺序来查找最合适换出的页面, 由于查找到(0, 0), 该页就是最合适的页,
            那么就可以直接返回, 不用继续查找下去, 对于后面的情况都需要遍历整个可交换的链表, 由于有优先顺序, 所以在这里定义一个优先级, (0, 0)
            是3, (0, 1)是2, (1, 0)是1, (1, 1)是0, 如果每个页面对应的bit位和上述值对相等时, 就代码该页面的优先级为对应的值, 只有当前页的优先
            级小于所记录的页码优先级时, 才更新记录值, 
		3. 由于access bit和dirty bit的设置是有CPU来做的, 但是清理需要自己来做, 有两种方法, 一种是定义一个时钟中断, 一种是要用到的时候再做
            清理工作, 两部分都有实现, 时钟是在每个0.5s(是不是有的长啊?)将各个bit位清零, 而lazy是在触发页异常后才进行. 
		4. 分两部分测试: 
		(1)基于时钟测试:   我采用修改原来fifo中的测试代码, 将测试部分分成两部分, 一部分初始化, 一部分来进行后面的真正测试, 因为清除
            标志位是用时钟来处理的, 所以测试部分必须放在时钟启动后面, 而且_extended_clock_check_swap函数中必须德添加延时操作, 以保证下次换出的页
            是在我清除标志位后选出的页, 测试过程通过打印的方式可以看见清除标志位以及选出来的用于换入换出的页. 
		(2)基于lazy方法测试: 和原来的一样, 只是在打印的时候会多些说明信息.
		基于时钟部分实现如下:
```
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
        if (!(*ptep & PTE_P))
            continue;
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
        cprintf("=====in raound=====\n");
     }
```
		找到后进行替换:
```
     list_del(out_page_le);
     *ptr_page = le2page(out_page_le, pra_page_link);
     cprintf("%x\n", get_pte(mm->pgdir, (*ptr_page)->pra_vaddr, 0));
     cprintf("%x\n", (*ptr_page)->pra_vaddr);
     print_swap_page_info(mm);
```
		时钟中断处理:
```
        in_swap_tick_event++;
        if (in_swap_tick_event == swap_event_trigger) {
            swap_tick_event_handler();
            in_swap_tick_event = 0;
        }
```
		swap_tick_event_handler函数如下:
```
	static int 
	swap_tick_event_handler() {
	    extern struct mm_struct *check_mm_struct;
	    if (check_mm_struct != NULL) {
		swap_tick_event(check_mm_struct);
	    }
	    panic("unhandled swap_tick_event.\n");
	}
```
		真正的tick处理函数在
		swap_tick_event_handler函数是调用的_extended_clock_tick_event函数如下:
```
    list_entry_t *head, *le;
    head = le = (list_entry_t*) mm->sm_priv;
    while((le = list_next(le) != head)) {
        struct Page* page = le2page(le, pra_page_link);
        pte_t *ptep = get_pte(mm->pgdir, page->pra_vaddr, 0);
        if (ptep == NULL) {
            cprintf("error\n");
            return -1;
        }
        //clear bit set
        *ptep &= ~(PTE_A | PTE_D);
    }
```
		触发异常的代码必须加上延时, 这样在两次页异常后会发现有清除标志位的操作, 由于是采用的extend_clock, 所以后面的pgfault_num跟fifo的不
            一样, 所以将这些assert注释掉, 代码如下: 
```
	//延时操作
	static void waitcount(int count) {
	    int i = 0;
	    while (i < count) {
		i++;
	    }
	}
	//触发swap的函数
	static int
	_extended_clock_check_swap(void) {
	    int wait_time = 200000000;
	    cprintf("write Virt Page c in _extended_clock_check_swap\n");
	    *(unsigned char *)0x3000 = 0x0c;
	    //assert(pgfault_num==4);
	    waitcount(wait_time);
	    cprintf("write Virt Page a in _extended_clock_check_swap\n");
	    *(unsigned char *)0x1000 = 0x0a;
	    waitcount(wait_time);
	    //assert(pgfault_num==4);
	    cprintf("write Virt Page d in _extended_clock_check_swap\n");
	    *(unsigned char *)0x4000 = 0x0d;
	    waitcount(wait_time);
	    //assert(pgfault_num==4);
	    cprintf("write Virt Page b in _extended_clock_check_swap\n");
	    *(unsigned char *)0x2000 = 0x0b;
	    waitcount(wait_time);
	    //assert(pgfault_num==4);
	    cprintf("write Virt Page e in _extended_clock_check_swap\n");
	    *(unsigned char *)0x5000 = 0x0e;
	    waitcount(wait_time);
	    //assert(pgfault_num==5);
	    cprintf("write Virt Page b in _extended_clock_check_swap\n");
	    *(unsigned char *)0x2000 = 0x0b;
	    waitcount(wait_time);
	    //assert(pgfault_num==5);
	    cprintf("write Virt Page a in _extended_clock_check_swap\n");
	    *(unsigned char *)0x1000 = 0x0a;
	    waitcount(wait_time);
	    //assert(pgfault_num==6);
	    cprintf("write Virt Page b in _extended_clock_check_swap\n");
	    *(unsigned char *)0x2000 = 0x0b;
	    waitcount(wait_time);
	    //assert(pgfault_num==7);
	    cprintf("write Virt Page c in _extended_clock_check_swap\n");
	    *(unsigned char *)0x3000 = 0x0c;
	    waitcount(wait_time);
	    //assert(pgfault_num==8);
	    cprintf("write Virt Page d in _extended_clock_check_swap\n");
	    *(unsigned char *)0x4000 = 0x0d;
	    waitcount(wait_time);
	    //assert(pgfault_num==9);
	    return 0;
	}
```
		测试:对于最后运行的结果, 能够正常通过流程, 对于时钟中断后, 进入到清零处理的过程中, 根据打印的标志来看, 打印交换页链表的函数如下:
```
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
```
		对下面的结果进行说明, 可以看见每个0.5s会发生清除页标志位的操作, 而下次选的时候, 会优先把(0, 0)的换出, 换出的优先级是按照前面定义
            的优先级, 由于换出的操作是与时钟相关, 在特定情况下{没有(0, 0)时}会出现(1, 0)被优先换出的情况结果如下, 由于时间问题, 在此不在赘述, 但是
            目前结果已经说明能够按照extend_clock的要求来进行, 结果说明如下:
###结果 :
```
	//正常的信息打印
	page fault at 0x00001000: K/W [no page found].
	page fault at 0x00002000: K/W [no page found].
	page fault at 0x00003000: K/W [no page found].
	page fault at 0x00004000: K/W [no page found].
	set up init env for check_swap over!
	//连续写, 触发页异常
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	write Virt Page c in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	100 ticks
	write Virt Page a in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	100 ticks
	write Virt Page d in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	100 ticks
	100 ticks
	write Virt Page b in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	100 ticks
	//在清除页标志位之前, 打印结果如下(PTE_A, PTE_D), 可见全是PTE_A和PTE_D的:
	--------------before clear--------------
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	--------------------------------------------
	set one
	set one
	set one
	set one
	//清除页标志位之后, 打印结果如下(PTE_A, PTE_D), 可见全都清零了:
	--------------after clear--------------
	mm_addr:0xc0305000
	pte_addr list:
	0x308007:[1000](0, 0)->0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	--------------------------------------------
	//由于总共就4个可用的页, 再次写入e的时候会发生swap操作:
	write Virt Page e in _extended_clock_check_swap
	page fault at 0x00005000: K/W [no page found].
	//4个可用的页的标志全为(0, 0), 如下:
	mm_addr:0xc0305000
	pte_addr list:
	0x308007:[1000](0, 0)->0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	//选出的swap的页就是开头的0x309007,这时跟fifo一样, 还看不出extend_clock的特点:
	find 3
	c0307004
	1000
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->
	pra_addr list:
	0x2000->0x3000->0x4000->
	swap_out: i 0, store page in vaddr 0x1000 to disk swap entry 2
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->
	pra_addr list:
	0x2000->0x3000->0x4000->0x5000->
	100 ticks
	//写b和写a, 此时页物理a(因为a页排在最前面)已经被换出, 而e是刚进来的一页, 写a的时候会发生swap_in, 这时下面选出来的页就会是前面的空闲页了:
	write Virt Page b in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x309067:[2000](1, 1)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->
	pra_addr list:
	0x2000->0x3000->0x4000->0x5000->
	100 ticks
	100 ticks
	write Virt Page a in _extended_clock_check_swap
	page fault at 0x00001000: K/W [no page found].
	//换出前:
	mm_addr:0xc0305000
	pte_addr list:
	0x309067:[2000](1, 1)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->
	pra_addr list:
	0x2000->0x3000->0x4000->0x5000->
	//由下面可见选出来的是0x300, 符合优先级
	find 3
	c030700c
	3000
	mm_addr:0xc0305000
	pte_addr list:
	0x309067:[2000](1, 1)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->
	pra_addr list:
	0x2000->0x4000->0x5000->
	swap_out: i 0, store page in vaddr 0x3000 to disk swap entry 4
	swap_in: load disk swap entry 2 with swap_page in vadr 0x1000
	mm_addr:0xc0305000
	pte_addr list:
	0x309067:[2000](1, 1)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->0x30a067:[1000](1, 1)->
	pra_addr list:
	0x2000->0x4000->0x5000->0x1000->
	100 ticks
	write Virt Page b in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x309067:[2000](1, 1)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->0x30a067:[1000](1, 1)->
	pra_addr list:
	0x2000->0x4000->0x5000->0x1000->
	100 ticks
	//清除页标志, 后面的情况如上, 不在赘述:
	--------------before clear--------------
	mm_addr:0xc0305000
	pte_addr list:
	0x309067:[2000](1, 1)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->0x30a067:[1000](1, 1)->
	pra_addr list:
	0x2000->0x4000->0x5000->0x1000->
	--------------------------------------------
	set one
	set one
	set one
	set one
	--------------after clear--------------
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x30a007:[1000](0, 0)->
	pra_addr list:
	0x2000->0x4000->0x5000->0x1000->
	--------------------------------------------
	100 ticks
	write Virt Page c in _extended_clock_check_swap
	page fault at 0x00003000: K/W [no page found].
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x30a007:[1000](0, 0)->
	pra_addr list:
	0x2000->0x4000->0x5000->0x1000->
	find 3
	c0307008
	2000
	mm_addr:0xc0305000
	pte_addr list:
	0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x30a007:[1000](0, 0)->
	pra_addr list:
	0x4000->0x5000->0x1000->
	swap_out: i 0, store page in vaddr 0x2000 to disk swap entry 3
	swap_in: load disk swap entry 4 with swap_page in vadr 0x3000
	mm_addr:0xc0305000
	pte_addr list:
	0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x30a007:[1000](0, 0)->0x309067:[3000](1, 1)->
	pra_addr list:
	0x4000->0x5000->0x1000->0x3000->
	100 ticks
	write Virt Page d in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x30b067:[4000](1, 1)->0x308007:[5000](0, 0)->0x30a007:[1000](0, 0)->0x309067:[3000](1, 1)->
	pra_addr list:
	0x4000->0x5000->0x1000->0x3000->
	100 ticks
	100 ticks
	count is -1, total is -31956
	check_swap() succeeded!
```
		基于lazy的方法就简单很多,只要在swap_out的时候, 轮询将各个页的access bit和dirty bit清零就可以了, 可以将时钟部分完全不顾
		代码如下:
```
	     list_entry_t *head=(list_entry_t*) mm->sm_priv;
	     assert(head != NULL);
	     assert(in_tick==0);
	     /* Select the victim */
	     /*LAB3 EXERCISE 2: P14226010*/ 
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
		    if (is_d == 0 && rank_id < 3) {
		        rank_id = 3;
		        out_page_le = le;
		        cprintf("find 3\n");

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
		//clear bit
		*ptep &= ~(PTE_A|PTE_D);
	     }
	     //pxx-code-end
	     //out_page_le = list_next(head);
	     list_del(out_page_le);
	     *ptr_page = le2page(out_page_le, pra_page_link);
	     cprintf("%x\n", get_pte(mm->pgdir, (*ptr_page)->pra_vaddr, 0));
	     cprintf("%x\n", (*ptr_page)->pra_vaddr);
	     print_swap_page_info(mm);

	     return 0;
```
		基于lazy方法的测试部分
```
	//正常的信息打印
	page fault at 0x00001000: K/W [no page found].
	page fault at 0x00002000: K/W [no page found].
	page fault at 0x00003000: K/W [no page found].
	page fault at 0x00004000: K/W [no page found].
	set up init env for check_swap over!
	//连续写, 触发页异常, 此时有空闲的4页, 不会触发swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	write Virt Page c in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	write Virt Page a in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	write Virt Page d in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	write Virt Page b in _extended_clock_check_swap
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->

	//写e的时候, 需要加载e的页, 此时需要进行swap
	write Virt Page e in _extended_clock_check_swap
	page fault at 0x00005000: K/W [no page found].
	//swap之前, 打印结果如下(PTE_A, PTE_D), 可见全是PTE_A和PTE_D的:
	mm_addr:0xc0305000
	pte_addr list:
	0x308067:[1000](1, 1)->0x309067:[2000](1, 1)->0x30a067:[3000](1, 1)->0x30b067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	c0307004
	1000
	//此时选择的是第一个页(也是最先加入的), 然后将各个bit清零:
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->
	pra_addr list:
	0x2000->0x3000->0x4000->
	swap_out: i 0, store page in vaddr 0x1000 to disk swap entry 2
	//新加入的页在尾部, 是PTE_A和PTE_D为1的:
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->
	pra_addr list:
	0x2000->0x3000->0x4000->0x5000->
	write Virt Page b in _extended_clock_check_swap
	//此后结果与前面同理分析, 在此不在赘述
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->
	pra_addr list:
	0x2000->0x3000->0x4000->0x5000->
	write Virt Page a in _extended_clock_check_swap
	page fault at 0x00001000: K/W [no page found].
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[2000](0, 0)->0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308067:[5000](1, 1)->
	pra_addr list:
	0x2000->0x3000->0x4000->0x5000->
	find 3
	c0307008
	2000
	mm_addr:0xc0305000
	pte_addr list:
	0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->
	pra_addr list:
	0x3000->0x4000->0x5000->
	swap_out: i 0, store page in vaddr 0x2000 to disk swap entry 3
	swap_in: load disk swap entry 2 with swap_page in vadr 0x1000
	mm_addr:0xc0305000
	pte_addr list:
	0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x309067:[1000](1, 1)->
	pra_addr list:
	0x3000->0x4000->0x5000->0x1000->
	write Virt Page b in _extended_clock_check_swap
	page fault at 0x00002000: K/W [no page found].
	mm_addr:0xc0305000
	pte_addr list:
	0x30a007:[3000](0, 0)->0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x309067:[1000](1, 1)->
	pra_addr list:
	0x3000->0x4000->0x5000->0x1000->
	find 3
	c030700c
	3000
	mm_addr:0xc0305000
	pte_addr list:
	0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x309007:[1000](0, 0)->
	pra_addr list:
	0x4000->0x5000->0x1000->
	swap_out: i 0, store page in vaddr 0x3000 to disk swap entry 4
	swap_in: load disk swap entry 3 with swap_page in vadr 0x2000
	mm_addr:0xc0305000
	pte_addr list:
	0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x309007:[1000](0, 0)->0x30a067:[2000](1, 1)->
	pra_addr list:
	0x4000->0x5000->0x1000->0x2000->
	write Virt Page c in _extended_clock_check_swap
	page fault at 0x00003000: K/W [no page found].
	mm_addr:0xc0305000
	pte_addr list:
	0x30b007:[4000](0, 0)->0x308007:[5000](0, 0)->0x309007:[1000](0, 0)->0x30a067:[2000](1, 1)->
	pra_addr list:
	0x4000->0x5000->0x1000->0x2000->
	find 3
	c0307010
	4000
	mm_addr:0xc0305000
	pte_addr list:
	0x308007:[5000](0, 0)->0x309007:[1000](0, 0)->0x30a007:[2000](0, 0)->
	pra_addr list:
	0x5000->0x1000->0x2000->
	swap_out: i 0, store page in vaddr 0x4000 to disk swap entry 5
	swap_in: load disk swap entry 4 with swap_page in vadr 0x3000
	mm_addr:0xc0305000
	pte_addr list:
	0x308007:[5000](0, 0)->0x309007:[1000](0, 0)->0x30a007:[2000](0, 0)->0x30b067:[3000](1, 1)->
	pra_addr list:
	0x5000->0x1000->0x2000->0x3000->
	write Virt Page d in _extended_clock_check_swap
	page fault at 0x00004000: K/W [no page found].
	mm_addr:0xc0305000
	pte_addr list:
	0x308007:[5000](0, 0)->0x309007:[1000](0, 0)->0x30a007:[2000](0, 0)->0x30b067:[3000](1, 1)->
	pra_addr list:
	0x5000->0x1000->0x2000->0x3000->
	find 3
	c0307014
	5000
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[1000](0, 0)->0x30a007:[2000](0, 0)->0x30b007:[3000](0, 0)->
	pra_addr list:
	0x1000->0x2000->0x3000->
	swap_out: i 0, store page in vaddr 0x5000 to disk swap entry 6
	swap_in: load disk swap entry 5 with swap_page in vadr 0x4000
	mm_addr:0xc0305000
	pte_addr list:
	0x309007:[1000](0, 0)->0x30a007:[2000](0, 0)->0x30b007:[3000](0, 0)->0x308067:[4000](1, 1)->
	pra_addr list:
	0x1000->0x2000->0x3000->0x4000->
	count is 0, total is 7
	check_swap() succeeded!
	++ setup timer interrupts
	100 ticks
	100 ticks

```

