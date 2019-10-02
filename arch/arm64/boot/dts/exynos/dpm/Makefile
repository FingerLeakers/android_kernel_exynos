# SPDX-License-Identifier: GPL-2.0
#dpm-$(CONFIG_ARCH_EXYNOS) := \
	exynos-dpm.dtbo

always	:= $(dpm-y)

targets += dpms

DPM_LIST := $(dpm-y)

dpms: $(addprefix $(obj)/, $(DPM_LIST))

clean-files := *.dtb*
