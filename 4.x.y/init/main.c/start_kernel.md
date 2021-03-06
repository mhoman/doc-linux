start_kernel - init/main.c
========================================

__mmap_switched在为内核跳转到start_kernel C函数准备运行环境:

https://github.com/leeminghao/doc-linux/blob/master/4.x.y/arch/arm/kernel/head-common.S/__mmap_switched.md

asmlinkage vs __init
----------------------------------------

```
asmlinkage void __init start_kernel(void)
{
    char * command_line;
    extern const struct kernel_param __start___param[], __stop___param[];
```

asmlinkage和__init这两个宏是写内核代码的一种特定表示，一种尽可能快的思想表达，一种尽可能占用空间
少的思路。

* asmlinkage
  是一个宏定义，它的作用主要有两个，一个是让传送给函数的参数全部使用栈式传送，不用寄存器来传送。
  因为寄存器的个数有限，使用栈可以传送更多的参数，比如在X86的CPU里只能使用6个寄存器传送，只能
  传送4个参数，而使用栈就没有这种限制；另外一个用处是声明这个函数是给汇编代码调用的。
  不过在ARM体系里，并没有使用栈传送参数的特性，原因何在？由于ARM体系的寄存器个数比较多，
  多达13个，这样绝大多数的函数参数都可以通过寄存器来传送，达到高效的目标。

* __init
  这个宏定义主要用来标志这个函数编译出来的目标代码放在那一段里。对于应用程序的编译和连接，
  不需要作这样的考虑，但是对于内核代码来说，就需要了，因为不同的段代码有着不同的作用，
  比如初始化段的代码，当系统运行正常以后，这段代码就没有什么用了，聪明的做法就是回收这
  段代码占用的内存，让内核的代码占最少的内存。还有另外一个作用，比如同一段的代码都是编译在一起，
  让相关联的代码尽可能同在一片内存里，这样当CPU加载代码到缓存时，就可以一起命中，提高缓存的
  命中率，这样就大大提高代码的执行速度。

```
#define __init		__section(.init.text) __cold notrace
```

使用这个宏声明的函数，编译时就会把目标代码放到段.init.text里，这段都是放置初始化的代码。

1.lockdep_init
----------------------------------------

lockdep是linux内核的一个调试模块，用来检查内核互斥机制尤其是自旋锁潜在的死锁问题。
自旋锁由于是查询方式等待，不释放处理器，比一般的互斥机制更容易死锁，故引入lockdep
检查以下几种情况可能的死锁:

* 同一个进程递归地加锁同一把锁；
* 一把锁既在中断（或中断下半部）使能的情况下执行过加锁操作，又在中断（或中断下半部）里
  执行过加锁操作。这样该锁有可能在锁定时由于中断发生又试图在同一处理器上加锁；
* 几把锁形成一个闭环死锁。加锁后导致依赖图产生成闭环，这是典型的死锁现象。

```

    /*
     * Need to run as early as possible, to initialize the
     * lockdep hash:
     */
    lockdep_init();
```

2.smp_setup_processor_id
----------------------------------------

这个函数主要作用是获取当前正在执行初始化的处理器ID。smp_setup_processor_id()函数每次都要中断CPU
去获取ID，这样效率比较低。由于单处理器的频率已经慢慢变得不能再高了，那么处理器的计算速度还要提高，
还有别的办法吗？这样自然就想到多个处理器的技术。这就好比物流公司，有很多货只让一辆卡车在高速公路
上来回运货，这样车的速度已经最快了，运的货就一定了，不可能再多得去。那么物流公司想提高运货量，那
只能多顾用几台卡车了，这样运货量就比以前提高了。处理器的制造厂家自然也想到这样的办法，就是几个
处理器放到一起，这样就可以提高处理速度。接着下来的问题又来，那么这几个处理器怎么样放在一起，
成本最低，性能最高。考虑到这样的一种情况，处理器只有共享主内存、总线、外围设备，其它每个处理器是
独立的，这样可以省掉很多外围硬件成本。当然所有这些处理器还共享一个操作系统，这样的结构就叫做
对称多处理器（SymmetricalMulti-Processing）。在对称多处理器系统里，所有处理器只有在初始化阶段
处理有主从之分，到系统初始化完成之后，大家是平等的关系，没有主从处理器之分了。在内核里所有以
smp开头的函数都是处理对称多处理器相关内容的，对称多处理器与单处理器在操作系统里，主要区别是
引导处理器与应用处理器，每个处理器不同的缓存，中断协作，锁的同步。因此，在内核初始化阶段需要区分，
在与缓存同步数据需要处理，在中断方面需要多个处理协作执行，在多个进程之间要做同步和通讯。如果内核
只是有单处理器系统，smp_setup_processor_id()函数是是空的，不必要做任保的处理。

```
    smp_setup_processor_id();
```

注意: 与smp_processor_id()函数区别，这两个函数都是获取多处理器的ID，为什么会需要两个函数呢？
其实这里有一个差别的，smp_setup_processor_id()函数可以不调用setup_arch()初始化函数就可以使用，
而smp_processor_id()函数是一定要调用setup_arch()初始化函数后，才能使用。smp_setup_processor_id()
函数是直接获取对称多处理器的ID，而smp_processor_id()函数是获取变量保存的处理器ID，因此一定要调用
初始化函数。由于smp_setup_processor_id()函数不用调用初始化函数，可以放在内核初始化start_kernel
函数的最前面使用，而函数smp_processor_id()只能放到setup_arch()函数调用的后面使用了。


3.debug_objects_early_init
----------------------------------------

这个函数主要作用是对调试对象进行早期的初始化，其实就是HASH锁和静态对象池进行初始化。

```
    debug_objects_early_init();
```

4.cgroup_init_early
----------------------------------------

cgroup: 它的全称为control group.即一组进程的行为控制.比如,我们限制进程/bin/sh的CPU使用为20%.
我们就可以建一个cpu占用为20%的cgroup. 然后将/bin/sh进程添加到这个cgroup中.当然,一个cgroup
可以有多个进程.

```
    cgroup_init_early();
```

5.local_irq_disable
----------------------------------------

这个函数主要作用是关闭当前CPU的所有中断响应。在ARM体系里主要就是对CPSR寄存器进行操作。

```
    local_irq_disable();
```

6.Disable interrupts
----------------------------------------

标记内核还在早期初始化代码阶段，并且中断在关闭状态，如果有任何中断打开或请求中断的事情出现，
都是会提出警告，以便跟踪代码错误情况。早期代码初始化结束之后，就会调用函数early_boot_irqs_disabled
来设置这个标志为false

```
    early_boot_irqs_disabled = true;
   /*
    * Interrupts are still disabled. Do necessary setups, then
    * enable them
   */
```

7.tick_init
----------------------------------------

这个函数主要作用是初始化时钟事件管理器的回调函数，比如当时钟设备添加时处理。
在内核里定义了时钟事件管理器，主要用来管理所有需要周期性地执行任务的设备。

```
    tick_init();
```

8.boot_cpu_init
----------------------------------------

在多CPU的系统里，内核需要管理多个CPU，那么就需要知道系统有多少个CPU，在内核里使用cpu_present_map
位图表达有多少个CPU，每一位表示一个CPU的存在。如果是单个CPU，就是第0位设置为1。虽然系统里有多个
CPU存在，但是每个CPU不一定可以使用，或者没有初始化，在内核使用cpu_online_map位图来表示那些CPU
可以运行内核代码和接受中断处理。随着移动系统的节能需求，需要对CPU进行节能处理，比如有多个CPU
运行时可以提高性能，但花费太多电能，导致电池不耐用，需要减少运行的CPU个数，或者只需要一个CPU运行。
这样内核又引入了一个cpu_possible_map位图，表示最多可以使用多少个CPU。
在本函数里就是依次设置这三个位图的标志，让引导的CPU物理上存在，已经初始化好，最少需要运行的CPU。

```
    boot_cpu_init();
```

9.page_address_init
----------------------------------------

这个函数主要作用是初始化高端内存的映射表。在这里引入了高端内存的概念，那么什么叫做高端内存呢？
为什么要使用高端内存呢？其实高端内存是相对于低端内存而存在的，那么先要理解一下低端内存了。
在32位的系统里，最多能访问的总内存是4G，其中3G空间给应用程序，而内核只占用1G的空间。因此，内核
能映射的内存空间，只有1G大小，但实际上比这个还要小一些，大概是896M，另外128M空间是用来映射
高端内存使用的。因此0到896M的内存空间，就叫做低端内存，而高于896M的内存，就叫高端内存了。
如果系统是64位系统，当然就没未必要有高端内存存在了，因为64位有足够多的地址空间给内核使用，
访问的内存可以达到10G都没有问题。在32位系统里，内核为了访问超过1G的物理内存空间，需要使用
高端内存映射表。比如当内核需要读取1G的缓存数据时，就需要分配高端内存来使用，这样才可以管理起来。
使用高端内存之后，32位的系统也可以访问达到64G内存。在移动操作系统里，目前还没有这个必要，最多才
1G多内存。

```
    page_address_init();
```

10.setup_arch
----------------------------------------

这个函数主要作用是对内核架构进行初始化。再次获取CPU类型和系统架构，分析引导程序传入的命令行参数，
进行页面内存初始化，处理器初始化，中断早期初始化等等。

```
    // 主要作用是在输出终端上显示版本信息、编译的电脑用户名称、编译器版本、编译时间。
    printk(KERN_NOTICE "%s", linux_banner);
    setup_arch(&command_line);
```

11.boot_init_stack_canary
----------------------------------------

初始化stack_canary栈,stack_canary的是带防止栈溢出攻击保护的堆栈。当user space的程序通过
int 0x80进入内核空间的时候，CPU自动完成一次堆栈切换，从user space的stack切换到
kernel space的stack。在这个进程exit之前所发生的所有系统调用所使用的kernel stack都是同一个。
kernel stack的大小一般为8192 / sizeof (long);

```
    /*
     * Set up the the initial canary ASAP:
     */
    boot_init_stack_canary();
```

Linux 0.11 Stack:

https://github.com/leeminghao/doc-linux/blob/master/0.11/misc/Stack.md

12.mm_init_owner
----------------------------------------

这个函数主要作用是设置最开始的初始化任务属于init_mm内存。在ARM里，这个函数为空。

```
    mm_init_owner(&init_mm, &init_task);
```

13.mm_init_cpumask
----------------------------------------

```
    mm_init_cpumask(&init_mm);
```

14.setup_command_line
----------------------------------------

这个函数主要作用是保存命令行，以便后面可以使用。

```
    setup_command_line(command_line);
```

15.setup_nr_cpu_ids
----------------------------------------

这个函数主要作用是设置最多有多少个nr_cpu_ids结构。

```
    setup_nr_cpu_ids();
```

16.setup_per_cpu_areas
----------------------------------------

setup_per_cpu_areas()函数给每个CPU分配内存，并拷贝.data.percpu段的数据。为系统中的每个
CPU的per_cpu变量申请空间。

```
    setup_per_cpu_areas();
```

17.smp_prepare_boot_cpu
----------------------------------------

如果是SMP环境，则设置boot CPU的一些数据。在引导过程中使用的CPU称为boot CPU.

```
    smp_prepare_boot_cpu();    /* arch-specific boot-cpu hooks */
```

18.build_all_zonelists
----------------------------------------

这个函数主要作用是初始化所有内存管理节点列表，以便后面进行内存管理初始化。

```
    build_all_zonelists(NULL);
```

19.page_alloc_init
----------------------------------------

这个函数主要作用是设置内存页分配通知器。

```
    page_alloc_init();
```

20.parse_early_param
----------------------------------------

解析内核参数.

```
    printk(KERN_NOTICE "Kernel command line: %s\n", boot_command_line);
    parse_early_param();
    parse_args("Booting kernel", static_command_line, __start___param,
           __stop___param - __start___param,
           0, 0, &unknown_bootoption);
```

21.jump_label_init
----------------------------------------

```
    jump_label_init();
```

22.setup_log_buf
----------------------------------------

```
    /*
     * These use large bootmem allocations and must precede
     * kmem_cache_init()
     */
    setup_log_buf(0);
```

23.pidhash_init
----------------------------------------

这个函数是进程ID的HASH表初始化，这样可以提供通PID进行高效访问进程结构的信息。
LINUX里共有四种类型的PID，因此就有四种HASH表相对应。

```
    pidhash_init();
```

24.vfs_caches_init_early
----------------------------------------

初始化VFS的两个重要数据结构dcache和inode的缓存。

```
    vfs_caches_init_early();
```

25.sort_main_extable
----------------------------------------

把编译期间,kbuild设置的异常表,也就是__start___ex_table和__stop___ex_table之中的所有元素进行排序

```
    sort_main_extable();
```

26.trap_init
----------------------------------------

初始化中断向量表,在ARM系统里是空函数，没有任何的初始化。

```
    trap_init();
```

27.mm_init
----------------------------------------

这个函数是标记那些内存可以使用，并且告诉系统有多少内存可以使用，当然是除了内核使用的内存以外。

```
    mm_init();
```

28.sched_init
----------------------------------------

这个函数主要作用是对进程调度器进行初始化，比如分配调度器占用的内存，初始化任务队列，设置当前任务
的空线程，当前任务的调度策略为CFS调度器, 核心进程调度器初始化，调度器的初始化的优先级要高于任何
中断的建立，并且初始化进程0，即idle进程，但是并没有设置idle进程的NEED_RESCHED标志，所以还会继续
完成内核初始化剩下的事情。这里仅仅为进程调度程序的执行做准备。它所做的具体工作是调用init_bh
函数(kernel/softirq.c)把timer,tqueue,immediate三个人物队列加入下半部分的数组

```
    /*
     * Set up the scheduler prior starting any interrupts (such as the
     * timer interrupt). Full topology setup happens at smp_init()
     * time - but meanwhile we still have a functioning scheduler.
     */
    sched_init();
```

29.preempt_disable
----------------------------------------

这个函数主要作用是关闭优先级调度。由于每个进程任务都有优先级，目前系统还没有完全初始化，还不能
打开优先级调度。

```
    /*
     * Disable preemption - early bootup scheduling is extremely
     * fragile until we cpu_idle() for the first time.
     */
    preempt_disable();
```

30.local_irq_disable
----------------------------------------

这段代码主要判断是否过早打开中断，如果是这样，就会提示，并把中断关闭。

```
    if (!irqs_disabled()) {
        printk(KERN_WARNING "start_kernel(): bug: interrupts were "
                "enabled *very* early, fixing it\n");
        local_irq_disable();
    }
```

31.idr_init_cache
----------------------------------------

这个函数是创建IDR机制的内存缓存对象。所谓的IDR就是整数标识管理机制（integerIDmanagement）。
引入的主要原因是管理整数的ID与对象的指针的关系，由于这个ID可以达到32位，也就是说，如果使用
线性数组来管理，那么分配的内存太大了；如果使用线性表来管理，又效率太低了，所以就引用IDR管理
机制来实现这个需求。

```
    idr_init_cache();
```

32.perf_event_init
----------------------------------------

```
    perf_event_init();
```

33.rcu_init
----------------------------------------

这个函数是初始化直接读拷贝更新的锁机制。RCU主要提供在读取数据机会比较多，但更新比较的少的场合，
这样减少读取数据锁的性能低下的问题。

Read-Copy-Update的初始化
RCU机制是Linux2.6之后提供的一种数据一致性访问的机制，从RCU（read-copy-update）的名称上看，
我们就能对他的实现机制有一个大概的了解，在修改数据的时候，首先需要读取数据，然后生成一个副本，
对副本进行修改，修改完成之后再将老数据update成新的数据，此所谓RCU.

```
    rcu_init();
```

34.radix_tree_init
----------------------------------------

Linux使用radix树来管理位于文件系统缓冲区中的磁盘块，radix树是trie树的一种.
这个函数是初始化radix树，radix树基于二进制键值的查找树。

```
    radix_tree_init();
```

35.early_irq_init
----------------------------------------

early_irq_init 则对数组中每个成员结构进行初始化,例如, 初始每个中断源的中断号.其他的函数基本为空.

```
    /* init some links before init_ISA_irqs() */
    early_irq_init();
```

36.init_IRQ
----------------------------------------

初始化IRQ中断和终端描述符.初始化系统中支持的最大可能的中断描述结构struct irqdesc
变量数组irq_desc[NR_IRQS],把每个结构变量irq_desc[n]都初始化为预先定义好的中断
描述结构变量bad_irq_desc,并初始化该中断的链表表头成员结构变量pend.

```
    init_IRQ();
```

37.prio_tree_init
----------------------------------------

这个函数是初始化优先搜索树，主要用在内存反向搜索方面。

```
    prio_tree_init();
```

38.init_timers
----------------------------------------

这个函数是主要初始化引导CPU的时钟相关的数据结构，注册时钟的回调函数，当时钟到达时可以回调
时钟处理函数，最后初始化时钟软件中断处理。

```
    init_timers();
```

39.hrtimers_init
----------------------------------------

对高精度时钟进行初始化.

```
    hrtimers_init();
```

40.softirq_init
----------------------------------------

这个函数是初始化软件中断，软件中断与硬件中断区别就是中断发生时，软件中断是使用线程来监视中断信号，
而硬件中断是使用CPU硬件来监视中断。

```
    softirq_init();
```

50.timekeeping_init
----------------------------------------

这个函数是初始化系统时钟计时，并且初始化内核里与时钟计时相关的变量。

```
    timekeeping_init();
```

51.time_init
----------------------------------------

初始化系统时间，检查系统定时器描述结构struct sys_timer全局变量system_timer是否为空，
如果为空将其指向dummy_gettimeoffset()函数。

```
    time_init();
```

52.profile_init
----------------------------------------

profile只是内核的一个调试性能的工具,这个可以通过menuconfig中的Instrumentation Support->profile.

```
    profile_init();
```

53.call_function_init
----------------------------------------

```
    call_function_init();
```

54.local_irq_enable
----------------------------------------

这个函数是打开本CPU的中断，也即允许本CPU处理中断事件，在这里打开引CPU的中断处理。如果有多核心，
别的CPU还没有打开中断处理。

```
    // 这两行代码是提示中断是否过早地打开。
    if (!irqs_disabled())
        printk(KERN_CRIT "start_kernel(): bug: interrupts were "
                 "enabled early\n");
    early_boot_irqs_disabled = false;
    local_irq_enable();
```

55.allow GFP
----------------------------------------


```
    /* Interrupts are enabled now so all GFP allocations are safe. */
    gfp_allowed_mask = __GFP_BITS_MASK;
```

56.kmem_cache_init_late
----------------------------------------

memory cache的初始化.

```
    kmem_cache_init_late();
```

57.console_init
----------------------------------------

初始化控制台以显示printk的内容，在此之前调用的printk，只是把数据存到缓冲区里，只有在这个函数调用
后，才会在控制台打印出内容该函数执行后可调用printk()函数将log_buf中符合打印级别要求的系统信息
打印到控制台上。

```
    /*
     * HACK ALERT! This is early. We're enabling the console before
     * we've done PCI setups etc, and console_init() must be aware of
     * this. But we do want output early, in case something goes wrong.
     */
    console_init();
    if (panic_later)
        panic(panic_later, panic_param);
```

58.lockdep_info
----------------------------------------

这个函数是打印锁的依赖信息，用来调试锁。通过这个函数可以查看目前锁的状态，以便可以发现那些锁产生
死锁，那些锁使用有问题。

```
    lockdep_info();
```

59.locking_selftest
----------------------------------------

这个函数是用来测试锁的API是否使用正常，进行自我测试。比如测试自旋锁、读写锁、一般信号量和
读写信号量。

```
    /*
     * Need to run this when irqs are enabled, because it wants
     * to self-test [hard/soft]-irqs on/off lock inversion bugs
     * too:
     */
    locking_selftest();
```

60.CONFIG_BLK_DEV_INITRD
----------------------------------------

```
#ifdef CONFIG_BLK_DEV_INITRD
    if (initrd_start && !initrd_below_start_ok &&
        page_to_pfn(virt_to_page((void *)initrd_start)) < min_low_pfn) {
        printk(KERN_CRIT "initrd overwritten (0x%08lx < 0x%08lx) - "
            "disabling it.\n",
            page_to_pfn(virt_to_page((void *)initrd_start)),
            min_low_pfn);
        initrd_start = 0;
    }
#endif
```

61.page_cgroup_init
----------------------------------------

page_cgroup_init()这个函数是容器组的页面内存分配。

```
    page_cgroup_init();
```

62.debug_objects_mem_init
----------------------------------------

```
    debug_objects_mem_init();
```

63.kmemleak_init
----------------------------------------

这个函数是设置内存分配是否需要输出调试信息，如果调用这个函数，当分配内存时，不会输出一些相关的信息

```
    kmemleak_init();
```

64.setup_per_cpu_pageset
----------------------------------------

这个函数是创建每个CPU的高速缓存集合数组。因为每个CPU都不定时需要使用一些页面内存和释放页面内存，
为了提高效率，就预先创建一些内存页面作为每个CPU的页面集合。

```
    setup_per_cpu_pageset();
```

65.numa_policy_init
----------------------------------------

这个函数是初始化NUMA的内存访问策略。所谓NUMA，它是NonUniform Memory AccessAchitecture的缩写，
主要用来提高多个CPU访问内存的速度。因为多个CPU访问同一个节点的内存速度远远比访问多个节点的
速度来得快。

```
    numa_policy_init();
```

66.late_time_init
----------------------------------------

这段代码是主要运行时钟相关后期的初始化功能。

```
    if (late_time_init)
        late_time_init();
```

67.sched_clock_init
----------------------------------------

这个函数是主要计算CPU需要校准的时间，这里说的时间是CPU执行时间。如果是引导CPU，这个函数计算出来
的校准时间是不需要使用的，主要使用在非引导CPU上，因为非引导CPU执行的频率不一样,导致时间计算不准确

```
    sched_clock_init();
```

68.calibrate_delay
----------------------------------------

```
    calibrate_delay();
```

69.pidmap_init
----------------------------------------

这个函数是进程位图初始化，一般情况下使用一页来表示所有进程占用情况。

```
    pidmap_init();
```

70.anon_vma_init
----------------------------------------

这个函数是初始化反向映射的匿名内存，提供反向查找内存的结构指针位置，快速地回收内存。

```
    anon_vma_init();
```

71.CONFIG_X86
----------------------------------------

这段代码是初始化EFI的接口，并进入虚拟模式。EFI是ExtensibleFirmware Interface的缩写，就是INTEL
公司新开发的BIOS接口。

```
#ifdef CONFIG_X86
    if (efi_enabled)
        efi_enter_virtual_mode();
#endif
```

72.thread_info_cache_init
----------------------------------------

这个函数是线程信息的缓存初始化.

```
    thread_info_cache_init();
```

73.cred_init
----------------------------------------

```
    cred_init();
```

74.fork_init
----------------------------------------

这个函数是根据当前物理内存计算出来可以创建进程（线程）的数量，并进行进程环境初始化。

```
    fork_init(totalram_pages);
```

75.proc_caches_init
----------------------------------------

这个函数是进程缓存初始化。

```
    proc_caches_init();
```

76.buffer_init
----------------------------------------

这个函数是初始化文件系统的缓冲区，并计算最大可以使用的文件缓存。

```
    buffer_init();
```

77.key_init
----------------------------------------

这个函数是初始化安全键管理列表和结构。

```
    key_init();
```

78.security_init
----------------------------------------

这个函数是初始化安全管理框架，以便提供访问文件／登录等权限。

```
    security_init();
```

79.dbg_late_init
----------------------------------------

```
    dbg_late_init();
```

80.vfs_caches_init
----------------------------------------

这个函数是虚拟文件系统进行缓存初始化，提高虚拟文件系统的访问速度。

```
    vfs_caches_init(totalram_pages);
```

81.signals_init
----------------------------------------

这个函数是初始化信号队列缓存。

```
    signals_init();
```

82.page_writeback_init
----------------------------------------

```
    /* rootfs populating might need page-writeback */
    page_writeback_init();
```

83.proc_root_init
----------------------------------------

这个函数是初始化系统进程文件系统，主要提供内核与用户进行交互的平台，方便用户实时查看进程的信息。

```
#ifdef CONFIG_PROC_FS
    proc_root_init();
#endif
```

84.cgroup_init
----------------------------------------

这个函数是初始化进程控制组，主要用来为进程和其子程提供性能控制.比如限定这组进程的CPU使用率为20％。

```
    cgroup_init();
```

85.cpuset_init
----------------------------------------

这个函数是初始化CPUSET，CPUSET主要为控制组提供CPU和内存节点的管理的结构。

```
    cpuset_init();
```

86.taskstats_init_early
----------------------------------------

这个函数是初始化任务状态相关的缓存、队列和信号量。任务状态主要向用户提供任务的状态信息。

```
    taskstats_init_early();
```

87.delayacct_init
----------------------------------------

这个函数是初始化每个任务延时计数。当一个任务等CPU运行，或者等IO同步时，都需要计算等待时间。

```
    delayacct_init();
```

88.check_bugs
----------------------------------------

这个函数是用来检查CPU配置、FPU等是否非法使用不具备的功能。

```
    check_bugs();
```

89.acpi_early_init
----------------------------------------

这个函数是初始化ACPI电源管理。高级配置与能源接口(ACPI)ACPI规范介绍ACPI能使软、硬件、
操作系统（OS），主机板和外围设备，依照一定的方式管理用电情况，系统硬件产生的Hot-Plug事件，
让操作系统从用户的角度上直接支配即插即用设备，不同于以往直接通过基于BIOS 的方式的管理。

```
    acpi_early_init(); /* before LAPIC and SMP init */
```

90.sfi_init_late
----------------------------------------

```
    sfi_init_late();
```

91.ftrace_init
----------------------------------------

这个函数是初始化内核跟踪模块，ftrace的作用是帮助开发人员了解Linux 内核的运行时行为，以便进行故障
调试或性能分析。

```
    ftrace_init();
```

92.rest_init
----------------------------------------

这个函数是后继初始化，主要是创建内核线程init，并运行。

```
    /* Do the rest non-__init'ed, we're now alive */
    rest_init();
}
```

https://github.com/leeminghao/doc-linux/blob/master/4.x.y/init/main.c/rest_init.md
