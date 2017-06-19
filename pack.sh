#!/bin/bash

#  pack script for preparing related updated files
#
#  Copyright:	(C) 2017 Conexant Systems, LLC.
#  Author:	Aimar Liu, <aimar.liu@conexant.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as
#  published by the Free Software Foundation.
#

DIR=$PWD
image="zImage"
vmlinuz="vmlinuz"
KERNEL_ARCH=arm
CC=arm-linux-gnueabihf-
mkdir -p "${DIR}/deploy/boot/dtbs/"

# copy kernel release version to kernel_version
cp ${DIR}/include/config/kernel.release ${DIR}/kernel_version
[ "$?" -eq 0 ] || exit $?;

# cat current kernel release version
temp_string=$(cat "${DIR}/include/generated/utsrelease.h")
[ "$?" -eq 0 ] || exit $?;
KERNEL_UTS=$(echo ${temp_string} | awk '{print $3}' | sed 's/\"//g')

echo "uname_r=$KERNEL_UTS" > ${DIR}/deploy/boot/uEnv.txt

if [ -f "${DIR}/deploy/boot/${vmlinuz}-${KERNEL_UTS}" ]; then
	rm -rf "${DIR}/deploy/boot/${image}-${KERNEL_UTS}"
fi

if [ -f ./arch/${KERNEL_ARCH}/boot/${image} ] ; then
	cp -v arch/${KERNEL_ARCH}/boot/${image} \
"${DIR}/deploy/boot/${vmlinuz}-${KERNEL_UTS}"
fi

echo "-----------------------------"
echo "Installing kernel modules and copy dtbs.."

make -s ARCH="${KERNEL_ARCH}" CROSS_COMPILE="${CC}" modules_install \
INSTALL_MOD_PATH="${DIR}/deploy/"
[ "$?" -eq 0 ] || exit $?;

mkdir -p "${DIR}/deploy/boot/dtbs/${KERNEL_UTS}"
cp ${DIR}/arch/${KERNEL_ARCH}/boot/dts/*.dtb \
"${DIR}/deploy/boot/dtbs/${KERNEL_UTS}"
[ "$?" -eq 0 ] || exit $?;


rm -rf deploy/lib/firmware

pkg="rootfs-update"
deployfile="-${pkg}.tar.gz"
tar_options="--create --gzip --file"
echo "Compressing ${KERNEL_UTS}${deployfile}..."
cd "${DIR}/deploy" || true
tar ${tar_options} "../${KERNEL_UTS}${deployfile}" ./*
[ "$?" -eq 0 ] || exit $?;
cd ..

