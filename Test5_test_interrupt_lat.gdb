tar rem:3333
file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/CROSSCON-Hypervisor-LASTREBASE/bin/lpc55s69/eval_dual_pervmtee_intlat/crossconhyp.elf
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/TestBareInterruptLatency/baremetal-nonsecure/build/lpc55s69/baremetal.elf
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/TestBareInterruptLatency/baremetal-secure/build/lpc55s69/baremetal.elf
set $pc=_reset_handler
set mem inaccessible-by-default off
b main
tui enable
b _irq_handler
display/5i $pc
define dump_mpu
    set $mpu_type = *(uint32_t *)0xE000ED90
    set $mpu_ctrl = *(uint32_t *)0xE000ED94
    set $mpu_regions = ($mpu_type >> 8) & 0xff
    set $old_rnr = *(uint32_t *)0xE000ED98

    printf "\n==== Secure MPU dump ====\n"
    printf "TYPE  = 0x%08x\n", $mpu_type
    printf "CTRL  = 0x%08x\n", $mpu_ctrl
    printf "MAIR0 = 0x%08x\n", *(uint32_t *)0xE000EDC0
    printf "MAIR1 = 0x%08x\n", *(uint32_t *)0xE000EDC4
    printf "Regions implemented = %u\n", $mpu_regions
    printf "MPU enabled = %u\n", $mpu_ctrl & 1

    set $i = 0
    while $i < $mpu_regions
        set *(uint32_t *)0xE000ED98 = $i
        set $rbar = *(uint32_t *)0xE000ED9C
        set $rlar = *(uint32_t *)0xE000EDA0

        set $base = $rbar & 0xffffffe0
        set $limit = ($rlar & 0xffffffe0) | 0x1f
        set $rbar_attr = $rbar & 0x1f
        set $rlar_attr = $rlar & 0x1f

        set $xn = $rbar & 1
        set $ap = ($rbar >> 1) & 3
        set $sh = ($rbar >> 3) & 3
        set $attridx = ($rlar >> 1) & 7
        set $en = $rlar & 1

        printf "\nRegion %u:\n", $i
        printf "  RBAR      = 0x%08x\n", $rbar
        printf "  RLAR      = 0x%08x\n", $rlar
        printf "  BASE      = 0x%08x\n", $base
        printf "  LIMIT     = 0x%08x\n", $limit
        printf "  SIZE      = 0x%x bytes\n", $limit - $base + 1
        printf "  RBAR_ATTR = 0x%x\n", $rbar_attr
        printf "  RLAR_ATTR = 0x%x\n", $rlar_attr
        printf "  EN        = %u\n", $en
        printf "  XN        = %u\n", $xn
        printf "  AP        = %u\n", $ap
        printf "  SH        = %u\n", $sh
        printf "  ATTRIDX   = %u\n", $attridx

        set $i = $i + 1
    end

    set *(uint32_t *)0xE000ED98 = $old_rnr
    printf "\n==== end MPU dump ====\n"
end

define dump_sau
    set $SAU_RNR  = 0xE000EDD8
    set $SAU_RBAR = 0xE000EDDC
    set $SAU_RLAR = 0xE000EDE0

    set $i = 0
    while $i < 8
      set *(unsigned int *)$SAU_RNR = $i
      set $rbar = *(unsigned int *)$SAU_RBAR
      set $rlar = *(unsigned int *)$SAU_RLAR
      printf "SAU[%d]: RBAR=0x%08x RLAR=0x%08x base=0x%08x limit=0x%08x EN=%d NSC=%d\n", \
        $i, $rbar, $rlar, ($rbar & 0xffffffe0), (($rlar & 0xffffffe0) | 0x1f), ($rlar & 1), (($rlar >> 1) & 1)
      set $i = $i + 1
    end
end

define set_interrupt
    set *0xe000e100=0x1000
    set *0xe000e200=0x1000
    set *0xE000E380=0x0
end

define clear_xn_all_mpu
    set $MPU_TYPE = *(uint32_t *)0xE000ED90
    set $MPU_RNR  = 0xE000ED98
    set $MPU_RBAR = 0xE000ED9C
    set $MPU_RLAR = 0xE000EDA0

    set $regions = ($MPU_TYPE >> 8) & 0xff
    set $old_rnr = *(uint32_t *)$MPU_RNR

    printf "\nClearing XN bit from all Secure MPU regions...\n"
    printf "Regions implemented = %u\n", $regions

    set $i = 0
    while $i < $regions
        set *(uint32_t *)$MPU_RNR = $i

        set $rbar_old = *(uint32_t *)$MPU_RBAR
        set $rlar     = *(uint32_t *)$MPU_RLAR
        set $enabled  = $rlar & 1
        set $xn_old   = $rbar_old & 1
        set $rbar_new = $rbar_old & 0xfffffffe

        set *(uint32_t *)$MPU_RBAR = $rbar_new

        printf "Region %u: EN=%u RBAR 0x%08x -> 0x%08x XN %u -> 0\n", \
               $i, $enabled, $rbar_old, $rbar_new, $xn_old

        set $i = $i + 1
    end

    set *(uint32_t *)$MPU_RNR = $old_rnr

    printf "Done. Execute DSB/ISB in target code if possible.\n"
end