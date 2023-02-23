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

# if [[ ! -e /media/david/boot ]]
# then
#     echo "/media/david/boot not found. Waiting"
# 
#     while [[ ! -e /media/david/boot ]]
#     do
#       sleep 1
#     done
# fi

if [[ ! -e /media/david/root ]]
then
    echo "/media/david/root not found. Waiting"

    while [[ ! -e /media/david/root ]]
    do
      sleep 1
    done
fi

ENCLV="sgx-nbench"
#ENCLV=sample_enclave

pushd ../sgx_anytee_enclave/
. sourceme.sh
echo $PWD
echo $ENCLV
cd ${ENCLV}
echo $PWD
make app
make enclave.so PLATFORM=imx8mq
popd

cp -v ../sgx_anytee_enclave/$ENCLV/app ../initramfs-aarch64/bin/enclave_app
cp -r ../sgx_anytee_enclave/$ENCLV/App ../initramfs-aarch64/App
cp -r ../sgx_anytee_enclave/$ENCLV/nbenchPortal ../initramfs-aarch64/

compiledb make PLATFORM=imx8mq CONFIG=anytee_sgx_enclave clean
compiledb make PLATFORM=imx8mq CONFIG=anytee_sgx_enclave -j16

cp -v ../bao-hypervisor/configs/anytee_sgx_enclave/anytee_sgx_enclave.bin \
      ../initramfs-aarch64/enclave.signed.so

cd ../linux/
#cp ../linux-aarch64-qemu.config build-aarch64/.config
make \
    CROSS_COMPILE=aarch64-none-linux-gnu- \
    ARCH=arm64 \
    -j16 \
    O=build-aarch64
cd -

make -C ../lloader \
    IMAGE=../linux/build-aarch64/arch/arm64/boot/Image  \
    DTB=../baoEnclave_ws/qemu.dtb TARGET=linux-aarch64-imx.bin \
    ARCH=aarch64 \
    clean

make -C ../lloader \
    IMAGE=../linux/build-aarch64/arch/arm64/boot/Image \
    DTB=../linux/build-aarch64/arch/arm64/boot/dts/freescale/imx8mq-evk.dtb \
    TARGET=linux-aarch64-imx.bin \
    ARCH=aarch64

make clean ;
compiledb make \
    PLATFORM=imx8mq \
    CONFIG_BUILTIN=y \
    CONFIG=enclave_imx \
    OPTIMIZATIONS=0





cd ../imx-mkimage
./steps.sh
cd -
