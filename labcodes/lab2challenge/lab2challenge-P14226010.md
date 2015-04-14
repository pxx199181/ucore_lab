#<center>lab2chellenge report</center>
* name:pxx
* id:P14226010

##练习1: 实现first-fit连续物理内存分配算法

###1. 设计实现过程
            这部分比较简单:
		first-fit算法就是说在分配内存的时候,从内存块链表中去搜索,当得到第一块内存大小比所需要分配的
            大的时候,就把这块分出去,在释放的时候,需要将地址邻接的块合并起来,为了这个要求,需要将内存链表块有
            序的进行链接, 所以在插入的时候,找到合适的位置(前面的地址比他小,或者为头节点,后面的地址比他大,或
            者为头节点),然后判断前后与插入块是否连续,连续则进行合并.
		实现需要改动的函数为:
		default_init_memmap():链表进行前插法list_add_before
		default_alloc_pages():从前往后搜索链表,有合适的就分出去,并把多出来的内存块的链到原来的位置
		default_free_pages():从前往后搜索链表,只要找到一个内存块开始的的地址比他大,或者为头节点),找
            的过程中如果发现连续就将哪些块摘出来跟他放在一块,然后将其查找找到的那个位置的前面.
		具体内容见代码,修改部分标志如下:
		//modified by pxx --------------begin
		//modified by pxx --------------end
###1. 你的first	fit算法是否有进一步的改进空间
		有改进的空间,比如分配的时候,找到合适的块后,如果该块的大小比所需的大,其实应该把后面一部分分出去,前面
            一部分在内存中的位置是不会变的,所以内存链表根本就不用调整,只需要修改其property就可以了,但是在实现的时候,
            我利用了代码中原来已有的实现,直接在那上面修改,所以偷了个懒.另外其实first-fit本身的缺陷就是每次都从第一个
            合适的就分,最后可能会导致需要大内存却没有的情况.

##练习2: 实现寻找虚拟地址对应的页表项

###1. 设计实现过程
		这部分就是填写get_page函数,原理比较简单,根据提示即可完成,但是理解起来比较有难度,尤其是地址转换,在函数
            实现的时候内存中的赋值之类的需要是线性地址,而在此时分配内存之类的全是物理地址,需要进行转换,尤其是内核地址,
            需要调高到3G以上,代码实现如下:
```
	    pde_t *tmp_pde = &pgdir[PDX(la)];
	    if (!(*tmp_pde & PTE_P)) {
		struct Page* new_page = NULL;
		if (create && (new_page = alloc_page()) != NULL) {
		    set_page_ref(new_page, 1);
		    memset(page2kva(new_page), 0, PGSIZE);
		    *tmp_pde = page2pa(new_page) | PTE_P | PTE_W | PTE_U;
		}
		else
		    return NULL;
	    }
	    return &((pte_t *)KADDR((*tmp_pde)&0xFFFFF000))[PTX(la)];
```
            对这部分当时在做的时候确实存在很多疑问,相关问题已经在piazza上面问过并得到了老师的回的
		问题题目为:1. 关于struct Page中的ref       2. 关于内核地址(lab2的第二个实验)
            主要是对内存地址的疑问,弄懂后确实对理解代码有很大的帮助.
###2. 如果ucore执行过程中访问内存,出现了页访问异常,请问硬件要做哪些事情?	
		保存异常发生的现场环境,硬件会跟据中断号找到相应的相应的异常处理函数的入口并跳转到那个地址.
###3. 如果ucore的缺页服务例程在执行过程中访问内存,出现了页访问异常,请问硬件要做哪些事情?
		这里跟正常的页异常一致,也是保存例程的环境,然后跟据中断号找到相应的相应的异常处理函数的入口并跳转到那个地址.
            相当于嵌套了一层函数.
##练习3: 释放某虚地址所在的页并取消对应二级页表项的映射

###1. 设计实现过程
		这个比较简单,首先判断是否在内存中,如果在ref减一后为零,则需要是否该页,代码实现如下:
```
	    if (!(*ptep & PTE_P)) {
		return ;
	    }
	    struct Page* page = pte2page(*ptep);
	    if (page_ref_dec(page) == 0){
		free_page(page);
	    }
	    *ptep = 0;
	    tlb_invalidate(pgdir, la);
```
###2. 数据结构Page的全局变量(其实是一个数组)的每一项与页表中的页目录项和页表项有无对应关系?如果有,其对应关系是啥
		有的,虽然有时候某些页没访问,在页目录页表中可能没有值,但是这种对应关系确是存在的,只要一访问,就会按这种关系去
            查找,对应关系在ucore代码中已经有张图,如下:
```
	// A linear address 'la' has a three-part structure as follows:
	//
	// +--------10------+-------10-------+---------12----------+
	// | Page Directory |   Page Table   | Offset within Page  |
	// |      Index     |     Index      |                     |
	// +----------------+----------------+---------------------+
	//  \--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
	//  \----------- PPN(la) -----------/
```
		这里面index就是Page的index,在ucore中Page的index位数为20位,都存在于PageTable中的前20位,不过说对应关系,应该
            反着来说, 是页目录项和页表项对应着Page数据结构全局变量, 具体查找的时候,需要从线性地址找过去, 可以说线性地址就是
            它们的对应关系,从线性地址的前十位在Page Directory数组中一个对应位置,其值的前20位为Page Table的地址, 即在Page全局
            变量中的索引index,然后根据线性地址的第11~20位找到一个具体页在Page Table数组中对应的位置,其值的前20位位具体物理页的
            地址,也即为在Page全局变量中的索引index
###3. 如果希望虚拟地址与物理地址相等,则需要如何修改lab2,完成此事
		这个实现的话就是在分配过程中,在页目录页表中进行映射的时候,根据物理地址在Page Directory和Page Table中查找相应
            的index,然后填写进去,以此保证线性地址和物理地址相等,而不是填了以后去计算虚拟地址,在这个过程中,还需要改动的一点就是
            内核地址抬高的3G需要取消,不然就会有问题.
##练习4: 扩展练习Challenge:buddy system(伙伴系统)分配算法
		这部分实现比较有意思,相关实现代码在lab2challenge中,实现的设计思路如下:
            1. 有限根据物理页的数量,确定一个空闲链表数组的大概类型:
		总大小位32261个物理页,2的15次方位32768,大概最大的块为2的14方(其实这个设置也没太多必要,因为设置少,可以多几个块,
            设置多,可以用来为以后做扩展)
            2. 如何找伙伴:
		在算法中最重要的就是查找伙伴, 一个正确的伙伴算法,并不是相邻而且同大小就会往上合并,而是在每一个的伙伴都是固定的,
            比如,对于一个物理页来说,编号0,1,2,3,4,5;这一个物理页的情况下,伙伴应该是这样的组合(0,1)(2,3)(4,5),1和2并不是伙伴,不然
            这个时候如果1和2合并的话,0号就永远不可能往上合并了,只有找对伙伴以后, 内存合并的效率才会提高, 而且定位伙伴也特别简单,
            可以这样说在不同层次,伙伴是不一样的, 关系概要如下:
		链表0(快大小为2的0次方):0,1为伙伴; 2,3为伙伴; ...;2n,(2n+1)=>2n*2^0,(2n+1)*2^0为伙伴
		链表1(快大小为2的1次方):0,2为伙伴; 4,6为伙伴; ...;4n,(4n+2)=>2n*2^1,(2n+1)*2^1为伙伴
		链表2(快大小为2的2次方):0,4为伙伴; 8,12为伙伴; ...;8n,(8n+4)=>2n*2^2,(2n+1)*2^2为伙伴
		链表3(快大小为2的3次方):0,8为伙伴; 16,24为伙伴; ...;16n,(16n+8)=>2n*2^3,(2n+1)*2^3为伙伴
            可以看到,对于2的幂次方的index来说, 只有2n*2^i或者(2n+1)*2^i才能在做为链表i中有伙伴,,而且在不同的链表层中,伙伴不一样,
            根据这个规律,得到一个伙伴查找的方法,总结为函数如下:
```
	//pageIndex为页的index, layer为链表层次
	static int 
	get_buddy(int pageIndex, int layer){
	//找到2n*2^i或者(2n+1)*2^i中的i值
	    int tmp_layer = layer;
		while ((pageIndex & 1) == 0 && tmp_layer > 0){
		pageIndex >>= 1;
		tmp_layer--;
	    }
	    int buddy_index = 0;
	    if (tmp_layer == 0) {
	//看对应这个2n*2^i或者(2n+1)*2^i中的2n还是2n+1,两个互为伙伴
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
```
            3. 查找分配层次:
		分配层次应该往上查找,也就是说对应于一个n大小的块来说,要分配,首先找到满足2^m>n的最小m值,从链表m找块分配,如果为空则
            依次往上查找,这个函数实现比较简单:
```
	//BUDDY_SYSTEM_SIZE为空闲链表组的大小
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
```
            4. 空闲链表块添加块:
		在这里首先要先实现空闲链表块添加块功能,其实在初始化,分配不完全,释放这三个地方都会将某一大小的内存块添加到空闲链表块
            中去,所以这个实现比较重要, 后面只需要调用即可.
		对于释放一个内存大小位n的内存块,其在Page中的位置struct Page *base, ,实现思路如下:
            首先将base转换位在Page中的index通过page2ppn(这个在后面会有修改),内存块示意图如下:
		------------------------------------------
		|index |index+1|index+2|.......|index+n-1|
		------------------------------------------
		添加到空闲链表块的逻辑为:
            (1) 转换n得到n对应的空闲链表的最大层次si(用get_size_index), 查找index在0到si各个层次中间的伙伴, 如果找到且其位置在index
            到index+n之间则继续(说明在本块里面), 如果找到且其位置不在index到index+n之间转至(2), 如果某一层没有伙伴就直接挑出至(3)
            (2) 对于找到伙伴了,查看器伙伴对应的层次数(buddy_layer,在page结构体中新加的域)是不是跟我跳出的层次数一致,摘除,与本块合并,然后
            往上继续合并,直到再次找到的伙伴的层次数(buddy_layer)和对应的层次数不一样或者查找的层次数大于BUDDY_SYSTEM_SIZE为止,跳出至(3)
            (3) 将最终的块添加到该层次(前面跳出的层次)的空闲链表中去,(不需要排序),设置相应的property和buddy_layer.

            5. 初始化:
		有了4的功能,初始化就简单了,首先对块进行前面一般的参数设置,然后调用4的功能即可.

            6. 分配:
		分配比较简单, 直接用3中所说的查找分配层次,如果有多余,也即2^m>n,则将2^m-n这一部分还有链回去,这个功能就是4的功能,然后返回分配的
            即可

            7. 释放:
		释放也简单, 首先清除一些标志然后直接用4的功能即可
            以上具体细节见代码

            8. 打印链表:
		直接对于链表数组进行打印,方便后面的检查
	
            9. 验证:
		修改basic_check函数,对于分配和释放的功能都是正确的,对于合并的实现,我是直接查看打印出来的链表信息做完检查结果,在最后贴部分示意图:
            感受:伙伴内存管理算法确实挺有意思的, 尤其是找伙伴的过程, 确实思考了很久, 不过直到那个添加到空闲链表的功能一实现,后续的就简单很多,因为
            三个地方都是用到这个方法, 直接写出一个函数确实简化了很多, 通过写这部分代码,对于这个框架又有了进一步的理解.发现管理内存的部分没有那么神秘.

		我发现内存页的起始位置位index为475.在这里,我用了两种方法来进行index的映射,一种是原始的index:page2ppn, 一种是修正过后的,也就是page2ppn
            在减去个mem_begin(475),虽然是个小改动,但是内存的最大块却变了.这样看起来也更明了,所以直接用修正后的index作为测试.
##buddy算法示意图:
###1. 初始化的:
```
	------------------------mem-map------------------------
	freelist(0)[size:1]:-> [offset:32260,size:1, buddy_layer:0]-> (HEAD)
	freelist(1)[size:0]:-> (HEAD)
	freelist(2)[size:4]:-> [offset:32256,size:4, buddy_layer:2]-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:0]:-> (HEAD)
	freelist(6)[size:0]:-> (HEAD)
	freelist(7)[size:0]:-> (HEAD)
	freelist(8)[size:0]:-> (HEAD)
	freelist(9)[size:512]:-> [offset:31744,size:512, buddy_layer:9]-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)
```
###2. 分配一页的:
```
	------------------------mem-map------------------------
	freelist(0)[size:0]:-> (HEAD)
	freelist(1)[size:0]:-> (HEAD)
	freelist(2)[size:4]:-> [offset:32256,size:4, buddy_layer:2]-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:0]:-> (HEAD)
	freelist(6)[size:0]:-> (HEAD)
	freelist(7)[size:0]:-> (HEAD)
	freelist(8)[size:0]:-> (HEAD)
	freelist(9)[size:512]:-> [offset:31744,size:512, buddy_layer:9]-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)
```
###3. 再分配一页的:
```
	------------------------mem-map------------------------
	freelist(0)[size:1]:-> [offset:32257,size:1, buddy_layer:0]-> (HEAD)
	freelist(1)[size:2]:-> [offset:32258,size:2, buddy_layer:1]-> (HEAD)
	freelist(2)[size:0]:-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:0]:-> (HEAD)
	freelist(6)[size:0]:-> (HEAD)
	freelist(7)[size:0]:-> (HEAD)
	freelist(8)[size:0]:-> (HEAD)
	freelist(9)[size:512]:-> [offset:31744,size:512, buddy_layer:9]-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)
```
###4. 又分配一页的:
```
	------------------------mem-map------------------------
	freelist(0)[size:0]:-> (HEAD)
	freelist(1)[size:2]:-> [offset:32258,size:2, buddy_layer:1]-> (HEAD)
	freelist(2)[size:0]:-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:0]:-> (HEAD)
	freelist(6)[size:0]:-> (HEAD)
	freelist(7)[size:0]:-> (HEAD)
	freelist(8)[size:0]:-> (HEAD)
	freelist(9)[size:512]:-> [offset:31744,size:512, buddy_layer:9]-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)
```
###5. 释放一页的:
```
	------------------------mem-map------------------------
	freelist(0)[size:1]:-> [offset:32260,size:1, buddy_layer:0]-> (HEAD)
	freelist(1)[size:2]:-> [offset:32258,size:2, buddy_layer:1]-> (HEAD)
	freelist(2)[size:0]:-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:0]:-> (HEAD)
	freelist(6)[size:0]:-> (HEAD)
	freelist(7)[size:0]:-> (HEAD)
	freelist(8)[size:0]:-> (HEAD)
	freelist(9)[size:512]:-> [offset:31744,size:512, buddy_layer:9]-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)
```
###6. 再释放一页的:
```
	------------------------mem-map------------------------
	freelist(0)[size:2]:-> [offset:32256,size:1, buddy_layer:0]-> [offset:32260,size:1, buddy_layer:0]-> (HEAD)
	freelist(1)[size:2]:-> [offset:32258,size:2, buddy_layer:1]-> (HEAD)
	freelist(2)[size:0]:-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:0]:-> (HEAD)
	freelist(6)[size:0]:-> (HEAD)
	freelist(7)[size:0]:-> (HEAD)
	freelist(8)[size:0]:-> (HEAD)
	freelist(9)[size:512]:-> [offset:31744,size:512, buddy_layer:9]-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)
	------------------------mem-map------------------------
	freelist(0)[size:1]:-> [offset:32260,size:1, buddy_layer:0]-> (HEAD)
	freelist(1)[size:0]:-> (HEAD)
	freelist(2)[size:4]:-> [offset:32256,size:4, buddy_layer:2]-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:0]:-> (HEAD)
	freelist(6)[size:0]:-> (HEAD)
	freelist(7)[size:0]:-> (HEAD)
	freelist(8)[size:0]:-> (HEAD)
	freelist(9)[size:512]:-> [offset:31744,size:512, buddy_layer:9]-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)
```
###7. 又释放一页的(出现合并,结果跟初始化的一模一样):
```
	------------------------mem-map------------------------
	freelist(0)[size:1]:-> [offset:32260,size:1, buddy_layer:0]-> (HEAD)
	freelist(1)[size:0]:-> (HEAD)
	freelist(2)[size:4]:-> [offset:32256,size:4, buddy_layer:2]-> (HEAD)
	freelist(3)[size:0]:-> (HEAD)
	freelist(4)[size:0]:-> (HEAD)
	freelist(5)[size:32]:-> [offset:31776,size:32, buddy_layer:5]-> (HEAD)
	freelist(6)[size:64]:-> [offset:31808,size:64, buddy_layer:6]-> (HEAD)
	freelist(7)[size:128]:-> [offset:31872,size:128, buddy_layer:7]-> (HEAD)
	freelist(8)[size:256]:-> [offset:32000,size:256, buddy_layer:8]-> (HEAD)
	freelist(9)[size:0]:-> (HEAD)
	freelist(10)[size:1024]:-> [offset:30720,size:1024, buddy_layer:10]-> (HEAD)
	freelist(11)[size:2048]:-> [offset:28672,size:2048, buddy_layer:11]-> (HEAD)
	freelist(12)[size:4096]:-> [offset:24576,size:4096, buddy_layer:12]-> (HEAD)
	freelist(13)[size:8192]:-> [offset:16384,size:8192, buddy_layer:13]-> (HEAD)
	freelist(14)[size:16384]:-> [offset:0,size:16384, buddy_layer:14]-> (HEAD)

