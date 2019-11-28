/*
 * Frequency variant cpufreq driver
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd
 * Park Bumgyu <bumgyu.park@samsung.com>
 */

#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/reciprocal_div.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include <trace/events/ems.h>
#include <trace/events/ems_debug.h>

#include <dt-bindings/soc/samsung/ems.h>

#include "../sched.h"
#include "ems.h"

#define NEED_OVERRIDING	(0xBABECAFE)

static char *stune_group_name[] = {
	"root",
	"foreground",
	"background",
	"top-app",
	"rt",
};

#define NONE 0	/* none compensation */
#define NEGATIVE 1	/* negative compensation */

static bool emstune_initialized = false;

static struct emstune_mode *emstune_modes;
static struct emstune_mode cur_mode;

static int max_mode_idx;
static int boost_mode_idx;

static struct kobject *emstune_kobj;

DEFINE_PER_CPU(int, freq_boost);	/* maximum boot ratio of cpu */

/**********************************************************************
 * common APIs                                                        *
 **********************************************************************/
#define DEFAULT_MODE	(0)
static void emstune_update_ontime(int target_mode_idx)
{
	int cpu, idx;
	struct emstune_mode *default_mode = &emstune_modes[DEFAULT_MODE];
	struct emstune_mode *target_mode = &emstune_modes[target_mode_idx];

	/* Update ontime_enabled */
	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++) {
		if (emstune_modes[target_mode_idx].groups[idx].ontime_enabled == NEED_OVERRIDING)
			cur_mode.groups[idx].ontime_enabled = default_mode->groups[idx].ontime_enabled;
		else
			cur_mode.groups[idx].ontime_enabled = target_mode->groups[idx].ontime_enabled;
	}

	/* Update upper_boundary */
	if (emstune_modes[target_mode_idx].ontime[0].upper_boundary == NEED_OVERRIDING) {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].upper_boundary =
					default_mode->ontime[cpu].upper_boundary;
	} else {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].upper_boundary =
					target_mode->ontime[cpu].upper_boundary;
	}

	/* Update lower_boundary */
	if (emstune_modes[target_mode_idx].ontime[0].lower_boundary == NEED_OVERRIDING) {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].lower_boundary =
					default_mode->ontime[cpu].lower_boundary;
	} else {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].lower_boundary =
					target_mode->ontime[cpu].lower_boundary;
	}

	/* Update upper_boundary_s */
	if (emstune_modes[target_mode_idx].ontime[0].upper_boundary_s == NEED_OVERRIDING) {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].upper_boundary_s =
					default_mode->ontime[cpu].upper_boundary_s;
	} else {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].upper_boundary_s =
					target_mode->ontime[cpu].upper_boundary_s;
	}

	/* Update lower_boundary_s */
	if (emstune_modes[target_mode_idx].ontime[0].lower_boundary_s == NEED_OVERRIDING) {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].lower_boundary_s =
					default_mode->ontime[cpu].lower_boundary_s;
	} else {
		for_each_possible_cpu(cpu)
			cur_mode.ontime[cpu].lower_boundary_s =
					target_mode->ontime[cpu].lower_boundary_s;
	}
}

static void emstune_update_cur_mode(int target_mode_idx)
{
	int cpu, idx;

	cur_mode.idx = emstune_modes[target_mode_idx].idx;
	cur_mode.desc = emstune_modes[target_mode_idx].desc;
	cur_mode.prefer_idle = emstune_modes[target_mode_idx].prefer_idle;

	/* Update target_sched_class of this mode */
	if (emstune_modes[target_mode_idx].target_sched_class == NEED_OVERRIDING)
		cur_mode.target_sched_class = emstune_modes[DEFAULT_MODE].target_sched_class;
	else
		cur_mode.target_sched_class = emstune_modes[target_mode_idx].target_sched_class;

	/* Update gov_data of this mode */
	if (emstune_modes[target_mode_idx].gov_data[0].step == NEED_OVERRIDING) {
		for_each_possible_cpu(cpu)
			cur_mode.gov_data[cpu].step = emstune_modes[DEFAULT_MODE].gov_data[cpu].step;
	} else {
		for_each_possible_cpu(cpu)
			cur_mode.gov_data[cpu].step = emstune_modes[target_mode_idx].gov_data[cpu].step;
	}

	/* Update ontime threshold of this mode */
	emstune_update_ontime(target_mode_idx);

	/* Update util-est enabled */
	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++) {
		if (emstune_modes[target_mode_idx].groups[idx].util_est_enabled == NEED_OVERRIDING)
			cur_mode.groups[idx].util_est_enabled =
					emstune_modes[DEFAULT_MODE].groups[idx].util_est_enabled;
		else
			cur_mode.groups[idx].util_est_enabled =
					emstune_modes[target_mode_idx].groups[idx].util_est_enabled;
	}

	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++) {
		struct emstune_group *cur_group = &cur_mode.groups[idx];
		struct emstune_group *target_group = &emstune_modes[target_mode_idx].groups[idx];
		struct emstune_group *default_group = &emstune_modes[DEFAULT_MODE].groups[idx];
		struct emstune_dom **cur_dom = cur_group->dom;
		struct emstune_dom **target_dom = target_group->dom;
		struct emstune_dom **default_dom = default_group->dom;

		/* Update candidate cpus for this cgroup */
		if (cpumask_empty(&target_group->cpus_allowed))
			cpumask_copy(&cur_group->cpus_allowed, &default_group->cpus_allowed);
		else
			cpumask_copy(&cur_group->cpus_allowed, &target_group->cpus_allowed);

		/* Update weight to calculate efficiency */
		if (target_dom[0]->weight == NEED_OVERRIDING) {
			for_each_possible_cpu(cpu)
				cur_dom[cpu]->weight = default_dom[cpu]->weight;
		} else {
			for_each_possible_cpu(cpu)
				cur_dom[cpu]->weight = target_dom[cpu]->weight;
		}

		/* Update idle-weight to calculate efficiency */
		if (target_dom[0]->idle_weight == NEED_OVERRIDING) {
			for_each_possible_cpu(cpu)
				cur_dom[cpu]->idle_weight = default_dom[cpu]->idle_weight;
		} else {
			for_each_possible_cpu(cpu)
				cur_dom[cpu]->idle_weight = target_dom[cpu]->idle_weight;
		}

		/* Update freq boost to control frequency ramp-up */
		if (target_dom[0]->freq_boost == NEED_OVERRIDING) {
			for_each_possible_cpu(cpu)
				cur_dom[cpu]->freq_boost = default_dom[cpu]->freq_boost;
		} else {
			for_each_possible_cpu(cpu)
				cur_dom[cpu]->freq_boost = target_dom[cpu]->freq_boost;
		}
	}
}

extern struct reciprocal_value schedtune_spc_rdiv;
unsigned long emstune_freq_boost(int cpu, unsigned long util)
{
	int boost = per_cpu(freq_boost, cpu);
	unsigned long capacity = capacity_cpu(cpu, 0);
	unsigned long boosted_util = 0;
	long long margin = 0;

	if (!boost)
		return util;

	/*
	 * Signal proportional compensation (SPC)
	 *
	 * The Boost (B) value is used to compute a Margin (M) which is
	 * proportional to the complement of the original Signal (S):
	 *   M = B * (capacity - S)
	 * The obtained M could be used by the caller to "boost" S.
	 */
	if (boost >= 0) {
		if (capacity > util) {
			margin  = capacity - util;
			margin *= boost;
		} else
			margin  = 0;
	} else
		margin = -util * boost;

	margin  = reciprocal_divide(margin, schedtune_spc_rdiv);

	if (boost < 0)
		margin *= -1;

	boosted_util = util + margin;

	trace_emstune_freq_boost(cpu, boost, util, boosted_util);

	return boosted_util;
}

/* Update maximum values of boost groups of this cpu */
void emstune_cpu_update(int cpu, u64 now)
{
	int idx, freq_boost_max = 0;
	struct emstune_group *groups;

	if (unlikely(!emstune_initialized))
		return;

	groups = cur_mode.groups;

	/* The root boost group is always active */
	freq_boost_max = groups[0].dom[cpu]->freq_boost;
	for (idx = 1; idx < STUNE_GROUP_COUNT; ++idx) {
		int val;

		if (!schedtune_cpu_boost_group_active(idx, cpu, now))
			continue;

		val = groups[idx].dom[cpu]->freq_boost;
		if (freq_boost_max < val)
			freq_boost_max = val;
	}

	/*
	 * Ensures grp_boost_max is non-negative when all cgroup boost values
	 * are neagtive. Avoids under-accounting of cpu capacity which may cause
	 * task stacking and frequency spikes.
	 */
	per_cpu(freq_boost, cpu) = freq_boost_max;
}

#define DEFAULT_WEIGHT	(100)
int emstune_eff_weight(struct task_struct *p, int cpu, int idle)
{
	int st_idx, weight;
	struct emstune_group *groups;
	struct emstune_dom *dom;

	if (unlikely(!emstune_initialized))
		return DEFAULT_WEIGHT;

	st_idx = schedtune_task_group_idx(p);
	groups = cur_mode.groups;
	dom = groups[st_idx].dom[cpu];

	if (idle)
		weight = dom->idle_weight;
	else
		weight = dom->weight;

	return weight;
}

static int get_sched_class_idx(const struct sched_class *class)
{
	if (class == &stop_sched_class)
		return EMS_SCHED_STOP;

	if (class == &dl_sched_class)
		return EMS_SCHED_DL;

	if (class == &rt_sched_class)
		return EMS_SCHED_RT;

	if (class == &fair_sched_class)
		return EMS_SCHED_FAIR;

	if (class == &idle_sched_class)
		return EMS_SCHED_IDLE;

	return NUM_OF_SCHED_CLASS;
}

const struct cpumask *emstune_cpus_allowed(struct task_struct *p)
{
	int st_idx;
	struct emstune_group *group;
	int class_idx;

	if (unlikely(!emstune_initialized))
		return cpu_active_mask;

	st_idx = schedtune_task_group_idx(p);
	group = &cur_mode.groups[st_idx];

	if (unlikely(cpumask_empty(&group->cpus_allowed)))
		return cpu_active_mask;

	class_idx = get_sched_class_idx(p->sched_class);
	if (!(cur_mode.target_sched_class & (1 << class_idx)))
		return cpu_active_mask;

	return &group->cpus_allowed;
}

bool emstune_can_migrate_task(struct task_struct *p, int dst_cpu)
{
	return cpumask_test_cpu(dst_cpu, emstune_cpus_allowed(p));
}

int emstune_prefer_idle(void)
{
	if (unlikely(!emstune_initialized))
		return 0;

	return cur_mode.prefer_idle;
}

int emstune_ontime(struct task_struct *p)
{
	int st_idx;

	if (unlikely(!emstune_initialized))
		return 0;

	st_idx = schedtune_task_group_idx(p);

	return cur_mode.groups[st_idx].ontime_enabled;
}

int emstune_util_est(struct task_struct *p)
{
	int st_idx;

	if (unlikely(!emstune_initialized))
		return 0;

	st_idx = schedtune_task_group_idx(p);

	return cur_mode.groups[st_idx].util_est_enabled;
}

int emstune_util_est_group(int st_idx)
{
	if (unlikely(!emstune_initialized))
		return 0;

	return cur_mode.groups[st_idx].util_est_enabled;
}

/**********************************************************************
 * Update event handler                                               *
 **********************************************************************/
static RAW_NOTIFIER_HEAD(emstune_mode_update_notifier_list);

/* emstune_mode_lock *MUST* be held before notifying */
static int emstune_notify_mode_update(void)
{
	int ret;

	ret = raw_notifier_call_chain(&emstune_mode_update_notifier_list, 0, &cur_mode);

	return 0;
}

int emstune_register_mode_update_notifier(struct notifier_block *nb)
{
	int ret;

	ret = raw_notifier_chain_register(&emstune_mode_update_notifier_list, nb);

	return ret;
}

int emstune_unregister_mode_update_notifier(struct notifier_block *nb)
{
	int ret;

	ret = raw_notifier_chain_unregister(&emstune_mode_update_notifier_list, nb);

	return ret;
}

/**********************************************************************
 * Multi requests interface (Platform/Kernel)                         *
 **********************************************************************/
struct emstune_mode_object {
	char *name;
	struct miscdevice emstune_mode_miscdev;
};
static struct emstune_mode_object emstune_mode_obj;

static DEFINE_SPINLOCK(emstune_mode_lock);

static struct plist_head emstune_req_list = PLIST_HEAD_INIT(emstune_req_list);

static int emstune_get_mode(void)
{
	if (plist_head_empty(&emstune_req_list))
		return 0;

	return plist_last(&emstune_req_list)->prio;
}

static int emstune_mode_request_active(struct emstune_mode_request *req)
{
	unsigned long flags;
	int active;

	spin_lock_irqsave(&emstune_mode_lock, flags);
	active = req->active;
	spin_unlock_irqrestore(&emstune_mode_lock, flags);

	return active;
}

static void emstune_work_fn(struct work_struct *work)
{
	struct emstune_mode_request *req = container_of(to_delayed_work(work),
						  struct emstune_mode_request,
						  work);

	emstune_update_request(req, 0);
}

void __emstune_add_request(struct emstune_mode_request *req, char *func, unsigned int line)
{
	if (emstune_mode_request_active(req))
		return;

	INIT_DELAYED_WORK(&req->work, emstune_work_fn);
	req->func = func;
	req->line = line;

	emstune_update_request(req, 0);
}

void emstune_remove_request(struct emstune_mode_request *req)
{
	unsigned long flags;

	if (!emstune_mode_request_active(req))
		return;

	if (delayed_work_pending(&req->work))
		cancel_delayed_work_sync(&req->work);
	destroy_delayed_work_on_stack(&req->work);

	emstune_update_request(req, 0);

	spin_lock_irqsave(&emstune_mode_lock, flags);
	req->active = 0;
	plist_del(&req->node, &emstune_req_list);
	spin_unlock_irqrestore(&emstune_mode_lock, flags);
}

void emstune_update_request(struct emstune_mode_request *req, s32 new_value)
{
	unsigned long flags;
	int next_mode_idx;

	/* ignore if the request is active and the value does not change */
	if (req->active && req->node.prio == new_value)
		return;

	/* ignore if the value is out of range */
	if (new_value < 0 || new_value > (s32)max_mode_idx)
		return;

	spin_lock_irqsave(&emstune_mode_lock, flags);

	/*
	 * If the request already added to the list updates the value, remove
	 * the request from the list and add it again.
	 */
	if (req->active)
		plist_del(&req->node, &emstune_req_list);
	else
		req->active = 1;

	plist_node_init(&req->node, new_value);
	plist_add(&req->node, &emstune_req_list);

	next_mode_idx = emstune_get_mode();
	if (cur_mode.idx != next_mode_idx) {
		emstune_update_cur_mode(next_mode_idx);
		emstune_notify_mode_update();
		trace_emstune_mode(cur_mode.idx, next_mode_idx);
	}

	spin_unlock_irqrestore(&emstune_mode_lock, flags);
}

void emstune_update_request_timeout(struct emstune_mode_request *req, s32 new_value,
				unsigned long timeout_us)
{
	if (delayed_work_pending(&req->work))
		cancel_delayed_work_sync(&req->work);

	emstune_update_request(req, new_value);

	schedule_delayed_work(&req->work, usecs_to_jiffies(timeout_us));
}

void emstune_boost(struct emstune_mode_request *req, int enable)
{
	if (enable) {
		emstune_add_request(req);
		emstune_update_request(req, boost_mode_idx);
	} else {
		emstune_update_request(req, 0);
		emstune_remove_request(req);
	}
}

void emstune_boost_timeout(struct emstune_mode_request *req, unsigned long timeout_us)
{
	emstune_update_request_timeout(req, boost_mode_idx, timeout_us);
}

static int emstune_mode_open(struct inode *inode, struct file *filp)
{
	struct emstune_mode_request *req;

	req = kzalloc(sizeof(struct emstune_mode_request), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	filp->private_data = req;

	return 0;
}

static int emstune_mode_release(struct inode *inode, struct file *filp)
{
	struct emstune_mode_request *req;

	req = filp->private_data;
	emstune_remove_request(req);
	kfree(req);

	return 0;
}

static ssize_t emstune_mode_read(struct file *filp, char __user *buf,
		size_t count, loff_t *f_pos)
{
	s32 value;
	unsigned long flags;

	spin_lock_irqsave(&emstune_mode_lock, flags);
	value = emstune_get_mode();
	spin_unlock_irqrestore(&emstune_mode_lock, flags);

	return simple_read_from_buffer(buf, count, f_pos, &value, sizeof(s32));
}

static ssize_t emstune_mode_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	s32 value;
	struct emstune_mode_request *req;

	if (count == sizeof(s32)) {
		if (copy_from_user(&value, buf, sizeof(s32)))
			return -EFAULT;
	} else {
		int ret;

		ret = kstrtos32_from_user(buf, count, 16, &value);
		if (ret)
			return ret;
	}

	req = filp->private_data;
	emstune_update_request(req, value);

	return count;
}

static const struct file_operations emstune_mode_fops = {
	.write = emstune_mode_write,
	.read = emstune_mode_read,
	.open = emstune_mode_open,
	.release = emstune_mode_release,
	.llseek = noop_llseek,
};

static int register_emstune_mode_misc(void)
{
	int ret;

	emstune_mode_obj.emstune_mode_miscdev.minor = MISC_DYNAMIC_MINOR;
	emstune_mode_obj.emstune_mode_miscdev.name = "mode";
	emstune_mode_obj.emstune_mode_miscdev.fops = &emstune_mode_fops;

	ret = misc_register(&emstune_mode_obj.emstune_mode_miscdev);

	return ret;
}

/**********************************************************************
 * Sysfs	                                                      *
 **********************************************************************/
struct emstune_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *, char *);
	ssize_t (*store)(struct kobject *, const char *, size_t count);
};

#define emstune_attr_rw(name)								\
static struct emstune_attr name##_attr =						\
__ATTR(name, 0644, show_##name, store_##name)

#define emstune_show(name, type)							\
static ssize_t show_##name(struct kobject *k, char *buf)				\
{											\
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);	\
	int cpu, ret = 0;								\
											\
	for_each_possible_cpu(cpu) {							\
		if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))			\
			continue;							\
		ret += sprintf(buf + ret, "cpu:%d ratio%3d\n",				\
					cpu, group->dom[cpu]->type);			\
	}										\
											\
	return ret;									\
}

#define emstune_store(name, type)							\
static ssize_t store_##name(struct kobject *k, const char *buf, size_t count)		\
{											\
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);	\
	int cpu, val, tmp;								\
											\
	if (sscanf(buf, "%d %d", &cpu, &val) != 2)					\
		return -EINVAL;								\
											\
	if (cpu < 0 || cpu >= NR_CPUS || val < -200 || val > 500)			\
		return -EINVAL;								\
											\
	for_each_cpu(tmp, cpu_coregroup_mask(cpu))					\
		group->dom[tmp]->type = val;						\
											\
	emstune_update_cur_mode(cur_mode.idx);						\
											\
	return count;									\
}

#define emstune_show_weight(name, type)							\
static ssize_t show_##name(struct kobject *k, char *buf)				\
{											\
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);	\
	int cpu, ret = 0;								\
											\
	for_each_possible_cpu(cpu) {							\
		ret += sprintf(buf + ret, "cpu:%d weight:%3d\n",			\
					cpu, group->dom[cpu]->type);			\
	}										\
											\
	return ret;									\
}

#define emstune_store_weight(name, type)						\
static ssize_t store_##name(struct kobject *k, const char *buf, size_t count)		\
{											\
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);	\
	int cpu, val;								\
											\
	if (sscanf(buf, "%d %d", &cpu, &val) != 2)					\
		return -EINVAL;								\
											\
	if (cpu < 0 || cpu >= NR_CPUS || val < 0)					\
		return -EINVAL;								\
											\
	group->dom[cpu]->type = val;							\
											\
	emstune_update_cur_mode(cur_mode.idx);						\
											\
	return count;									\
}

emstune_store(freq_boost, freq_boost);
emstune_show(freq_boost, freq_boost);
emstune_attr_rw(freq_boost);

emstune_store_weight(weight, weight);
emstune_show_weight(weight, weight);
emstune_attr_rw(weight);

emstune_store_weight(idle_weight, idle_weight);
emstune_show_weight(idle_weight, idle_weight);
emstune_attr_rw(idle_weight);

#define STR_LEN 10
static ssize_t
store_cpus_allowed(struct kobject *k, const char *buf, size_t count)
{
	char str[STR_LEN];
	int i;
	struct cpumask cpus_allowed;
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);

	if (strlen(buf) >= STR_LEN)
		return -EINVAL;

	if (!sscanf(buf, "%s", str))
		return -EINVAL;

	if (str[0] == '0' && str[1] == 'x') {
		for (i = 0; i+2 < STR_LEN; i++) {
			str[i] = str[i + 2];
			str[i+2] = '\n';
		}
	}

	cpumask_parse(str, &cpus_allowed);
	cpumask_copy(&group->cpus_allowed, &cpus_allowed);

	emstune_update_cur_mode(cur_mode.idx);

	return count;
}

static ssize_t
show_cpus_allowed(struct kobject *k, char *buf)
{
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);
	return snprintf(buf, 30, "cpus_allowed : 0x%x\n",
		*(unsigned int *)cpumask_bits(&group->cpus_allowed));
}

emstune_attr_rw(cpus_allowed);

static ssize_t
store_ontime_enabled(struct kobject *k, const char *buf, size_t count)
{
	int input;
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);

	if (sscanf(buf, "%d", &input) != 1)
		return -EINVAL;

	group->ontime_enabled = !!input;

	emstune_update_cur_mode(cur_mode.idx);

	return count;
}

static ssize_t
show_ontime_enabled(struct kobject *k, char *buf)
{
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);

	return snprintf(buf, 30, "%d\n", group->ontime_enabled);
}

emstune_attr_rw(ontime_enabled);

static ssize_t
store_util_est_enabled(struct kobject *k, const char *buf, size_t count)
{
	int input;
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);

	if (sscanf(buf, "%d", &input) != 1)
		return -EINVAL;

	group->util_est_enabled = !!input;

	emstune_update_cur_mode(cur_mode.idx);

	return count;
}

static ssize_t
show_util_est_enabled(struct kobject *k, char *buf)
{
	struct emstune_group *group = container_of(k, struct emstune_group, kobj);

	return snprintf(buf, 30, "%d\n", group->util_est_enabled);
}

emstune_attr_rw(util_est_enabled);

static ssize_t show(struct kobject *kobj, struct attribute *at, char *buf)
{
	struct emstune_attr *attr = container_of(at, struct emstune_attr, attr);
	return attr->show(kobj, buf);
}

static ssize_t store(struct kobject *kobj, struct attribute *at,
					const char *buf, size_t count)
{
	struct emstune_attr *attr = container_of(at, struct emstune_attr, attr);
	return attr->store(kobj, buf, count);
}

static const struct sysfs_ops emstune_sysfs_ops = {
	.show	= show,
	.store	= store,
};

static struct attribute *groups_attrs[] = {
	&freq_boost_attr.attr,
	&weight_attr.attr,
	&idle_weight_attr.attr,
	&cpus_allowed_attr.attr,
	&ontime_enabled_attr.attr,
	&util_est_enabled_attr.attr,
	NULL
};

static struct kobj_type ktype_groups = {
	.sysfs_ops	= &emstune_sysfs_ops,
	.default_attrs	= groups_attrs,
};

static ssize_t
show_mode_idx(struct kobject *k, char *buf)
{
	struct emstune_mode *mode = container_of(k, struct emstune_mode, kobj);
	int ret;

	ret = sprintf(buf, "%d\n", mode->idx);

	return ret;
}

static struct emstune_attr mode_idx_attr =
__ATTR(idx, 0444, show_mode_idx, NULL);

static ssize_t
show_mode_desc(struct kobject *k, char *buf)
{
	struct emstune_mode *mode = container_of(k, struct emstune_mode, kobj);
	int ret;

	ret = sprintf(buf, "%s\n", mode->desc);

	return ret;
}

static struct emstune_attr mode_desc_attr =
__ATTR(desc, 0444, show_mode_desc, NULL);

static ssize_t
show_mode_prefer_idle(struct kobject *k, char *buf)
{
	struct emstune_mode *mode = container_of(k, struct emstune_mode, kobj);
	int ret;

	ret = sprintf(buf, "%d\n", mode->prefer_idle);

	return ret;
}

static ssize_t
store_mode_prefer_idle(struct kobject *k, const char *buf, size_t count)
{
	int prefer_idle;
	struct emstune_mode *mode = container_of(k, struct emstune_mode, kobj);

	if (sscanf(buf, "%d", &prefer_idle) != 1)
		return -EINVAL;

	mode->prefer_idle = !!prefer_idle;

	emstune_update_cur_mode(cur_mode.idx);

	return count;
}

static struct emstune_attr mode_prefer_idle_attr =
__ATTR(prefer_idle, 0644, show_mode_prefer_idle, store_mode_prefer_idle);

static ssize_t
store_mode_target_sched_class(struct kobject *k, const char *buf, size_t count)
{
	struct emstune_mode *mode = container_of(k, struct emstune_mode, kobj);
	char str[STR_LEN];
	int i;
	long cand_class;

	if (strlen(buf) >= STR_LEN)
		return -EINVAL;

	if (!sscanf(buf, "%s", str))
		return -EINVAL;

	if (str[0] == '0' && str[1] == 'x') {
		for (i = 0; i+2 < STR_LEN; i++) {
			str[i] = str[i + 2];
			str[i+2] = '\n';
		}
	}

	kstrtol(str, 16, &cand_class);
	cand_class &= EMS_SCHED_CLASS_MASK;

	mode->target_sched_class = 0;

	for (i = 0; i < NUM_OF_SCHED_CLASS; i++)
		if (cand_class & (1 << i))
			mode->target_sched_class |= 1 << i;

	emstune_update_cur_mode(cur_mode.idx);

	return count;
}

static ssize_t
show_mode_target_sched_class(struct kobject *k, char *buf)
{
	struct emstune_mode *mode = container_of(k, struct emstune_mode, kobj);
	int ret = 0;

	ret = sprintf(buf, "%#01x\n", mode->target_sched_class);

	if (mode->target_sched_class & (1 << EMS_SCHED_STOP))
		ret += sprintf(buf + ret, "%s\n", "STOP");
	if (mode->target_sched_class & (1 << EMS_SCHED_DL))
		ret += sprintf(buf + ret, "%s\n", "DL");
	if (mode->target_sched_class & (1 << EMS_SCHED_RT))
		ret += sprintf(buf + ret, "%s\n", "RT");
	if (mode->target_sched_class & (1 << EMS_SCHED_FAIR))
		ret += sprintf(buf + ret, "%s\n", "FAIR");
	if (mode->target_sched_class & (1 << EMS_SCHED_IDLE))
		ret += sprintf(buf + ret, "%s\n", "IDLE");

	return ret;
}

static struct emstune_attr mode_target_sched_class_attr =
__ATTR(target_sched_class, 0644, show_mode_target_sched_class, store_mode_target_sched_class);

static struct attribute *emstune_mode_attrs[] = {
	&mode_idx_attr.attr,
	&mode_desc_attr.attr,
	&mode_prefer_idle_attr.attr,
	&mode_target_sched_class_attr.attr,
	NULL
};

static struct kobj_type ktype_emstune_mode = {
	.sysfs_ops	= &emstune_sysfs_ops,
	.default_attrs	= emstune_mode_attrs,
};

static ssize_t
show_esg_step(struct kobject *k, char *buf)
{
	struct emstune_mode *mode = container_of(k, struct emstune_mode, gov_data_kobj);
	int cpu, ret = 0;

	for_each_possible_cpu(cpu) {
		if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
			continue;

		ret += sprintf(buf + ret, "cpu%d: step=%2d\n", cpu, mode->gov_data[cpu].step);
	}

	return ret;
}

static ssize_t
store_esg_step(struct kobject *k, const char *buf, size_t count)
{
	int cpu, step, cursor_cpu;
	unsigned long flags;
	struct emstune_mode *mode = container_of(k, struct emstune_mode, gov_data_kobj);

	if (sscanf(buf, "%d %d", &cpu, &step) != 2)
		return -EINVAL;

	cpu = cpumask_first(cpu_coregroup_mask(cpu));

	for_each_cpu(cursor_cpu, cpu_coregroup_mask(cpu))
		mode->gov_data[cursor_cpu].step = step;

	spin_lock_irqsave(&emstune_mode_lock, flags);
	if (mode->idx == cur_mode.idx)
		emstune_update_cur_mode(cur_mode.idx);
	spin_unlock_irqrestore(&emstune_mode_lock, flags);

	return count;
}

static struct emstune_attr esg_step_attr =
__ATTR(esg_step, 0644, show_esg_step, store_esg_step);

static struct attribute *emstune_gov_data_attrs[] = {
	&esg_step_attr.attr,
	NULL
};

static struct kobj_type ktype_emstune_gov_data = {
	.sysfs_ops	= &emstune_sysfs_ops,
	.default_attrs	= emstune_gov_data_attrs,
};

static unsigned long read_upper_boundary(struct emstune_ontime *ontime, int cpu)
{
	return capacity_cpu(cpu, 0) * ontime->upper_boundary / 100;
}

static unsigned long read_upper_boundary_s(struct emstune_ontime *ontime, int cpu)
{
	return capacity_cpu(cpu, 1) * ontime->upper_boundary_s / 100;
}

static unsigned long read_lower_boundary(struct emstune_ontime *ontime, int cpu)
{
	return capacity_cpu(cpu, 0) * ontime->lower_boundary / 100;
}

static unsigned long read_lower_boundary_s(struct emstune_ontime *ontime, int cpu)
{
	return capacity_cpu(cpu, 1) * ontime->lower_boundary_s / 100;
}

#define show_store_attr(_name, _type, _max)						\
static ssize_t show_##_name(struct kobject *k, char *buf)				\
{											\
	int cpu, cnt = 0, ret = 0;							\
	struct emstune_mode *mode = container_of(k, struct emstune_mode, ontime_kobj);	\
											\
	for_each_possible_cpu(cpu) {							\
		if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))			\
			continue;							\
											\
		ret += sprintf(buf + ret, "coregroup%d: %u%% (cap=%u)\n",		\
				cnt++,							\
				(unsigned int)mode->ontime[cpu]._name,			\
				(unsigned int)read_##_name(&mode->ontime[cpu], cpu));	\
	}										\
											\
	return ret;									\
}											\
											\
static ssize_t store_##_name(struct kobject *k, const char *buf, size_t count)		\
{											\
	int cpu;									\
	unsigned int val;								\
	struct emstune_mode *mode = container_of(k, struct emstune_mode, ontime_kobj);	\
											\
	if (!sscanf(buf, "%d, %u", &cpu, &val))						\
		return -EINVAL;								\
											\
	if (cpu < 0 || cpu >= NR_CPUS)							\
		return -EINVAL;								\
											\
	val = val > _max ? _max : val;							\
	mode->ontime[cpu]._name = (_type)val;						\
											\
	emstune_update_cur_mode(cur_mode.idx);						\
											\
	return count;									\
}											\
											\
static struct emstune_attr _name##_attr =						\
__ATTR(_name, 0644, show_##_name, store_##_name)

show_store_attr(upper_boundary, unsigned long, 100);
show_store_attr(lower_boundary, unsigned long, 100);
show_store_attr(upper_boundary_s, unsigned long, 100);
show_store_attr(lower_boundary_s, unsigned long, 100);

static struct attribute *emstune_ontime_attrs[] = {
	&upper_boundary_attr.attr,
	&lower_boundary_attr.attr,
	&upper_boundary_s_attr.attr,
	&lower_boundary_s_attr.attr,
	NULL
};

static struct kobj_type ktype_emstune_ontime = {
	.sysfs_ops	= &emstune_sysfs_ops,
	.default_attrs	= emstune_ontime_attrs,
};

static struct emstune_mode_request emstune_user_req;
static ssize_t
show_user_mode(struct kobject *k, struct kobj_attribute *attr, char *buf)
{
	int ret;

	if (!emstune_user_req.active)
		ret = sprintf(buf, "user request has never come\n");
	else
		ret = sprintf(buf, "req_mode:%d (%s:%d)\n",
					(emstune_user_req.node).prio,
					emstune_user_req.func,
					emstune_user_req.line);

	return ret;
}

static ssize_t
store_user_mode(struct kobject *k, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int mode;

	if (sscanf(buf, "%d", &mode) != 1)
		return -EINVAL;

	/* ignore if requested mode is out of range */
	if (mode < 0 || mode > max_mode_idx)
		return -EINVAL;

	emstune_update_request(&emstune_user_req, mode);

	return count;
}

static struct kobj_attribute user_mode_attr =
__ATTR(user_mode, 0644, show_user_mode, store_user_mode);

static ssize_t
show_req_mode(struct kobject *k, struct kobj_attribute *attr, char *buf)
{
	struct emstune_mode_request *req;
	int ret = 0;
	int tot_reqs = 0;
	unsigned long flags;

	spin_lock_irqsave(&emstune_mode_lock, flags);
	plist_for_each_entry(req, &emstune_req_list, node) {
		tot_reqs++;
		ret += sprintf(buf + ret, "%d: %d (%s:%d)\n", tot_reqs,
							(req->node).prio,
							req->func,
							req->line);
	}

	ret += sprintf(buf + ret, "Current mode: %d, Requests: total=%d\n",
							emstune_get_mode(), tot_reqs);
	spin_unlock_irqrestore(&emstune_mode_lock, flags);

	return ret;
}
static struct kobj_attribute req_mode_attr =
__ATTR(req_mode, 0444, show_req_mode, NULL);

static ssize_t
show_cur_mode(struct kobject *k, struct kobj_attribute *attr, char *buf)
{
	int ret = 0;
	int cpu, idx;

	ret += sprintf(buf + ret, "Current mode: %d (%s)\n", cur_mode.idx, cur_mode.desc);
	ret += sprintf(buf + ret, "prefer-idle: %d\n", cur_mode.prefer_idle);
	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++) {
		ret += sprintf(buf + ret, "\n\"%s\" group info\n", stune_group_name[idx]);

		/* Running Weight */
		ret += sprintf(buf + ret, "[Weight]\n");
		for_each_possible_cpu(cpu)
			ret += sprintf(buf + ret, "cpu:%d weight:%3d\n",
						cpu, cur_mode.groups[idx].dom[cpu]->weight);

		/* Idle Weight */
		ret += sprintf(buf + ret, "[Idle weight]\n");
		for_each_possible_cpu(cpu)
			ret += sprintf(buf + ret, "cpu:%d weight:%3d\n",
						cpu, cur_mode.groups[idx].dom[cpu]->idle_weight);

		/* Freq Boost */
		ret += sprintf(buf + ret, "[Freq boost]\n");
		for_each_possible_cpu(cpu) {
			if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
				continue;
			ret += sprintf(buf + ret, "cpu:%d ratio:%3d\n",
						cpu, cur_mode.groups[idx].dom[cpu]->freq_boost);
		}

		/* ESG step */
		ret += sprintf(buf + ret, "[ESG step]\n");
		for_each_possible_cpu(cpu) {
			if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
				continue;
			ret += sprintf(buf + ret, "cpu:%d step:%3d\n",
						cpu, cur_mode.gov_data[cpu].step);
		}

		/* Ontime migration */
		ret += sprintf(buf + ret, "[Ontime migration]\n");
		if (cur_mode.groups[idx].ontime_enabled) {
			int cnt = 0;
			ret += sprintf(buf + ret, "enabled\n");
			for_each_possible_cpu(cpu) {
				if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
					continue;
				ret += sprintf(buf + ret, "coregroup%d upper:%3d lower:%3d ",
							cnt++, cur_mode.ontime[cpu].upper_boundary,
							cur_mode.ontime[cpu].lower_boundary);
				ret += sprintf(buf + ret, "upper_s:%3d lower_s:%3d\n",
							cur_mode.ontime[cpu].upper_boundary_s,
							cur_mode.ontime[cpu].lower_boundary_s);
			}
		} else
			ret += sprintf(buf + ret, "disabled\n");

		/* Utilization est. */
		ret += sprintf(buf + ret, "[Utilization estimation]\n");
		ret += sprintf(buf + ret, "%s\n",
					cur_mode.groups[idx].util_est_enabled ? "enabled" : "disabled");

		/* Candidate CPUs */
		ret += sprintf(buf + ret, "[Candidate cpu]\n");
		ret += sprintf(buf + ret, "candidate cpu : 0x%x\n",
				*(unsigned int *)cpumask_bits(&cur_mode.groups[idx].cpus_allowed));

		/* Sched class for candidate CPUs */
		ret += sprintf(buf + ret, "[Candidate class]\n");
		if (cur_mode.target_sched_class & (1 << EMS_SCHED_STOP))
			ret += sprintf(buf + ret, "%s\n", "STOP");
		if (cur_mode.target_sched_class & (1 << EMS_SCHED_DL))
			ret += sprintf(buf + ret, "%s\n", "DL");
		if (cur_mode.target_sched_class & (1 << EMS_SCHED_RT))
			ret += sprintf(buf + ret, "%s\n", "RT");
		if (cur_mode.target_sched_class & (1 << EMS_SCHED_FAIR))
			ret += sprintf(buf + ret, "%s\n", "FAIR");
		if (cur_mode.target_sched_class & (1 << EMS_SCHED_IDLE))
			ret += sprintf(buf + ret, "%s\n", "IDLE");
	}

	return ret;
}
static struct kobj_attribute cur_mode_attr =
__ATTR(cur_mode, 0444, show_cur_mode, NULL);

/**********************************************************************
 * initialization                                                     *
 **********************************************************************/
#define parse_member(member)							\
static int									\
emstune_parse_##member(struct device_node *dn,					\
		struct emstune_dom **dom, char *grp_name, bool negative)	\
{										\
	int cpu, pivot;								\
	u32 val[NR_CPUS];							\
										\
	pivot = negative ? 100 : 0;						\
										\
	if (of_property_read_u32_array(dn, grp_name, val, NR_CPUS)) {		\
		pr_warn("%s: failed to get %s of %s\n",				\
					__func__, dn->name, grp_name);		\
		return -EINVAL;							\
	}									\
										\
	for_each_possible_cpu(cpu) 						\
		dom[cpu]->member = val[cpu] - pivot;				\
										\
	return 0;								\
}

parse_member(weight);
parse_member(idle_weight);
parse_member(freq_boost);

static int
emstune_parse_target_sched_class(struct device_node *dn, struct emstune_mode *mode)
{
	int class_cnt, i;
	u32 val[NUM_OF_SCHED_CLASS];

	mode->target_sched_class = 0;

	class_cnt = of_property_count_u32_elems(dn, "target-sched-class");
	if (class_cnt > 0
		&& !of_property_read_u32_array(dn, "target-sched-class", (u32 *)&val, class_cnt)) {
			for (i = 0; i < class_cnt; i++)
				mode->target_sched_class |= 1 << val[i];
	}

	return 0;
}

static int
emstune_parse_cpus_allowed(struct device_node *dn,
			struct emstune_group *group, char *grp_name)
{
	const char *val;
	struct cpumask mask;

	if (of_property_read_string(dn, grp_name, &val)) {
		pr_warn("%s: failed to get %s of %s\n",
					__func__, dn->name, grp_name);
		return -EINVAL;
	}

	cpulist_parse(val, &mask);
	cpumask_copy(&group->cpus_allowed, &mask);

	return 0;
}

static int get_coregroup_count(void)
{
	int cnt = 0, cpu;

	for_each_possible_cpu(cpu) {
		if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
			continue;

		cnt++;
	}

	return cnt;
}

static int
emstune_parse_gov_step(struct device_node *dn,
			struct emstune_mode *mode, char *data_name)
{
	int cpu, tmp, cnt;
	u32 val[NR_CPUS];
	int coregrp_cnt = get_coregroup_count();

	if (of_property_read_u32_array(dn, data_name, val, coregrp_cnt)) {
		pr_warn("%s: failed to get %s of %s\n",
				__func__, data_name, dn->name);
		return -EINVAL;
	}

	cnt = 0;
	for_each_possible_cpu(cpu) {
		if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
			continue;

		for_each_cpu(tmp, cpu_coregroup_mask(cpu))
			mode->gov_data[tmp].step = val[cnt];

		cnt++;
	}

	return 0;
}

static int emstune_parse_ontime(struct device_node *dn, struct emstune_mode *mode)
{
	struct device_node *coregroup;
	char name[16];
	u32 val[STUNE_GROUP_COUNT];
	int idx, cpu, tmp, cnt = 0, prop;

	if (of_property_read_u32_array(dn, "enabled", (u32 *)&val, STUNE_GROUP_COUNT))
		return -EINVAL;

	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++)
		mode->groups[idx].ontime_enabled = val[idx];

	for_each_possible_cpu(cpu) {
		if (cpu != cpumask_first(cpu_coregroup_mask(cpu)))
			continue;

		snprintf(name, sizeof(name), "coregroup%d", cnt++);
		coregroup = of_get_child_by_name(dn, name);
		if (!coregroup)
			continue;

		for_each_cpu(tmp, cpu_coregroup_mask(cpu)) {
			if (of_property_read_s32(coregroup, "upper-boundary", &prop))
				mode->ontime[tmp].upper_boundary = 100;
			else
				mode->ontime[tmp].upper_boundary = prop;

			if (of_property_read_s32(coregroup, "lower-boundary", &prop))
				mode->ontime[tmp].lower_boundary = 0;
			else
				mode->ontime[tmp].lower_boundary = prop;

			if (of_property_read_s32(coregroup, "upper-boundary-s", &prop))
				mode->ontime[tmp].upper_boundary_s = 100;
			else
				mode->ontime[tmp].upper_boundary_s = prop;

			if (of_property_read_s32(coregroup, "lower-boundary-s", &prop))
				mode->ontime[tmp].lower_boundary_s = 0;
			else
				mode->ontime[tmp].lower_boundary_s = prop;
		}
	}

	return 0;
}

static int emstune_parse_util_est(struct device_node *dn, struct emstune_mode *mode)
{
	u32 val[STUNE_GROUP_COUNT];
	int idx;

	if (of_property_read_u32_array(dn, "enabled", (u32 *)&val, STUNE_GROUP_COUNT))
		return -EINVAL;

	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++)
		mode->groups[idx].util_est_enabled = val[idx];

	return 0;
}

static int
emstune_mode_parse_dt(struct device_node *mode_dn,
				struct emstune_group *groups, int mode_idx)
{
	int idx;
	int cpu;
	struct device_node *dn;

	/* Get index of this mode */
	if (of_property_read_u32(mode_dn, "idx", &emstune_modes[mode_idx].idx)) {
		pr_warn("%s: no idx member in this mode\n", __func__);
		return -EINVAL;
	}

	/* Get description of this mode */
	if (of_property_read_string(mode_dn, "desc", &emstune_modes[mode_idx].desc)) {
		pr_warn("%s: no desc member in this mode\n", __func__);
		return -EINVAL;
	}

	/* Get prefer_idle flag of this mode */
	if (of_property_read_u32(mode_dn, "prefer-idle", &emstune_modes[mode_idx].prefer_idle))
		emstune_modes[mode_idx].prefer_idle = 0;

	dn = of_get_child_by_name(mode_dn, "gov-data");
	if (!dn) {
		for_each_possible_cpu(cpu)
			emstune_modes[mode_idx].gov_data[cpu].step = NEED_OVERRIDING;
	} else {
		emstune_parse_gov_step(dn, &emstune_modes[mode_idx], "step");
		of_node_put(dn);
	}

	/* Parsing ontime migration thresholds */
	dn = of_get_child_by_name(mode_dn, "ontime");
	if (!dn) {
		for (idx = 0; idx < STUNE_GROUP_COUNT; idx++)
			emstune_modes[mode_idx].groups[idx].ontime_enabled = NEED_OVERRIDING;

		for_each_possible_cpu(cpu) {
			emstune_modes[mode_idx].ontime[cpu].upper_boundary = NEED_OVERRIDING;
			emstune_modes[mode_idx].ontime[cpu].lower_boundary = NEED_OVERRIDING;
			emstune_modes[mode_idx].ontime[cpu].upper_boundary_s = NEED_OVERRIDING;
			emstune_modes[mode_idx].ontime[cpu].lower_boundary_s = NEED_OVERRIDING;
		}
	} else {
		emstune_parse_ontime(dn, &emstune_modes[mode_idx]);
		of_node_put(dn);
	}

	/* Parsing utilization estimation thresholds */
	dn = of_get_child_by_name(mode_dn, "util-est");
	if (!dn) {
		for (idx = 0; idx < STUNE_GROUP_COUNT; idx++)
			emstune_modes[mode_idx].groups[idx].util_est_enabled = NEED_OVERRIDING;
	} else {
		emstune_parse_util_est(dn, &emstune_modes[mode_idx]);
		of_node_put(dn);
	}

	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++) {
		char grp_name[16];
		struct emstune_dom **dom = groups[idx].dom;

		/* Get the stune group name */
		snprintf(grp_name, sizeof(grp_name), "%s", stune_group_name[idx]);

		/* Allocate emstune_dom */
		for_each_possible_cpu(cpu) {
			struct emstune_dom *cur = kzalloc(sizeof(struct emstune_dom), GFP_KERNEL);

			if (!cur) {
				pr_warn("%s: failed to allocate emstune_dom\n", __func__);
				continue;
			}

			dom[cpu] = cur;
		}

		/* parsing cpus allowed for this cgroup */
		dn = of_find_node_by_name(mode_dn, "cpus-allowed");
		if (!dn) {
			cpumask_clear(&groups[idx].cpus_allowed);
			emstune_modes[mode_idx].target_sched_class = NEED_OVERRIDING;
		} else {
			/* Get target_sched_class of this mode */
			if (!of_find_property(dn, "target-sched-class", NULL))
				emstune_modes[mode_idx].target_sched_class = NEED_OVERRIDING;
			else {
				emstune_parse_target_sched_class(dn, &emstune_modes[mode_idx]);
				of_node_put(dn);
			}

			emstune_parse_cpus_allowed(dn, &groups[idx], grp_name);
			of_node_put(dn);
		}

		/* parsing weight to calculate efficiency */
		dn = of_get_child_by_name(mode_dn, "weight");
		if (!dn) {
			for_each_possible_cpu(cpu)
				dom[cpu]->weight = NEED_OVERRIDING;
		} else {
			emstune_parse_weight(dn, dom, grp_name, NONE);
			of_node_put(dn);
		}

		/* parsing idle weight to calculate efficiency */
		dn = of_get_child_by_name(mode_dn, "idle-weight");
		if (!dn) {
			for_each_possible_cpu(cpu)
				dom[cpu]->idle_weight = NEED_OVERRIDING;
		} else {
			emstune_parse_idle_weight(dn, dom, grp_name, NONE);
			of_node_put(dn);
		}

		/* parsing freq boost to speed up frequency ramp-up */
		dn = of_get_child_by_name(mode_dn, "freq-boost");
		if (!dn) {
			for_each_possible_cpu(cpu)
				dom[cpu]->freq_boost = NEED_OVERRIDING;
		} else {
			emstune_parse_freq_boost(dn, dom, grp_name, NEGATIVE);
			of_node_put(dn);
		}
	}

	return 0;
}

static void emstune_init_cur_mode(void)
{
	int cpu, idx;

	for (idx = 0; idx < STUNE_GROUP_COUNT; idx++) {
		struct emstune_dom **dom = cur_mode.groups[idx].dom;

		for_each_possible_cpu(cpu) {
			struct emstune_dom *cur = kzalloc(sizeof(struct emstune_dom), GFP_KERNEL);
			if (!cur) {
				pr_warn("%s: failed to allocate emstune_dom\n", __func__);
				continue;
			}

			dom[cpu] = cur;
		}
	}

	emstune_update_cur_mode(DEFAULT_MODE);
	emstune_notify_mode_update();
}

static int __init emstune_init(void)
{
	struct device_node *emstune_dn, *mode_dn;
	int child_count, mode_idx;

	emstune_dn = of_find_node_by_path("/ems/ems-tune");
	if (!emstune_dn)
		return -EINVAL;

	child_count = of_get_child_count(emstune_dn);
	if (!child_count)
		return -EINVAL;

	/* Get index of boosting mode */
	if (of_property_read_u32(emstune_dn, "boost-mode", &boost_mode_idx)) {
		pr_warn("%s: no idx member in boosting mode\n", __func__);
		return -EINVAL;
	}

	emstune_modes = kzalloc(sizeof(struct emstune_mode) * child_count, GFP_KERNEL);
	if (!emstune_modes)
		return -ENOMEM;

	emstune_kobj = kobject_create_and_add("emstune", ems_kobj);
	if (!emstune_kobj)
		return -EINVAL;

	if (sysfs_create_file(emstune_kobj, &user_mode_attr.attr))
		return -ENOMEM;

	if (sysfs_create_file(emstune_kobj, &req_mode_attr.attr))
		return -ENOMEM;

	if (sysfs_create_file(emstune_kobj, &cur_mode_attr.attr))
		return -ENOMEM;

	mode_idx = 0;
	for_each_child_of_node(emstune_dn, mode_dn) {
		int idx;
		struct kobject *mode_kobj = &emstune_modes[mode_idx].kobj;
		struct kobject *ontime_kobj = &emstune_modes[mode_idx].ontime_kobj;
		struct kobject *gov_data_kobj = &emstune_modes[mode_idx].gov_data_kobj;
		struct emstune_group *groups = emstune_modes[mode_idx].groups;

		/* Connect kobject for this mode */
		if (kobject_init_and_add(mode_kobj, &ktype_emstune_mode, emstune_kobj, mode_dn->name))
			return -EINVAL;

		/* Create and connect kobject for CPUFreq gov data */
		if (kobject_init_and_add(gov_data_kobj, &ktype_emstune_gov_data, mode_kobj, "gov_data"))
			pr_warn("%s: failed to initialize kobject of gov_data\n", __func__);

		/* Create and connect kobject for Ontime migration */
		if (kobject_init_and_add(ontime_kobj, &ktype_emstune_ontime, mode_kobj, "ontime"))
			pr_warn("%s: failed to initialize kobject of ontime\n", __func__);

		for (idx = 0; idx < STUNE_GROUP_COUNT; idx++) {
			char grp_name[16];
			struct kobject *grp_kobj = &groups[idx].kobj;

			snprintf(grp_name, sizeof(grp_name), "%s", stune_group_name[idx]);
			if (kobject_init_and_add(grp_kobj, &ktype_groups, mode_kobj, grp_name))
				pr_warn("%s: failed to initialize kobject of %s\n", __func__, grp_name);
		}

		if (emstune_mode_parse_dt(mode_dn, groups, mode_idx)) {
			/* not allow failure to parse emstune dt */
			BUG();
		}

		mode_idx++;
	}

	register_emstune_mode_misc();
	max_mode_idx = mode_idx - 1;
	emstune_init_cur_mode();
	emstune_initialized = true;

	emstune_add_request(&emstune_user_req);
	emstune_boost_timeout(&emstune_user_req, 40 * USEC_PER_SEC);

	return 0;
}
late_initcall(emstune_init);
