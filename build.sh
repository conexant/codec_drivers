#!/bin/bash

if [ "$#" -ne 1 ]; then
	MAKE_DEF_CONFIG=bb.org_defconfig
else
	MAKE_DEF_CONFIG=$1
fi

echo "Checking coding style"
patch_head="$(git rev-parse HEAD^{commit})"
/usr/bin/checkpatch.pl --no-changeid --no-signoff --min-conf-desc-length=2 \
 --codespell -g $patch_head
[ "$?" -eq 0 ] || exit $?;
echo "Build Beagle Bone Black kernel starts now"
echo "default configuration = $MAKE_DEF_CONFIG"
export PATH="/opt/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin:$PATH"
echo "PATH = $PATH"
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- $MAKE_DEF_CONFIG
[ "$?" -eq 0 ] || exit $?;
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage modules dtbs -j 6
[ "$?" -eq 0 ] || exit $?;
