tar rem:3334
file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/CROSSCON-Hypervisor-LASTREBASE/bin/lpc55s69/pervmtee-m/crossconhyp.elf
set $pc=_reset_handler
set mem inaccessible-by-default off
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/bao-baremetal-guest1/build/lpc55s69/baremetal.elf
b vmm_init
b *0x40000
b *0x20000