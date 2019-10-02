// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series dsp driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 */

#include <linux/platform_device.h>
#include <linux/io.h>

#include "dsp-log.h"
#include "dsp-device.h"
#include "dsp-binary.h"
#include "hardware/dsp-system.h"

#define DSP_WAIT_BOOT_TIME	(100)

int dsp_system_execute_task(struct dsp_system *sys, struct dsp_task *task)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

void dsp_system_iovmm_fault_dump(struct dsp_system *sys)
{
	dsp_enter();
	dsp_ctrl_dump();
	dsp_leave();
}

static int __dsp_system_wait_boot(struct dsp_system *sys)
{
	int ret;
	long timeout;

	dsp_enter();
	timeout = wait_event_timeout(sys->boot_wq, sys->boot_flag,
			msecs_to_jiffies(DSP_WAIT_BOOT_TIME));
	if (!timeout) {
		ret = -ETIMEDOUT;
		dsp_err("Failed to boot DSP\n");
		dsp_ctrl_dump();
		goto p_err;
	} else {
		dsp_info("Completed to boot DSP\n");
	}

	dsp_leave();
	return 0;
p_err:
	return ret;
}

int dsp_system_boot(struct dsp_system *sys)
{
	int ret;

	dsp_enter();
	sys->boot_flag = false;
	dsp_ctrl_all_init(&sys->ctrl);
	dsp_ctrl_start(&sys->ctrl);
	ret = __dsp_system_wait_boot(sys);
	if (ret)
		goto p_err_boot;

	dsp_leave();
	return 0;
p_err_boot:
	return ret;
}

int dsp_system_reset(struct dsp_system *sys)
{
	dsp_enter();
	dsp_ctrl_reset(&sys->ctrl);
	dsp_leave();
	return 0;
}

int dsp_system_power_active(struct dsp_system *sys)
{
	dsp_check();
	return dsp_pm_devfreq_active(&sys->pm);
}

int dsp_system_runtime_resume(struct dsp_system *sys)
{
	int ret;

	dsp_enter();
	ret = dsp_pm_enable(&sys->pm);
	if (ret)
		goto p_err_pm;

	ret = dsp_clk_enable(&sys->clk);
	if (ret)
		goto p_err_clk;

	dsp_leave();
	return 0;
p_err_clk:
	dsp_pm_disable(&sys->pm);
p_err_pm:
	return ret;
}

int dsp_system_runtime_suspend(struct dsp_system *sys)
{
	dsp_enter();
	dsp_clk_disable(&sys->clk);
	dsp_pm_disable(&sys->pm);
	dsp_leave();
	return 0;
}

int dsp_system_resume(struct dsp_system *sys)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

int dsp_system_suspend(struct dsp_system *sys)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

int dsp_system_npu_start(struct dsp_system *sys, bool boot, dma_addr_t fw_iova)
{
	return 0;
}

int dsp_system_start(struct dsp_system *sys)
{
	int ret;
	struct dsp_memory *mem;

	dsp_enter();
	mem = &sys->memory;

	ret = dsp_binary_load("pm_ca5.bin",
			mem->priv_mem[DSP_PRIV_MEM_FW].kvaddr,
			mem->priv_mem[DSP_PRIV_MEM_FW].size);
	if (ret < 0)
		goto p_err_load;

	dsp_leave();
	return 0;
p_err_load:
	return ret;
}

int dsp_system_stop(struct dsp_system *sys)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

int dsp_system_open(struct dsp_system *sys)
{
	int ret;

	dsp_enter();
	ret = dsp_pm_open(&sys->pm);
	if (ret)
		goto p_err_pm;

	ret = dsp_clk_open(&sys->clk);
	if (ret)
		goto p_err_clk;

	ret = dsp_memory_open(&sys->memory);
	if (ret)
		goto p_err_memory;

	ret = dsp_interface_open(&sys->interface);
	if (ret)
		goto p_err_interface;

	ret = dsp_ctrl_open(&sys->ctrl);
	if (ret)
		goto p_err_ctrl;

	ret = dsp_hw_debug_open(&sys->debug);
	if (ret)
		goto p_err_hw_debug;

	dsp_leave();
	return 0;
p_err_hw_debug:
	dsp_ctrl_close(&sys->ctrl);
p_err_ctrl:
	dsp_interface_close(&sys->interface);
p_err_interface:
	dsp_memory_close(&sys->memory);
p_err_memory:
	dsp_clk_close(&sys->clk);
p_err_clk:
	dsp_pm_close(&sys->pm);
p_err_pm:
	return ret;
}

int dsp_system_close(struct dsp_system *sys)
{
	dsp_enter();
	dsp_hw_debug_close(&sys->debug);
	dsp_ctrl_close(&sys->ctrl);
	dsp_interface_close(&sys->interface);
	dsp_memory_close(&sys->memory);
	dsp_clk_close(&sys->clk);
	dsp_pm_close(&sys->pm);
	dsp_leave();
	return 0;
}

int dsp_system_probe(struct dsp_device *dspdev)
{
	int ret;
	struct dsp_system *sys;
	struct platform_device *pdev;
	struct resource *res;
	void __iomem *regs;

	dsp_enter();
	sys = &dspdev->system;
	sys->dev = dspdev->dev;
	pdev = to_platform_device(sys->dev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -EINVAL;
		dsp_err("Failed to get resource\n");
		goto p_err_resource;
	}

	regs = devm_ioremap_resource(sys->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		dsp_err("Failed to remap resource(%d)\n", ret);
		goto p_err_ioremap;
	}

	sys->sfr_pa = res->start;
	sys->sfr = regs;
	sys->sfr_size = resource_size(res);
	init_waitqueue_head(&sys->boot_wq);

	ret = dsp_pm_probe(sys);
	if (ret)
		goto p_err_pm;

	ret = dsp_clk_probe(sys);
	if (ret)
		goto p_err_clk;

	ret = dsp_memory_probe(sys);
	if (ret)
		goto p_err_memory;

	ret = dsp_interface_probe(sys);
	if (ret)
		goto p_err_interface;

	ret = dsp_ctrl_probe(sys);
	if (ret)
		goto p_err_ctrl;

	ret = dsp_task_manager_probe(sys);
	if (ret)
		goto p_err_task;

	ret = dsp_hw_debug_probe(dspdev);
	if (ret)
		goto p_err_hw_debug;

	dsp_leave();
	return 0;
p_err_hw_debug:
	dsp_task_manager_remove(&sys->task_manager);
p_err_task:
	dsp_ctrl_remove(&sys->ctrl);
p_err_ctrl:
	dsp_interface_remove(&sys->interface);
p_err_interface:
	dsp_memory_remove(&sys->memory);
p_err_memory:
	dsp_clk_remove(&sys->clk);
p_err_clk:
	dsp_pm_remove(&sys->pm);
p_err_pm:
	devm_iounmap(sys->dev, sys->sfr);
p_err_ioremap:
p_err_resource:
	return ret;
}

void dsp_system_remove(struct dsp_system *sys)
{
	dsp_enter();
	dsp_hw_debug_remove(&sys->debug);
	dsp_task_manager_remove(&sys->task_manager);
	dsp_ctrl_remove(&sys->ctrl);
	dsp_interface_remove(&sys->interface);
	dsp_memory_remove(&sys->memory);
	dsp_clk_remove(&sys->clk);
	dsp_pm_remove(&sys->pm);
	devm_iounmap(sys->dev, sys->sfr);
	dsp_leave();
}
