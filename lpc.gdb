tar rem:3333
file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/CROSSCON-Hypervisor/bin/lpc55s69/pervmtee-m/crossconhyp.elf
set $pc=_reset_handler
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/mTower/build/secure/arch/cortex-m33/lpc/src/lpc55s69/secure/bl32.elf
b vmm_init
b *0x60000