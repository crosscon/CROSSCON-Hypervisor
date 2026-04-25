tar rem:3334
file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/CROSSCON-Hypervisor-LASTREBASE/bin/lpc55s69/pervmtee-m/crossconhyp.elf
set $pc=_reset_handler
set mem inaccessible-by-default off
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/mTower/build/secure/arch/cortex-m33/lpc/src/lpc55s69/secure/bl32.elf
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/freertos-over-CROSSCONHyp/build/lpc55s69/freertos.elf
b mem_init
b *0x60000
b *0x20000
b _systick_handler
tui enable
c