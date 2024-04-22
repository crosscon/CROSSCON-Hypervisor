#!/bin/bash -e

# make clean ; compiledb make PLATFORM=qemu-aarch64-virt CONFIG_BUILTIN=y CONFIG=enclave_qemu OPTIMIZATIONS=0; exit

# ENCLV=sgx-nbench
# ENCLV=sample_enclave
# ENCLV=YCSB-C-SGX
# ENCLV=sgx-mpk
# ENCLV=SGX-OpenSSL/SampleCode/Simple_TLS_Server
ENCLV=SGX-OpenSSL/SampleCode/Simple_TLS_Client


# pushd ../anytee-sdsgx/
# 	. sourceme.sh
# 	cd sdk
# 	find . -name "*.o" -delete
# 	find . -name "*.a" -delete
# 	make all -j16
# 	cd -
# 	cd ${ENCLV}
# 	cd App
# 	find . -name "*.o" -delete
# 	cd -
# 	make app
#
# 	rm -r ../sdk/libOS/build* || true
# 	make enclave.so PLATFORM=qemu-aarch64-virt
# popd
#
#
# make PLATFORM=qemu-aarch64-virt CONFIG=anytee_sgx_enclave clean
# make PLATFORM=qemu-aarch64-virt CONFIG=anytee_sgx_enclave -j16
# cp -v ../crossconhyp-hypervisor/configs/anytee-sdsgx/anytee_sgx_enclave.bin ../initramfs-aarch64/enclave.signed.so
#
# mkdir -p ../to_buildroot_sgx/bin
# mkdir -p ../to_buildroot_sgx/root
# cp -v ../sgx_anytee_enclave/$ENCLV/app ../to_buildroot_sgx/bin/enclave_app
# cp -r ../sgx_anytee_enclave/sgx-nbench/nbenchPortal ../to_buildroot_sgx/root/
# cp -r ../sgx_anytee_enclave/nbench/nbench ../to_buildroot_sgx/bin
# cp -v ../crossconhyp-hypervisor/configs/anytee_sgx_enclave/anytee_sgx_enclave.bin ../to_buildroot_sgx/root/enclave.signed.so
#
# cd ../sgx-server-echo
# make all
# cd -

cd ../buildroot
export PERL_MM_OPT=
make O=build-aarch64
cd -

cd ../linux/
#cp ../linux-aarch64-qemu.config build-aarch64/.config
make CROSS_COMPILE=aarch64-none-linux-gnu- ARCH=arm64 -j`nproc` O=build-aarch64
cd -

dtc -I dts -O dtb ../anytee-qemu-ws/qemu.dts > ../anytee-qemu-ws/qemu.dtb

make -C ../lloader IMAGE=../linux/build-aarch64/arch/arm64/boot/Image DTB=../anytee-qemu-ws/qemu.dtb TARGET=linux-aarch64.bin ARCH=aarch64 clean

make -C ../lloader IMAGE=../linux/build-aarch64/arch/arm64/boot/Image DTB=../anytee-qemu-ws/qemu.dtb TARGET=linux-aarch64.bin ARCH=aarch64



#CONFIG=bm-tz
CONFIG=anyteetz-aarch64-qemu
PLAT=qemu-aarch64-virt


make \
    PLATFORM=$PLAT \
    CONFIG_BUILTIN=y \
    CONFIG=$CONFIG \
    clean

compiledb \
    make \
    PLATFORM=$PLAT \
    CONFIG_BUILTIN=y \
    CONFIG=$CONFIG \
    OPTIMIZATIONS=0 \
    CROSS_COMPILE=aarch64-none-elf-


