## SPDX-License-Identifier: Apache-2.0
## Copyright (c) Bao Project and Contributors. All rights reserved.

cpu-objs-y+=$(riscv_mem_prot)/boot.o
cpu-objs-y+=$(riscv_mem_prot)/page_table.o
cpu-objs-y+=$(riscv_mem_prot)/mem.o
cpu-objs-y+=$(riscv_mem_prot)/root_pt.o
cpu-objs-y+=$(riscv_mem_prot)/vm.o
cpu-objs-y+=$(riscv_mem_prot)/relocate.o
cpu-objs-y+=$(riscv_mem_prot)/iommu.o
