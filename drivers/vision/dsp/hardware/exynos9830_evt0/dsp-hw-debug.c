// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series dsp driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 */

#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/syscalls.h>

#include "dsp-log.h"
#include "dsp-device.h"
#include "hardware/dsp-system.h"
#include "hardware/dsp-ctrl.h"
#include "hardware/dsp-dump.h"
#include "dsp-binary.h"
#include "dsp-debug.h"
#include "hardware/dsp-debug.h"

static int dsp_hw_debug_power_show(struct seq_file *file, void *unused)
{
	struct dsp_hw_debug *debug;
	struct dsp_device *dspdev;

	dsp_enter();
	debug = file->private;
	dspdev = debug->dspdev;

	mutex_lock(&dspdev->lock);
	if (dsp_device_power_active(dspdev))
		seq_puts(file, "DSP power on\n");
	else
		seq_puts(file, "DSP power off\n");

	seq_printf(file, "open count %u / start count %u\n",
			dspdev->open_count, dspdev->start_count);
	mutex_unlock(&dspdev->lock);

	dsp_leave();
	return 0;
}

static int dsp_hw_debug_power_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, dsp_hw_debug_power_show, inode->i_private);
}

static ssize_t dsp_hw_debug_power_write(struct file *filp,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	int ret;
	struct seq_file *file;
	struct dsp_hw_debug *debug;
	char command[10];
	ssize_t size;

	dsp_enter();
	file = filp->private_data;
	debug = file->private;

	size = simple_write_to_buffer(command, sizeof(command) - 1, ppos,
			user_buf, count);
	if (size < 0) {
		ret = size;
		dsp_err("Failed to get user parameter(%d)\n", ret);
		goto p_err;
	}

	command[size] = '\0';
	if (sysfs_streq(command, "open")) {
		ret = dsp_device_open(debug->dspdev);
		if (ret)
			goto p_err;
	} else if (sysfs_streq(command, "close")) {
		dsp_device_close(debug->dspdev);
	} else if (sysfs_streq(command, "start")) {
		ret = dsp_device_start(debug->dspdev, 0);
		if (ret)
			goto p_err;
	} else if (sysfs_streq(command, "stop")) {
		dsp_device_stop(debug->dspdev, 1);
	} else {
		ret = -EINVAL;
		dsp_err("power command[%s] of debugfs is invalid\n", command);
		goto p_err;
	}

	dsp_leave();
	return count;
p_err:
	return ret;
}

static const struct file_operations dsp_hw_debug_power_fops = {
	.open		= dsp_hw_debug_power_open,
	.read		= seq_read,
	.write		= dsp_hw_debug_power_write,
	.llseek		= seq_lseek,
	.release	= single_release
};

static int dsp_hw_debug_clk_show(struct seq_file *file, void *unused)
{
	struct dsp_hw_debug *debug;
	struct dsp_device *dspdev;

	dsp_enter();
	debug = file->private;
	dspdev = debug->dspdev;

	mutex_lock(&dspdev->lock);
	if (dsp_device_power_active(dspdev))
		dsp_clk_user_dump(&dspdev->system.clk, file);
	else
		seq_puts(file, "power off\n");
	mutex_unlock(&dspdev->lock);

	dsp_leave();
	return 0;
}

static int dsp_hw_debug_clk_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, dsp_hw_debug_clk_show, inode->i_private);
}

static const struct file_operations dsp_hw_debug_clk_fops = {
	.open		= dsp_hw_debug_clk_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};

static int dsp_hw_debug_devfreq_show(struct seq_file *file, void *unused)
{
	struct dsp_hw_debug *debug;
	struct dsp_pm *pm;
	struct dsp_pm_devfreq *devfreq;
	int count, idx;

	dsp_enter();
	debug = file->private;
	pm = &debug->dspdev->system.pm;

	mutex_lock(&pm->lock);

	for (count = 0; count < DSP_DEVFREQ_COUNT; ++count) {
		devfreq = &pm->devfreq[count];
		seq_printf(file, "[%s] id : %d\n", devfreq->name, count);
		seq_printf(file, "[%s] available level count [0 - %u]\n",
				devfreq->name, devfreq->count - 1);
		for (idx = 0; idx < devfreq->count; ++idx)
			seq_printf(file, "[%s] [L%u] %u\n",
					devfreq->name, idx,
					devfreq->table[idx]);

		if (devfreq->default_qos < 0)
			seq_printf(file, "[%s] default: not set\n",
					devfreq->name);
		else
			seq_printf(file, "[%s] default: L%u\n",
					devfreq->name, devfreq->default_qos);

		if (devfreq->resume_qos < 0)
			seq_printf(file, "[%s] resume: not set\n",
					devfreq->name);
		else
			seq_printf(file, "[%s] resume: L%u\n",
					devfreq->name, devfreq->resume_qos);

		if (devfreq->current_qos < 0)
			seq_printf(file, "[%s] current: off\n",
					devfreq->name);
		else
			seq_printf(file, "[%s] current: L%u\n",
					devfreq->name, devfreq->current_qos);
	}

	seq_puts(file, "Command to change devfreq level\n");
	seq_puts(file, " echo {id} {level} > /d/dsp/hardware/devfreq\n");
	seq_puts(file, " (If power is on, the motion freq is changed,\n");
	seq_puts(file, "  if it is off, the default freq is changed\n");

	mutex_unlock(&pm->lock);
	dsp_leave();
	return 0;
}

static int dsp_hw_debug_devfreq_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, dsp_hw_debug_devfreq_show, inode->i_private);
}

static ssize_t dsp_hw_debug_devfreq_write(struct file *filp,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	int ret;
	struct seq_file *file;
	struct dsp_hw_debug *debug;
	struct dsp_pm *pm;
	char buf[30];
	ssize_t size;
	unsigned int id, level;

	dsp_enter();
	file = filp->private_data;
	debug = file->private;
	pm = &debug->dspdev->system.pm;

	size = simple_write_to_buffer(buf, sizeof(buf) - 1, ppos, user_buf,
			count);
	if (size < 0) {
		ret = size;
		dsp_err("Failed to get user parameter(%d)\n", ret);
		goto p_err;
	}
	buf[size] = '\0';

	ret = sscanf(buf, "%u %u", &id, &level);
	if (ret != 2) {
		dsp_err("Failed to get devfreq parameter(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	if (id >= DSP_DEVFREQ_COUNT) {
		ret = -EINVAL;
		dsp_err("devfreq id(%u) of command is invalid(0 ~ %u)\n",
				id, DSP_DEVFREQ_COUNT - 1);
		goto p_err;
	}

	mutex_lock(&pm->lock);

	if (dsp_pm_devfreq_active(pm))
		dsp_pm_update_devfreq_nolock(pm, id, level);
	else
		dsp_pm_set_default_devfreq_nolock(pm, id, level);

	mutex_unlock(&pm->lock);

	dsp_leave();
	return count;
p_err:
	return ret;
}

static const struct file_operations dsp_hw_debug_devfreq_fops = {
	.open		= dsp_hw_debug_devfreq_open,
	.read		= seq_read,
	.write		= dsp_hw_debug_devfreq_write,
	.llseek		= seq_lseek,
	.release	= single_release
};

static int dsp_hw_debug_sfr_show(struct seq_file *file, void *unused)
{
	int ret;
	struct dsp_hw_debug *debug;

	dsp_enter();
	debug = file->private;

	ret = dsp_device_open(debug->dspdev);
	if (ret)
		goto p_err_open;

	ret = dsp_device_start(debug->dspdev, 0);
	if (ret)
		goto p_err_start;

	dsp_dump_ctrl_user(file);
	dsp_device_stop(debug->dspdev, 1);
	dsp_device_close(debug->dspdev);

	dsp_leave();
	return 0;
p_err_start:
	dsp_device_close(debug->dspdev);
p_err_open:
	return 0;
}

static int dsp_hw_debug_sfr_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, dsp_hw_debug_sfr_show, inode->i_private);
}

static const struct file_operations dsp_hw_debug_sfr_fops = {
	.open		= dsp_hw_debug_sfr_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release
};

static int dsp_hw_debug_mem_show(struct seq_file *file, void *unused)
{
	struct dsp_hw_debug *debug;
	struct dsp_memory *mem;
	int idx;

	dsp_enter();
	debug = file->private;
	mem = &debug->dspdev->system.memory;

	for (idx = 0; idx < DSP_PRIV_MEM_COUNT; ++idx)
		seq_printf(file, "[id:%d][%15s] : %zu KB (%zu KB ~ %zu KB)\n",
				idx, mem->priv_mem[idx].name,
				mem->priv_mem[idx].size / SZ_1K,
				mem->priv_mem[idx].min_size / SZ_1K,
				mem->priv_mem[idx].max_size / SZ_1K);

	seq_puts(file, "Command to change allocated memory size\n");
	seq_puts(file, " echo {mem_id} {size} > /d/dsp/hardware/mem\n");
	seq_puts(file, " (Size must be in KB\n");
	seq_puts(file, "  Command only works when power is off)\n");

	dsp_leave();
	return 0;
}

static int dsp_hw_debug_mem_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, dsp_hw_debug_mem_show, inode->i_private);
}

static ssize_t dsp_hw_debug_mem_write(struct file *filp,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	int ret;
	struct seq_file *file;
	struct dsp_hw_debug *debug;
	struct dsp_device *dspdev;
	struct dsp_memory *mem;
	char buf[128];
	ssize_t len;
	unsigned int id, size;
	struct dsp_priv_mem *pmem;

	dsp_enter();
	file = filp->private_data;
	debug = file->private;
	dspdev = debug->dspdev;
	mem = &dspdev->system.memory;

	if (count > sizeof(buf)) {
		dsp_err("writing size(%zd) is larger than buffer\n", count);
		goto out;
	}

	len = simple_write_to_buffer(buf, sizeof(buf), ppos, user_buf, count);
	if (len <= 0) {
		dsp_err("Failed to get user parameter(%zd)\n", len);
		goto out;
	}

	buf[len] = '\0';

	ret = sscanf(buf, "%u %u\n", &id, &size);
	if (ret != 2) {
		dsp_err("Failed to get command changing memory size(%d)\n",
				ret);
		goto out;
	}

	if (id >= DSP_PRIV_MEM_COUNT) {
		dsp_err("memory id(%u) of command is invalid(0 ~ %u)\n",
				id, DSP_PRIV_MEM_COUNT - 1);
		goto out;
	}

	mutex_lock(&dspdev->lock);
	if (dspdev->open_count) {
		dsp_err("device was already running(%u)\n", dspdev->open_count);
		mutex_unlock(&dspdev->lock);
		goto out;
	}

	pmem = &mem->priv_mem[id];
	size = PAGE_ALIGN(size * SZ_1K);
	if (size >= pmem->min_size && size <= pmem->max_size) {
		dsp_info("size of %s is changed(%zu KB -> %u KB)\n",
				pmem->name, pmem->size / SZ_1K, size / SZ_1K);
		pmem->size = size;
	} else {
		dsp_warn("invalid size %u KB (%s, %zu KB ~ %zu KB)\n",
				size / SZ_1K, pmem->name,
				pmem->min_size / SZ_1K,
				pmem->max_size / SZ_1K);
	}

	mutex_unlock(&dspdev->lock);

	dsp_leave();
out:
	return count;
}

static const struct file_operations dsp_hw_debug_mem_fops = {
	.open		= dsp_hw_debug_mem_open,
	.read		= seq_read,
	.write		= dsp_hw_debug_mem_write,
	.llseek		= seq_lseek,
	.release	= single_release
};

static int dsp_hw_debug_test_show(struct seq_file *file, void *unused)
{
	struct dsp_hw_debug *debug;

	dsp_enter();
	debug = file->private;

	seq_puts(file, "cmd: echo {test_id} {repeat} > /d/dsp/hardware/test\n");
	seq_puts(file, "test1 : ASB test (IVP0)\n");
	seq_puts(file, "test2 : ASB test (IVP1)\n");
	seq_puts(file, "test3 : ASB test (DMA)\n");

	dsp_leave();
	return 0;
}

static int dsp_hw_debug_test_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, dsp_hw_debug_test_show, inode->i_private);
}

static void __dsp_hw_debug_print_userdefined(struct dsp_hw_debug *debug)
{
	int idx;
	struct dsp_system *sys;

	dsp_enter();
	sys = &debug->dspdev->system;
	for (idx = 0; idx < 10; ++idx)
		dsp_info("USERDEFINED[%d] %8x\n", idx,
				readl(sys->sfr + 0x38000 + (idx * 4)));
	dsp_leave();
}

static void __dsp_hw_debug_set_priv_mem(struct dsp_priv_mem *mem,
		const char *name)
{
	dsp_enter();
	snprintf(mem->name, DSP_PRIV_MEM_NAME_LEN, name);
	mem->size = SZ_1M;
	mem->flags = 0;
	mem->dir = DMA_TO_DEVICE;
	mem->kmap = true;
	mem->fixed_iova = false;
	dsp_leave();
}

static int __dsp_hw_debug_ivp_test(struct dsp_hw_debug *debug,
		int test_id, int repeat)
{
	int ret;
	struct dsp_system *sys;
	struct dsp_memory *mem;
	struct dsp_priv_mem pm, dm;
	int idx, repeat_idx;
	unsigned int reg, timeout, result;

	dsp_enter();
	sys = &debug->dspdev->system;
	mem = &sys->memory;

	if (test_id == 1) {
		__dsp_hw_debug_set_priv_mem(&pm, "pm_ivp0.bin");
		__dsp_hw_debug_set_priv_mem(&dm, "dm_ivp0.bin");
	} else if (test_id == 2) {
		__dsp_hw_debug_set_priv_mem(&pm, "pm_ivp1.bin");
		__dsp_hw_debug_set_priv_mem(&dm, "dm_ivp1.bin");
	} else {
		ret = -EFAULT;
		goto p_err;
	}

	ret = dsp_memory_ion_alloc(mem, &pm);
	if (ret)
		goto p_err_alloc_pm;

	ret = dsp_binary_load(pm.name, NULL, NULL, pm.kvaddr, pm.size);
	if (ret < 0)
		goto p_err_load_pm;

	ret = dsp_memory_ion_alloc(mem, &dm);
	if (ret)
		goto p_err_alloc_dm;

	ret = dsp_binary_load(dm.name, NULL, NULL, dm.kvaddr + 0x8000,
			dm.size - 0x8000);
	if (ret < 0)
		goto p_err_load_dm;

	reg = dsp_ctrl_readl(DSPC_HOST_INTR_ENABLE);
	dsp_ctrl_writel(DSPC_HOST_INTR_ENABLE, reg & ~0x1);

	for (repeat_idx = 0; repeat_idx < repeat; ++repeat_idx) {
		/* initialize SM */
		for (idx = 0; idx < 10; ++idx)
			writel(0x0, sys->sfr + 0x38000 + (idx * 4));

		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS0, test_id);
		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS1, pm.iova);
		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS2, dm.iova);
		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS_INTR, 0x103);

		timeout = 1000;
		while (1) {
			reg = dsp_ctrl_readl(DSPC_TO_HOST_MAILBOX_NS_INTR);
			if ((reg >> 8) & 0x1 || !timeout)
				break;
			timeout--;
			udelay(100);
		};

		if (reg & 0x1) {
			result = dsp_ctrl_readl(DSPC_TO_HOST_MAILBOX_NS0);
			if (result != 0x900dc0de) {
				dsp_err("test[%d] fail: %8x\n",
						test_id, result);
				__dsp_hw_debug_print_userdefined(debug);
				dsp_dump_ctrl();
				dsp_ctrl_writel(DSPC_TO_HOST_MAILBOX_NS_INTR,
						0x0);
				ret = -EFAULT;
				goto p_err_test;
			}
			dsp_ctrl_writel(DSPC_TO_HOST_MAILBOX_NS_INTR, 0x0);
		} else {
			dsp_err("test[%d] response not came\n", test_id);
			__dsp_hw_debug_print_userdefined(debug);
			dsp_dump_ctrl();
			ret = -EFAULT;
			goto p_err_test;
		}
	}
	dsp_info("test[%d] pass (repeat:%d)\n", test_id, repeat_idx);

	reg = dsp_ctrl_readl(DSPC_HOST_INTR_ENABLE);
	dsp_ctrl_writel(DSPC_HOST_INTR_ENABLE, reg | 0x1);

	dsp_memory_ion_free(mem, &dm);
	dsp_memory_ion_free(mem, &pm);

	dsp_leave();
	return 0;
p_err_test:
	dsp_err("test[%d] stop [%d/%d]\n", test_id, repeat_idx, repeat);
	reg = dsp_ctrl_readl(DSPC_HOST_INTR_ENABLE);
	dsp_ctrl_writel(DSPC_HOST_INTR_ENABLE, reg | 0x1);
p_err_load_dm:
	dsp_memory_ion_free(mem, &dm);
p_err_alloc_dm:
p_err_load_pm:
	dsp_memory_ion_free(mem, &pm);
p_err_alloc_pm:
p_err:
	return ret;
}

static int __dsp_hw_debug_dma_test(struct dsp_hw_debug *debug,
		int test_id, int repeat)
{
	int ret;
	struct dsp_system *sys;
	struct dsp_memory *mem;
	struct dsp_priv_mem in, out;
	int idx, repeat_idx;
	unsigned int reg, timeout, result;
	int *dma_in, *dma_out;

	dsp_enter();
	sys = &debug->dspdev->system;
	mem = &sys->memory;

	if (test_id == 3) {
		__dsp_hw_debug_set_priv_mem(&in, "dma_data.bin");
		__dsp_hw_debug_set_priv_mem(&out, "dma_out");
	} else {
		ret = -EFAULT;
		goto p_err;
	}

	ret = dsp_memory_ion_alloc(mem, &in);
	if (ret)
		goto p_err_alloc_in;

	ret = dsp_binary_load(in.name, NULL, NULL, in.kvaddr, in.size);
	if (ret < 0)
		goto p_err_load_in;

	ret = dsp_memory_ion_alloc(mem, &out);
	if (ret)
		goto p_err_alloc_out;

	reg = dsp_ctrl_readl(DSPC_HOST_INTR_ENABLE);
	dsp_ctrl_writel(DSPC_HOST_INTR_ENABLE, reg & ~0x1);

	for (repeat_idx = 0; repeat_idx < repeat; ++repeat_idx) {
		/* initialize SM */
		for (idx = 0; idx < 10; ++idx)
			writel(0x0, sys->sfr + 0x38000 + (idx * 4));

		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS0, test_id);
		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS1, in.iova);
		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS2, out.iova);
		dsp_ctrl_writel(DSPC_HOST_MAILBOX_NS_INTR, 0x103);

		timeout = 1000;
		while (1) {
			reg = dsp_ctrl_readl(DSPC_TO_HOST_MAILBOX_NS_INTR);
			if ((reg >> 8) & 0x1 || !timeout)
				break;
			timeout--;
			udelay(100);
		};

		if (reg & 0x1) {
			result = dsp_ctrl_readl(DSPC_TO_HOST_MAILBOX_NS0);

			if (result == 0x900dc0de) {
				dsp_ctrl_writel(DSPC_TO_HOST_MAILBOX_NS_INTR,
						0x0);

				dma_in = (int *)in.kvaddr;
				dma_out = (int *)out.kvaddr;
				for (idx = 0; idx < (16384 >> 2); ++idx) {
					if (dma_in[idx] != dma_out[idx]) {
						result = 0xdead0000;
						break;
					}
				}

				if (result != 0x900dc0de) {
					dsp_err("test[%d] fail (idx:%d)\n",
							test_id, idx);
					__dsp_hw_debug_print_userdefined(debug);
					dsp_dump_ctrl();
					ret = -EFAULT;
					goto p_err_test;
				}
			} else {
				dsp_err("test[%d] fail: %8x\n",
						test_id, result);
				__dsp_hw_debug_print_userdefined(debug);
				dsp_dump_ctrl();
				dsp_ctrl_writel(DSPC_TO_HOST_MAILBOX_NS_INTR,
						0x0);
				ret = -EFAULT;
				goto p_err_test;
			}
		} else {
			dsp_err("test[%d] response not came\n", test_id);
			__dsp_hw_debug_print_userdefined(debug);
			dsp_dump_ctrl();
			ret = -EFAULT;
			goto p_err_test;
		}
	}
	dsp_info("test[%d] pass (repeat:%d)\n", test_id, repeat_idx);

	reg = dsp_ctrl_readl(DSPC_HOST_INTR_ENABLE);
	dsp_ctrl_writel(DSPC_HOST_INTR_ENABLE, reg | 0x1);

	dsp_memory_ion_free(mem, &out);
	dsp_memory_ion_free(mem, &in);

	dsp_leave();
	return 0;
p_err_test:
	dsp_err("test[%d] stop [%d/%d]\n", test_id, repeat_idx, repeat);
	reg = dsp_ctrl_readl(DSPC_HOST_INTR_ENABLE);
	dsp_ctrl_writel(DSPC_HOST_INTR_ENABLE, reg | 0x1);
	dsp_memory_ion_free(mem, &out);
p_err_alloc_out:
p_err_load_in:
	dsp_memory_ion_free(mem, &in);
p_err_alloc_in:
p_err:
	return ret;
}

static int __dsp_hw_debug_asb_test(struct dsp_hw_debug *debug,
		int test_id, int repeat)
{
	int ret;

	dsp_enter();
	ret = dsp_device_open(debug->dspdev);
	if (ret)
		goto p_err_open;

	ret = dsp_device_start(debug->dspdev, 0);
	if (ret)
		goto p_err_start;

	if (test_id == 1 || test_id == 2) {
		ret = __dsp_hw_debug_ivp_test(debug, test_id, repeat);
	} else if (test_id == 3) {
		ret = __dsp_hw_debug_dma_test(debug, test_id, repeat);
	} else {
		ret = -EINVAL;
		dsp_warn("test id is invalid(%d)\n", test_id);
	}

	if (ret)
		goto p_err_test;

	dsp_device_stop(debug->dspdev, 1);
	dsp_device_close(debug->dspdev);

	dsp_leave();
	return 0;
p_err_test:
	dsp_device_stop(debug->dspdev, 1);
p_err_start:
	dsp_device_close(debug->dspdev);
p_err_open:
	return ret;
}

static ssize_t dsp_hw_debug_test_write(struct file *filp,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	int ret;
	struct seq_file *file;
	struct dsp_hw_debug *debug;
	char buf[30];
	ssize_t len;
	int test_id, repeat;

	dsp_enter();
	file = filp->private_data;
	debug = file->private;

	if (count > sizeof(buf)) {
		ret = -EINVAL;
		dsp_err("writing size(%zd) is larger than buffer\n", count);
		goto p_err;
	}

	len = simple_write_to_buffer(buf, sizeof(buf), ppos, user_buf, count);
	if (len <= 0) {
		ret = -EINVAL;
		dsp_err("Failed to get user buf(%zu)\n", len);
		goto p_err;
	}
	buf[len] = '\0';

	ret = sscanf(buf, "%d %d\n", &test_id, &repeat);
	if (ret != 2) {
		dsp_err("Failed to get user parameter(%d)\n", ret);
		ret = -EINVAL;
		goto p_err;
	}

	ret = __dsp_hw_debug_asb_test(debug, test_id, repeat);
	if (ret)
		goto p_err;

	dsp_leave();
	return count;
p_err:
	return ret;
}

static const struct file_operations dsp_hw_debug_test_fops = {
	.open		= dsp_hw_debug_test_open,
	.read		= seq_read,
	.write		= dsp_hw_debug_test_write,
	.llseek		= seq_lseek,
	.release	= single_release
};

void dsp_hw_debug_log_flush(struct dsp_hw_debug *debug)
{
	dsp_enter();
	dsp_leave();
}

int dsp_hw_debug_log_start(struct dsp_hw_debug *debug)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

int dsp_hw_debug_log_stop(struct dsp_hw_debug *debug)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

int dsp_hw_debug_open(struct dsp_hw_debug *debug)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

int dsp_hw_debug_close(struct dsp_hw_debug *debug)
{
	dsp_enter();
	dsp_leave();
	return 0;
}

int dsp_hw_debug_probe(struct dsp_device *dspdev)
{
	int ret;
	struct dsp_hw_debug *debug;

	dsp_enter();
	debug = &dspdev->system.debug;
	debug->dspdev = dspdev;

	debug->root = debugfs_create_dir("hardware", dspdev->debug.root);
	if (!debug->root) {
		ret = -EFAULT;
		dsp_err("Failed to create hw debug root file\n");
		goto p_err_root;
	}

	debug->power = debugfs_create_file("power", 0640, debug->root, debug,
			&dsp_hw_debug_power_fops);
	if (!debug->power)
		dsp_warn("Failed to create power debugfs file\n");

	debug->clk = debugfs_create_file("clk", 0640, debug->root, debug,
			&dsp_hw_debug_clk_fops);
	if (!debug->clk)
		dsp_warn("Failed to create clk debugfs file\n");

	debug->devfreq = debugfs_create_file("devfreq", 0640, debug->root,
			debug, &dsp_hw_debug_devfreq_fops);
	if (!debug->devfreq)
		dsp_warn("Failed to create devfreq debugfs file\n");

	debug->sfr = debugfs_create_file("sfr", 0640, debug->root, debug,
			&dsp_hw_debug_sfr_fops);
	if (!debug->sfr)
		dsp_warn("Failed to create sfr debugfs file\n");

	debug->mem = debugfs_create_file("mem", 0640, debug->root, debug,
			&dsp_hw_debug_mem_fops);
	if (!debug->mem)
		dsp_warn("Failed to create mem debugfs file\n");

	debug->test = debugfs_create_file("test", 0640, debug->root, debug,
			&dsp_hw_debug_test_fops);
	if (!debug->mem)
		dsp_warn("Failed to create test debugfs file\n");

	dsp_leave();
	return 0;
p_err_root:
	return ret;
}

void dsp_hw_debug_remove(struct dsp_hw_debug *debug)
{
	dsp_enter();
	debugfs_remove_recursive(debug->root);
	dsp_leave();
}
