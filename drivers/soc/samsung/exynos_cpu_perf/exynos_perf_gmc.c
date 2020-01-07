/*
 * cpu mode driver
 * Jungwook Kim <jwook1.kim@samsung.com>
 */

#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/thermal.h>
#include <asm/topology.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/gpu_cooling.h>
#include <linux/of.h>
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-devfreq.h>
#include <linux/pm_qos.h>
#include <linux/ems.h>
#include <linux/miscdevice.h>
#include "../../../kernel/sched/ems/ems.h"

#define GAME_NORMAL_CL2_MAX	1690000
#define GAME_NORMAL_CL1_MAX	2400000
#define GAME_LITE_GPU	260000

static struct emstune_mode_request emstune_req_gmc;

extern unsigned int get_cpufreq_max_limit(void);
extern unsigned int get_ocp_clipped_freq(void);

static const char *prefix = "exynos_perf";

static uint cal_id_mif = 0;
static uint cal_id_g3d = 0;
static uint devfreq_mif = 0;

static int is_running = 0;
static int run = 0;
static uint polling_ms = 1000;
static uint bind_cpu = 0;
static uint is_game = 0;
static uint maxlock_delay_sec = 10;
static int cl2_max = GAME_NORMAL_CL2_MAX;
static int cl1_max = GAME_NORMAL_CL1_MAX;
static int cl0_max = PM_QOS_CLUSTER0_FREQ_MAX_DEFAULT_VALUE;
static int gpu_lite = GAME_LITE_GPU;
static int mif_max = PM_QOS_BUS_THROUGHPUT_MAX_DEFAULT_VALUE;
static int mif_min = PM_QOS_BUS_THROUGHPUT_DEFAULT_VALUE;

static int prev_is_game = 0;

static struct pm_qos_request pm_qos_cl2_max;
static struct pm_qos_request pm_qos_cl1_max;
static struct pm_qos_request pm_qos_cl0_max;
static struct pm_qos_request pm_qos_mif_max;
static struct pm_qos_request pm_qos_mif_min;

enum {
	NORMAL_MODE,
	POWER_SCENARIO_MODE,
	PERFORMANCE_MODE,
	LIGHT_GAME_MODE,
};

//---------------------------------------
// thread main
static int gmc_thread(void *data)
{
	int gpu_util = 0;
	uint gpu_freq = 0;
	int sus_array[2];
	int gpu_max_lock = 0;
	uint time_cnt = 1;
	int delay_cnt = 0;
	int online_cpus = 0;
	int cpu = 0;
	int cpu_util_avg = 0;

	if (is_running) {
		pr_info("[%s] gmc already running!!\n", prefix);
		return 0;
	}

	// start
	is_running = 1;
	pr_info("[%s] gmc start\n", prefix);

	pm_qos_add_request(&pm_qos_cl2_max, PM_QOS_CLUSTER2_FREQ_MAX, PM_QOS_CLUSTER2_FREQ_MAX_DEFAULT_VALUE);
	pm_qos_add_request(&pm_qos_cl1_max, PM_QOS_CLUSTER1_FREQ_MAX, PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE);
	pm_qos_add_request(&pm_qos_cl0_max, PM_QOS_CLUSTER0_FREQ_MAX, PM_QOS_CLUSTER0_FREQ_MAX_DEFAULT_VALUE);
	pm_qos_add_request(&pm_qos_mif_max, PM_QOS_BUS_THROUGHPUT_MAX, PM_QOS_BUS_THROUGHPUT_MAX_DEFAULT_VALUE);
	pm_qos_add_request(&pm_qos_mif_min, PM_QOS_BUS_THROUGHPUT, PM_QOS_BUS_THROUGHPUT_DEFAULT_VALUE);
	emstune_add_request(&emstune_req_gmc);

	while (is_running) {

		// cpu util
		cpu_util_avg = 0;
		for_each_online_cpu(cpu) {
			cpu_util_avg += ml_cpu_util(cpu);
			online_cpus++;
		}
		cpu_util_avg /= online_cpus;

		// gpu
		gpu_util = gpu_dvfs_get_utilization();
		gpu_freq = (uint)cal_dfs_cached_get_rate(cal_id_g3d);
		sus_array[0] = gpu_dvfs_get_sustainable_info_array(0);
		sus_array[1] = gpu_dvfs_get_sustainable_info_array(1);
		gpu_max_lock = gpu_dvfs_get_max_lock();

		pr_info("[%s] gmc -----> time:%ds cpu_util_avg:%d gpu_util:%d gpu_freq:%d / sus_array0:%d sus_array1:%d gpu_maxlock:%d\n", 
				prefix, time_cnt++, cpu_util_avg, gpu_util, gpu_freq, sus_array[0], sus_array[1], gpu_max_lock);

		if (is_game) {

			if (delay_cnt++ >= maxlock_delay_sec) {
				pr_info("[%s] gmc [game] >>> pm_qos limit all %d %d %d %d %d\n", prefix, cl2_max, cl1_max, cl0_max, mif_max, mif_min);
				pm_qos_update_request(&pm_qos_cl2_max, cl2_max);
				pm_qos_update_request(&pm_qos_cl1_max, cl1_max);
				pm_qos_update_request(&pm_qos_cl0_max, cl0_max);
				pm_qos_update_request(&pm_qos_mif_max, mif_max);
				pm_qos_update_request(&pm_qos_mif_min, mif_min);
			}

#if 1
			//emstune_mode_change(NORMAL_MODE);
			//emstune_update_request(&emstune_req_gmc, 0);
#else
			if (gpu_freq <= gpu_lite) {
				// light game
				emstune_mode_change(LIGHT_GAME_MODE);
				emstune_update_request(&emstune_req_gmc, 0);
			} else {
				// heavy game
				emstune_mode_change(NORMAL_MODE);
				emstune_update_request(&emstune_req_gmc, 0);
			}
#endif
			prev_is_game = 1;

		} else {
			delay_cnt = 0;

			// release first
			pr_info("[%s] gmc [not game] >>> pm_qos release all\n", prefix);
			pm_qos_update_request(&pm_qos_cl2_max, PM_QOS_CLUSTER2_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&pm_qos_cl1_max, PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&pm_qos_cl0_max, PM_QOS_CLUSTER0_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&pm_qos_mif_max, PM_QOS_BUS_THROUGHPUT_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&pm_qos_mif_min, PM_QOS_BUS_THROUGHPUT_DEFAULT_VALUE);

			if (emstune_get_cur_mode() == LIGHT_GAME_MODE && prev_is_game) {
				emstune_mode_change(NORMAL_MODE);
				emstune_update_request(&emstune_req_gmc, 0);
			}
#if 0
			// cause: conflicting between MCD and slsi
			if (gpu_freq <= gpu_lite) {
				emstune_mode_change(NORMAL_MODE);
				emstune_update_request(&emstune_req_gmc, 0);
			} else {
				if (sus_array[0] > 0) {
					if (((gpu_freq <= sus_array[0]) || (gpu_max_lock == sus_array[0]))
							&& gpu_util > sus_array[1]) {
						emstune_mode_change(LIGHT_GAME_MODE);
						emstune_update_request(&emstune_req_gmc, 0);
					} else {
						emstune_mode_change(NORMAL_MODE);
						emstune_update_request(&emstune_req_gmc, 0);
					}
				}
			}
#endif
			prev_is_game = 0;
		}

		msleep(polling_ms);
	}

	return 0;
}

static struct task_struct *task;
static void gmc_start(void)
{
	if (is_running) {
		pr_err("[%s] already running!!\n", prefix);
		return;
	}
	// run
	task = kthread_create(gmc_thread, NULL, "exynos_gmc_thread%u", 0);
	kthread_bind_mask(task, cpu_coregroup_mask(bind_cpu));
	wake_up_process(task);
	return;
}

static void gmc_stop(void)
{
	is_running = 0;
	pr_info("[%s] gmc done\n", prefix);
}

/*********************************************************************
 *                          Sysfs interface                          *
 *********************************************************************/
//----------------------------------------
// DBGFS macro for MISC node
#define DBGFS_NODE(name) \
	static int name##_seq_show(struct seq_file *file, void *iter) {	\
		seq_printf(file, "%d\n", name);	\
		return 0;	\
	}	\
	static ssize_t name##_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off) {	\
		char buf[20];	\
		count = (count > 20)? 20 : count;	\
		if (copy_from_user(buf, buffer, count) != 0)	\
			return -EFAULT;	\
		sscanf(buf, "%d", &name);	\
		return count;	\
	}	\
	static int name##_debugfs_open(struct inode *inode, struct file *file) { \
		return single_open(file, name##_seq_show, inode->i_private);	\
	}	\
	static const struct file_operations name##_debugfs_fops = {	\
		.owner		= THIS_MODULE,	\
		.open		= name##_debugfs_open,	\
		.read		= seq_read,	\
		.write		= name##_seq_write,	\
		.llseek		= seq_lseek,	\
		.release	= single_release,	\
	};
DBGFS_NODE(is_game)

// misc
static struct miscdevice is_game_miscdev;

static int register_is_game_misc(void)
{
	int ret;
	is_game_miscdev.minor = MISC_DYNAMIC_MINOR;
	is_game_miscdev.name = "is_game";
	is_game_miscdev.fops = &is_game_debugfs_fops;
	ret = misc_register(&is_game_miscdev);
	return ret;
}

#define DEF_NODE(name) \
	static ssize_t show_##name(struct kobject *k, struct kobj_attribute *attr, char *buf) { \
		int ret = 0; \
		ret += sprintf(buf + ret, "%d", name); \
		return ret; } \
	static ssize_t store_##name(struct kobject *k, struct kobj_attribute *attr, const char *buf, size_t count) { \
		if (sscanf(buf, "%d", &name) != 1) \
			return -EINVAL; \
		return count; } \
	static struct kobj_attribute name##_attr = __ATTR(name, 0644, show_##name, store_##name);

DEF_NODE(is_game)
DEF_NODE(polling_ms)
DEF_NODE(bind_cpu)
DEF_NODE(cl2_max)
DEF_NODE(cl1_max)
DEF_NODE(cl0_max)
DEF_NODE(mif_max)
DEF_NODE(mif_min)
DEF_NODE(gpu_lite)
DEF_NODE(maxlock_delay_sec)

// run
static ssize_t show_run(struct kobject *k, struct kobj_attribute *attr, char *buf) 
{
	int ret = 0;
	ret += sprintf(buf + ret, "%d", run);
	return ret;
}
static ssize_t store_run(struct kobject *k, struct kobj_attribute *attr, const char *buf, size_t count)
{
	if (sscanf(buf, "%d", &run) != 1)
		return -EINVAL;
	if (run)
		gmc_start();
	else
		gmc_stop();
	return count;
}
static struct kobj_attribute run_attr = __ATTR(run, 0644, show_run, store_run);


/*--------------------------------------*/
// MAIN

static struct kobject *gmc_kobj;
static struct attribute *gmc_attrs[] = {
	&run_attr.attr,
	&polling_ms_attr.attr,
	&bind_cpu_attr.attr,
	&is_game_attr.attr,
	&cl2_max_attr.attr,
	&cl1_max_attr.attr,
	&cl0_max_attr.attr,
	&mif_max_attr.attr,
	&mif_min_attr.attr,
	&gpu_lite_attr.attr,
	&maxlock_delay_sec_attr.attr,
	NULL
};
static struct attribute_group gmc_group = {
	.attrs = gmc_attrs,
};

static int __init exynos_perf_gmc_init(void)
{
	struct device_node *dn = NULL;
	int ret;

	dn = of_find_node_by_name(dn, "exynos_perf_ncmemcpy");
	if (!dn) {
		printk("%s: exynos_perf_ncmemcpy node is not exist\n", __FILE__);
		return -EINVAL;
	}

	of_property_read_u32(dn, "cal-id-mif", &cal_id_mif);
	of_property_read_u32(dn, "cal-id-g3d", &cal_id_g3d);
	of_property_read_u32(dn, "devfreq-mif", &devfreq_mif);

	gmc_kobj = kobject_create_and_add("gmc", kernel_kobj);

	if (!gmc_kobj) {
		pr_info("[%s] gmc create node failed: %s\n", prefix, __FILE__);
		return -EINVAL;
	}

	ret = sysfs_create_group(gmc_kobj, &gmc_group);
	if (ret) {
		pr_info("[%s] gmc create group failed: %s\n", prefix, __FILE__);
		return -EINVAL;
	}

	register_is_game_misc();

	run = 1;
	gmc_start();

	return 0;
}
late_initcall(exynos_perf_gmc_init);
