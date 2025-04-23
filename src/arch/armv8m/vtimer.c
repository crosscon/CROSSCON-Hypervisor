#include <arch/vtimer.h>
#include <vm.h>
#include <arch/sysregs.h>
#include <arch/systick.h>

void vtimer_init(struct vtimer* vtimer)
{
    UNUSED_ARG(vtimer);
}

void vtimer_reset(struct vtimer* vtimer)
{
    vtimer->rvr = 0;
    vtimer->cvr = 0;
}

void vtimer_save_state(struct vtimer* vtimer)
{
    vtimer->csr = systick_get_csr(systick_ns);
    vtimer->rvr = systick_get_rvr(systick_ns);
    vtimer->cvr = systick_get_cvr(systick_ns);
}

void vtimer_restore_state(struct vtimer* vtimer)
{
    /* Note: Restoring the CVR implies that we simulate a new count routine, i.e., we need to
    disable the timer, reload the previous CVR value to RVR, start the count and immediately stop
    so that the CVR is recharged. This will lead to a small drift on the the counting. */

    // Disable the systick timer
    systick_set_csr(systick_ns, systick_get_csr(systick_ns) & ~SYSTICK_CSR_ENABLE);
    // Clear CVR
    systick_set_cvr(systick_ns, 0);
    // Set RVR with the cvr value to be restored
    systick_set_rvr(systick_ns, vtimer->cvr);
    // Enable the systick timer to force the reload of the cvr
    systick_set_csr(systick_ns, systick_get_csr(systick_ns) | SYSTICK_CSR_ENABLE);

    // Restore the CSR and RVR
    systick_set_csr(systick_ns, vtimer->csr);
    systick_set_rvr(systick_ns, vtimer->rvr);
}
