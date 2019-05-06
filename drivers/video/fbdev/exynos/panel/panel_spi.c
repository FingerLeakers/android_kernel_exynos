/*
 * linux/drivers/video/fbdev/exynos/panel/panel_spi.c
 *
 * Samsung Panel SPI Driver.
 *
 * Copyright (c) 2019 Samsung Electronics
 * Kimyung Lee <kernel.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/miscdevice.h>

#include "panel_drv.h"
#include "panel_spi.h"

#define PANEL_SPI_MAX_CMD_SIZE 16
#define PANEL_SPI_RX_BUF_SIZE 2048


static const struct file_operations panel_spi_drv_fops = {
	.owner = THIS_MODULE,
//	.unlocked_ioctl = aod_drv_fops_ioctl,
//	.open = panel_spi_drv_open,
//	.release = panel_spi_drv_release,
//	.write = aod_drv_fops_write,
};

#define PANEL_SPI_DEV_NAME	"panel_spi"

static int panel_spi_reset(struct spi_device *spi) {
	int status, i;
	u8 reset_enable = 0x66;
	u8 reset_device = 0x99;
	struct spi_message msg;
	struct spi_transfer xfer[] = {
		{ .bits_per_word = 8, .len = 1, .tx_buf = &reset_enable, },
		{ .bits_per_word = 8, .len = 1, .tx_buf = &reset_device, .delay_usecs = 30 },
	};

	pr_info("%s: reset called!", __func__);

	if (unlikely(!spi)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	spi_message_init(&msg);
	for (i = 0; i < (unsigned int)ARRAY_SIZE(xfer); i++)
		if (xfer[i].len > 0)
			spi_message_add_tail(&xfer[i], &msg);

	status = spi_sync(spi, &msg);
	if (status < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, status);
		return status;
	}

	return 0;
}
static int panel_spi_ctrl(struct spi_device *spi, int ctrl_msg)
{
	int ret = 0;
	switch(ctrl_msg) {
		case PANEL_SPI_CTRL_RESET:
			ret = panel_spi_reset(spi);
			break;
		case PANEL_SPI_STATUS_1:
			break;
		case PANEL_SPI_STATUS_2:
			break;
		default:
			break;
	}

	return ret;
}

static int panel_spi_cmd_setparam(struct panel_spi_dev *spi_dev, const u8 *wbuf, int wsize)
{
	if(wsize < 1 || !wbuf) {
		return -EINVAL;
	}
	memcpy(spi_dev->setparam_buffer, wbuf, wsize);
	spi_dev->setparam_buffer_size = wsize;
	return wsize;
}

static int panel_spi_command(struct panel_spi_dev *spi_dev, const u8 *wbuf, int wsize, u8 *rbuf, int rsize)
{
	int ret = 0;
	u32 speed_hz;
	struct spi_device *spi;
	struct spi_message msg;
	struct spi_transfer x_write = { .bits_per_word = 8, };
	struct spi_transfer x_read = { .bits_per_word = 8, };

	spi = spi_dev->spi;

	spi_message_init(&msg);
	if(wsize < 1 || !wbuf) {
		return -EINVAL;
	}

	speed_hz = spi_dev->speed_hz;
	if (speed_hz <= 0)
		speed_hz = 0;
	else if (speed_hz > MAX_PANEL_SPI_SPEED_HZ)
		speed_hz = MAX_PANEL_SPI_SPEED_HZ;


	x_write.len = wsize;
	x_write.tx_buf = wbuf;
	x_write.speed_hz = speed_hz;
	spi_message_add_tail(&x_write, &msg);

	if(rsize) {
		memset(spi_dev->read_buf_data, 0, PANEL_SPI_RX_BUF_SIZE);
		x_read.len = rsize;
		x_read.rx_buf = spi_dev->read_buf_data;
		x_read.speed_hz = speed_hz;
		spi_message_add_tail(&x_read, &msg);
		spi_dev->read_buf_cmd = wbuf[0];
		spi_dev->read_buf_size = rsize;
	}

	ret = spi_sync(spi, &msg);
	if (ret < 0) {
		pr_err("%s, failed to spi_sync %d\n", __func__, ret);
		return ret;
	}

	if (rbuf != NULL)
		memcpy(rbuf, spi_dev->read_buf_data, rsize);

//	print_hex_dump(KERN_ERR, "spi_cmd_print ", DUMP_PREFIX_ADDRESS, 16, 1, wbuf, wsize, false);
	return rsize;
}

static int panel_spi_read(struct panel_spi_dev *spi_dev, const u8 rcmd, u8 *rbuf, int rsize)
{
	int ret = 0;
	const u8 *wbuf;
	int wsize;

	//check setparam cmd for read operation
	if (!spi_dev->setparam_buffer) {
		return -EINVAL;
	}

	wbuf = &rcmd;
	wsize = 1;

	if (spi_dev->setparam_buffer_size > 0 && spi_dev->setparam_buffer[0] == rcmd) {
		//setparam cmd
		wbuf = spi_dev->setparam_buffer;
		wsize = spi_dev->setparam_buffer_size;
	}
	ret = panel_spi_command(spi_dev, wbuf, wsize, rbuf, rsize);
	if (!ret) {
		return -EINVAL;
	}
	//setparam_buffer_size set to 0 if setparam once
	spi_dev->setparam_buffer_size = 0;

	return rsize;
}

static int panel_spi_read_buffer(struct panel_spi_dev *spi_dev, u8 cmd, u8 *buf, int size)
{
	struct spi_device *spi;

	spi = spi_dev->spi;

	pr_debug("%s, %02x %d %02x %d\n", __func__, spi_dev->read_buf_cmd, spi_dev->read_buf_size, cmd, size);

	if (!buf) {
		return -EINVAL+1;
	}

	if (spi_dev->read_buf_size < 1 || spi_dev->read_buf_size != size) {
		return -EINVAL+2;
	}

	if (spi_dev->read_buf_cmd != cmd) {
		return -EINVAL+3;
	}

	memcpy(buf, spi_dev->read_buf_data, size);

	return size;

}

static int panel_spi_probe_dt(struct spi_device *spi)
{
	struct device_node *nc = spi->dev.of_node;
	int rc;
	u32 value;

	rc = of_property_read_u32(nc, "bits-per-word", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'bits-per-word' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->bits_per_word = value;

	rc = of_property_read_u32(nc, "spi-max-frequency", &value);
	if (rc) {
		dev_err(&spi->dev, "%s has no valid 'spi-max-frequency' property (%d)\n",
				nc->full_name, rc);
		return rc;
	}
	spi->max_speed_hz = value;

	return 0;
}

static int panel_spi_probe(struct spi_device *spi)
{
	int ret;

	if (unlikely(!spi)) {
		pr_err("%s, invalid spi\n", __func__);
		return -EINVAL;
	}

	ret = panel_spi_probe_dt(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to parse device tree, ret %d\n",
				__func__, ret);
		return ret;
	}

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "%s, failed to setup spi, ret %d\n", __func__, ret);
		return ret;
	}

//	panel_spi = spi;


	return 0;
}

static int panel_spi_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id panel_spi_match_table[] = {
	{   .compatible = "poc_spi",},
	{}
};

static struct spi_drv_ops panel_spi_drv_ops = {
	.readbuf = panel_spi_read_buffer,
	.read = panel_spi_read,
//	.write = panel_spi_write,
	.ctl = panel_spi_ctrl,
	.cmd = panel_spi_command,
	.setparam = panel_spi_cmd_setparam,
};

static int __match_panel_spi(struct device *dev, void *data)
{
	struct spi_device *spi;
/*
	spi = (struct spi_device *)dev_get_drvdata(dev);
	if (spi == NULL) {
		pr_err("PANEL:ERR:%s:failed to get spi drvdata\n", __func__);
		return 0;
	}
*/
	spi = (struct spi_device *)dev;
	if (spi == NULL) {
		pr_err("PANEL:ERR:%s:failed to get spi drvdata\n", __func__);
		return 0;
	}

	pr_info("%s spi driver %s found!\n",
		__func__, spi->dev.driver->name);

	return 1;
}

static struct spi_driver panel_spi_driver = {
	.driver = {
		.name		= PANEL_SPI_DRIVER_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = panel_spi_match_table,
	},
	.probe		= panel_spi_probe,
	.remove		= panel_spi_remove,
};

int panel_spi_drv_probe(struct panel_device *panel, struct spi_data *spi_data)
{
//	int i;
	int ret = 0;
	struct panel_spi_dev *spi_dev;
	struct device_driver *driver;
	struct device *dev;
//	struct aod_ioctl_props *props;

	if (!panel || !spi_data) {
		pr_err("%s panel(%p) or spi_data(%p) not exist\n",
				__func__, panel, spi_data);
		goto rest_init;
	}

//	spi_data: from panel.h defined data sets (aod_tune)

	spi_dev = &panel->panel_spi_dev;

	/*
	props = &aod->props;
	aod->seqtbl = aod_tune->seqtbl;
	aod->nr_seqtbl = aod_tune->nr_seqtbl;

	aod->maptbl = aod_tune->maptbl;
	aod->nr_maptbl = aod_tune->nr_maptbl;

	aod->ops = &aod_drv_ops;
	aod->reset_flag = 1;
	props->first_clk_update = 1;

	props->self_mask_en = aod_tune->self_mask_en;
	props->self_reset_cnt = 0;
	mutex_init(&aod->lock);
*/

#if 0
	//for test
	props->self_move_en = 1;
	props->self_move_interval = INTERVAL_DEBUG;

	//for test
	props->self_move_en = 1;

	// self icon
	props->icon_img_updated = 1;
	props->self_icon.en = 1;
	props->self_icon.pos_x = 100;
	props->self_icon.pos_y = 1500;
	props->self_icon.width = 0x34;
	props->self_icon.height = 0x34;

	// self grid
	props->self_grid.en = 1;
	props->self_grid.end_pos_x = 1440;
	props->self_grid.end_pos_y = 2960;

	// for analog clock
	aod->ac_img.up_flag = 1;
	props->analog.en = 1;
	props->analog.pos_x = 1440/2;
	props->analog.pos_y = 2960/2;
	props->debug_interval = ALG_INTERVAL_1000;
	props->analog.rotate = ALG_ROTATE_0;

	// for digital clock
	aod->dc_img.up_flag = 1;
	props->digital.en = 1;
	props->digital.en_hh = 1;
	props->digital.en_mm = 1;
	props->digital.pos1_x = 280;
	props->digital.pos1_y = 1480;
	props->digital.pos2_x = 500;
	props->digital.pos2_y = 1480;
	props->digital.pos3_x = 740;
	props->digital.pos3_y = 1480;
	props->digital.pos4_x = 960;
	props->digital.pos4_y = 1480;
	props->digital.img_width = 200;
	props->digital.img_height = 356;
	props->digital.b_en = 1;
	props->digital.b1_pos_x = 720;
	props->digital.b1_pos_y = 1580;
	props->digital.b2_pos_x = 720;
	props->digital.b2_pos_y = 1480 + 256;
	props->digital.b_color = 0x00ff00;
	props->digital.b_radius = 0x0a;
#endif
/*
	for (i = 0; i < aod->nr_maptbl; i++) {
		aod->maptbl[i].pdata = aod;
		maptbl_init(&aod->maptbl[i]);
	}
*/
	spi_dev->ops = &panel_spi_drv_ops;
	spi_dev->speed_hz = 10000000;
	spi_dev->setparam_buffer = (u8 *)devm_kzalloc(panel->dev, PANEL_SPI_MAX_CMD_SIZE * sizeof(u8), GFP_KERNEL);
	spi_dev->read_buf_data= (u8 *)devm_kzalloc(panel->dev, PANEL_SPI_RX_BUF_SIZE * sizeof(u8), GFP_KERNEL);
	spi_dev->dev.minor = MISC_DYNAMIC_MINOR;
	spi_dev->dev.fops = &panel_spi_drv_fops;
	spi_dev->dev.name = DRIVER_NAME;
	ret = misc_register(&spi_dev->dev);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to register for spi_dev\n", __func__);
		goto rest_init;
	}

	ret = spi_register_driver(&panel_spi_driver);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to register for spi device\n", __func__);
		goto rest_init;
	}

	driver = driver_find(PANEL_SPI_DRIVER_NAME, &spi_bus_type);
	if (IS_ERR_OR_NULL(driver)) {
		dsim_err("PANEL:ERR:%s failed to find driver\n", __func__);
		return -ENODEV;
	}

//	spi_dev->spi = (struct spi_device *)driver_find_device(driver, NULL, spi_dev->spi, __match_panel_spi);
	dev	= driver_find_device(driver, NULL, NULL, __match_panel_spi);
	if (IS_ERR_OR_NULL(dev)) {
		panel_err("PANEL:ERR:%s:failed to find device\n", __func__);
		return -ENODEV;
	}

	spi_dev->spi = to_spi_device(dev);
	if (IS_ERR_OR_NULL(spi_dev->spi)) {
		panel_err("PANEL:ERR:%s:failed to find spi device\n", __func__);
		return -ENODEV;
	}

	panel_info("%s: spi_dev probe done! id: %d\n", __func__, spi_data->spi_addr);

rest_init:
	return ret;
}


/*
struct spi_device *of_find_panel_spi_by_node(struct device_node *node)
{
	//return (spi->dev.of_node == node) ? panel_spi : NULL;
	return panel_spi;
}
EXPORT_SYMBOL(of_find_panel_spi_by_node);
*/
/*
static int __init panel_spi_init(void)
{
	return spi_register_driver(&panel_spi_driver);
}
subsys_initcall(panel_spi_init);

static void __exit panel_spi_exit(void)
{
	spi_unregister_driver(&panel_spi_driver);
}

module_exit(panel_spi_exit);
*/

