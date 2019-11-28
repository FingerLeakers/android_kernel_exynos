/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS MOBILE SCHEDULER
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_MOBILE_SCHEDULER
#define __EXYNOS_MOBILE_SCHEDULER

/* SCHED CLASS */
#define EMS_SCHED_STOP		(0)
#define EMS_SCHED_DL		(1)
#define EMS_SCHED_RT		(2)
#define EMS_SCHED_FAIR		(3)
#define EMS_SCHED_IDLE		(4)
#define NUM_OF_SCHED_CLASS	(5)

#define EMS_SCHED_CLASS_MASK	(0x1F)

#endif /* __EXYNOS_MOBILE_SCHEDULER__ */
