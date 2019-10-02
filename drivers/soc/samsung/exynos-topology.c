/*
 * drivers/soc/samsung/exynos-topology.c
 *
 * Copyright (C) 2018 Samsung Electronics.
 *
 * Based on the arm64 version written by Mark Brown in turn based on
 * arch/arm64/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/acpi.h>
#include <linux/arch_topology.h>
#include <linux/cacheinfo.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/sched/topology.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/string.h>

#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/topology.h>

static int __init get_cpu_for_node(struct device_node *node)
{
	struct device_node *cpu_node;
	int cpu;

	cpu_node = of_parse_phandle(node, "cpu", 0);
	if (!cpu_node)
		return -1;

	cpu = of_cpu_node_to_id(cpu_node);
	if (cpu >= 0)
		topology_parse_cpu_capacity(cpu_node, cpu);
	else
		pr_crit("Unable to find CPU node for %pOF\n", cpu_node);

	of_node_put(cpu_node);
	return cpu;
}

static int __init parse_core(struct device_node *core, int package_id,
					int coregroup_id, int core_id)
{
	char name[10];
	bool leaf = true;
	int thread_id = 0;
	int cpu;
	struct device_node *t;

	do {
		snprintf(name, sizeof(name), "thread%d", thread_id);
		t = of_get_child_by_name(core, name);
		if (t) {
			leaf = false;
			cpu = get_cpu_for_node(t);
			if (cpu >= 0) {
				cpu_topology[cpu].package_id = package_id;
				cpu_topology[cpu].coregroup_id = coregroup_id;
				cpu_topology[cpu].core_id = core_id;
				cpu_topology[cpu].thread_id = thread_id;
			} else {
				pr_err("%pOF: Can't get CPU for thread\n",
				       t);
				of_node_put(t);
				return -EINVAL;
			}
			of_node_put(t);
		}
		thread_id++;
	} while (t);

	cpu = get_cpu_for_node(core);
	if (cpu >= 0) {
		if (!leaf) {
			pr_err("%pOF: Core has both threads and CPU\n",
			       core);
			return -EINVAL;
		}

		cpu_topology[cpu].package_id = package_id;
		cpu_topology[cpu].coregroup_id = coregroup_id;
		cpu_topology[cpu].core_id = core_id;
	} else if (leaf) {
		pr_err("%pOF: Can't get CPU for leaf core\n", core);
		return -EINVAL;
	}

	return 0;
}

static int __init parse_coregroup(struct device_node *coregroup, int cluster_id,
					int coregroup_id)
{
	char name[10];
	bool has_cores = false;
	struct device_node *c;
	int core_id;
	int ret;

	core_id = 0;
	do {
		snprintf(name, sizeof(name), "core%d", core_id);
		c = of_get_child_by_name(coregroup, name);
		if (c) {
			has_cores = true;
			ret = parse_core(c, cluster_id, coregroup_id, core_id++);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
	} while (c);

	if (!has_cores)
		pr_warn("%pOF: empty coregroup\n", coregroup);

	return 0;
}

static int __init parse_cluster(struct device_node *cluster, int cluster_id)
{
	char name[20];
	bool leaf = true;
	bool has_cores = false;
	struct device_node *c;
	int coregroup_id, core_id;
	int ret;

	/* First check for child coregroup */
	coregroup_id = 0;
	do {
		snprintf(name, sizeof(name), "coregroup%d", coregroup_id);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			leaf = false;
			ret = parse_coregroup(c, cluster_id, coregroup_id++);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
	} while (c);

	/* Cluster shouldn't have child coregroup and core simultaneously */
	if (!leaf)
		return 0;

	/* If cluster doesn't have coregroup, check for cores */
	core_id = 0;
	do {
		snprintf(name, sizeof(name), "core%d", core_id);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			has_cores = true;
			ret = parse_core(c, cluster_id, coregroup_id, core_id++);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
	} while (c);

	if (!has_cores)
		pr_warn("%pOF: empty cluster\n", cluster);

	return 0;
}

static int __init parse_dt_topology(void)
{
	struct device_node *cn, *map, *cluster;
	int ret = 0;
	int cpu;
	char name[10];
	bool has_cluster = false;
	int cluster_id;

	cn = of_find_node_by_path("/cpus");
	if (!cn) {
		pr_err("No CPU information found in DT\n");
		return 0;
	}

	/*
	 * When topology is provided cpu-map is essentially a root
	 * cluster with restricted subnodes.
	 */
	map = of_get_child_by_name(cn, "cpu-map");
	if (!map)
		goto out;

	cluster_id = 0;
	do {
		snprintf(name, sizeof(name), "cluster%d", cluster_id);
		cluster = of_get_child_by_name(map, name);
		if (cluster) {
			has_cluster = true;
			ret = parse_cluster(cluster, cluster_id++);
			of_node_put(cluster);
			if (ret != 0)
				goto out_map;
		}
	} while (cluster);

	if (!has_cluster) {
		pr_err("%pOF: cpu-map children should be clusters\n", map);
		ret = -EINVAL;
		goto out_map;
	}

	topology_normalize_cpu_scale();

	/*
	 * Check that all cores are in the topology; the SMP code will
	 * only mark cores described in the DT as possible.
	 */
	for_each_possible_cpu(cpu)
		if (cpu_topology[cpu].package_id == -1)
			ret = -EINVAL;

out_map:
	of_node_put(map);
out:
	of_node_put(cn);
	return ret;
}

/*
 * cpu topology table
 */
struct cpu_topology cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

const struct cpumask *cpu_cluster_mask(int cpu)
{
	return &cpu_topology[cpu].coregroup_sibling;
}

static void update_siblings_masks(unsigned int cpuid)
{
	struct cpu_topology *cpu_topo, *cpuid_topo = &cpu_topology[cpuid];
	int cpu;

	/* update core and thread sibling masks */
	for_each_online_cpu(cpu) {
		cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->package_id != cpu_topo->package_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->coregroup_sibling);
		cpumask_set_cpu(cpu, &cpuid_topo->coregroup_sibling);

		if (cpuid_topo->coregroup_id != cpu_topo->coregroup_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
		cpumask_set_cpu(cpu, &cpuid_topo->core_sibling);

		if (cpuid_topo->core_id != cpu_topo->core_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->thread_sibling);
		cpumask_set_cpu(cpu, &cpuid_topo->thread_sibling);
	}
}

void store_cpu_topology(unsigned int cpuid)
{
	struct cpu_topology *cpuid_topo = &cpu_topology[cpuid];
	u64 mpidr;

	if (cpuid_topo->package_id != -1)
		goto topology_populated;

	mpidr = read_cpuid_mpidr();

	/* Uniprocessor systems can rely on default topology values */
	if (mpidr & MPIDR_UP_BITMASK)
		return;

	/* Create cpu topology mapping based on MPIDR. */
	if (mpidr & MPIDR_MT_BITMASK) {
		/* Multiprocessor system : Multi-threads per core */
		cpuid_topo->thread_id  = MPIDR_AFFINITY_LEVEL(mpidr, 0);
		cpuid_topo->core_id    = MPIDR_AFFINITY_LEVEL(mpidr, 1);
		cpuid_topo->package_id = MPIDR_AFFINITY_LEVEL(mpidr, 2) |
					 MPIDR_AFFINITY_LEVEL(mpidr, 3) << 8;
	} else {
		/* Multiprocessor system : Single-thread per core */
		cpuid_topo->thread_id  = -1;
		cpuid_topo->core_id    = MPIDR_AFFINITY_LEVEL(mpidr, 0);
		cpuid_topo->package_id = MPIDR_AFFINITY_LEVEL(mpidr, 1) |
					 MPIDR_AFFINITY_LEVEL(mpidr, 2) << 8 |
					 MPIDR_AFFINITY_LEVEL(mpidr, 3) << 16;
	}

	pr_debug("CPU%u: cluster %d core %d thread %d mpidr %#016llx\n",
		 cpuid, cpuid_topo->package_id, cpuid_topo->core_id,
		 cpuid_topo->thread_id, mpidr);

topology_populated:
	update_siblings_masks(cpuid);
}

#define NR_SD_LEVELS	4
static struct sched_domain_topology_level exynos_topology[NR_SD_LEVELS];

#ifdef CONFIG_SCHED_DEBUG
#define sd_init_name(topology, type)	topology.name = #type
#else
#define sd_init_name(topology, type)
#endif

static void __init build_sched_topology(void)
{
	struct cpu_topology *cpuid_topo, *cpu_topo = &cpu_topology[0];
	bool cluster_level = false;
	bool coregroup_level = false;
	bool core_level = false;
	int cpu;
	int level = 0;

	for_each_possible_cpu(cpu) {
		cpuid_topo = &cpu_topology[cpu];

		if (cpuid_topo->package_id != cpu_topo->package_id)
			cluster_level = true;
		if (cpuid_topo->coregroup_id != cpu_topo->coregroup_id)
			coregroup_level = true;
		if (cpuid_topo->core_id != cpu_topo->core_id)
			core_level = true;
	}

	if (core_level) {
		exynos_topology[level].mask = cpu_coregroup_mask;
		exynos_topology[level].sd_flags = cpu_core_flags;
		sd_init_name(exynos_topology[level], MC);

		level++;
	}
	if (coregroup_level) {
		exynos_topology[level].mask = cpu_cluster_mask;
		exynos_topology[level].sd_flags = cpu_core_flags;
		sd_init_name(exynos_topology[level], DSU);

		level++;
	}
	if (cluster_level) {
		exynos_topology[level].mask = cpu_cpu_mask;
		exynos_topology[level].sd_flags = NULL;
		sd_init_name(exynos_topology[level], DIE);

		level++;
	}
	exynos_topology[level].mask = NULL;
}

static void clear_cpu_topology(int cpu)
{
	struct cpu_topology *cpu_topo = &cpu_topology[cpu];

	cpumask_clear(&cpu_topo->llc_sibling);
	cpumask_set_cpu(cpu, &cpu_topo->llc_sibling);

	cpumask_clear(&cpu_topo->coregroup_sibling);
	cpumask_set_cpu(cpu, &cpu_topo->coregroup_sibling);
	cpumask_clear(&cpu_topo->core_sibling);
	cpumask_set_cpu(cpu, &cpu_topo->core_sibling);
	cpumask_clear(&cpu_topo->thread_sibling);
	cpumask_set_cpu(cpu, &cpu_topo->thread_sibling);
}

static void __init reset_cpu_topology(void)
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct cpu_topology *cpu_topo = &cpu_topology[cpu];

		cpu_topo->core_id = 0;
		cpu_topo->coregroup_id = -1;
		cpu_topo->package_id = -1;
		cpu_topo->llc_id = -1;

		clear_cpu_topology(cpu);
	}
}

void remove_cpu_topology(unsigned int cpu)
{
	int sibling;

	for_each_cpu(sibling, topology_coregroup_cpumask(cpu))
		cpumask_clear_cpu(cpu, topology_coregroup_cpumask(sibling));
	for_each_cpu(sibling, topology_core_cpumask(cpu))
		cpumask_clear_cpu(cpu, topology_core_cpumask(sibling));
	for_each_cpu(sibling, topology_sibling_cpumask(cpu))
		cpumask_clear_cpu(cpu, topology_sibling_cpumask(sibling));
	for_each_cpu(sibling, topology_llc_cpumask(cpu))
		cpumask_clear_cpu(cpu, topology_llc_cpumask(sibling));

	clear_cpu_topology(cpu);
}

void __init init_cpu_topology(void)
{
	reset_cpu_topology();

	/*
	 * Discard anything that was parsed if we hit an error so we
	 * don't use partial information.
	 */
	if (of_have_populated_dt() && parse_dt_topology())
		reset_cpu_topology();
	else {
		build_sched_topology();
		set_sched_topology(exynos_topology);
	}
}
