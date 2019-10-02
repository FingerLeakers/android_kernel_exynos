/*
 * Samsung Exynos SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_CIS_2LD_H
#define IS_CIS_2LD_H

#include "is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_2LD_MAX_WIDTH		(4032 + 0)
#define SENSOR_2LD_MAX_HEIGHT		(3024 + 0)

#define SENSOR_2LD_FINE_INTEGRATION_TIME_MIN                0x100
#define SENSOR_2LD_FINE_INTEGRATION_TIME_MAX                0x100
#define SENSOR_2LD_COARSE_INTEGRATION_TIME_MIN              0x10
#define SENSOR_2LD_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x24

#define USE_GROUP_PARAM_HOLD	(0)

enum sensor_2ld_mode_enum {
	/* MODE 3 */
	SENSOR_2LD_4032X3024_30FPS = 0,
	SENSOR_2LD_4032X2268_60FPS = 1,
	SENSOR_2LD_4032X2268_30FPS = 2,
	SENSOR_2LD_2016X1134_30FPS = 3,
	/* MODE 3 - 24fps LIVE FOCUS */
	SENSOR_2LD_4032X3024_24FPS = 4,
	SENSOR_2LD_4032X2268_24FPS = 5,
	/* MODE 3 - SM */
	SENSOR_2LD_2016X1134_480FPS = 6,
	SENSOR_2LD_2016X1134_240FPS = 7,
	/* MODE 2 */
	SENSOR_2LD_1008X756_120FPS_MODE2 = 8,
	/* MODE 2 SSM */
	SENSOR_2LD_2016X1134_60FPS_MODE2_SSM_960 = 9,
	SENSOR_2LD_2016X1134_60FPS_MODE2_SSM_480 = 10,
	SENSOR_2LD_1280X720_60FPS_MODE2_SSM_960 = 11,
	SENSOR_2LD_1280X720_60FPS_MODE2_SSM_480 = 12,
	SENSOR_2LD_MODE_MAX,
};

static bool sensor_2ld_support_wdr[] = {
	/* MODE 3 */
	true, //SENSOR_2LD_4032X3024_30FPS = 0,
	true, //SENSOR_2LD_4032X2268_60FPS = 1,
	true, //SENSOR_2LD_4032X2268_30FPS = 2,
	false, //SENSOR_2LD_2016X1134_30FPS = 3,
	/* MODE 3 - 24fps LIVE FOCUS */
	true, //SENSOR_2LD_4032X3024_24FPS = 4,
	true, //SENSOR_2LD_4032X2268_24FPS = 5,
	/* MODE 3 - SM */
	false, //SENSOR_2LD_2016X1134_480FPS = 6,
	false, //SENSOR_2LD_2016X1134_240FPS = 7,
	/* MODE 2 */
	false, //SENSOR_2LD_1008X756_120FPS_MODE2 = 8,
	/* MODE 2 SSM */
	false, //SENSOR_2LD_2016X1134_60FPS_MODE2_SSM_960 = 9,
	false, //SENSOR_2LD_2016X1134_60FPS_MODE2_SSM_480 = 10,
	false, //SENSOR_2LD_1280X720_60FPS_MODE2_SSM_960 = 11,
	false, //SENSOR_2LD_1280X720_60FPS_MODE2_SSM_480 = 12,
};

enum sensor_2ld_load_sram_mode {
	SENSOR_2LD_4032x3024_30FPS_LOAD_SRAM = 0,
	SENSOR_2LD_4032x2268_30FPS_LOAD_SRAM,
//	SENSOR_2LD_4032x3024_24FPS_LOAD_SRAM,
//	SENSOR_2LD_4032x2268_24FPS_LOAD_SRAM,
	SENSOR_2LD_4032x2268_60FPS_LOAD_SRAM,
	SENSOR_2LD_1008x756_120FPS_LOAD_SRAM,
};

int sensor_2ld_cis_stream_on(struct v4l2_subdev *subdev);
int sensor_2ld_cis_stream_off(struct v4l2_subdev *subdev);
#ifdef CONFIG_SENSOR_RETENTION_USE
int sensor_2ld_cis_retention_crc_check(struct v4l2_subdev *subdev);
int sensor_2ld_cis_retention_prepare(struct v4l2_subdev *subdev);
#endif
int sensor_2ld_cis_set_lownoise_mode_change(struct v4l2_subdev *subdev);
#endif
