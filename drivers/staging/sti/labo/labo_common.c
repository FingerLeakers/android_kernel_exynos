/* labo_common.c
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

#include <linux/sti/labo_common.h>

static int labo_sysfs_write_read(struct labo_data *data)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *filep;
	char *c, *p;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filep = filp_open(data->sysfs_path, O_RDWR, 0);
	if (IS_ERR(filep)) {
		ret = PTR_ERR(filep);
		set_fs(old_fs);
		pr_err("%s: %s open fail\n", __func__, data->sysfs_path);
		return ret;
	}

	if (data->write_flag == 1) {
		ret = filep->f_op->write(filep, data->sysfs_value,
			sizeof(data->sysfs_value), &filep->f_pos);
		if (ret < 0) {
			pr_err("%s: write fail (path : %s, value : %s)\n", __func__, data->sysfs_path, data->sysfs_value);
			filp_close(filep, current->files);
			set_fs(old_fs);
			return -EIO;
		}
		pr_info("%s: write success (path : %s, value : %s)\n", __func__, data->sysfs_path, data->sysfs_value);
	} else {
		ret = filep->f_op->read(filep, data->sysfs_value,
			sizeof(data->sysfs_value), &filep->f_pos);
		if (ret < 0) {
			pr_err("%s: read fail (path : %s)\n", __func__, data->sysfs_path);
			filp_close(filep, current->files);
			set_fs(old_fs);
			return -EIO;
		}
		p = data->sysfs_value;
		c = strsep(&p, "\n");
		strlcpy(data->sysfs_value, c, sizeof(data->sysfs_value));
		pr_info("%s: read success (path : %s, value : %s)\n", __func__, data->sysfs_path, data->sysfs_value);
	}	

	filp_close(filep, current->files);
	set_fs(old_fs);

	return 0;
}

static void labo_work_func(struct work_struct *work)
{
	struct labo_data *data = container_of(work, struct labo_data, work);
	int ret = 0;
	ret = labo_sysfs_write_read(data);
	if (ret)
		pr_err("%s: labo_work_func error (ret : %d)\n", __func__, ret);
	complete(&data->wq_done);
}

static ssize_t labo_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct labo_data *data = dev_get_drvdata(dev);
	char *param[3] = {0,};
	int idx = 0;
	char *c;
	unsigned long timeout;

#ifdef LABO_DEBUG
	pr_info("%s: buf(%s)\n", __func__, buf);
#endif

	memset(data->sysfs_path, 0, sizeof(data->sysfs_path));
	memset(data->sysfs_value, 0, sizeof(data->sysfs_value));
	data->write_flag = 0;
	
	while (((c = strsep((char **)&buf, ",")) != NULL) && (idx < LABO_CMD_PARAM_MAX)) {
		param[idx] = c;
		idx++;
	}
	c = strsep(&param[idx-1], "\n");
	param[idx-1] = c;

	if (!strcmp(param[0], "w")) {
		if ((param[1] == 0) || (param[2] == 0))
			goto error;
		data->write_flag = 1;
		strlcpy(data->sysfs_path, param[1], sizeof(data->sysfs_path));
		strlcpy(data->sysfs_value, param[2], sizeof(data->sysfs_value));
		pr_info("%s: try to write sysfs (path : %s, value : %s)\n", __func__, data->sysfs_path, data->sysfs_value);
	} else if (!strcmp(param[0], "r")) {
		if (param[1] == 0)
			goto error;
		data->write_flag = 0;
		strlcpy(data->sysfs_path, param[1], sizeof(data->sysfs_path));
		pr_info("%s: try to read sysfs (path : %s)\n", __func__, data->sysfs_path);
	} else {
		goto error;
	}

	reinit_completion(&data->wq_done);

	queue_work(data->workqueue, &data->work);

	timeout = wait_for_completion_timeout(&data->wq_done,
					      msecs_to_jiffies(LABO_WQ_WAIT_TIMEOUT));
	if (timeout == 0) {
		pr_err("%s : timeout!\n", __func__);
	}

	pr_info("%s: complete [%s](path : %s, value : %s)\n", __func__, param[0], data->sysfs_path, data->sysfs_value);

	return count;

error:
	pr_info("%s: unsupported format(%s)\n", __func__, buf);
	return count;
}

static ssize_t labo_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct labo_data *data = dev_get_drvdata(dev);

	pr_info("%s: value(%s)\n", __func__, data->sysfs_value);

	return snprintf(buf, PAGE_SIZE, "%s\n", data->sysfs_value);
}

static DEVICE_ATTR(cmd, 0600, labo_cmd_show, labo_cmd_store);

static struct attribute *labo_attributes[] = {
	&dev_attr_cmd.attr,
	NULL,
};

static struct attribute_group labo_attr_group = {
	.attrs = labo_attributes,
};

static int __init labo_init(void)
{
	struct labo_data *data;
	int ret;

	data = kzalloc(sizeof(struct labo_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->dev = sec_device_create(data, "sec_labo");
	if (IS_ERR(data->dev)) {
		pr_err("%s: Failed to create device!\n", __func__);
		ret = PTR_ERR(data->dev);
		goto err_sec_device_create;
	}

	dev_set_drvdata(data->dev, data);

	ret = sysfs_create_group(&data->dev->kobj, &labo_attr_group);
	if (ret < 0) {
		pr_err("%s: Failed to create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}

	INIT_WORK(&data->work, labo_work_func);

	data->workqueue = create_singlethread_workqueue("sec_labo_wq");
	if (!data->workqueue)
		goto err_create_labo_wq;

	init_completion(&data->wq_done);

	pr_info("%s: Success\n", __func__);

	return 0;

err_create_labo_wq:
	sysfs_remove_group(&data->dev->kobj, &labo_attr_group);
err_sysfs_create_group:
	sec_device_destroy(data->dev->devt);
err_sec_device_create:
	kfree(data);

	return ret;
}

static void __exit labo_exit(void)
{
}

module_init(labo_init);
module_exit(labo_exit);

MODULE_DESCRIPTION("Samsung LABO driver");
MODULE_AUTHOR("Sangsu Ha <sangsu.ha@samsung.com>");
MODULE_LICENSE("GPL");
