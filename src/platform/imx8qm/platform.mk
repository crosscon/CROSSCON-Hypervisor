##
 # CROSSCONHyp, a Lightweight Static Partitioning Hypervisor
 #
 # Copyright (c) CROSSCONHyp Project (www.crossconhyp-project.org), 2019-
 #
 # Authors:
 #      Jose Martins <jose.martins@bao-project.org>
 #
 # CROSSCONHyp is free software; you can redistribute it and/or modify it under the
 # terms of the GNU General Public License version 2 as published by the Free
 # Software Foundation, with a special exception exempting guest code from such
 # license. See the COPYING file in the top-level directory for details.
 #
##

# Architecture definition
ARCH:=armv8
# CPU definition
CPU:=cortex-a53

GIC_VERSION:=GICV3

drivers = nxp_uart

platform-cppflags =
platform-cflags = -mcpu=$(CPU)
platform-asflags =
platform-ldflags =
