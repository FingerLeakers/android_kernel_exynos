#!/bin/bash

temp_export_common_kernel_variable

arch_name=${build__dash__kernel__colon__arch_name}
if [ "$arch_name" == "" ]; then
arch_name="arm"
fi
vmware_target_path=${top__dash__dir}/android/vendor/vmware/temp
vmware_source_path=${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}/arch/${arch_name}/mvp
kernel_path=${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}

NEW_CROSS_COMPILE="${cross__dash__compile}"
NEW_CC="${tool__colon__clang__dash__cc}"
NEW_CLANG_TRIPLE="${tool__colon__clang__dash__triple}"

function check_cross_compile {

    pushd . > /dev/null

    # Change CROSS COMPILE if RKP_CFP_JOPP is enabled
    # Change CROSS COMPILE if INDIRECT_BRANCH_VERIFIER is enabled
    cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
    CFP_JOPP=`./scripts/config --file .config --state CONFIG_RKP_CFP_JOPP`
    IBV=`./scripts/config --file .config --state CONFIG_SEC_DEBUG_INDIRECT_BRANCH_VERIFIER`
    if [[ "${CFP_JOPP}" == "y" ]]; then
      echo "USE JOPP COMPILER"
      if [[ "${IBV}" == "y" ]]; then
        NEW_CROSS_COMPILE="${top__dash__dir}/android/prebuilts/gcc-cfp/gcc-ibv-jopp/aarch64-linux-android-4.9/bin/aarch64-linux-android-"
      else
        NEW_CROSS_COMPILE="${top__dash__dir}/android/prebuilts/gcc-cfp/gcc-cfp-jopp-only/aarch64-linux-android-4.9/bin/aarch64-linux-android-"
      fi
      echo "${build__dash__kernel__colon__subtype}"
      if [[ "${build__dash__kernel__colon__subtype}" == "exynos9820" ]]; then
        if [[ "${IBV}" == "y" ]]; then
          NEW_CC="android/prebuilts/clang/host/linux-x86/clang-4639204-jopp-ibv/bin/clang"
          NEW_CLANG_TRIPLE="android/prebuilts/clang/host/linux-x86/clang-4639204-jopp-ibv/bin/aarch64-linux-gnu-"
        else
          NEW_CC="android/prebuilts/clang/host/linux-x86/clang-4639204-cfp-jopp/bin/clang"
          NEW_CLANG_TRIPLE="android/prebuilts/clang/host/linux-x86/clang-4639204-cfp-jopp/bin/aarch64-linux-gnu-"
        fi
      fi
    else
      echo "NOT USE JOPP COMPILER"
    fi

    popd > /dev/null
}

function kbuild {

	pushd . > /dev/null

  check_cross_compile
	cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
	make ${make__dash__options} -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} all
	if [ "$arch_name" == "arm" ]; then
      cp -v -f arch/${arch_name}/boot/zImage ${android__dash__product__dash__out__dash__dir}/kernel
    else
      cp -v -f arch/${arch_name}/boot/Image ${android__dash__product__dash__out__dash__dir}/kernel
    fi
	popd > /dev/null
}

function ekbuild {

  pushd . > /dev/null

  check_cross_compile
  cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
  etrace make ${make__dash__options} -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} all
  if [ "$arch_name" == "arm" ]; then
      cp -v -f arch/${arch_name}/boot/zImage ${android__dash__product__dash__out__dash__dir}/kernel
    else
      cp -v -f arch/${arch_name}/boot/Image ${android__dash__product__dash__out__dash__dir}/kernel
    fi
  popd > /dev/null
}

function kclean {

	pushd . > /dev/null
	cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
	make ${make__dash__options} -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} clean
	popd > /dev/null
}

function kdistclean {

	pushd . > /dev/null
	cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
	make ${make__dash__options} -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} distclean
	popd > /dev/null
}

function kconfig {

	pushd . > /dev/null
  check_cross_compile
	cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
	config_generate
	make ${make__dash__options} -C . ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} oldnoconfig
	popd > /dev/null
}

function kheaders {

	pushd . > /dev/null
	cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
	rm -rf ${android__dash__product__dash__out__dash__dir}/obj/KERNEL_OBJ
	mkdir -p ${android__dash__product__dash__out__dash__dir}/obj/KERNEL_OBJ
	make ${make__dash__options} -C . INSTALL_HDR_PATH=${android__dash__product__dash__out__dash__dir}/obj/KERNEL_OBJ ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} headers_install
	popd > /dev/null
}

function kdtb {

	pushd . > /dev/null
	cd ${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}
	DTB_DIR="${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}/arch/${arch_name}/boot/dts"
  # Remove unused dtbs
  rm -v -f ${DTB_DIR}/**/*.dtb
  rm -v -f ${DTB_DIR}/*.dtb
  rm -v -f ${android__dash__product__dash__out__dash__dir}/*.dtb

  # Make dtbs
  for f in ${build__dash__kernel__colon__dtb__dash__list}
  do
    echo $f

    if [[ "${NEW_CC}" != "" && "${NEW_CLANG_TRIPLE}" != "" ]]; then
      make ${make__dash__options} ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} "$f.dtb"
    else
      make ${make__dash__options} ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} "$f.dtb"
    fi

    cp -v -f "${DTB_DIR}/$f.dtb" ${android__dash__product__dash__out__dash__dir}/
  done

  # Make dtbtool
  pushd . > /dev/null

  if [ "${build__dash__kernel__colon__dtbo__dash__list}" == "" ]; then
    echo "Make dtbtool for dt.img"
    cd ${top__dash__dir}/buildscript/tools/mk_dt_image/
    make ${make__dash__s__dash__options}
    # Make dt.img
    ./dtbtool -o ${android__dash__product__dash__out__dash__dir}/dt.img ${android__dash__product__dash__out__dash__dir}/
    cp -v -f dtbtool ${kernel_path}/tools
  else
    echo "Make mkdtimg for dt.img"
    cd ${top__dash__dir}/android/prebuilts/misc/linux-x86/libufdt/
    ./mkdtimg create ${android__dash__product__dash__out__dash__dir}/dt.img  --custom0=/:dtb-hw_rev --custom1=/:dtb-hw_rev_end ${android__dash__product__dash__out__dash__dir}/*.dtb
  fi

  popd > /dev/null

  DTBO_DIR="${top__dash__dir}/android/kernel/${build__dash__kernel__colon__subtype}/arch/${arch_name}/boot/dts"
  # Remove unused dtboes
  rm -v -f ${DTB_DIR}/**/*.dtbo
  rm -v -f ${DTB_DIR}/*.dtbo
  rm -v -f ${android__dash__product__dash__out__dash__dir}/*.dtbo

  # Make dtbos
  for f in ${build__dash__kernel__colon__dtbo__dash__list}
  do
    echo $f

    if [[ "${NEW_CC}" != "" && "${NEW_CLANG_TRIPLE}" != "" ]]; then
      make ${make__dash__options} ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} CC=${top__dash__dir}/${NEW_CC} CLANG_TRIPLE=${top__dash__dir}/${NEW_CLANG_TRIPLE} "$f.dtbo"
    else
      make ${make__dash__options} ARCH=${arch_name} CROSS_COMPILE=${NEW_CROSS_COMPILE} "$f.dtbo"
    fi

    cp -v -f "${DTB_DIR}/$f.dtbo" ${android__dash__product__dash__out__dash__dir}/
  done

  pushd . > /dev/null

  if [ "${build__dash__kernel__colon__dtbo__dash__list}" == "" ]; then
    echo "Make dtbtool for dtbo.img"
    cd ${top__dash__dir}/buildscript/tools/mk_dt_image/
    ./dtbtool -o ${android__dash__product__dash__out__dash__dir}/dtbo.img -l model_info-label -t model_info-type ${android__dash__product__dash__out__dash__dir}/
  else
    echo "Make mkdtimg for dtbo.img"
    cd ${top__dash__dir}/android/prebuilts/misc/linux-x86/libufdt/
    ./mkdtimg create ${android__dash__product__dash__out__dash__dir}/dtbo.img --custom0=/:dtbo-hw_rev --custom1=/:dtbo-hw_rev_end ${android__dash__product__dash__out__dash__dir}/*.dtbo
  fi
  popd > /dev/null

	popd > /dev/null
}

function kimg {

  pushd ${android__dash__product__dash__out__dash__dir} > /dev/null
  echo "Entering ${android__dash__product__dash__out__dash__dir}"
  rm -rf empty
  mkdir -p empty
  unzip -o target_files.zip 'META/*' || true
  PATH=$PATH:${ANDROID_HOST_OUT}/bin:${top__dash__dir}/android/out/dist

  # select binaries with each build module
  imgs="boot.img"
  
  for img in $imgs ; do
      args=`echo "${file__dash__system__dash__info__colon__temp__dash__module__dash__build__dash__info}" | tr ',' '\n' | sed -n "s/^${img//\//\\\/}://p"`
      mount=`_get_arg "$args" mount`
      fs_img_cmd="run_fs_img_cmd '${img}' '${args}'"
      if [[ "$mount" == "data" ]]; then
          BGRun "run_fs_img_cmd \"${img}\" \"${args}\""
      else
          run_fs_img_cmd "${img}" "${args}"
      fi
  done

  popd > /dev/null

}

function signimgs {
  
  pushd ${android__dash__product__dash__out__dash__dir} > /dev/null
  
  type=${sign__dash__sec__dash__secureboot__colon__kernel__colon__type}
  project=${sign__dash__sec__dash__secureboot__colon__kernel__colon__project}
  binaries="$@"
  suffix="${sign__dash__sec__dash__secureboot__colon__kernel__colon__suffix}"
  P_NAME=""
  
  if [[ $suffix != "" ]] ; then
    project=$project$suffix
  else
    # knox code
    if [[ "$SEC_BUILD_OPTION_KNOX_CSB" == true ]] ; then
      if [ "$project" != "" ] ; then
        P_NAME=$project
      fi
      if [[ $P_NAME == "SGH-I337_NA_ATT" || $P_NAME == "SCH-I545_NA_VZW" || $P_NAME == "SGH-M919_NA_TMB" || $P_NAME == "SPH-L720_NA_SPR" || $P_NAME == "SCH-R970_NA_USC" || $P_NAME == "GT-I9505_EUR_XX" || $P_NAME == "SGH-I337M_NA_BMC" || $P_NAME == "GT-I9508_CHN_ZM" || $P_NAME == "SGH-N045_JPN_DCM" || $P_NAME == "SGH-I537_NA_ATT" || $CARRIER == "SCH-R970C_NA_CRI" ]] ; then
        suffix="_C"
      else
        suffix="_B"
      fi
      project=$project$suffix
    fi
  fi

  if [[ "$SEC_BUILD_OPTION_TEMP_RP_VALUE" != "" ]] ; then
    project=${project}_RP${SEC_BUILD_OPTION_TEMP_RP_VALUE}
  fi

  if [[ "$SEC_BUILD_OPTION_KNOX_EXYNOS_KERN_LOCK"  == true && $project != "" ]] ; then
    P_NAME=$project
  fi

  # Didn't porting about padding_size - not used MAIN, JBP_MAIN

  multi_pit=`echo "${pit__dash__info__colon__pit__dash__multi}" | tr -d '\n'`
  if [[ $multi_pit != "" ]] ; then
    IFS_muiti_pit_info=$IFS
    IFS=$','
    for pit_list in $multi_pit ; do
      multi_pit_info=${pit_list##*:}
      binaries="$binaries ${multi_pit_info}"
    done
    IFS=${IFS_muiti_pit_info}
  fi

  for binary in $binaries ; do
    if [ ! -f $binary ] ; then
      echo "sign-sec-secureboot-kernel : skip signing $binary !!!" >&2
      continue
    fi

    # knox code
    if [[ "$SEC_BUILD_OPTION_KNOX_CERT_TYPE" != "" && "$SEC_BUILD_OPTION_KNOX_EXYNOS_KERN_LOCK" == true ]] ; then
      info "Prepending KNOX magic to $binary for model=$P_NAME"
      knox_secure_boot_sign $binary $P_NAME
    fi

    echo "sign-sec-secureboot : signing $binary ..."

    if [ "$type" == "renesas_eos2_prot" ] ; then
      secure_dir_for_rmc=${top__dash__dir}/buildscript/vSign/eos2
      ${secure_dir_for_rmc}/client/SSG_archiver.sh -input $binary -config ${secure_dir_for_rmc}/client/cert_config_samples/${binary%%.*}.cert.ini ${secure_dir_for_rmc}/client/cert_config_samples/env.source -output ./
      sign_new_server ${top__dash__dir}/buildscript/tools/signclient.jar $type $project ${binary%%.*}.zip ${binary%%.*}.sign

      cat ${binary} > signed_${binary}
      cat ${binary%%.*}.sign >> signed_${binary}

      mv -f ${binary} unsigned_${binary}
      mv -f signed_${binary} ${binary}
    else
      if [ "${new__dash__signserver}" == "true" ] ; then
        if [[ $binary != "boot.img" && $binary != "recovery.img" ]]; then
          if [[ "${signerversion}" != "" ]] ; then
            echo "signerversion v.${signerversion}"
                if [[ $binary == *.pit ]]; then
                    $SEC_BUILD_CONF_SIGNER_ROOT_PATH/make_signerv2_header.sh "pit" "bin"
                else
                    $SEC_BUILD_CONF_SIGNER_ROOT_PATH/make_signerv2_header.sh $binary "bin"
                fi

            binary_info=$SEC_BUILD_CONF_SIGNER_ROOT_PATH/signerheader.bin
            chmod 777 $binary
            cat ${binary_info} >> $binary
          fi
        fi
        if [[ "$type" == "ss_exynos40_all" ]] ; then
            echo "add dummy signature"
            dd if=/dev/zero of=sig_dummy.bin bs=272 count=1
            cat sig_dummy.bin >> $binary
        elif [[ "$type" == "ss_exynos50_all" ]] ; then
            echo "add dummy signature"
            dd if=/dev/zero of=sig_dummy.bin bs=528 count=1
            cat sig_dummy.bin >> $binary
        fi

        sign_new_server ${top__dash__dir}/buildscript/tools/signclient.jar $type $project $binary
      else
        perl ${top__dash__dir}/buildscript/tools/SecureBootSign.pl $type $project $binary
      fi
      if [ "`echo $type | cut -c 1-3`" == "bc_" ] ; then
        cat ${binary} > temp_signed_${binary}
        cat signed_${binary} ${top__dash__dir}/buildscript/pk/${project}${type}_PK.img >> temp_signed_${binary}
        mv -f ${binary} unsigned_${binary}
        mv -f temp_signed_${binary} ${binary}
      else
        mv -f ${binary} unsigned_${binary}
        mv -f signed_${binary} ${binary}
      fi
    fi
    if [[ "${svb}" == "true" ]]; then
      if [[ "${chip__dash__vendor}" == "qcom" ]]; then
        if [[ ${binary} == "tz.mbn" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n TZ "${pit__dash__info__colon__pit__dash__default}"`
          tzpartitionsize=$((img_size))
          echo "get from pit Partition size is $tzpartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer --image $binary --partition_name tz --partition_size ${tzpartitionsize} --algorithm SHA256_RSA4096 --key ${top__dash__dir}/android/external/avb/test/data/pub_$SEC_BUILD_CONF_MODEL_SIGNING_NAME.pem --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar --model_name $project
        fi
        if [[ ${binary} == "abl.elf" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n ABL "${pit__dash__info__colon__pit__dash__default}"`
          ablpartitionsize=$((img_size))
          echo "get from pit Partition size is $ablpartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer --image $binary --partition_name abl --partition_size ${ablpartitionsize} --algorithm SHA256_RSA4096 --key ${top__dash__dir}/android/external/avb/test/data/pub_$SEC_BUILD_CONF_MODEL_SIGNING_NAME.pem --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar --model_name $project
        fi
        if [[ ${binary} == "xbl.elf" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n XBL "${pit__dash__info__colon__pit__dash__default}"`
          xblpartitionsize=$((img_size))
          echo "get from pit Partition size is $xblpartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer --image $binary --partition_name xbl --partition_size ${xblpartitionsize} --algorithm SHA256_RSA4096 --key ${top__dash__dir}/android/external/avb/test/data/pub_$SEC_BUILD_CONF_MODEL_SIGNING_NAME.pem --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar --model_name $project
        fi
        if [[ ${binary} == "hyp.mbn" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n HYP "${pit__dash__info__colon__pit__dash__default}"`
          hyppartitionsize=$((img_size))
          echo "get from pit Partition size is $hyppartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer --image $binary --partition_name hyp --partition_size ${hyppartitionsize} --algorithm SHA256_RSA4096 --key ${top__dash__dir}/android/external/avb/test/data/pub_$SEC_BUILD_CONF_MODEL_SIGNING_NAME.pem --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar --model_name $project
        fi
        if [[ ${binary} == "boot.img" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n BOOT "${pit__dash__info__colon__pit__dash__default}"`
          bootpartitionsize=$((img_size))
          echo "get from pit Partition size is $bootpartitionsize" 
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer --image $binary --partition_name boot --partition_size ${bootpartitionsize} --algorithm SHA256_RSA4096 --key ${top__dash__dir}/android/external/avb/test/data/pub_$SEC_BUILD_CONF_MODEL_SIGNING_NAME.pem --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar --model_name $project
        fi
        if [[ ${binary} == "recovery.img" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n RECOVERY "${pit__dash__info__colon__pit__dash__default}"`
          recoverypartitionsize=$((img_size))
          echo "get from pit Partition size is $recoverypartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer --image $binary --partition_name recovery --partition_size ${recoverypartitionsize} --algorithm SHA256_RSA4096 --key ${top__dash__dir}/android/external/avb/test/data/pub_$SEC_BUILD_CONF_MODEL_SIGNING_NAME.pem --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar --model_name $project
        fi
        if [[ ${binary} == "dtbo.img" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n DTBO "${pit__dash__info__colon__pit__dash__default}"`
          dtbopartitionsize=$((img_size))
          echo "get from pit Partition size is $dtbopartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer --image $binary --partition_name dtbo --partition_size ${dtbopartitionsize} --algorithm SHA256_RSA4096 --key ${top__dash__dir}/android/external/avb/test/data/pub_$SEC_BUILD_CONF_MODEL_SIGNING_NAME.pem --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar --model_name $project
        fi
      fi
      if [[ "${chip__dash__vendor}" == "slsi" ]]; then
        if [[ ${binary} == "sboot.bin" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n BOOTLOADER "${pit__dash__info__colon__pit__dash__default}"`
          sbootpartitionsize=$((img_size))
          echo "get from pit Partition size is $sbootpartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer \
            --image $binary \
            --partition_name bootloader \
            --partition_size ${sbootpartitionsize} \
            --algorithm SHA256_RSA4096 \
            --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
            --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
            --model_name $project
        fi
        if [[ ${binary} == "boot.img" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n BOOT "${pit__dash__info__colon__pit__dash__default}"`
          bootpartitionsize=$((img_size))
          echo "get from pit Partition size is $bootpartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer \
            --image $binary \
            --partition_name boot \
            --partition_size ${bootpartitionsize} \
            --algorithm SHA256_RSA4096 \
            --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
            --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
            --model_name $project
        fi
        if [[ ${binary} == "recovery.img" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n RECOVERY "${pit__dash__info__colon__pit__dash__default}"`
          recoverypartitionsize=$((img_size))
          echo "get from pit Partition size is $recoverypartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer \
            --image $binary \
            --partition_name recovery \
            --partition_size ${recoverypartitionsize} \
            --algorithm SHA256_RSA4096 \
            --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
            --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
            --model_name $project
        fi
        if [[ ${binary} == "dt.img" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n DTB "${pit__dash__info__colon__pit__dash__default}"`
          dtbpartitionsize=$((img_size))
          echo "get from pit Partition size is $dtbpartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer \
            --image $binary \
            --partition_name dtb \
            --partition_size ${dtbpartitionsize} \
            --algorithm SHA256_RSA4096 \
            --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
            --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
            --model_name $project
        fi
        if [[ ${binary} == "dtbo.img" ]] ; then
          echo "$binary avb signing"
          img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n DTBO "${pit__dash__info__colon__pit__dash__default}"`
          dtbopartitionsize=$((img_size))
          echo "get from pit Partition size is $dtbopartitionsize"
          ${top__dash__dir}/android/external/avb/avbtool add_hash_footer \
            --image $binary \
            --partition_name dtbo \
            --partition_size ${dtbopartitionsize} \
            --algorithm SHA256_RSA4096 \
            --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
            --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
            --model_name $project
        fi
      fi
    fi
    if [[ "${chip__dash__vendor}" == "sprd" && ${binary} == "spl.img" ]] ; then
        spr_spl_gen=${top__dash__dir}/android/bootable/bootloader/sboot/spl/gen/sprd_spl_gen
        mv -f spl.img spl_temp.img
                ${spr_spl_gen} spl_temp.img spl.img
    fi
    if [[ "$binary" == "${pit__dash__info__colon__pit__dash__default}" ]]; then
      echo "Signed PIT copied to ..."
      copy_to_release ./${binary}
    fi
  done

  popd > /dev/null

}

function signvbmeta {
  
  pushd . > /dev/null

  type=${sign__dash__sec__dash__secureboot__colon__vbmeta__colon__type}
  project=$SEC_BUILD_CONF_MODEL_SIGNING_NAME
  #binaries="${sign__dash__sec__dash__secureboot__colon__vbmeta__colon__binary}"
  binaries="boot.img recovery.img dt.img dtbo.img"
  rbindexlocation=1

  cd ${package__dash__dir}
  if [[ "${chip__dash__vendor}" == "slsi" ]]; then
    for binary in $binaries ; do
      if [[ ${binary} == "keystorage.bin" && -f $binary ]] ; then
        echo "$binary avb signing"
        img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n KEYSTORAGE "${pit__dash__info__colon__pit__dash__default}"`
        kspartitionsize=$((img_size))
        echo "get from pit Partition size is $kspartitionsize"
        ${top__dash__dir}/android/external/avb/avbtool add_hash_footer \
          --image $binary \
          --partition_name keystorage \
          --partition_size ${kspartitionsize} \
          --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
          --algorithm SHA256_RSA4096 \
          --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
          --model_name $project
      fi
      if [[ ${binary} == "cm.bin" && -f $binary ]] ; then
        echo "$binary avb signing"
        img_size=`eval ${top__dash__dir}/buildscript/readpit.dat -n CM "${pit__dash__info__colon__pit__dash__default}"`
        cmpartitionsize=$((img_size))
        echo "get from pit Partition size is $cmpartitionsize"
        ${top__dash__dir}/android/external/avb/avbtool add_hash_footer \
          --image $binary \
          --partition_name cm \
          --partition_size ${cmpartitionsize} \
          --algorithm SHA256_RSA4096 \
          --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
          --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
          --model_name $project
      fi
    done
  fi

  ${top__dash__dir}/android/external/avb/avbtool extract_public_key \
    --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
    --output pubkey_for_chain.pem

#DEBUG VBMETA 
  echo "Generate Debug Vbmeta image(Chain Partition descriptor)"
  gen_vbmeta_cmd="${top__dash__dir}/android/external/avb/avbtool make_vbmeta_image \
    --rollback_index 0 \
    --algorithm SHA256_RSA4096 \
    --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
    --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
    --model_name $project \
    --output vbmeta.img"

  for binary in $binaries ; do
    imgExt=${binary#*.}
    if [[ ${binary} == "sboot.bin" ]] ; then
      imgName="bootloader"
    elif [[ ${binary} == "dt.img" ]] ; then
      imgName="dtb"
    else
      imgName=${binary%%*.$imgExt}
    fi
    gen_vbmeta_cmd="$gen_vbmeta_cmd --chain_partition $imgName:$rbindexlocation:pubkey_for_chain.pem"

    rbindexlocation=$((rbindexlocation+1))
  done

  echo "Gen Debug vbmeta is ready"
  echo $gen_vbmeta_cmd
  $gen_vbmeta_cmd

  $SEC_BUILD_CONF_SIGNER_ROOT_PATH/make_signerv2_header.sh "vbmeta.img" "bin"
  binary_info=$SEC_BUILD_CONF_SIGNER_ROOT_PATH/signerheader.bin
  chmod 777 "vbmeta.img"
  cat ${binary_info} >> "vbmeta.img"

  if [[ -d debug_vbmeta ]] ; then
    rm -rf debug_vbmeta
  fi
  mkdir debug_vbmeta
  mv vbmeta.img debug_vbmeta/

#RELEASE VBMEATA
  echo "Generate Release Vbmeta image"
  is_ready=true
  rbindexlocation=1
  gen_vbmeta_cmd="${top__dash__dir}/android/external/avb/avbtool make_vbmeta_image \
    --rollback_index 0 \
    --algorithm SHA256_RSA4096 \
    --key ${top__dash__dir}/android/external/avb/test/data/pub_$project.pem \
    --signing_helper_with_files ${top__dash__dir}/buildscript/tools/signclient.jar \
    --model_name $project \
    --output vbmeta.img"

  for binary in $binaries ; do
    imgExt=${binary#*.}
    if [[ ${binary} == "sboot.bin" ]] ; then
      imgName="bootloader"
    elif [[ ${binary} == "dt.img" ]] ; then
      imgName="dtb"
    else
      imgName=${binary%%*.$imgExt}
    fi
    echo $binary $imgExt $imgName
    if [[ ! -f $binary ]] ; then
      if [[ $imgName == "product" || $imgName == "odm" ]] ; then
        echo "product/odm image always have chain partition descriptor"
        gen_vbmeta_cmd="$gen_vbmeta_cmd --chain_partition $imgName:$rbindexlocation:pubkey_for_chain.pem"
      else
        is_ready=false
        echo "Cannot found $binary on this path"
      fi
    else
      if [[ $imgName == "recovery" || $imgName == "product" || $imgName == "odm" || $imgName == "dtbo" || $imgName == "dtb" ]] ; then
        echo "recovery, product/odm & dtbo(dt) image always have chain partition descriptor"
        gen_vbmeta_cmd="$gen_vbmeta_cmd --chain_partition $imgName:$rbindexlocation:pubkey_for_chain.pem"
      else
        gen_vbmeta_cmd="$gen_vbmeta_cmd --include_descriptors_from_image $binary"
      fi
      rbindexlocation=$((rbindexlocation+1))
    fi
  done

  if [[ $is_ready == true ]]; then
    echo "gen release vbmeta is ready"
    echo $gen_vbmeta_cmd
    $gen_vbmeta_cmd
    $SEC_BUILD_CONF_SIGNER_ROOT_PATH/make_signerv2_header.sh "vbmeta.img" "bin"
    binary_info=$SEC_BUILD_CONF_SIGNER_ROOT_PATH/signerheader.bin
    chmod 777 "vbmeta.img"
    cat ${binary_info} >> "vbmeta.img"
  else
    echo "gen release vbmeta is not ready so will use test vbmeta"
    cp debug_vbmeta/vbmeta.img ./vbmeta.img
  fi


  if [[ "${chip__dash__vendor}" == "slsi" ]]; then
    if [ "${new__dash__signserver}" == "true" ] ; then
      if [[ "$type" == "ss_exynos40_all" ]] ; then
        echo "add dummy signature"
        dd if=/dev/zero of=sig_dummy.bin bs=272 count=1
        cat sig_dummy.bin >> "vbmeta.img"
        cat sig_dummy.bin >> "./debug_vbmeta/vbmeta.img"
      elif [[ "$type" == "ss_exynos50_all" ]] ; then
        echo "add dummy signature"
        dd if=/dev/zero of=sig_dummy.bin bs=528 count=1
        cat sig_dummy.bin >> "vbmeta.img"
        cat sig_dummy.bin >> "./debug_vbmeta/vbmeta.img"
      fi
      sign_new_server ${top__dash__dir}/buildscript/tools/signclient.jar $type $project "vbmeta.img"
      mv -f vbmeta.img unsigned_vbmeta.img
      mv -f signed_vbmeta.img vbmeta.img
      cd debug_vbmeta
      sign_new_server ${top__dash__dir}/buildscript/tools/signclient.jar $type $project "vbmeta.img"
      mv -f vbmeta.img unsigned_vbmeta.img
      mv -f signed_vbmeta.img vbmeta.img
      pwd
      tar cvf vbmeta.tar vbmeta.img
      if [[ "${package__dash__odin__colon___meta_ver}" == "1" ]]; then
        chmod a+x ${top__dash__dir}/buildscript/tools/odinMeta
        echo "${top__dash__dir}/buildscript/tools/odinMeta 1716 vbmeta.tar ./1.mf ${project%%%%_*}"
        ${top__dash__dir}/buildscript/tools/odinMeta 1716 vbmeta.tar ./1.mf ${project%%%%_*}
        rm -v meta-data -rf
        mkdir -pv meta-data
        mv -v 1.mf meta-data/
        rm -v vbmeta.tar
        tar cfv vbmeta.tar vbmeta.img meta-data/
      fi
    fi
  elif [[ "${chip__dash__vendor}" == "qcom" ]]; then
    if [[ "$type" == "secsecureboot_sha256" ]] ; then
      echo "Do QC download signing"
      sign_new_server ${top__dash__dir}/buildscript/tools/signclient.jar $type $project "vbmeta.img"
      mv -f vbmeta.img unsigned_vbmeta.img
      mv -f signed_vbmeta.img vbmeta.img
      cd debug_vbmeta
      sign_new_server ${top__dash__dir}/buildscript/tools/signclient.jar $type $project "vbmeta.img"
      mv -f vbmeta.img unsigned_vbmeta.img
      mv -f signed_vbmeta.img vbmeta.img
      pwd
      tar cvf vbmeta.tar vbmeta.img
      if [[ "${package__dash__odin__colon___meta_ver}" == "1" ]]; then
        chmod a+x ${top__dash__dir}/buildscript/tools/odinMeta
        echo "${top__dash__dir}/buildscript/tools/odinMeta 1716 vbmeta.tar ./1.mf ${project%%%%_*}"
        ${top__dash__dir}/buildscript/tools/odinMeta 1716 vbmeta.tar ./1.mf ${project%%%%_*}
        rm -v meta-data -rf
        mkdir -pv meta-data
        mv -v 1.mf meta-data/
        rm -v vbmeta.tar
        tar cfv vbmeta.tar vbmeta.img meta-data/
      fi
    else
      echo "signing Type is wrong"
    fi
  fi

  popd > /dev/null
}

function ksign {

  signimgs "boot.img"
}

function dtbsign {
  
  signimgs "dt.img dtbo.img"
}

function signpit {
  
  signimgs "${pit__dash__info__colon__pit__dash__default}"
}

function signall {
  
  signimgs "${sign__dash__sec__dash__secureboot__colon__kernel__colon__binary}"
}

function kpackage {

  pushd ${android__dash__product__dash__out__dash__dir} > /dev/null

  info "Packing pack-Odin binary KERNEL : boot.img recovery.img dt.img dtbo.img vbmeta.img..."
  metadir=$(get_odin_metadir KERNEL)
  if [[ ! -z $metadir ]]; then
      rm -rf $metadir/*
  fi
  if [[ "KERNEL" == "AP" || "KERNEL" == "CSC-"* ]]; then
      gen_fota_zip KERNEL
  fi
  if [[ "KERNEL" == "ALL"* ]]; then
      if [[ "yes" == "yes" && "true" == "true" ]]; then
          gen_hash_val KERNEL
      fi
  fi
  pack_odin KERNEL ${package__dash__KERNEL__dash__full_name}.tar "boot.img recovery.img dt.img dtbo.img vbmeta.img"
  cp -f ${package__dash__KERNEL__dash__full_name}.tar $BUILD_SCRIPT_DIR/../output

  popd > /dev/null
}

function distcleanbuild {

  kclean
  kdistclean
  kconfig
  kbuild
  kimg
  kdtb
  ksign
  dtbsign
  signvbmeta
  kpackage
}

function cleanbuild {

  kclean
  kbuild
  kimg
  ksign
  signvbmeta
  kpackage
}

function kbi {

  kbuild
  kimg
  ksign
  signvbmeta
  kpackage
}

function kapply {
  
  # Dependencies
  #   ${clean__dash__kernel_source}: original kernel source from synced sources for a given release
  #   ${kernel__dash__udpate__dash__repo}: global kernel repository for a given architecture (i.e. android_kernel_exynos)
  #   ${kernel__dash__udpate__dash__init__dash__tag}: initial tag for global kernel repository source sync for this product
  #   ${verbose__dash__flag}: enable/disable verbosity during rsync

  # List of branches to apply passed through function parameters
  UPDATE_DEV_BRANCH_LIST=($@)
  
  pushd . > /dev/null

  if [ "${verbose__dash__flag}" = " -v" ]; then
    rsync -av --delete ${clean__dash__kernel_source}/ ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/ || { exit 2; }
  else
    rsync -a --delete ${clean__dash__kernel_source}/ ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/ || { exit 2; }
  fi

  rm -rf ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820__
  mv ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820 ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820__
  cd ${kernel__dash__udpate__dash__repo}
  git checkout master
  git pull --all || { exit 2; }
  if [ "${verbose__dash__flag}" = " -v" ]; then
    rsync -av --delete ${kernel__dash__udpate__dash__repo}/ ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/ || { exit 2; }
  else
    rsync -a --delete ${kernel__dash__udpate__dash__repo}/ ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/ || { exit 2; }
  fi
  cd ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820
  git checkout ${kernel__dash__udpate__dash__init__dash__tag}
  git checkout -b build
  if [ "${verbose__dash__flag}" = " -v" ]; then
    rsync -av --delete ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820__/ ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/ --exclude '.git' || { exit 2; }
  else
    rsync -a --delete ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820__/ ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820/ --exclude '.git' || { exit 2; }
  fi
  git add -A
  git commit -m 'upstream sync' --allow-empty || { exit 2; }
  for UPDATE_DEV_BRANCH in '${UPDATE_DEV_BRANCH_LIST}'; do
    echo 'Merging ${UPDATE_DEV_BRANCH}...'
    if [ "${UPDATE_DEV_BRANCH}" != "master" ]; then
        git checkout -b ${UPDATE_DEV_BRANCH} origin/${UPDATE_DEV_BRANCH}
        git checkout build
      fi
    git merge ${UPDATE_DEV_BRANCH} --no-commit || { exit 2; }
    git commit -m "Merged branch ${UPDATE_DEV_BRANCH} to build branch" --allow-empty || { exit 2; }
  done
  rm -rf .git
  rm -rf ${PRODUCT_ROOT_DIRECTORY}/android/kernel/exynos9820__

  popd > /dev/null

}
