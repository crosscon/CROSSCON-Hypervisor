tar rem:3333
file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/CROSSCON-Hypervisor-LASTREBASE/bin/lpc55s69/dual-pervmtee-m/crossconhyp.elf
set $pc=_reset_handler
set mem inaccessible-by-default off
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/mTower2/build/secure/arch/cortex-m33/lpc/src/lpc55s69/secure/bl32.elf
add-symbol-file /home/mjs/CROSSCON/TOREMOVE/RebaseTester/freertos-over-CROSSCONHyp/build/lpc55s69/freertos.elf
b teeHelloWorldTask
b Hello_World_TA_OpenSessionEntryPoint
tui enable