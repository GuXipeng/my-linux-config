#!/bin/bash
make -C kernel O=../out/target/product/$TARGET_PRODUCT/obj/KERNEL_OBJ ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- KCFLAGS=-mno-android -j8
cp out/target/product/$TARGET_PRODUCT/obj/KERNEL_OBJ/arch/arm64/boot/Image.gz-dtb out/target/product/$TARGET_PRODUCT/kernel
rm out/target/product/$TARGET_PRODUCT/boot.img
make bootimage-nodeps
