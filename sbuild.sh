#!/bin/bash

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

temp_export_common_kernel_variable

function kbuild {

	pushd . > /dev/null
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

function signall {
  
  pushd ${android__dash__product__dash__out__dash__dir} > /dev/null
  
  type=${sign__dash__sec__dash__secureboot__colon__kernel__colon__type}
  project=${sign__dash__sec__dash__secureboot__colon__kernel__colon__project}
  binaries="${sign__dash__sec__dash__secureboot__colon__kernel__colon__binary}"
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

  # For PIT Signing
  if [ "${pit__dash__info__colon__pit__dash__default}" != "" ] ; then
    binaries="$binaries ${pit__dash__info__colon__pit__dash__default}"
  fi

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

function ksign {
  
  pushd ${android__dash__product__dash__out__dash__dir} > /dev/null
  
  type=${sign__dash__sec__dash__secureboot__colon__kernel__colon__type}
  project=${sign__dash__sec__dash__secureboot__colon__kernel__colon__project}
  binaries="boot.img"
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

function signpit {
  
  pushd ${android__dash__product__dash__out__dash__dir} > /dev/null
  
  type=${sign__dash__sec__dash__secureboot__colon__kernel__colon__type}
  project=${sign__dash__sec__dash__secureboot__colon__kernel__colon__project}
  binaries=""
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

  # For PIT Signing
  if [ "${pit__dash__info__colon__pit__dash__default}" != "" ] ; then
    binaries="$binaries ${pit__dash__info__colon__pit__dash__default}"
  fi

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
  pack_odin KERNEL KERNEL_G970FXXU2ASF2_CL16215970_REV01_eng_mid_noship.tar "boot.img recovery.img dt.img dtbo.img vbmeta.img"
  cp -f KERNEL_G970FXXU2ASF2_CL16215970_REV01_eng_mid_noship.tar $BUILD_SCRIPT_DIR/../output

  popd > /dev/null  
}

function distcleanbuild {

  kclean
  kdistclean
  kconfig
  kbuild
  kimg
  ksign
}

function cleanbuild {

  kclean
  kbuild
  kimg
  ksign
}

function kbi {

  kbuild
  kimg
  ksign
}
