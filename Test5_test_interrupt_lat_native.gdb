tar rem:3333
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/TestBareInterruptLatency/baremetal-secure/build/lpc55s69/baremetal.elf
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/TestBareInterruptLatency/baremetal-nonsecure/build/lpc55s69/baremetal.elf
set $pc=0x10000000
set mem inaccessible-by-default off
b main