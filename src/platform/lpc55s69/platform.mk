## SPDX-License-Identifier: Apache-2.0
## Copyright (c) Bao Project and Contributors. All rights reserved.

# Architecture definition
ARCH:=armv8m
ARCH_PROFILE:=main
drivers = lpc_uart

platform_description:=lpc55s69_desc.c

platform-cppflags =
platform-cflags =
platform-asflags =
platform-ldflags =

plat_core_arch := single_core