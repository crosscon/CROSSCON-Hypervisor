## SPDX-License-Identifier: Apache-2.0
## Copyright (c) Bao Project and Contributors. All rights reserved.

# Architecture definition
ARCH:=riscv
# CPU definition
CPU:=
# Interrupt controller definition
IRQC:=PLIC
# Core IPIs controller
IPIC:=

drivers := 8250_uart

platform_description:=cva6_desc.c

arch_mem_prot:=mpu

platform-cppflags =
platform-cflags = 
platform-asflags =
platform-ldflags =

riscv_march:=rv64g
riscv_mabi:=lp64d
