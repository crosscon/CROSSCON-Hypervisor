#!/bin/bash -e




# cp -rv ../sgx_anytee_enclave/ ../../initramfs/
cd ../sgx_anytee_enclave/
make app
make enclave.so
cd -
cp -v ../sgx_anytee_enclave/app ../initramfs/bin/wallet_app
cp -r ../sgx_anytee_enclave/App ../initramfs/App

compiledb make PLATFORM=qemu-aarch64-virt CONFIG=wallet_enclave -j16
cp -v ../bao-hypervisor/configs/wallet_enclave/wallet_enclave.bin ../initramfs/enclave_config.bin

cd ../linux/
# cp qemu_enclave.config .config
make CROSS_COMPILE=aarch64-none-linux-gnu- ARCH=arm64 -j`nproc`
cd -

cd ../lloader
./runme.sh
cd -

make clean ; compiledb make PLATFORM=qemu-aarch64-virt CONFIG_BUILTIN=y CONFIG=enclave_qemu OPTIMIZATIONS=0 -j`nproc`

