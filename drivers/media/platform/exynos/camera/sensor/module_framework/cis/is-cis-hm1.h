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

#ifndef IS_CIS_HM1_H
#define IS_CIS_HM1_H

#include "is-cis.h"

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
#undef USE_CAMERA_MIPI_CLOCK_VARIATION
#endif

#define EXT_CLK_Mhz (26)

#define SENSOR_HM1_MAX_WIDTH		(12000 + 0)
#define SENSOR_HM1_MAX_HEIGHT		(9000 + 0)

#define SENSOR_HM1_FINE_INTEGRATION_TIME_MIN                0x100
#define SENSOR_HM1_FINE_INTEGRATION_TIME_MAX                0x100
#define SENSOR_HM1_COARSE_INTEGRATION_TIME_MIN              0x10
#define SENSOR_HM1_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x24

#define USE_GROUP_PARAM_HOLD	(0)

enum sensor_hm1_mode_enum {
	SENSOR_HM1_12000X9000_10FPS = 0,
	SENSOR_HM1_4000X3000_30FPS = 1,
	SENSOR_HM1_1984X1488_30FPS = 2,
	SENSOR_HM1_4000X3000_60FPS = 3,
#if 0 // TEMP_2020
	SENSOR_HM1_4032X3024_24FPS = 4,
	SENSOR_HM1_4032X2268_24FPS = 5,
	SENSOR_HM1_2016X1134_480FPS = 6,
	SENSOR_HM1_2016X1134_240FPS = 7,
	SENSOR_HM1_1008X756_120FPS_MODE2 = 8,
#endif
	SENSOR_HM1_MODE_MAX,
};

static bool sensor_hm1_support_wdr[] = {
	/* MODE 3 */
	false, //SENSOR_HM1_12000X9000_10FPS = 0,
	false, //SENSOR_HM1_4000X3000_30FPS = 1,
	false, //SENSOR_HM1_1984X1488_30FPS = 2,
	false, //SENSOR_HM1_4000X3000_60FPS = 3,
#if 0 // TEMP_2020
	true, //SENSOR_HM1_4032X3024_24FPS = 4,
	true, //SENSOR_HM1_4032X2268_24FPS = 5,
	false, //SENSOR_HM1_2016X1134_480FPS = 6,
	false, //SENSOR_HM1_2016X1134_240FPS = 7,
	false, //SENSOR_HM1_1008X756_120FPS_MODE2 = 8,
#endif
};

enum sensor_hm1_load_sram_mode {
	SENSOR_HM1_4032x3024_30FPS_LOAD_SRAM = 0,
	SENSOR_HM1_4032x2268_30FPS_LOAD_SRAM,
//	SENSOR_HM1_4032x3024_24FPS_LOAD_SRAM,
//	SENSOR_HM1_4032x2268_24FPS_LOAD_SRAM,
	SENSOR_HM1_4032x2268_60FPS_LOAD_SRAM,
	SENSOR_HM1_1008x756_120FPS_LOAD_SRAM,
};

int sensor_hm1_cis_stream_on(struct v4l2_subdev *subdev);
int sensor_hm1_cis_stream_off(struct v4l2_subdev *subdev);
#ifdef CONFIG_SENSOR_RETENTION_USE
int sensor_hm1_cis_retention_crc_check(struct v4l2_subdev *subdev);
int sensor_hm1_cis_retention_prepare(struct v4l2_subdev *subdev);
#endif
int sensor_hm1_cis_set_lownoise_mode_change(struct v4l2_subdev *subdev);
#endif
