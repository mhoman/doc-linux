Linux内存管理
========================================

概述
----------------------------------------

内存管理的实现涵盖了许多领域：

* 内存中的物理内存页的管理；

https://github.com/leeminghao/doc-linux/tree/master/4.x.y/mm/pm_manage.md

* 分配大块内存的伙伴系统；

* 分配较小块内存的slab、slub和slob分配器；

* 分配非连续内存块的vmalloc机制；

* 进程的地址空间。

就我们所知，Linux内核一般将处理器的虚拟地址空间划分为两个部分。底部比较大的部分用于用户进程，
顶部则专用于内核。虽然（在两个用户进程之间的）上下文切换期间会改变下半部分，但虚拟地址空间
的内核部分总是保持不变。地址空间在用户进程和内核之间划分的典型比例为3∶1。给出4 GiB虚拟地址空间，
3 GiB将用于用户空间而1 GiB将用于内核。通过修改相关的配置选项可以改变该比例。但只有对非常特殊的
配置和应用程序，这种修改才会带来好处。目前，我们只需假定比例为3∶1,其他比例以后再讨论。

**注意**:

可用的物理内存将映射到内核的地址空间中。访问内存时，如果所用的虚拟地址与内核区域的起始地址之间的
偏移量不超出可用物理内存的长度，那么该虚拟地址会自动关联到物理页帧。这是可行的，因为在采用
该方案时，在内核区域中的内存分配总是落入到物理内存中。不过，还有一个问题。虚拟地址空间的内核
部分必然小于CPU理论地址空间的最大长度。如果物理内存比可以映射到内核地址空间中的数量要多，
那么内核必须借助于高端内存(highmem)方法来管理“多余的”内存。在IA-32系统上，可以直接管理
的物理内存数量不超过896 MiB。超过该值（直到最大4 GiB为止）的内存只能通过高端内存寻址。
4 GiB是32位系统上可以寻址的最大内存（232 = 4 GiB）。如果使用一点技巧，现代的IA-32实现
（Pentium PRO或更高版本）在启用PAE模式的情况下可以管理多达64 GiB内存。PAE是
page address extension的缩写，该特性为内存指针提供了额外的比特位。但并非所有64 GiB都可以同时寻址，
而是每次只能寻址一个4 GiB的内存段。由于大多数内存管理数据结构只能分配在内存范围0和1GiB之间，
因此最大的内存实际极限小于64 GiB。准确的数值依内核配置而有所不同。例如，可以在高端内存中分配
三级页表的项，以减少对普通内存域的占用。由于内存超过4 GiB的IA-32系统比较罕见，这种情况下实际上
一般会使用64位体系结构AMD64来代替IA-32，提供一个简洁得多的解决方案，因此第二种高端内存模式我就
不打算在此介绍了。在64位计算机上，由于可用的地址空间非常巨大，因此不需要高端内存模式。
即使物理寻址受限于地址字的比特位数（例如48或52），也是如此。32位系统超出4 GiB地址空间的限制也
才刚刚几年，有人可能据此认为64位系统地址空间的限制被突破似乎也只是时间问题，但理论上的16 EiB
也能撑些时间了。但技术的发展是我们无法预测的.

只有内核自身使用高端内存页时，才会有问题。在内核使用高端内存页之前，必须使用kmap和kunmap
函数将其映射到内核虚拟地址空间中，对普通内存页这是不必要的。但对用户空间进程来说，是高端
内存页还是普通页完全没有任何差别。因为用户空间进程总是通过页表访问内存，决不会直接访问。

内存布局
----------------------------------------

### msm8960

https://github.com/leeminghao/doc-linux/blob/master/arch/arm/msm8960/memory_map.md

### aries

https://github.com/leeminghao/doc-linux/blob/master/arch/arm/msm8960/memory_layout.md

初始化内存管理
----------------------------------------

https://github.com/leeminghao/doc-linux/tree/master/4.x.y/init/InitMemory.md

进程虚拟地址空间
----------------------------------------

https://github.com/leeminghao/doc-linux/blob/master/4.x.y/mm/task_vm_layout.md
