#ifndef _LINUX_MM_H
#define _LINUX_MM_H

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

#include <linux/fs.h>
#include <linux/kernel.h>

/*
 * BAD_PAGE is the page that is used for page faults when linux
 * is out-of-memory. Older versions of linux just did a
 * do_exit(), but using this instead means there is less risk
 * for a process dying in kernel mode, possibly leaving a inode
 * unused etc..
 *
 * BAD_PAGETABLE is the accompanying page-table: it is initialized
 * to point to BAD_PAGE entries.
 */
extern unsigned long inline __bad_page(void)
{
	extern char empty_bad_page[PAGE_SIZE];

	__asm__ __volatile__("cld ; rep ; stosl"
		::"a" (0),
		  "D" ((long) empty_bad_page),
		  "c" (1024)
		:"di","cx");
	return (unsigned long) empty_bad_page;
}
#define BAD_PAGE __bad_page()

extern unsigned long inline __bad_pagetable(void)
{
	extern char empty_bad_page_table[PAGE_SIZE];

	__asm__ __volatile__("cld ; rep ; stosl"
		::"a" (7+BAD_PAGE),
		  "D" ((long) empty_bad_page_table),
		  "c" (1024)
		:"di","cx");
	return (unsigned long) empty_bad_page_table;
}
#define BAD_PAGETABLE __bad_pagetable()

extern volatile short free_page_ptr; /* used by malloc and tcp/ip. */

extern int nr_free_pages;
extern unsigned long free_page_list;
extern int nr_secondary_pages;
extern unsigned long secondary_page_list;

#define MAX_SECONDARY_PAGES 10

extern void rw_swap_page(int rw, unsigned int nr, char * buf);

#define read_swap_page(nr,buf) \
	rw_swap_page(READ,(nr),(buf))
#define write_swap_page(nr,buf) \
	rw_swap_page(WRITE,(nr),(buf))

/* memory.c */
	
extern unsigned long get_free_page(int priority);
extern unsigned long put_dirty_page(struct task_struct * tsk,unsigned long page,
	unsigned long address);
extern void free_page(unsigned long addr);
extern void free_page_tables(struct task_struct * tsk);
extern void clear_page_tables(struct task_struct * tsk);
extern int copy_page_tables(struct task_struct * new);
extern int unmap_page_range(unsigned long from, unsigned long size);
extern int remap_page_range(unsigned long from, unsigned long to, unsigned long size,
	 int permiss);
extern void write_verify(unsigned long address);

extern void do_wp_page(unsigned long error_code, unsigned long address,
	struct task_struct *tsk, unsigned long user_esp);
extern void do_no_page(unsigned long error_code, unsigned long address,
	struct task_struct *tsk, unsigned long user_esp);

extern void mem_init(unsigned long low_start_mem,
		     unsigned long start_mem, unsigned long end_mem);
extern void show_mem(void);
extern void do_page_fault(unsigned long *esp, unsigned long error_code);
extern void oom(struct task_struct * task);
extern void malloc_grab_pages(void);

/* swap.c */

extern void swap_free(unsigned int page_nr);
extern void swap_duplicate(unsigned int page_nr);
extern void swap_in(unsigned long *table_ptr);

#define invalidate() \
__asm__ __volatile__("movl %%cr3,%%eax\n\tmovl %%eax,%%cr3":::"ax")

extern unsigned long high_memory;

#define MAP_NR(addr) ((addr) >> PAGE_SHIFT)
#define MAP_PAGE_RESERVED (1<<15)
#define USED 100

extern unsigned short * mem_map;

#define PAGE_DIRTY	0x40
#define PAGE_ACCESSED	0x20
#define PAGE_USER	0x04
#define PAGE_RW		0x02
#define PAGE_PRESENT	0x01

#define GFP_BUFFER	0x00
#define GFP_ATOMIC	0x01
#define GFP_USER	0x02
#define GFP_KERNEL	0x03

#endif