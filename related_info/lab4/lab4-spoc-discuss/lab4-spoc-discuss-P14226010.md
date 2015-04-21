# lab4 spoc discuss
* name:pxx
* id:P14226010


## 掌握知识点
1. 内核线程的启动、运行、就绪、等待、退出
2. 内核线程的管理与简单调度
3. 内核线程的切换过程

请完成如下练习，完成代码填写，并形成spoc练习报告

## 1. 分析并描述创建分配进程的过程

> 注意 state、pid、cr3，context，trapframe的含义
		这部分的填写还是比较简单的, 只要对于进程管理结构体的各个字段赋上初值就可以了。
            对于proc_struct *proc, 为了避免产生不必要的差错, 在初始化的时候, 将大部分值都赋值为0.
            state:是指	当前进程的状态, 只有当前状态是runnable的时候, 进程才能被调度.
            pid: 当前新创建的进程的id号
            cr3: 进程的页目录地址, 这个由于是内核进程, 所以所有的cr3基本上都是boot_cr3
            context: 进程的环境上下文, 用于切换到对应环境后恢复现场的值.
            trapframe: 子进程的函数栈帧, 本来应该和父进程一样,但是在这里被直接修改为, 一个监督的饿处理,
            用于调用函数, 这个在练习2中会细说.

## 练习2：分析并描述新创建的内核线程是如何分配资源的
###设计实现

> 注意 理解对kstack, trapframe, context等的初始化
		而do_fork函数就是将里面的值赋上一些有意义的值, 并根据具体环境, 将值设为专属于该进程的.
            根据代码中的提示, 设置父子进程关系进程号, 以及将进程块加入的进程链表中去
		对于新建的进程来说, 所有环境本应该都是和父进程一模一样, 但是ucore中为了简化, 将子进程
            的函数栈帧做了更改, 如下:
```

    copy_thread(struct proc_struct *proc, uintptr_t esp, struct trapframe *tf) {
    proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
    *(proc->tf) = *tf;
    proc->tf->tf_regs.reg_eax = 0;
    proc->tf->tf_esp = esp;
    proc->tf->tf_eflags |= FL_IF;

    proc->context.eip = (uintptr_t)forkret;
    proc->context.esp = (uintptr_t)(proc->tf);
}

```
		对于函数的返回地址和堆栈都修改了, 当时还奇怪为什么只有两2个page来做父子进程的栈, 其实根
            本就没有拷贝父进程的堆栈, 而是直接将返回值指定到了forkret的地方, 在kernel_thread_entry处的处理如下:

```

    kernel_thread_entry:        # void kernel_thread(void)

    pushl %edx              # push arg
    call *%ebx              # call fn

    pushl %eax              # save the return value of fn(arg)
    call do_exit            # call do_exit to terminate current thread

```
		也就是说, 只要执行完函数后就会退出, 

当前进程中唯一，操作系统的整个生命周期不唯一，在get_pid中会循环使用pid，耗尽会等待

## 练习3：阅读代码，在现有基础上再增加一个内核线程，并通过增加cprintf函数到ucore代码中
能够把进程的生命周期和调度动态执行过程完整地展现出来
		这个内在c代码中看的东西确实比较少, 只有在kernel_thread和do_fork等函数中加上cprintf打印出一部分消息, 
            但是很多汇编代码中还有更重要的东西, 没法打印, 直接跳转过去, 只能在跳转的同时加上打印语句.
		对于一个进程来说, 单独将其打印语句摘出来, 如下:
```
	in func alloc_proc
	in func kernel_thread
	in func do_fork
	in func proc_run
	in func forkret
	in func do_exit
```
		整个过程在lab4的分析中写有, 在这里直接将其拿过来:
这部分代码是调度的关键地方, 在ucore启动后, 进程进入到idle进程中去, 这个进程没有别的功能, 就是查看当前的进程是否参与需要调度, 
            是的话, 就调用schedule函数, 这个函数就是检查进程链表里面是否有准备好的(PROC_RUNNABLE)进程, 如果有, 且不是当前进程, 就参与进程切换, 
            进程切换的函数就是proc_run, 这个函数的实现非常简单, 最基本的就是三个功能函数如下:
```
            load_esp0(next->kstack + KSTACKSIZE);
            lcr3(next->cr3);
            switch_to(&(prev->context), &(next->context));
```
            这三部分分别实现三个方面, 一是确定好下一个进程的内核栈空间, 再是加载对应的页目录地址, 然后赋恢复进程寄存器环境, 就是对应的上述的三个
            函数, 这三个地方一做完, 相当于进程就切换到上一个进程的状态了, 此时在switch_to中返回的时候, 就是返回到前面压入的eip中去, 看看有哪几个
            地方压入了eip, 首先swaitch_to是一个地方, 因为下次返回到该进程的时候, 依然从switch_to函数后面接着返回到idle的schedule中去继续调度, 
            其次是do_fork函数, 在do_fork函数中有个操作copy_thread(proc, stack, tf); 就是将父进程的环境上下文拷贝到子进程中去, 那么而且此时函数
            栈也一样, 在copy的时候, 专门将做了以下处理:
```
	    proc->context.eip = (uintptr_t)forkret;
	    proc->context.esp = (uintptr_t)(proc->tf);
```
            专门将返回的eip置为forkret函数的地址, 而esp置为上一个函数栈帧的结构, 在forkret中调用forkrets(current->tf)函数, 它用汇编实现, 如下:
```
	.globl forkrets
	forkrets:
	    # set stack to this new process's trapframe
	    movl 4(%esp), %esp
	    jmp __trapret
```
            此时, 如果在switch_to中如果切换的话, 那么新进程就会根据eip进入到forkret中去, 由于传入的参数为current->tf, 而且当前的栈帧已经在switch_to
            中设置好, 所以, 此时eip最终跳转到__trapret地方, 其各个段的值都已经在esp中存在, 此时由于ucore中处理的比较简单直接弹出即可, 其代码如下:
```
	.globl __trapret
	__trapret:
	    # restore registers from stack
	    popal

	    # restore %ds, %es, %fs and %gs
	    popl %gs
	    popl %fs
	    popl %es
	    popl %ds

	    # get rid of the trap number and error code
	    addl $0x8, %esp
	    iret
```
            在此返回就会进入到, do_fork函数的上一个函数帧中去, 照理说应该就是do_fork返回的地方, 但其实不是, 在这里把栈帧以及修改了, 应该是tf指定的栈帧.
		从上面两个进程切换返回的地方来看, 进程都是可以正常往下执行的, 但是ucore中实现的比较简单, 要一直等待进程执行完毕, 而且在细看
            进程的初始化中kernel_thread函数, 如下:
```
	int
	kernel_thread(int (*fn)(void *), void *arg, uint32_t clone_flags) {
	    struct trapframe tf;
	    memset(&tf, 0, sizeof(struct trapframe));
	    tf.tf_cs = KERNEL_CS;
	    tf.tf_ds = tf.tf_es = tf.tf_ss = KERNEL_DS;
	    tf.tf_regs.reg_ebx = (uint32_t)fn;
	    tf.tf_regs.reg_edx = (uint32_t)arg;
	    tf.tf_eip = (uint32_t)kernel_thread_entry;
	    return do_fork(clone_flags | CLONE_VM, 0, &tf);
	}
```
		在这里, kernel_thread人为的把传入给子进程的tf栈帧修改了, eip改为kernel_thread_entry, 也就是说, 函数的上一个栈帧入口即为
            kernel_thread_entry, 所以最终子进程第一次切换的时候, 会进入到kernel_thread_entry中去, 其代码实现如下:
```
	.globl kernel_thread_entry
	kernel_thread_entry:        # void kernel_thread(void)

	    pushl %edx              # push arg
	    call *%ebx              # call fn

	    pushl %eax              # save the return value of fn(arg)
	    call do_exit            # call do_exit to terminate current thread
```
		可以看见, 在进入到这里的时候, 首先调用了*%ebx, 这个在前面已经赋值:tf.tf_regs.reg_ebx = (uint32_t)fn;这个就是功能函数, 所以功能
            函数执行完毕后, 会在此进入到这个call后的里面去处理, 最终调用call do_exit, 来释放进程代码, 其实这里没有释放工作, 只是引发了一个panic
            操作, 这部分应该是下个lab需要实现的功能, 在这里已经完成了整个调度工作, 如果把do_exit工作做好完成, 后面还能进入到调度里面去, 参与下一
            轮的调度. 
		至此, 整个切换工作算是完成了.在进程完成后, 最终完成调用do_exit, 退出, 由最终的父进程的do_wait来释放相关资源.
## 扩展练习4：增加可以睡眠的内核线程，睡眠的条件和唤醒的条件可自行设计，并给出测试用例，并在spoc练习报告中给出设计实现说明
		在这里实现的时候, 为了简化, 我是用的循环来替代时钟的计数, 刚开始的时候, 初始化两个进程, 将其中一个置为sleeping, 然后在另外一个函
            执行到固定的位置时, 会设置其状态位runnable, 然后进入调度...
		睡眠进程sleep_func函数如下:
```
	void sleep_func(void *args) { 
	    int pid = global_pid;
	    struct proc_struct *proc = find_proc(pid);
	    proc->wait_state = PROC_SLEEPING;
	    schedule();
	    cprintf(" kernel_thread, pid = %d, name = %s\n", current->pid, get_proc_name(current));
	    cprintf("sleep over\n");
	    cprintf(" kernel_thread, pid = %d, name = %s\n", current->pid, get_proc_name(current));
	    cprintf("go back\n");
	}
```
		唤醒进程wake_up_func函数实现如下:
```
	void wake_up_func(void *args) {
	    int pid = global_pid;
	    cprintf(" kernel_thread, pid = %d, name = %s\n", current->pid, get_proc_name(current));
	    int i;
	    for (i = 1; i < 1000000; ++i) {
		if (i % 5000 == 0)  {
		    cprintf(" kernel_thread, pid = %d, name = %s\n", current->pid, get_proc_name(current));
		    cprintf("add 5000\n");
		    schedule();
		}
	    }
	    cprintf(" kernel_thread, pid = %d, name = %s\n", current->pid, get_proc_name(current));
	    cprintf("ready to wake up pid = %d\n", pid);
	    struct proc_struct *proc = find_proc(pid);
	    proc->wait_state = PROC_RUNNABLE;
	    schedule();
	    cprintf(" kernel_thread, pid = %d, name = %s\n", current->pid, get_proc_name(current));
	    cprintf("ready to wake up pid = %d over\n", pid);
	}
```
		初始化两个进程的代码如下:
```
	    int pid3= kernel_thread(sleep_func, "nothing", 0);
	    int pid4= kernel_thread(wake_up_func, "nothing", 0);
```
		通过输出, 可以看见, 只有当进程wake_up_func执行到将另外一个进程的状态设置为runnable的时候, 再次进行切换才能真正的进入到sleep进程
            中去. 结果如下:(前面进程5无论切换多少次, 都不会调度到进程4去, 只有当进程5将进程4的状态设置为runnable后, 进程4才能出于运行)
```
 kernel_thread, pid = 5, name = 

in func proc_run
 kernel_thread, pid = 1, name = init1 , arg  init main1: Hello world!! 
in func proc_run
 kernel_thread, pid = 2, name = init2 , arg  init main2: Hello world!! 
in func proc_run
 kernel_thread, pid = 4, name = 
sleep over
 kernel_thread, pid = 4, name = 
go back
in func do_exit
 do_exit: proc pid 4 will exit
 do_exit: proc  parent c02ff008
in func proc_run

in func proc_run
 kernel_thread, pid = 1, name = init1 ,  en.., Bye, Bye. :)
in func do_exit
 do_exit: proc pid 1 will exit
 do_exit: proc  parent c02ff008
in func proc_run
 kernel_thread, pid = 2, name = init2 ,  en.., Bye, Bye. :)
in func do_exit
 do_exit: proc pid 2 will exit
 do_exit: proc  parent c02ff008
in func proc_run
 kernel_thread, pid = 5, name = 
add 5000
......//many times 
 kernel_thread, pid = 5, name = 
add 5000
 kernel_thread, pid = 5, name = 
ready to wake up pid = 4
 kernel_thread, pid = 5, name = 
ready to wake up pid = 4 over
in func do_exit
 do_exit: proc pid 5 will exit
 do_exit: proc  parent c02ff008
in func proc_run
do_wait: begin
do_wait: has kid find child  pid1
do_wait: begin
do_wait: has kid find child  pid2
do_wait: begin
do_wait: has kid find child  pid3
do_wait: begin
do_wait: has kid find child  pid4
do_wait: begin
do_wait: has kid find child  pid5
do_wait: begin
100 ticks

```
