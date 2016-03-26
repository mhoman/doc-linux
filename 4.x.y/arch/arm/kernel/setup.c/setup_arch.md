setup_arch
========================================

参数
----------------------------------------

在我们的实验环境传递进来的cmdline如下所示:

```
console=ttyHSL0,115200,n8 androidboot.hardware=qcom ehci-hcd.park=3 maxcpus=2 androidboot.bootdevice=msm_sdcc.1 androidboot.emmc=true androidboot.serialno=c0d9b895 androidboot.sdcard.type=mixed syspart=system  androidboot.baseband=mdm
```

path: arch/arm/kernel/setup.c
```
void __init setup_arch(char **cmdline_p)
{
```

setup_processor
----------------------------------------

```
    const struct machine_desc *mdesc;

    setup_processor();
```

https://github.com/leeminghao/doc-linux/tree/master/4.x.y/arch/arm/kernel/setup.c/setup_processor.md

setup_machine_fdt
----------------------------------------

如果atags中带有device tree的信息则调用函数setup_machine_fdt来解析对应的atags.

```
    mdesc = setup_machine_fdt(__atags_pointer);
```

https://github.com/leeminghao/doc-linux/tree/master/4.x.y/arch/arm/kernel/devtree.c/setup_machine_fdt.md

setup_machine_tags
----------------------------------------

如果atags中没有带有设备树相关信息就调用setup_machine_tags来获取对应的机器描述符.

```
    if (!mdesc)
        mdesc = setup_machine_tags(__atags_pointer, __machine_arch_type);
    machine_desc = mdesc;
    machine_name = mdesc->name;
    dump_stack_set_arch_desc("%s", mdesc->name);
```

https://github.com/leeminghao/doc-linux/tree/master/4.x.y/arch/arm/kernel/atags_parse.c/setup_machine_tags.md

**注意**: 对应带有Device Tree信息和不带Device Tree信息的ATAGS是由bootloader来组织的:

https://github.com/leeminghao/doc-linux/blob/master/bootloader/lk/apps/aboot/aboot_c/boot_linux.md

设置init_mm的代码段和数据段
----------------------------------------

```
    /* 这个是重启方式，”s”为软件，”h”为硬件 */
    if (mdesc->reboot_mode != REBOOT_HARD)
        reboot_mode = mdesc->reboot_mode;

    init_mm.start_code = (unsigned long) _text;
    init_mm.end_code   = (unsigned long) _etext;
    init_mm.end_data   = (unsigned long) _edata;
    init_mm.brk       = (unsigned long) _end;

    /* populate cmd_line too for later use, preserving boot_command_line */
    strlcpy(cmd_line, boot_command_line, COMMAND_LINE_SIZE);
    *cmdline_p = cmd_line;
```

parse_early_param
----------------------------------------

```
    parse_early_param();
```

https://github.com/leeminghao/doc-linux/blob/master/4.x.y/init/main.c/parse_early_param.md

early_paging_init
----------------------------------------

```
    early_paging_init(mdesc, lookup_processor_type(read_cpuid_id()));
```

```
    setup_dma_zone(mdesc);
    sanity_check_meminfo();
    arm_memblock_init(mdesc);

    paging_init(mdesc);
    request_standard_resources(mdesc);

    if (mdesc->restart)
        arm_pm_restart = mdesc->restart;

    unflatten_device_tree();

    arm_dt_init_cpu_maps();
    psci_init();
#ifdef CONFIG_SMP
    if (is_smp()) {
        if (!mdesc->smp_init || !mdesc->smp_init()) {
            if (psci_smp_available())
                smp_set_ops(&psci_smp_ops);
            else if (mdesc->smp)
                smp_set_ops(mdesc->smp);
        }
        smp_init_cpus();
        smp_build_mpidr_hash();
    }
#endif

    if (!is_smp())
        hyp_mode_check();

    reserve_crashkernel();

#ifdef CONFIG_MULTI_IRQ_HANDLER
    handle_arch_irq = mdesc->handle_irq;
#endif

#ifdef CONFIG_VT
#if defined(CONFIG_VGA_CONSOLE)
    conswitchp = &vga_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
    conswitchp = &dummy_con;
#endif
#endif

    if (mdesc->init_early)
        mdesc->init_early();
}
```