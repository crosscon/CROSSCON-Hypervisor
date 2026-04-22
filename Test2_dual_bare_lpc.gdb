tar rem:3333
file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/CROSSCON-Hypervisor/bin/lpc55s69/dual-bare/crossconhyp.elf
set $pc=_reset_handler
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/bao-baremetal-guest1/build/lpc55s69/baremetal.elf
b vmm_init
b *0x40000
b *0x20000
b _systick_handler