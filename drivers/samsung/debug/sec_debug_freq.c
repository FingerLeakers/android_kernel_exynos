/*
 * Copyright (c) 2014-2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <soc/samsung/ect_parser.h>
#include <linux/sec_debug.h>

#define TYPE_CPU 2
#define TYPE_INT 3
#define TYPE_MIF 4

#define TABLE1 1
#define TABLE2 2

enum sdg_freq_type {
	CPUFREQ,
	INTFREQ,
	MIFFREQ,
};

static unsigned int sdg_freq_cur[3]; /*cpufreq, intfreq, miffreq*/

struct table_domain {
	unsigned int main_freq;
	unsigned int sub_freq;
};

#define NUM_OF_LEV 24
static struct table_domain sdg_freq_table1[NUM_OF_LEV]; /* 24: ect_domain1->num_of_level */
static struct table_domain sdg_freq_table2[NUM_OF_LEV];

static void __init secdbg_freq_mini_table(struct ect_minlock_domain *ect_domain, struct table_domain table[])
{
	unsigned int i = 0, j, k;

	for (j = 0; j < ect_domain->num_of_level - 1; j++) {
		if (ect_domain->level[j].sub_frequencies != ect_domain->level[j+1].sub_frequencies) {
			pr_debug("secdbg_freq: i:%d, %d %d\n", i, ect_domain->level[j].main_frequencies, ect_domain->level[j].sub_frequencies);
			pr_debug("secdbg_freq: i:%d, %d %d\n", i, ect_domain->level[j+1].main_frequencies, ect_domain->level[j+1].sub_frequencies);
			table[i].main_freq = ect_domain->level[j].main_frequencies;
			table[i].sub_freq = ect_domain->level[j].sub_frequencies;
			i++;
		}
	}
	table[i].main_freq = ect_domain->level[ect_domain->num_of_level-1].main_frequencies;
	table[i].sub_freq = ect_domain->level[ect_domain->num_of_level-1].sub_frequencies;

	for (k = 0; k <= i; k++)
		pr_info("secdbg_freq: %d %d\n", table[k].main_freq, table[k].sub_freq);
}

static void __init secdbg_freq_gen_table(struct ect_minlock_domain *ect_domain1, struct ect_minlock_domain *ect_domain2)
{
	secdbg_freq_mini_table(ect_domain1, sdg_freq_table1);
	secdbg_freq_mini_table(ect_domain2, sdg_freq_table2);
}

static void __init secdbg_freq_init(void)
{
	void *min_block;
	struct ect_minlock_domain *ect_domain1, *ect_domain2;
	char *domain1 = "CPUCL2";
	char *domain2 = "MIF";

	pr_info("%s\n", __func__);

	/* get ect domain1, domain2 */
	min_block = ect_get_block(BLOCK_MINLOCK);
	if (min_block == NULL) {
		pr_info("%s: There is not a min block in ECT\n", __func__);
		return;
	}
	ect_domain1 = ect_minlock_get_domain(min_block, domain1);
	if (ect_domain1 == NULL)
		pr_info("%s: There is not a domain1 in min block\n", __func__);

	ect_domain2 = ect_minlock_get_domain(min_block, domain2);
	if (ect_domain2 == NULL)
		pr_info("%s: There is not a domain2 in min block\n", __func__);

	if (ect_domain1 == NULL || ect_domain2 == NULL)
		return;
	else {
		/* generate minimized ect minlock table */
		secdbg_freq_gen_table(ect_domain1, ect_domain2);
		return;
	}
}
subsys_initcall(secdbg_freq_init);

static void secdbg_freq_set(int type, unsigned int freq)
{
	if (type == TYPE_CPU) { /* 2:BIG */
		sdg_freq_cur[CPUFREQ] = freq;
	} else if (type == TYPE_INT) {/* 3:INT */
		sdg_freq_cur[INTFREQ] = freq;
	} else if (type == TYPE_MIF) { /* 4:MIF */
		sdg_freq_cur[MIFFREQ] = freq;
	}
	pr_debug("%s: type: %d, cpufreq: %d, miffreq: %d, intfreq: %d\n", __func__, type, sdg_freq_cur[CPUFREQ], sdg_freq_cur[MIFFREQ], sdg_freq_cur[INTFREQ]);
}

static void secdbg_freq_ac_print(unsigned int type, unsigned long index,
		unsigned int main_val, unsigned int sub_val, unsigned int table_sub, unsigned int table_type)
{
	const char *freq_type = (type == TYPE_CPU ? "big" : ((type == TYPE_INT) ? "int" : "mif"));
	const char *main_type = (table_type == TABLE1 ? "big" : "mif");
	const char *sub_type = (table_type == TABLE1 ? "mif" : "int");

	/* e.g. SKEW (mif, index): big 2106 mif 1014 < 1539 @  BIG-MIF table */
	pr_auto(ASL1, "SKEW (%s, %d): %s %d %s %d < %d @ %s\n",
			freq_type, index, main_type, main_val/1000, sub_type, sub_val/1000, table_sub, (table_type == TABLE1) ? "BIG-MIF table" : "MIF-INT table");
}

static void secdbg_freq_main_sub(int type, unsigned long index, unsigned int m, unsigned int s, struct table_domain *table)
{
	unsigned int i;

	if ((sdg_freq_cur[m] == 0) || (sdg_freq_cur[s] == 0))
		return;

	for (i = 0; i < NUM_OF_LEV; i++) {
		pr_debug("%s[%d]: main_val: %d, table_main: %d, sub_val: %d, table_sub: %d\n",
				__func__, i, sdg_freq_cur[m], table[i].main_freq, sdg_freq_cur[s], table[i].sub_freq);

		if ((sdg_freq_cur[m] >= table[i].main_freq) && (sdg_freq_cur[s] < table[i].sub_freq)) {
			secdbg_freq_ac_print(type, index, sdg_freq_cur[m], sdg_freq_cur[s], table[i].sub_freq, (m == CPUFREQ) ? TABLE1 : TABLE2);
			break;
		}

		if (!table[i+1].main_freq) {
			pr_debug("%s: table[%d].main_freq is 0\n", __func__, i+1);
			break;
		}
	}
}

static void secdbg_freq_judge_skew(int type, unsigned long index, struct table_domain *table1, struct table_domain *table2)
{
	if (type == TYPE_CPU) /* 2:BIG */
		secdbg_freq_main_sub(type, index, CPUFREQ, MIFFREQ, table1);
	else if (type == TYPE_INT)  /* 3:INT */
		secdbg_freq_main_sub(type, index, MIFFREQ, INTFREQ, table2);
	else if (type == TYPE_MIF) { /* 4:MIF */
		/* check MIF constraint freq */
		secdbg_freq_main_sub(type, index, CPUFREQ, MIFFREQ, table1);
		/* check INT constraint freq */
		secdbg_freq_main_sub(type, index, MIFFREQ, INTFREQ, table2);
	}
}

void secdbg_freq_check(int type, unsigned long index, unsigned long freq)
{
	if (type < TYPE_CPU || TYPE_MIF < type)
		return;

	pr_debug("%s: start\n", __func__);

	/* set freq. domain and target freq. to sdg_freq_cur*/
	secdbg_freq_set(type, (unsigned int)freq);

	/* judge skew and ac print */
	secdbg_freq_judge_skew(type, index, sdg_freq_table1, sdg_freq_table2);
}
