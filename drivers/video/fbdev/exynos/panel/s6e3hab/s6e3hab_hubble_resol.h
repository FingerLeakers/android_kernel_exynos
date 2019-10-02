/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3hab/s6e3hab_hubble_resol.h
 *
 * Header file for Panel Driver
 *
 * Copyright (c) 2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HAB_HUBBLE_RESOL_H__
#define __S6E3HAB_HUBBLE_RESOL_H__

#include "../panel.h"
static struct panel_vrr s6e3hab_hubble_wqhd_vrrtbl[] = {
	[S6E3HAB_VRR_60_NORMAL] = {
		.fps = 60,
		.mode = VRR_NORMAL_MODE,
	}
};

static struct panel_vrr s6e3hab_hubble_preliminary_vrrtbl[] = {
	[S6E3HAB_VRR_60_NORMAL] = {
		.fps = 60,
		.mode = VRR_NORMAL_MODE,
	},
	[S6E3HAB_VRR_120_HS] = {
		.fps = 120,
		.mode = VRR_HS_MODE,
	},
};

static struct panel_resol s6e3hab_hubble_preliminary_resol[] = {
	{
		.w = 1440,
		.h = 3200,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 720,
				.slice_h = 40,
			},
		},
		.available_vrr = s6e3hab_hubble_preliminary_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(s6e3hab_hubble_preliminary_vrrtbl),
	},
	{
		.w = 1080,
		.h = 2400,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 540,
				.slice_h = 40,
			},
		},
		.available_vrr = s6e3hab_hubble_preliminary_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(s6e3hab_hubble_preliminary_vrrtbl),
	},
	{
		.w = 720,
		.h = 1600,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 360,
				.slice_h = 80,
			},
		},
		.available_vrr = s6e3hab_hubble_preliminary_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(s6e3hab_hubble_preliminary_vrrtbl),
	},
};

static struct panel_vrr s6e3hab_hubble_default_vrrtbl[] = {
	[S6E3HAB_VRR_60_NORMAL] = {
		.fps = 60,
		.mode = VRR_NORMAL_MODE,
	},
	[S6E3HAB_VRR_60_HS] = {
		.fps = 60,
		.mode = VRR_HS_MODE,
	},
	[S6E3HAB_VRR_120_HS] = {
		.fps = 120,
		.mode = VRR_HS_MODE,
	},
};

static struct panel_resol s6e3hab_hubble_default_resol[] = {
	{
		.w = 1440,
		.h = 3200,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 720,
				.slice_h = 40,
			},
		},
		.available_vrr = s6e3hab_hubble_default_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(s6e3hab_hubble_default_vrrtbl),
	},
	{
		.w = 1080,
		.h = 2400,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 540,
				.slice_h = 40,
			},
		},
		.available_vrr = s6e3hab_hubble_default_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(s6e3hab_hubble_default_vrrtbl),
	},
	{
		.w = 720,
		.h = 1600,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 360,
				.slice_h = 80,
			},
		},
		.available_vrr = s6e3hab_hubble_default_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(s6e3hab_hubble_default_vrrtbl),
	},
};
#endif /* __S6E3HAB_HUBBLE_RESOL_H__ */
