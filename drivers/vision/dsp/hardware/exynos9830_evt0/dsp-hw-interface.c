// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series dsp driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 */

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include "dsp-log.h"
#include "hardware/dsp-ctrl.h"
#include "hardware/dsp-system.h"
#include "hardware/dsp-interface.h"

static irqreturn_t dsp_interface_isr0(int irq, void *data)
{
	struct dsp_interface *itf;
	struct dsp_system *sys;
	unsigned int status;

	dsp_enter();
	itf = (struct dsp_interface *)data;
	sys = itf->sys;

	status = dsp_ctrl_readl(DSPC_TO_HOST_MAILBOX_NS_INTR);
	//TODO temp log
	dsp_info("isr0 %#x\n", status);

	dsp_ctrl_writel(DSPC_TO_HOST_MAILBOX_NS_INTR, 0x0);
	if (!sys->boot_flag) {
		sys->boot_flag = true;
		wake_up(&sys->boot_wq);
	}

	dsp_leave();
	return IRQ_HANDLED;
}

static irqreturn_t dsp_interface_isr1(int irq, void *data)
{
	//struct dsp_interface *itf = (struct dsp_interface *)data;

	dsp_enter();
	dsp_info("isr1\n");
	dsp_leave();
	return IRQ_HANDLED;
}

static irqreturn_t dsp_interface_isr2(int irq, void *data)
{
	//struct dsp_interface *itf = (struct dsp_interface *)data;

	dsp_enter();
	dsp_info("isr2\n");
	dsp_leave();
	return IRQ_HANDLED;
}

static irqreturn_t dsp_interface_isr3(int irq, void *data)
{
	//struct dsp_interface *itf = (struct dsp_interface *)data;

	dsp_enter();
	dsp_info("isr3\n");
	dsp_leave();
	return IRQ_HANDLED;
}

int dsp_interface_open(struct dsp_interface *itf)
{
	int ret;

	dsp_enter();
	ret = devm_request_irq(itf->sys->dev, itf->irq0, dsp_interface_isr0, 0,
			dev_name(itf->sys->dev), itf);
	if (ret) {
		dsp_err("Failed to request irq0(%d)\n", ret);
		goto p_err_req_irq0;
	}

	ret = devm_request_irq(itf->sys->dev, itf->irq1, dsp_interface_isr1, 0,
			dev_name(itf->sys->dev), itf);
	if (ret) {
		dsp_err("Failed to request irq1(%d)\n", ret);
		goto p_err_req_irq1;
	}

	ret = devm_request_irq(itf->sys->dev, itf->irq2, dsp_interface_isr2, 0,
			dev_name(itf->sys->dev), itf);
	if (ret) {
		dsp_err("Failed to request irq2(%d)\n", ret);
		goto p_err_req_irq2;
	}

	ret = devm_request_irq(itf->sys->dev, itf->irq3, dsp_interface_isr3, 0,
			dev_name(itf->sys->dev), itf);
	if (ret) {
		dsp_err("Failed to request irq3(%d)\n", ret);
		goto p_err_req_irq3;
	}

	dsp_leave();
	return 0;
p_err_req_irq3:
	devm_free_irq(itf->sys->dev, itf->irq2, itf);
p_err_req_irq2:
	devm_free_irq(itf->sys->dev, itf->irq1, itf);
p_err_req_irq1:
	devm_free_irq(itf->sys->dev, itf->irq0, itf);
p_err_req_irq0:
	return ret;
}

int dsp_interface_close(struct dsp_interface *itf)
{
	dsp_enter();
	devm_free_irq(itf->sys->dev, itf->irq3, itf);
	devm_free_irq(itf->sys->dev, itf->irq2, itf);
	devm_free_irq(itf->sys->dev, itf->irq1, itf);
	devm_free_irq(itf->sys->dev, itf->irq0, itf);
	dsp_leave();
	return 0;
}

int dsp_interface_probe(struct dsp_system *sys)
{
	int ret;
	struct dsp_interface *itf;
	struct platform_device *pdev;
	int irq;

	dsp_enter();
	itf = &sys->interface;
	itf->sys = sys;
	itf->sfr = sys->sfr;
	pdev = to_platform_device(sys->dev);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		dsp_err("Failed to get irq0(%d)", ret);
		goto p_err_get_irq0;
	}
	itf->irq0 = irq;

	irq = platform_get_irq(pdev, 1);
	if (irq < 0) {
		ret = irq;
		dsp_err("Failed to get irq1(%d)", ret);
		goto p_err_get_irq1;
	}
	itf->irq1 = irq;

	irq = platform_get_irq(pdev, 2);
	if (irq < 0) {
		ret = irq;
		dsp_err("Failed to get irq2(%d)", ret);
		goto p_err_get_irq2;
	}
	itf->irq2 = irq;

	irq = platform_get_irq(pdev, 3);
	if (irq < 0) {
		ret = irq;
		dsp_err("Failed to get irq3(%d)", ret);
		goto p_err_get_irq3;
	}
	itf->irq3 = irq;

	dsp_leave();
	return 0;
p_err_get_irq3:
p_err_get_irq2:
p_err_get_irq1:
p_err_get_irq0:
	return ret;
}

void dsp_interface_remove(struct dsp_interface *itf)
{
	dsp_enter();
	dsp_leave();
}
