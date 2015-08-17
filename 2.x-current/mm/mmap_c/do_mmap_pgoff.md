do_mmap_pgoff
========================================

do_mmap_pgoff是一个体系结构无关的函数,此函数曾经是内核中最长的函数之一,
它现在已经分成两个部分,但仍然相当长,第一部分需要彻底检查用户应用程序
传递的参数; 第二部分需要考虑大量特殊情况和微妙之处.

path: mm/mmap.c
```
/*
 * The caller must hold down_write(&current->mm->mmap_sem).
 */

unsigned long do_mmap_pgoff(struct file *file, unsigned long addr,
               unsigned long len, unsigned long prot,
               unsigned long flags, unsigned long pgoff,
               unsigned long *populate)
{
     struct mm_struct *mm = current->mm;
     vm_flags_t vm_flags;

     *populate = 0;

     ...

     /* Do simple checking here so the lower-level routines won't have
      * to. we assume access permissions have been handled by the open
      * of the memory object, so we don't do any here.
      */
     vm_flags = calc_vm_prot_bits(prot) | calc_vm_flag_bits(flags) |
               mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;

     if (flags & MAP_LOCKED)
          if (!can_do_mlock())
               return -EPERM;

     if (mlock_future_check(mm, vm_flags, len))
          return -EAGAIN;

     if (file) {
          struct inode *inode = file_inode(file);

          switch (flags & MAP_TYPE) {
          case MAP_SHARED:
               if ((prot&PROT_WRITE) && !(file->f_mode&FMODE_WRITE))
                    return -EACCES;

               /*
                * Make sure we don't allow writing to an append-only
                * file..
                */
               if (IS_APPEND(inode) && (file->f_mode & FMODE_WRITE))
                    return -EACCES;

               /*
                * Make sure there are no mandatory locks on the file.
                */
               if (locks_verify_locked(file))
                    return -EAGAIN;

               vm_flags |= VM_SHARED | VM_MAYSHARE;
               if (!(file->f_mode & FMODE_WRITE))
                    vm_flags &= ~(VM_MAYWRITE | VM_SHARED);

               /* fall through */
          case MAP_PRIVATE:
               if (!(file->f_mode & FMODE_READ))
                    return -EACCES;
               if (file->f_path.mnt->mnt_flags & MNT_NOEXEC) {
                    if (vm_flags & VM_EXEC)
                         return -EPERM;
                    vm_flags &= ~VM_MAYEXEC;
               }

               if (!file->f_op->mmap)
                    return -ENODEV;
               if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
                    return -EINVAL;
               break;

          default:
               return -EINVAL;
          }
     } else {
          switch (flags & MAP_TYPE) {
          case MAP_SHARED:
               if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
                    return -EINVAL;
               /*
                * Ignore pgoff.
                */
               pgoff = 0;
               vm_flags |= VM_SHARED | VM_MAYSHARE;
               break;
          case MAP_PRIVATE:
               /*
                * Set pgoff according to addr for anon_vma.
                */
               pgoff = addr >> PAGE_SHIFT;
               break;
          default:
               return -EINVAL;
          }
     }

     /*
      * Set 'VM_NORESERVE' if we should not account for the
      * memory use of this mapping.
      */
     if (flags & MAP_NORESERVE) {
          /* We honor MAP_NORESERVE if allowed to overcommit */
          if (sysctl_overcommit_memory != OVERCOMMIT_NEVER)
               vm_flags |= VM_NORESERVE;

          /* hugetlb applies strict overcommit unless MAP_NORESERVE */
          if (file && is_file_hugepages(file))
               vm_flags |= VM_NORESERVE;
     }

     addr = mmap_region(file, addr, len, vm_flags, pgoff);
     if (!IS_ERR_VALUE(addr) &&
         ((vm_flags & VM_LOCKED) ||
          (flags & (MAP_POPULATE | MAP_NONBLOCK)) == MAP_POPULATE))
          *populate = len;
     return addr;
}
```

1.检查参数是否正确
----------------------------------------

```
     ...
     /*
      * Does the application expect PROT_READ to imply PROT_EXEC?
      *
      * (the exception is when the underlying filesystem is noexec
      *  mounted, in which case we dont add PROT_EXEC.)
      */
     if ((prot & PROT_READ) && (current->personality & READ_IMPLIES_EXEC))
          if (!(file && (file->f_path.mnt->mnt_flags & MNT_NOEXEC)))
               prot |= PROT_EXEC;

     if (!len)
          return -EINVAL;

     if (!(flags & MAP_FIXED))
          addr = round_hint_to_min(addr);

     /* Careful about overflows.. */
     len = PAGE_ALIGN(len);
     if (!len)
          return -ENOMEM;

     /* offset overflow? */
     if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
          return -EOVERFLOW;

     /* Too many mappings? */
     /* 每个进程有最多可以用来映射的内存个数sysctl_max_map_count.
      * 如果已经存在了足够多的内存映射，则返回失败。
      */
     if (mm->map_count > sysctl_max_map_count)
          return -ENOMEM;
     ...
```

2.get_unmapped_area
----------------------------------------

get_unmapped_area函数用于在进程虚拟地址空间中找到一个合适的区域用于映射.

```
     ...
     /* Obtain the address to map to. we verify (or select) it and ensure
      * that it represents a valid section of the address space.
      */
     addr = get_unmapped_area(file, addr, len, pgoff, flags);
     if (addr & ~PAGE_MASK)
          return addr;
     ...
```

get_unmapped_area具体实现如下所示:

https://github.com/leeminghao/doc-linux/blob/master/2.x-current/mm/mmap_c/get_unmapped_area.md