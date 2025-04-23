
#include <asm_defs.h>


// The arguments to this function are:
//     a0: hart id
//     a1: image load address
.section ".boot", "ax"
.global boot_arch
boot_arch:

    /* Save ra in s1 */
    mv s1, ra

    /* Set sscratch to point to cpu struct and clear it*/
    la  t0, _image_end
    li  t1, CPU_SIZE
    mul t1, t1, a0
    add a3, t0, t1
    csrw sscratch, a3
    add a4, a3, t1
	call boot_clear

    /* Restore ra and return */
    mv ra, s1
    ret

