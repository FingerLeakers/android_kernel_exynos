/* labo_common.h
 *
 * Driver to support LABO APK
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 * Sangsu Ha <sangsu.ha@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sec_class.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/workqueue.h>
#include <linux/rtc.h>
#include <linux/sched/clock.h>

#define SYSFS_LEN 300
#define LABO_WQ_WAIT_TIMEOUT 10000
#define LABO_CMD_PARAM_MAX 3

// #define LABO_DEBUG

struct labo_data {
	struct device *dev;
	struct workqueue_struct *workqueue;
	struct work_struct work;
	char sysfs_path[SYSFS_LEN];
	char sysfs_value[SYSFS_LEN];
	int write_flag;
	struct completion wq_done;
};
