/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - UFC(User Frequency Change) driver
 * Author : HEUNGSIK CHOI (hs0413.choi@samsung.com)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt
#define SCALE_SIZE 8

#include <linux/init.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/pm_opp.h>
#include <linux/ems_service.h>

#include <soc/samsung/exynos-cpuhp.h>

#include "exynos-acme.h"

enum exynos_ufc_execution_mode {
	AARCH64_MODE = 0,
	AARCH32_MODE,
	MODE_END,
};

enum exynos_ufc_ctrl_type {
	PM_QOS_MIN_LIMIT = 0,
	PM_QOS_MIN_WO_BOOST_LIMIT,
	PM_QOS_MAX_LIMIT,
	TYPE_END,
};

char *stune_group_name2[] = {
	"min-limit",
	"max-limit",
	"min-wo-boost-limit",
};

struct ufc_table_info {
	int			ctrl_type;
	int			mode;
	unsigned int		cur_index;
	u32			**ufc_table;
	struct list_head	list;
};

struct ufc_domain {
	struct cpumask		cpus;
	int			pm_qos_min_class;
	int			pm_qos_max_class;
	unsigned int		min_freq;
	unsigned int		max_freq;
	unsigned int		clear_freq;

	struct pm_qos_request	user_min_qos_req;
	struct pm_qos_request	user_max_qos_req;
	struct pm_qos_request	user_min_qos_wo_boost_req;

	struct list_head	list;
};

struct exynos_ufc {
	unsigned int		table_row;
	unsigned int		table_col;
	unsigned int		lit_table_row;

	int			sse_mode;

	int			last_min_input;
	int			last_min_wo_boost_input;
	int			last_max_input;

	u32			**ufc_lit_table;
	struct list_head	ufc_domain_list;
	struct list_head	ufc_table_list;
};

struct exynos_ufc ufc;
struct cpufreq_policy *little_policy;

/*********************************************************************
 *                          HELPER FUNCTION                           *
 *********************************************************************/
static char* get_mode_string(enum exynos_ufc_execution_mode mode)
{
	switch (mode) {
	case AARCH64_MODE:
		return "AARCH64_MODE";
	case AARCH32_MODE:
		return "AARCH32_MODE";
	case MODE_END:
		return NULL;
	}

	return NULL;
}

static char* get_ctrl_type_string(enum exynos_ufc_ctrl_type type)
{
	switch (type) {
	case PM_QOS_MIN_LIMIT:
		return "PM_QOS_MIN_LIMIT";
	case PM_QOS_MAX_LIMIT:
		return "PM_QOS_MAX_LIMIT";
	case PM_QOS_MIN_WO_BOOST_LIMIT:
		return "PM_QOS_MIN_WO_BOOST_LIMIT";
	case TYPE_END:
		return NULL;
	}

	return NULL;
}

static struct exynos_cpufreq_domain* first_domain(void)
{
	struct list_head *domain_list = get_domain_list();

	if (!domain_list)
		return NULL;

	return list_first_entry(domain_list,
			struct exynos_cpufreq_domain, list);
}

static bool ufc_check_little_domain(struct ufc_domain *ufc_dom)
{
	struct cpumask mask;
	struct exynos_cpufreq_domain *domain = first_domain();

	/* If domain is NULL, then return true for nothing to do */
	if (!domain)
		return true;

	cpumask_xor(&mask, &ufc_dom->cpus, &domain->cpus);

	if (cpumask_empty(&mask))
		return true;

	return false;
}

static void
disable_domain_cpus(struct ufc_domain *ufc_dom)
{
	struct cpumask mask;

	if (ufc_check_little_domain(ufc_dom))
		return;

	cpumask_andnot(&mask, cpu_online_mask, &ufc_dom->cpus);
	exynos_cpuhp_request("UFC", mask, 0);
}

static void
enable_domain_cpus(struct ufc_domain *ufc_dom)
{
	struct cpumask mask;

	if (ufc_check_little_domain(ufc_dom))
		return;

	cpumask_or(&mask, cpu_online_mask, &ufc_dom->cpus);
	exynos_cpuhp_request("UFC", mask, 0);
}

unsigned int get_cpufreq_max_limit(void)
{
	struct ufc_domain *ufc_dom;
	unsigned int pm_qos_max;

	/* Big --> Mid --> Lit */
	list_for_each_entry(ufc_dom, &ufc.ufc_domain_list, list) {
		/* get value of minimum PM QoS */
		pm_qos_max = pm_qos_request(ufc_dom->pm_qos_max_class);
		if (pm_qos_max > 0) {
			pm_qos_max = min(pm_qos_max, ufc_dom->max_freq);
			pm_qos_max = max(pm_qos_max, ufc_dom->min_freq);

			return pm_qos_max;
		}
	}

	/*
	 * If there is no QoS at all domains, it prints big maxfreq
	 */
	return first_domain()->max_freq;
}

static unsigned int ufc_get_table_col(struct device_node *child)
{
	unsigned int col;

	of_property_read_u32(child, "table-col", &col);

	return col;
}

static unsigned int ufc_get_table_row(struct device_node *child)
{
	unsigned int row;

	of_property_read_u32(child, "table-row", &row);

	return row;
}

static int ufc_get_proper_min_table_index(unsigned int freq,
		struct ufc_table_info *table_info)
{
	int index;

	for (index = 0; table_info->ufc_table[0][index] > freq; index++)
		;
	if (table_info->ufc_table[0][index] < freq)
		index--;

	return index;
}

static int ufc_get_proper_max_table_index(unsigned int freq,
		struct ufc_table_info *table_info)
{
	int index;

	for (index = 0; table_info->ufc_table[0][index] > freq; index++)
		;

	return index;
}

static int ufc_get_proper_table_index(unsigned int freq,
		struct ufc_table_info *table_info, int ctrl_type)
{
	int target_idx = 0;

	switch (ctrl_type) {
	case PM_QOS_MIN_LIMIT:
	case PM_QOS_MIN_WO_BOOST_LIMIT:
		target_idx = ufc_get_proper_min_table_index(freq, table_info);
		break;

	case PM_QOS_MAX_LIMIT:
		target_idx = ufc_get_proper_max_table_index(freq, table_info);
		break;
	}
	return target_idx;
}

static void ufc_clear_min_qos(void)
{
	struct ufc_domain *ufc_dom;
	list_for_each_entry(ufc_dom, &ufc.ufc_domain_list, list)
		pm_qos_update_request(&ufc_dom->user_min_qos_req, 0);
}

static void ufc_clear_max_qos(void)
{
	struct ufc_domain *ufc_dom;

	list_for_each_entry(ufc_dom, &ufc.ufc_domain_list, list) {
		enable_domain_cpus(ufc_dom);
		pm_qos_update_request(&ufc_dom->user_max_qos_req, ufc_dom->max_freq);
	}
}

static void ufc_clear_pm_qos(int ctrl_type)
{
	switch (ctrl_type) {
	case PM_QOS_MIN_LIMIT:
	case PM_QOS_MIN_WO_BOOST_LIMIT:
		ufc_clear_min_qos();
		break;

	case PM_QOS_MAX_LIMIT:
		ufc_clear_max_qos();
		break;
	}
}

static bool ufc_need_adjust_freq(unsigned int freq, struct ufc_domain *dom)
{
	if ((freq < dom->min_freq) || (freq > dom->max_freq))
		return true;

	return false;
}

static unsigned int ufc_adjust_freq(unsigned int freq, struct ufc_domain *dom, int ctrl_type)
{
	if (freq > dom->max_freq)
		return dom->max_freq;

	if (freq < dom->min_freq) {
		switch(ctrl_type) {
		case PM_QOS_MIN_LIMIT:
		case PM_QOS_MIN_WO_BOOST_LIMIT:
			return dom->min_freq;

		case PM_QOS_MAX_LIMIT:
			if (freq == 0)
				return 0;
			return dom->min_freq;
		}
	}
	return freq;
}

/*********************************************************************
 *                          SYSFS FUNCTION                           *
 *********************************************************************/

static void ufc_update_limit(int input_freq, int ctrl_type, int mode)
{
	struct ufc_domain *ufc_dom;
	struct ufc_table_info *table_info, *target_table_info = NULL;
	int target_idx = 0;
	int col_idx = 1;

	if (input_freq <= 0) {
		ufc_clear_pm_qos(ctrl_type);
		return;
	}
	list_for_each_entry(table_info, &ufc.ufc_table_list, list) {
		if (table_info->ctrl_type == ctrl_type && table_info->mode == mode) {
			target_table_info = table_info;
			break;
		}
	}

	if (!target_table_info) {
		pr_err("failed to find target table\n");
		return;
	}
	target_idx = ufc_get_proper_table_index(input_freq, target_table_info, ctrl_type);

	/* Big --> Mid --> Lit */
	list_for_each_entry(ufc_dom, &ufc.ufc_domain_list, list) {
		unsigned int target_freq;
		struct pm_qos_request *target_pm_qos;

		target_freq = target_table_info->ufc_table[col_idx++][target_idx];
		if (ufc_need_adjust_freq(target_freq, ufc_dom))
			target_freq = ufc_adjust_freq(target_freq, ufc_dom, ctrl_type);

		switch (ctrl_type) {
		case PM_QOS_MIN_LIMIT:
			target_pm_qos = &ufc_dom->user_min_qos_req;
			break;

		case PM_QOS_MAX_LIMIT:
			if (target_freq == 0) {
				disable_domain_cpus(ufc_dom);
				continue;
			}
			enable_domain_cpus(ufc_dom);
			target_pm_qos = &ufc_dom->user_max_qos_req;
			break;

		case PM_QOS_MIN_WO_BOOST_LIMIT:
			target_pm_qos = &ufc_dom->user_min_qos_req;
			break;
		}
		pm_qos_update_request(target_pm_qos, target_freq);
	}
}

static ssize_t ufc_show_cpufreq_table(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct ufc_table_info *table_info;
	ssize_t count = 0;
	int r_idx;

	table_info = list_first_entry(&ufc.ufc_table_list, struct ufc_table_info, list);

	for (r_idx = 0; r_idx < (ufc.table_row + ufc.lit_table_row); r_idx++)
		count += snprintf(&buf[count], 10, "%d ", table_info->ufc_table[0][r_idx]);

	return count - 1;
}

static ssize_t ufc_show_cpufreq_table_debug(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct ufc_table_info *table_info;
	int count = 0;
	int c_idx, r_idx;
	char *ctrl_str, *mode_str;

	list_for_each_entry(table_info, &ufc.ufc_table_list, list) {
		ctrl_str = get_ctrl_type_string(table_info->ctrl_type);
		mode_str = get_mode_string(table_info->mode);

		count += snprintf(buf + count, PAGE_SIZE - count, "Table Ctrl Type: %s(%d), Mode: %s(%d)\n",
				ctrl_str, table_info->ctrl_type, mode_str, table_info->mode);

		for (r_idx = 0; r_idx < (ufc.table_row + ufc.lit_table_row); r_idx++) {
			for (c_idx = 0; c_idx < ufc.table_col; c_idx++) {
				count += snprintf(buf + count, PAGE_SIZE - count, "%9d",
						table_info->ufc_table[c_idx][r_idx]);
			}
			count += snprintf(buf + count, PAGE_SIZE - count,"\n");
		}
		count += snprintf(buf + count, PAGE_SIZE - count,"\n");
	}

	return count;
}

static ssize_t ufc_show_cpufreq_max_limit(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%d\n", ufc.last_max_input);
}

static ssize_t ufc_show_cpufreq_min_limit(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%d\n", ufc.last_min_input);
}

static ssize_t ufc_show_execution_mode_change(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 10, "%d\n", ufc.sse_mode);
}

static ssize_t ufc_store_cpufreq_min_limit(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf,
				size_t count)
{
	int input;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	/* Save the input for sse change */
	ufc.last_min_input = input;

	ufc_update_limit(input, PM_QOS_MIN_LIMIT, ufc.sse_mode);

	return count;
}

static ssize_t ufc_store_cpufreq_min_limit_wo_boost(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf,
		size_t count)
{
	int input;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	/* Save the input for sse change */
	ufc.last_min_wo_boost_input = input;

	ufc_update_limit(input, PM_QOS_MIN_WO_BOOST_LIMIT, ufc.sse_mode);

	return count;
}

static ssize_t ufc_store_cpufreq_max_limit(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	int input;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	/* Save the input for sse change */
	ufc.last_max_input = input;

	ufc_update_limit(input, PM_QOS_MAX_LIMIT, ufc.sse_mode);

	return count;
}

static ssize_t ufc_store_execution_mode_change(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	int input;
	int prev_mode;

	if (!sscanf(buf, "%8d", &input))
		return -EINVAL;

	prev_mode = ufc.sse_mode;
	ufc.sse_mode = !!input;

	if (prev_mode != ufc.sse_mode) {
		ufc_update_limit(ufc.last_max_input, PM_QOS_MAX_LIMIT, ufc.sse_mode);
		ufc_update_limit(ufc.last_min_input, PM_QOS_MIN_LIMIT, ufc.sse_mode);
		ufc_update_limit(ufc.last_min_wo_boost_input,
				PM_QOS_MIN_WO_BOOST_LIMIT, ufc.sse_mode);
	}

	return count;
}

static ssize_t ufc_store_table_change(struct kobject *kobj, struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	int ret;

	unsigned int ctrl_type, mode;
	unsigned int edit_row, edit_col;
	unsigned int edit_freq;

	struct ufc_table_info *table_info;

	ret= sscanf(buf, "%d %d %d %d %d\n",
			&ctrl_type, &mode, &edit_row, &edit_col, &edit_freq);
	if (ret != 5) {
		pr_err("We need 5 inputs. Enter ctrl_type, mode, row, col, frequency");
		return -EINVAL;
	}

	if (0 > edit_row || edit_row >= (ufc.table_row + ufc.lit_table_row)) {
		pr_err("Row to edit is out of the table boundary (%d <= row < %d)",
				0, (ufc.table_row + ufc.lit_table_row));
		return -EINVAL;
	}
	if (0 > edit_col || edit_col >= ufc.table_col) {
		pr_err("Collum to edit is out of the table boundary (%d <= col < %d)",
				0, ufc.table_col);
		return -EINVAL;
	}

	list_for_each_entry(table_info, &ufc.ufc_table_list, list) {
		if ((table_info->ctrl_type == ctrl_type) && (table_info->mode ==mode)) {
			table_info->ufc_table[edit_col][edit_row] = edit_freq;

			return count;
		}
	}

	pr_err("Failed to find right table. Check your ctrl_type & mode which you enter");
	return -EINVAL;
}

static struct kobj_attribute cpufreq_table =
	__ATTR(cpufreq_table, 0444, ufc_show_cpufreq_table, NULL);
static struct kobj_attribute cpufreq_table_debug =
	__ATTR(cpufreq_table_debug, 0444, ufc_show_cpufreq_table_debug, NULL);
static struct kobj_attribute cpufreq_min_limit =
	__ATTR(cpufreq_min_limit, 0644,
		ufc_show_cpufreq_min_limit, ufc_store_cpufreq_min_limit);
static struct kobj_attribute cpufreq_min_limit_wo_boost =
	__ATTR(cpufreq_min_limit_wo_boost, 0644,
		ufc_show_cpufreq_min_limit, ufc_store_cpufreq_min_limit_wo_boost);
static struct kobj_attribute cpufreq_max_limit =
	__ATTR(cpufreq_max_limit, 0644,
		ufc_show_cpufreq_max_limit, ufc_store_cpufreq_max_limit);
static struct kobj_attribute execution_mode_change =
	__ATTR(execution_mode_change, 0644,
		ufc_show_execution_mode_change, ufc_store_execution_mode_change);
static struct kobj_attribute ufc_table_change =
	__ATTR(ufc_table_change, 0644,
		ufc_show_cpufreq_table_debug, ufc_store_table_change);

/*********************************************************************
 *                          INIT FUNCTION                          *
 *********************************************************************/

static __init int ufc_init_pm_qos(void)
{
	struct cpufreq_policy *policy;
	struct ufc_domain *ufc_dom;

	list_for_each_entry(ufc_dom, &ufc.ufc_domain_list, list) {
		policy = cpufreq_cpu_get(cpumask_first(&ufc_dom->cpus));

		if (!policy)
			return -EINVAL;
		pm_qos_add_request(&ufc_dom->user_min_qos_req,
				ufc_dom->pm_qos_min_class, policy->min);
		pm_qos_add_request(&ufc_dom->user_max_qos_req,
				ufc_dom->pm_qos_max_class, policy->max);
		pm_qos_add_request(&ufc_dom->user_min_qos_wo_boost_req,
				ufc_dom->pm_qos_min_class, policy->min);
	}

	/* Success */
	return 0;
}

static __init int ufc_init_sysfs(void)
{
	int ret = 0;

	ret = sysfs_create_file(power_kobj, &cpufreq_table.attr);
	if (ret) {
		pr_err("failed to create cpufreq_table node\n");
		return ret;
	}

	ret = sysfs_create_file(power_kobj, &cpufreq_table_debug.attr);
	if (ret) {
		pr_err("failed to create cpufreq_table_debug node\n");
		return ret;
	}

	ret = sysfs_create_file(power_kobj, &cpufreq_min_limit.attr);
	if (ret) {
		pr_err("failed to create cpufreq_min_limit node\n");
		return ret;
	}

	ret = sysfs_create_file(power_kobj, &cpufreq_min_limit_wo_boost.attr);
	if (ret) {
		pr_err("failed to create cpufreq_min_limit_wo_boost node\n");
		return ret;
	}

	ret = sysfs_create_file(power_kobj, &cpufreq_max_limit.attr);
	if (ret) {
		pr_err("failed to create cpufreq_max_limit node\n");
		return ret;
	}

	ret = sysfs_create_file(power_kobj, &execution_mode_change.attr);
	if (ret) {
		pr_err("failed to create execution mode change node\n");
		return ret;
	}

	ret = sysfs_create_file(power_kobj, &ufc_table_change.attr);
	if (ret) {
		pr_err("failed to create ufc table change node\n");
		return ret;
	}

	return ret;
}

static int ufc_parse_init_table(struct device_node *dn,
		struct ufc_table_info *ufc_info, struct cpufreq_policy *policy)
{
	int col_idx, row_idx, row_backup;
	int ret;
	struct cpufreq_frequency_table *pos;

	for (col_idx = 0; col_idx < ufc.table_col; col_idx++) {
		for (row_idx = 0; row_idx < ufc.table_row; row_idx++) {
			ret = of_property_read_u32_index(dn, "table",
					(col_idx + (row_idx * ufc.table_col)),
					&(ufc_info->ufc_table[col_idx][row_idx]));
			if (ret)
				return -EINVAL;
		}
	}

	cpufreq_for_each_entry(pos, policy->freq_table) {
		if (pos->frequency > policy->max)
			continue;
	}

	row_backup = row_idx;
	for (col_idx = 0; col_idx < ufc.table_col; col_idx++) {
		row_idx = row_backup;
		cpufreq_for_each_entry(pos, policy->freq_table) {
			if (pos->frequency > policy->max)
				continue;

			if (col_idx == 0)
				ufc_info->ufc_table[col_idx][row_idx++]
						= pos->frequency / SCALE_SIZE;
			else if (col_idx == (ufc.table_col-1))
				ufc_info->ufc_table[col_idx][row_idx++]
						= pos->frequency;
			else
				ufc_info->ufc_table[col_idx][row_idx++] = 0;
		}
	}

	/* Success */
	return 0;
}

static int init_ufc_table(struct device_node *dn)
{
	struct ufc_table_info *table_info;
	int col_idx;

	table_info = kzalloc(sizeof(struct ufc_table_info), GFP_KERNEL);

	if (of_property_read_u32(dn, "ctrl-type", &table_info->ctrl_type))
		return -EINVAL;

	if (of_property_read_u32(dn, "sse-mode", &table_info->mode))
		return -EINVAL;

	table_info->cur_index = 0;

	table_info->ufc_table = kzalloc(sizeof(u32 *) * ufc.table_col, GFP_KERNEL);
	if (!table_info->ufc_table)
		return -ENOMEM;

	for (col_idx = 0; col_idx < ufc.table_col; col_idx++) {
		table_info->ufc_table[col_idx] = kzalloc(sizeof(u32 *) * ufc.table_row,
							GFP_KERNEL);

		if (!table_info->ufc_table[col_idx])
			return -ENOMEM;
	}

	ufc_parse_init_table(dn, table_info, little_policy);
	list_add_tail(&table_info->list, &ufc.ufc_table_list);

	/* Success */
	return 0;
}

static int init_ufc_domain(struct device_node *dn)
{
	struct cpumask shared_mask;
	struct ufc_domain *ufc_dom;
	struct cpufreq_policy *policy;
	const char *buf;
	int ret = 0;

	ufc_dom = kzalloc(sizeof(struct ufc_domain), GFP_KERNEL);
	if (!ufc_dom)
		return -ENOMEM;

	ret = of_property_read_string(dn, "shared-cpus", &buf);
	if (ret) {
		pr_err("Failed to get shared-cpus for ufc\n");
		return -EINVAL;
	}

	cpulist_parse(buf, &shared_mask);
	ufc_dom->cpus = shared_mask;

	if (of_property_read_u32(dn, "pm_qos-min-class", &ufc_dom->pm_qos_min_class)) {
		pr_err("Failed to get pm_qos-min-class for ufc\n");
		return -EINVAL;
	}

	if (of_property_read_u32(dn, "pm_qos-max-class", &ufc_dom->pm_qos_max_class)) {
		pr_err("Failed to get pm_qos-max-class for ufc\n");
		return -EINVAL;
	}

	policy = cpufreq_cpu_get(cpumask_first(&shared_mask));
	ufc_dom->min_freq = policy->min;
	ufc_dom->max_freq = policy->max;

	list_add(&ufc_dom->list, &ufc.ufc_domain_list);

	if (ufc_dom->pm_qos_min_class == PM_QOS_CLUSTER0_FREQ_MIN) {
		struct cpufreq_frequency_table *pos;
		int lit_row = 0;

		little_policy = cpufreq_cpu_get(cpumask_first(&shared_mask));
		if (!little_policy)
			return -EINVAL;

		cpufreq_for_each_entry(pos, little_policy->freq_table) {
			if (pos->frequency > little_policy->max)
				continue;

			lit_row++;
		}

		ufc.lit_table_row = lit_row;
	}

	/* Success */
	return 0;
}

static int init_ufc(struct device_node *dn)
{
	INIT_LIST_HEAD(&ufc.ufc_domain_list);
	INIT_LIST_HEAD(&ufc.ufc_table_list);

	ufc.table_row = ufc_get_table_row(dn);
	if (ufc.table_row < 0)
		return -EINVAL;

	ufc.table_col = ufc_get_table_col(dn);
	if (ufc.table_col < 0)
		return -EINVAL;

	ufc.sse_mode = 0;
	ufc.last_min_input = 0;
	ufc.last_max_input = 0;
	ufc.last_min_wo_boost_input = 0;

	return 0;
}

static int __init exynos_ufc_init(void)
{
	struct device_node *dn = NULL;
	struct ufc_table_info *table_info;
	struct ufc_domain *ufc_dom;
	int col_idx;
	int ret = 0;

	dn = of_find_node_by_type(dn, "cpufreq_ufc");
	if (!dn)
		return -EINVAL;

	ret = init_ufc(dn);
	if (ret)
		return ret;

	while((dn = of_find_node_by_type(dn, "ufc_domain"))) {
		ret = init_ufc_domain(dn);
		if (ret)
			goto ufc_dom_free;
	}

	while((dn = of_find_node_by_type(dn, "ufc_table"))) {
		ret = init_ufc_table(dn);
		if (ret)
			goto all_free;
	}

	ret = ufc_init_sysfs();
	if (ret)
		goto all_free;

	ret = ufc_init_pm_qos();
	if (ret)
		goto all_free;

	ret = exynos_cpuhp_register("UFC", *cpu_online_mask, 0);
	if (ret)
		goto all_free;

	return ret;

all_free:
	pr_err("Failed: can't initialize ufc table");

	list_for_each_entry(table_info, &ufc.ufc_table_list, list) {
		for (col_idx = 0; table_info->ufc_table[col_idx]; col_idx++)
			kfree(table_info->ufc_table[col_idx]);
		kfree(table_info->ufc_table);
		kfree(table_info);
	}

ufc_dom_free:
	pr_err("Failed: can't initialize ufc domain");

	list_for_each_entry(ufc_dom, &ufc.ufc_domain_list, list)
		kfree(ufc_dom);

	return ret;
}
late_initcall(exynos_ufc_init);