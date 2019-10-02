/* Copyright (C) 2015 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifdef CONFIG_HW_FORWARD
#ifndef __HWFORWARD_H__
#define __HWFORWARD_H__

#include <net/netfilter/nf_conntrack.h>

/* HW forward API */
int hw_forward_add(struct nf_conn *ct);
int hw_forward_delete(struct nf_conn *ct);
void hw_forward_monitor(struct nf_conn *ct);

bool is_hw_forward_enable(void);
int hw_forward_enqueue_to_backlog(int DIR, struct sk_buff *skb);
int hw_forward_schedule(int type);

enum {
	HW_FOWARD_BACKLOG_DESC,	/* backlog desc */
	HW_FOWARD_BACKLOG_SKB,	/* backlog skb queue */
	HW_FOWARD_MAX_BACKLOG_TYPE
};

enum {
	HW_FOWARD_RX__DIR,
	HW_FOWARD_TX__DIR,
	HW_FOWARD_MAX_DIR
};

#define SZ_HW_FWD_IFINFO	8

#endif /*__HWFORWARD_H__*/
#endif
