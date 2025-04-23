#include <arch/crossconhyp.h>
#include <arch/csrs.h>
#include <arch/page_table.h>
#include <asm_defs.h>
#include <platform_defs.h>

#define PT_SIZE PAGE_SIZE
#if RV64
#define PT_LVLS 3
#define PTE_INDEX_SHIFT(LEVEL) ((9 * (PT_LVLS - 1 - (LEVEL))) + 12)
#else
#define PT_LVLS 2
#define PTE_INDEX_SHIFT(LEVEL) ((10 * (PT_LVLS - 1 - (LEVEL))) + 12)
#endif 


/**
 * Calculates the index or offset of a page table entry for given virtual address(addr) at a given
 * level of page table.
 */
.macro PTE_INDEX_ASM	index, addr, level
	srl \index, \addr, PTE_INDEX_SHIFT(\level) 
    li  s0, ((PAGE_SIZE/REGLEN)-1)
	and \index, \index, s0
    li  s0, REGLEN
	mul \index, \index, s0
.endm

/**
 * Calculates the pointer to a pte given the page table pointer(pt), the page table level (levle)
 * and the target virtual address (va)
 */
.macro PTE_PTR  pte, pt, level, va
    PTE_INDEX_ASM	s1, \va, \level
    add     \pte, s1, \pt
.endm

/**
 * Creates a page table entry (pte) for a given physical address (pa) and set of flags.
 */
.macro PTE_FILL    pte, pa, flags
    srl     \pte, \pa, 2
    or      \pte, \pte, \flags
.endm

.macro LD_SYM       rd, sym
    la      \rd, \sym
    LOAD      \rd, 0(\rd)
.endm

.data 
.align 3
/**
 * barrier is used to minimal synchronization in boot - other cores wait for bsp to set it.
 */
_barrier: .8byte 0

.section ".rodata", "a"
/* Symbol values */
_image_start_sym: .8byte _image_start
_image_load_end_sym: .8byte _image_load_end
_image_noload_start_sym: .8byte _image_noload_start
_image_end_sym: .8byte _image_end
_dmem_beg_sym: .8byte _dmem_beg
_enter_vas_sym: .8byte _enter_vas
_extra_allocated_phys_mem_sym: .8byte extra_allocated_phys_mem

 .section ".boot", "ax"
.global boot_arch
boot_arch:

    /* s11 is reserved in procedure to store the return address offset */
    sub     s11, ra, a1
    LD_SYM  s6, _extra_allocated_phys_mem_sym

    /* Setup bootstrap page tables. Assuming sv39 or sv32 support. */ 

 	/* Skip initialy global page tables setup if not hart */
    LD_SYM  t0, CPU_MASTER
	bne     a0, t0, wait_for_bsp   

 	la	    a3, _page_tables_start	
	la	    a4, _page_tables_end	
    add     a3, a3, s6
    add     a4, a4, s6
	call	boot_clear		 


    la          t0, root_l1_pt
    add         t0, t0, s6
#if RV64
    la          t1, root_l2_pt
    add         t1, t1, s6
    PTE_FILL    t1, t1, PTE_TABLE
    li          t2, BAO_VAS_BASE
    PTE_PTR     t2, t0, 1, t2
    STORE       t1, 0(t2)
    la          t0, root_l2_pt
    add         t0, t0, s6
#endif
    LD_SYM      t1, _image_start_sym
    PTE_PTR     t1, t0, (PT_LVLS - 1), t1
    LD_SYM      t2, _image_load_end_sym
    PTE_PTR     t2, t0, (PT_LVLS - 1), t2

    la          t0, _image_start
    PTE_FILL    t0, t0, PTE_HYP_FLAGS | PTE_PAGE
1:
    bge     t1, t2, 2f
    STORE   t0, 0(t1)
    add     t1, t1, REGLEN
    add     t0, t0, (PAGE_SIZE >> 2)
    j       1b
2:
#if RV64
    la          t0, root_l2_pt
#else
    la          t0, root_l1_pt
#endif
    add         t0, t0, s6
    LD_SYM      t2, _image_end_sym
    PTE_PTR     t2, t0, (PT_LVLS - 1), t2
    bge         t1, t2, 3f
    la          t0, _image_noload_start
    PTE_FILL    t0, t0, PTE_HYP_FLAGS | PTE_PAGE
    j 1b
3:
    fence   w, w
    la      t0, _barrier
    li      t1, 1
    STORE   t1, 0(t0)
    j       map_cpu

wait_for_bsp:
    la      t0, _barrier
    li      t1, 1
1:
    LOAD      t2, 0(t0)
    blt     t2, t1, 1b

map_cpu:
    /* Calculate base phys address of CPU struct -> t0 */
    la      t0, _dmem_phys_beg
    li      t1, (CPU_SIZE)
    li      t2, (PT_SIZE*PT_LVLS)  # Precompute the value in a temporary register
    add     t1, t1, t2       # Add the precomputed value to t1
    mul     t2, t1, a0
    add     t0, t0, t2
    mv      a3, t0
    add     a4, a3, t1
    call    boot_clear

    /* Calculate phys address page table -> t1 */
    li      t2, CPU_SIZE
    add     t1, t0, t2

    /* Add root l1 page table pointer to root page table */
    la          t2, root_l1_pt
    add         t2, t2, s6
    PTE_FILL    t2, t2, PTE_TABLE
    li          t3, BAO_VAS_BASE
    PTE_PTR     t3, t1, 0, t3
    STORE       t2, 0(t3)

    li      t4, PAGE_SIZE

    add         t2, t1, t4
    PTE_FILL    t2, t2, PTE_TABLE
    li          t3, BAO_CPU_BASE
    PTE_PTR     t3, t1, 0, t3
    STORE       t2, 0(t3)

#if RV64
    add         t1, t1, t4
    add         t2, t1, t4
    PTE_FILL    t2, t2, PTE_TABLE
    li          t3, BAO_CPU_BASE
    PTE_PTR     t3, t1, 1, t3
    STORE       t2, 0(t3)
#endif

    add         t1, t1, t4
    li          t2, BAO_CPU_BASE
    PTE_PTR     t1, t1, 2, t2
    PTE_FILL    t2, t0, PTE_HYP_FLAGS | PTE_PAGE
    li          t3, CPU_SIZE+PT_SIZE
1:
    blez    t3, setup_cpu
    STORE   t2, 0(t1)
    add     t1, t1, REGLEN
    add     t2, t2, (PAGE_SIZE >> 2)
    sub     t3, t3, t4
    j       1b
    
setup_cpu:
    la      t0, _dmem_phys_beg
    li      t1, (CPU_SIZE + (PT_SIZE*PT_LVLS))
    mul     t1, t1, a0
    add     t0, t0, t1   
    li      t1, CPU_SIZE
    add     t0, t0, t1
    srl     t0, t0, PAGE_SHIFT
    li      t2, SATP_MODE_DFLT
    or      t0, t0, t2

    LD_SYM      t2, _enter_vas_sym
    and     t3, t2, ~STVEC_MODE_MSK
    or      t3, t3, STVEC_MODE_DIRECT
    csrw    stvec, t3

    sfence.vma
    csrw   satp, t0

    jr      t2

_enter_vas:
    la      a2, _image_start
    add     ra, s11, a2

    li      t0, BAO_CPU_BASE
    csrw    sscratch, t0
    ret
