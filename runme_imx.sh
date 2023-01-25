#!/bin/bash -e


RZ_DIR=/home/david/PhD/Research/GreyZone
TEEOD_DIR=/home/david/PhD/Research/TEEOD/

DEV=/dev/mmcblk0
PART=p1


echo ""
if [[ ! -e $DEV$PART ]]
then
    echo "Waiting for card."

    while [[ ! -e $DEV$PART ]]
    do
      sleep 1
    done
fi

if [[ ! -e /media/david/boot ]]
then
    echo "/media/david/boot not found. Waiting"

    while [[ ! -e /media/david/boot ]]
    do
      sleep 1
    done
fi

if [[ ! -e /media/david/root ]]
then
    echo "/media/david/root not found. Waiting"

    while [[ ! -e /media/david/root ]]
    do
      sleep 1
    done
fi

cd ~/PhD/Research/MSc/baoEnclave/benchmarks/bao-baremetal-wallet
compiledb make PLATFORM=imx8mq
cd -

cd ~/PhD/Research/MSc/baoEnclave/benchmarks/cma_malloc
aarch64-none-linux-gnu-gcc userspace/finalApp.c userspace/src/tx.c userspace/src/baoenclave.c userspace/src/contiguousMalloc.c -g -static -I./userspace/inc/ -o wallet
sudo cp -vr userspace /media/david/root/my_applications/
sudo cp -vr wallet /media/david/root/my_applications/
cd -

compiledb make PLATFORM=imx8mq CONFIG=wallet -j16
sudo cp -v ~/PhD/Research/MSc/baoEnclave/benchmarks/bao-hypervisor/configs/wallet/wallet.bin /media/david/root/my_configs/enclave_config.bin


cd ~/PhD/Research/MSc/baoEnclave/debug_vmstack/linux 
make CROSS_COMPILE=aarch64-none-linux-gnu- ARCH=arm64 -j16
cd -

cd ~/PhD/Research/MSc/baoEnclave/benchmarks/lloader
./runme.sh
cd -

make PLATFORM=imx8mq CONFIG_BUILTIN=y CONFIG=enclave DEBUG=y -j16

cd ~/PhD/Research/MSc/baoEnclave/benchmarks/imx-mkimage
./steps.sh
cd -
