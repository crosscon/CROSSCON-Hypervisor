#include "arch/csrs.h"

void tee_arch_interrupt_disable()
{
    /* TODO disable interrupts */
    CSRC(CSR_HIE, HIE_VSEIE);
    CSRC(CSR_HIDELEG, HIE_VSEIE);
}

void tee_arch_interrupt_enable()
{
    CSRS(CSR_HIE, HIE_VSEIE);
    CSRS(CSR_HIDELEG, HIE_VSEIE);
}
