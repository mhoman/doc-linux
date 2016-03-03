__enable_mmu
========================================

path: arch/arm/kernel/head.S
```
/*
 * Setup common bits before finally enabling the MMU.  Essentially
 * this is just loading the page table pointer and domain access
 * registers.
 *
 *  r0  = cp#15 control register
 *  r1  = machine ID
 *  r2  = atags or dtb pointer
 *  r4  = page table pointer
 *  r9  = processor ID
 *  r13 = *virtual* address to jump to upon completion
 */
__enable_mmu:
#if defined(CONFIG_ALIGNMENT_TRAP) && __LINUX_ARM_ARCH__ < 6
    orr    r0, r0, #CR_A
#else
    bic    r0, r0, #CR_A
#endif
#ifdef CONFIG_CPU_DCACHE_DISABLE
    bic    r0, r0, #CR_C
#endif
#ifdef CONFIG_CPU_BPREDICT_DISABLE
    bic    r0, r0, #CR_Z
#endif
#ifdef CONFIG_CPU_ICACHE_DISABLE
    bic    r0, r0, #CR_I
#endif
#ifdef CONFIG_ARM_LPAE
    mov    r5, #0
    mcrr    p15, 0, r4, r5, c2        @ load TTBR0
#else
#ifdef CONFIG_EMULATE_DOMAIN_MANAGER_V7
    mov    r5, #(domain_val(DOMAIN_USER, DOMAIN_CLIENT) | \
              domain_val(DOMAIN_KERNEL, DOMAIN_CLIENT) | \
              domain_val(DOMAIN_TABLE, DOMAIN_CLIENT) | \
              domain_val(DOMAIN_IO, DOMAIN_CLIENT))
#else
    mov    r5, #(domain_val(DOMAIN_USER, DOMAIN_CLIENT) | \
              domain_val(DOMAIN_KERNEL, DOMAIN_MANAGER) | \
              domain_val(DOMAIN_TABLE, DOMAIN_CLIENT) | \
              domain_val(DOMAIN_IO, DOMAIN_CLIENT))
#endif
    mcr    p15, 0, r5, c3, c0, 0        @ load domain access register
    mcr    p15, 0, r4, c2, c0, 0        @ load page table pointer
#endif
    b    __turn_mmu_on
ENDPROC(__enable_mmu)
```

该函数根据内核配置对CP15和r0寄存器进行配置，工作如下：

* 操作CP15的c1寄存器打开L2 cache
* 根据内核配置，关闭指令或者数据cache
* 配置CP15 c3寄存器，配置ARM域的访问权限
* 配置CP15 c2寄存器，指定内存页表地址

需要注意的是前面__v7_setup中只是无效掉cache，无效是cache操作一种，表示cache中数据无效了，
下次由cache读取数据，则需要cache从内存中重新获取数据。这是保证cache数据一致性的手段。

接下来再次跳转到__turn_mmu_on执行:

path: arch/arm/kernel/head.S
```
/*
 * Enable the MMU.  This completely changes the structure of the visible
 * memory space.  You will not be able to trace execution through this.
 * If you have an enquiry about this, *please* check the linux-arm-kernel
 * mailing list archives BEFORE sending another post to the list.
 *
 *  r0  = cp#15 control register
 *  r1  = machine ID
 *  r2  = atags or dtb pointer
 *  r9  = processor ID
 *  r13 = *virtual* address to jump to upon completion
 *
 * other registers depend on the function called upon completion
 */
    .align    5
    .pushsection    .idmap.text, "ax"
ENTRY(__turn_mmu_on)
    mov    r0, r0
    instr_sync
    /* 将r0值写入CP15的c0寄存器，查看CP15说明，c0寄存器的第0位表征MMU的开启，
     * 刚才__v7_setup中v7_crval给出的mmuset值已经将第0位置1了，因此写入c0后MMU就开启了 */
    mcr    p15, 0, r0, c1, c0, 0        @ write control reg
    /* 接下来CPU取指地址是当前pc+4,由于做了平映射，虽然接下来是虚拟地址，
     * 但是该虚拟地址跟之前的物理地址是完全一致的，pc+4可以取到
     * mrc p15, 0, r3, c0, c0, 0指令，并不会导致CPU取指上的问题 */
    mrc    p15, 0, r3, c0, c0, 0        @ read id reg
    instr_sync
    mov    r3, r3
#ifdef CONFIG_ARCH_MSM_KRAIT
    movw    r3, 0xfc00
    movt    r3, 0xff00
    and    r3, r9, r3
    movw    r4, 0x0400
    movt     r4, 0x5100
    cmp    r3, r4
    mrceq    p15, 7, r3, c15, c0, 2
    biceq    r3, r3, #0x400
    mcreq    p15, 7, r3, c15, c0, 2
#endif
    /* 开启MMU后，修改PC值为r13执行，也就是__mmap_switched，该函数已经是运行在MMU之上的虚拟地址了 */
    mov    r3, r13
    mov    pc, r3
__turn_mmu_on_end:
ENDPROC(__turn_mmu_on)
    .popsection
```

接下来将执行__mmap_switched函数:

https://github.com/leeminghao/doc-linux/blob/master/4.x.y/arch/arm/kernel/head-common.S/__mmap_switched.md