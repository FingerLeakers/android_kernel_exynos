#!/bin/bash

arch_name=${KERNEL_ARCH_NAME}
NEW_CROSS_COMPILE=${PRODUCT_ROOT_DIRECTORY}/android/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
NEW_CC=${PRODUCT_ROOT_DIRECTORY}/android/prebuilts/clang/host/linux-x86/clang-4639204/bin/clang
NEW_CLANG_TRIPLE=${PRODUCT_ROOT_DIRECTORY}/android/prebuilts/clang/host/linux-x86/clang-4639204/bin/aarch64-linux-gnu-

function kbuild {

	temp_export_common_kernel_variable
	cd ${PRODUCT_ROOT_DIRECTORY}/android/${KERNEL_SRC_DIRECTORY}
	make -j8  SEC_KNOX_CONTAINER_VERSION=v00 -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${NEW_CC} CLANG_TRIPLE=${NEW_CLANG_TRIPLE} all
	cp -v -f arch/${arch_name}/boot/Image ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/kernel
	cd -
}

function kclean {

	temp_export_common_kernel_variable
	cd ${PRODUCT_ROOT_DIRECTORY}/android/${KERNEL_SRC_DIRECTORY}
	make -j8  SEC_KNOX_CONTAINER_VERSION=v00 -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${NEW_CC} CLANG_TRIPLE=${NEW_CLANG_TRIPLE} clean
	cd -
}

function kdistclean {

	temp_export_common_kernel_variable
	cd ${PRODUCT_ROOT_DIRECTORY}/android/${KERNEL_SRC_DIRECTORY}
	make -j8 SEC_KNOX_CONTAINER_VERSION=v00 -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${NEW_CC} CLANG_TRIPLE=${NEW_CLANG_TRIPLE} distclean
	cd -
}

function kconfig {

	temp_export_common_kernel_variable
	cd ${PRODUCT_ROOT_DIRECTORY}/android/${KERNEL_SRC_DIRECTORY}
	config_generate
	make -j8  SEC_KNOX_CONTAINER_VERSION=v00 -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${NEW_CC} CLANG_TRIPLE=${NEW_CLANG_TRIPLE} oldnoconfig
	cd -
}

function kheaders {

	temp_export_common_kernel_variable
	cd ${PRODUCT_ROOT_DIRECTORY}/android/${KERNEL_SRC_DIRECTORY}
	rm -rf ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/obj/KERNEL_OBJ
	mkdir -p ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/obj/KERNEL_OBJ
	make -j8  SEC_KNOX_CONTAINER_VERSION=v00 -C . INSTALL_HDR_PATH=${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/obj/KERNEL_OBJ ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${NEW_CC} CLANG_TRIPLE=${NEW_CLANG_TRIPLE} headers_install
	cd -
}

function kdtb {

	temp_export_common_kernel_variable
	cd ${PRODUCT_ROOT_DIRECTORY}/android/${KERNEL_SRC_DIRECTORY}
	DTB_DIR="${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/arch/${arch_name}/boot/dts"
	rm -v -f ${DTB_DIR}/**/*.dtb
	rm -v -f ${DTB_DIR}/*.dtb
	rm -v -f ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/*.dtb

	for f in ${KERNEL_DTBLIST}
	do
		echo "Make DTB: $f"
		make -j8  SEC_KNOX_CONTAINER_VERSION=v00 ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${NEW_CC} CLANG_TRIPLE=${NEW_CLANG_TRIPLE} "$f.dtb"
		cp -v -f "${DTB_DIR}/$f.dtb" ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/
	done

	pushd .
	echo "Make mkdtimg for dt.img"
	cd ${PRODUCT_ROOT_DIRECTORY}/android/prebuilts/misc/linux-x86/libufdt/
	./mkdtimg create ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/dt.img  --custom0=/:dtb-hw_rev --custom1=/:dtb-hw_rev_end ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/*.dtb
	popd

	DTBO_DIR="${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/arch/${arch_name}/boot/dts"
	rm -v -f ${DTB_DIR}/**/*.dtbo
	rm -v -f ${DTB_DIR}/*.dtbo
	rm -v -f ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/*.dtbo

	for f in ${KERNEL_DTBOLIST}
	do
		echo "Make DTBO: $f"
		make -j8  SEC_KNOX_CONTAINER_VERSION=v30 ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${NEW_CC} CLANG_TRIPLE=${NEW_CLANG_TRIPLE} "$f.dtbo"
		cp -v -f "${DTB_DIR}/$f.dtbo" ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/
	done

	pushd .
	echo "Make mkdtimg for dtbo.img"
	cd ${PRODUCT_ROOT_DIRECTORY}/android/prebuilts/misc/linux-x86/libufdt/
	./mkdtimg create ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/dtbo.img --custom0=/:dtbo-hw_rev --custom1=/:dtbo-hw_rev_end ${PRODUCT_ROOT_DIRECTORY}/android/${PRODUCT_OUT_DIRECTORY}/*.dtbo
	popd
	cd -

}
