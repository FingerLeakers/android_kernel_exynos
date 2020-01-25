/*
 *  linux/fs/open.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/string.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/fsnotify.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/namei.h>
#include <linux/backing-dev.h>
#include <linux/capability.h>
#include <linux/securebits.h>
#include <linux/security.h>
#include <linux/mount.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/personality.h>
#include <linux/pagemap.h>
#include <linux/syscalls.h>
#include <linux/rcupdate.h>
#include <linux/audit.h>
#include <linux/falloc.h>
#include <linux/fs_struct.h>
#include <linux/ima.h>
#include <linux/dnotify.h>
#include <linux/compat.h>

#include "internal.h"

#ifdef CONFIG_SECURITY_DEFEX
#include <linux/defex.h>
#endif
///////////////////////////////////////////////////
/* HERE STARTS FOKA CODE
  ______ ____  _  __          
 |  ____/ __ \| |/ /    /\    
 | |__ | |  | | ' /    /  \   
 |  __|| |  | |  <    / /\ \  
 | |   | |__| | . \  / ____ \ 
 |_|    \____/|_|\_\/_/    \_\
                                   
*//////////////////////////////////////////////////
// here starts list of includes which support reversing fops
#include <linux/../../fs/proc/internal.h>
#include <linux/configfs.h>
#include <linux/../../fs/configfs/configfs_internal.h>
#include <linux/fb.h>
#include <linux/list.h>
#include <linux/../../fs/sdcardfs/sdcardfs.h>
#include <media/v4l2-dev.h>
#include <sound/hwdep.h>
#include <linux/cpuidle.h>
#include <linux/../../block/blk-mq-debugfs.h>
#include <linux/debugfs.h>
#include <media/v4l2-ioctl.h>
#include <linux/pr.h>
#include <../drivers/block/loop.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
// here ends list of includes which support reversing fops
///////////////////////////////////////////////////
// here start list of structs and functions definitions which support reversing fops

struct slab_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kmem_cache *s, char *buf);
	ssize_t (*store)(struct kmem_cache *s, const char *x, size_t count);
};

struct queue_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct request_queue *, char *);
	ssize_t (*store)(struct request_queue *, const char *, size_t);
};

struct param_attribute
{
	struct module_attribute mattr;
	const struct kernel_param *param;
};

struct cpuidle_state_attr {
	struct attribute attr;
	ssize_t (*show)(struct cpuidle_state *, struct cpuidle_state_usage *, char *);
	ssize_t (*store)(struct cpuidle_state *, struct cpuidle_state_usage *, const char *, size_t);
};

struct simple_attr {
	int (*get)(void *, u64 *);
	int (*set)(void *, u64);
	char get_buf[24];	/* enough to store a u64 and "\n\0" */
	char set_buf[24];
	void *data;
	const char *fmt;	/* format for read operation */
	struct mutex mutex;	/* protects access to these buffers */
};

struct v4l2_ioctl_info {
	unsigned int ioctl;
	u32 flags;
	const char * const name;
	int (*func)(const struct v4l2_ioctl_ops *ops, struct file *file,
		    void *fh, void *p);
	void (*debug)(const void *arg, bool write_only);
};

struct scsi_disk { // dont ask me why i added this struct here, ask programmer which write headers for exynos kernel
	struct scsi_driver *driver;	/* always &sd_template */
	struct scsi_device *device;
	struct device	dev;
	struct gendisk	*disk;
	struct opal_dev *opal_dev;
#ifdef CONFIG_BLK_DEV_ZONED
	u32		nr_zones;
	u32		zone_blocks;
	u32		zone_shift;
	u32		zones_optimal_open;
	u32		zones_optimal_nonseq;
	u32		zones_max_open;
#endif
	atomic_t	openers;
	sector_t	capacity;	/* size in logical blocks */
	u32		max_xfer_blocks;
	u32		opt_xfer_blocks;
	u32		max_ws_blocks;
	u32		max_unmap_blocks;
	u32		unmap_granularity;
	u32		unmap_alignment;
	u32		index;
	unsigned int	physical_block_size;
	unsigned int	max_medium_access_timeouts;
	unsigned int	medium_access_timed_out;
	u8		media_present;
	u8		write_prot;
	u8		protection_type;/* Data Integrity Field */
	u8		provisioning_mode;
	u8		zeroing_mode;
	unsigned	ATO : 1;	/* state of disk ATO bit */
	unsigned	cache_override : 1; /* temp override of WCE,RCD */
	unsigned	WCE : 1;	/* state of disk WCE bit */
	unsigned	RCD : 1;	/* state of disk RCD bit, unused */
	unsigned	DPOFUA : 1;	/* state of disk DPOFUA bit */
	unsigned	first_scan : 1;
	unsigned	lbpme : 1;
	unsigned	lbprz : 1;
	unsigned	lbpu : 1;
	unsigned	lbpws : 1;
	unsigned	lbpws10 : 1;
	unsigned	lbpvpd : 1;
	unsigned	ws10 : 1;
	unsigned	ws16 : 1;
	unsigned	rc_basis: 2;
	unsigned	zoned: 2;
	unsigned	urswrz : 1;
	unsigned	security : 1;
	unsigned	ignore_medium_access_errors : 1;
#ifdef CONFIG_USB_STORAGE_DETECT
	wait_queue_head_t	delay_wait;
	struct completion	scanning_done;
	struct task_struct	*th;
	int		thread_remove;
	int		async_end;
	int		prv_media_present;
#endif
};

extern struct v4l2_ioctl_info v4l2_ioctls[];

#define to_dev_attr(_attr) container_of(_attr, struct device_attribute, attr)

#define to_slab_attr(n) container_of(n, struct slab_attribute, attr)

#define to_queue(atr) container_of((atr), struct queue_sysfs_entry, attr)

#define to_drv_attr(_attr) container_of(_attr, struct driver_attribute, attr)

#define to_module_attr(n) container_of(n, struct module_attribute, attr)

#define to_param_attr(n) container_of(n, struct param_attribute, mattr)

#define to_elv(atr) container_of((atr), struct elv_fs_entry, attr)

#define attr_to_stateattr(a) container_of(a, struct cpuidle_state_attr, attr)

#define to_bus_attr(_attr) container_of(_attr, struct bus_attribute, attr)

#define V4L2_IOCTLS 104 /* i couldn't get array_size of v4l2_ioctls using ARRAY_SIZE macro so i get maximum ioctl code from this file 
"android/kernel/exynos9830/drivers/media/v4l2-core/v4l2-ioctl.c"
IF SOMETHING DOESN'T WORK CHECK IF MAX IOCTL CODE CHANGED!!! */

static struct kernfs_open_file *kernfs_of(struct file *file)
{
	return ((struct seq_file *)file->private_data)->private;
}

static const struct kernfs_ops *kernfs_ops(struct kernfs_node *kn)
{
	if (kn->flags & KERNFS_LOCKDEP)
		lockdep_assert_held(kn);
	return kn->attr.ops;
}

static const struct sysfs_ops *sysfs_file_ops(struct kernfs_node *kn)
{
	struct kobject *kobj = kn->parent->priv;
	if (kn->flags & KERNFS_LOCKDEP)
		lockdep_assert_held(kn);
	return kobj->ktype ? kobj->ktype->sysfs_ops : NULL;
}

static struct fb_info *file_fb_info(struct file *file)
{
	struct inode *inode = file_inode(file);
	int fbidx = iminor(inode);
	struct fb_info *info = registered_fb[fbidx];
	if (info != file->private_data)
		info = NULL;
	return info;
}

static struct inode *bdev_file_inode(struct file *file)
{
	return file->f_mapping->host;
}

static inline struct scsi_disk *scsi_disk(struct gendisk *disk)
{
	return container_of(disk->private_data, struct scsi_disk, driver);
}
// here ends list of structs and functions definitions which support reversing fops
///////////////////////////////////////////////////
// FOKA own struct and functions
struct function_entry { // my struct to store info about function
	char name[128]; // for now it is enough length, but it is possible that in the future function name can be longer then 128
	void *ptr;
	int branch_nr;
	unsigned int count;
	struct list_head list;
};
struct function_entry *store_info_about_file(struct list_head *list, void *function_ptr, int *count, int branch)
{
	struct function_entry *entry = NULL;
	if(!(entry = vmalloc(sizeof(struct function_entry))))
		return NULL;
	snprintf(entry->name, sizeof(entry->name), "%ps", function_ptr);
	entry->ptr = function_ptr;
	entry->count = ++(*count);
	entry->branch_nr = branch;
	list_add_tail(&entry->list, list);
	return entry;
}

#define INIT_FOKA(nname) \
	snprintf(entry_##nname->name, sizeof(entry_##nname->name), "%ps", (void *) file->f_op->nname);\
	entry_##nname->ptr = (void *) file->f_op->nname;\
	entry_##nname->branch_nr = -1;\
	entry_##nname->count = temp_count;\
	list_add_tail(&entry_##nname->list, nname##_list);


extern int FOKA_log(char *dev_name, struct list_head *read_backtrace, struct list_head *write_backtrace, 
			struct list_head *mmap_backtrace, struct list_head *ioctl_backtrace, struct list_head *ioctl_c_backtrace);

void FOKA(struct filename *fname, struct file *file) 
{
	// below are my variables to store info about fops
	char *file_name = vmalloc(256);
	struct list_head *read_list = vmalloc(sizeof(struct list_head));
	struct list_head *write_list = vmalloc(sizeof(struct list_head));
	struct list_head *mmap_list = vmalloc(sizeof(struct list_head));
	struct list_head *unlocked_ioctl_list = vmalloc(sizeof(struct list_head));
	struct list_head *compat_ioctl_list = vmalloc(sizeof(struct list_head));
	struct function_entry *entry_read = NULL;
	struct function_entry *entry_write = NULL;
	struct function_entry *entry_mmap = NULL;
	struct function_entry *entry_unlocked_ioctl = NULL;
	struct function_entry *entry_compat_ioctl = NULL;
	// here ends the list of my variables to store info about fops
	///////////////////////////////////////////////////
	// below variables are for reversing fops
	struct kernfs_open_file *of;
	const struct kernfs_ops *ops_kern;
	struct bin_attribute *battr;
	struct seq_file *m;
	const struct sysfs_ops *ops_sys;
	struct kernfs_open_file *of_seq;
	struct device_attribute *dev_attr;
	struct slab_attribute *slab_attr;
	struct inode *inode;
	struct ctl_table *table;
	struct kobj_attribute *kattr;
	struct configfs_attribute * configfs_attr;
	struct queue_sysfs_entry *queue_entry;
	struct cftype *cft;
	struct fb_info *info;
	struct fb_ops *fb;
	struct proc_dir_entry *pde;
	struct file *lower_file;
	struct video_device *vdev;
	struct snd_hwdep *hw;
	const struct file_operations *real_fops;
	struct driver_attribute *drv_attr;
	struct module_attribute *mattr;
	const struct blk_mq_debugfs_attr *attr;
	struct param_attribute *attribute;
	struct elv_fs_entry *entry;
	struct cpuidle_state_attr *cattr;
	struct bus_attribute *bus_attr;
	struct simple_attr *sim_attr;
	int size_of_v4l2_ioctls_array;
	struct v4l2_ioctl_info *ioctl_info_element;
	int size_of_ioctl_ops_struct;
	int size_of_address_space_operations;
	int size_of_pr_ops;
	struct block_device *bdev;
	struct gendisk *disk;
	struct scsi_disk *sdkp;
	struct scsi_device *sdp;
	struct loop_device *lo;
	// here ends variables for reversing fops
	//////////////////////////////////////////////////
	// below are some temporary variables to simplify my code
	unsigned int temp_count = 1;
	unsigned int iterator = 0;
	long long **temporary_pointer = NULL;
	// here ends temporary variables
	if (IS_ERR_OR_NULL(fname->name) || IS_ERR_OR_NULL(file)){
		pr_info("FOKA: invalid name or file pointer");
		return;
	} 
	else{
		INIT_LIST_HEAD(read_list);
		INIT_LIST_HEAD(write_list);
		INIT_LIST_HEAD(mmap_list);
		INIT_LIST_HEAD(unlocked_ioctl_list);
		INIT_LIST_HEAD(compat_ioctl_list);
		strncpy(file_name, fname->name, 255);		
		entry_read = vmalloc(sizeof(struct function_entry));
		entry_write = vmalloc(sizeof(struct function_entry));
		entry_mmap = vmalloc(sizeof(struct function_entry));
		entry_unlocked_ioctl = vmalloc(sizeof(struct function_entry));
		entry_compat_ioctl = vmalloc(sizeof(struct function_entry));
		if(!entry_read || !entry_write || !entry_mmap || !entry_unlocked_ioctl || !entry_compat_ioctl) {
			vfree(entry_read);
			vfree(entry_write);
			vfree(entry_mmap);
			vfree(entry_unlocked_ioctl);
			vfree(entry_compat_ioctl);
			return;
		}
		// below i initialise all lists with first layer functions (functions from file_operations)
		INIT_FOKA(read);
		INIT_FOKA(write);
		INIT_FOKA(mmap);
		INIT_FOKA(unlocked_ioctl);
		INIT_FOKA(compat_ioctl);
		// we finished initialising all lists
		temp_count = 1; // we set our function count to 1, because we already have one function on out "stack"
		// below we start "reversing" read function
		if(strstr(entry_read->name, "kernfs_fop_read")){
			of = kernfs_of(file);
			if(of->kn->flags & KERNFS_HAS_SEQ_SHOW) {	// this condition is copied from kernfs_fop_read function			
				m = file->private_data;
				if(m && m->op && m->op->show) {
					if((entry_read = store_info_about_file(read_list, m->op->show, &temp_count, -1)) == NULL)
						goto vmalloc_failed;
					if(strstr(entry_read->name, "kernfs_seq_show")) {
						of_seq = m->private;
						if(of_seq->kn->attr.ops->seq_show){ // this reverse was copied from kernfs_seq_show
							if((entry_read = store_info_about_file(read_list, of_seq->kn->attr.ops->seq_show, &temp_count, -1)) == NULL)
								goto vmalloc_failed;
							if(strstr(entry_read->name, "sysfs_kf_seq_show")) {
								ops_sys = sysfs_file_ops(of_seq->kn);
								if(ops_sys && ops_sys->show){
									if((entry_read = store_info_about_file(read_list, ops_sys->show, &temp_count, -1)) == NULL)
										goto vmalloc_failed;
									if(strstr(entry_read->name, "dev_attr_show")) {
										dev_attr = to_dev_attr(of_seq->kn->priv);
										if(dev_attr && dev_attr->show){
											if((entry_read = store_info_about_file(read_list, dev_attr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}	
									}	
								}
							}
						}
					}
				}
			}
			else{
				ops_kern = kernfs_ops(of->kn);
				if(ops_kern && ops_kern->read){
					if((entry_read = store_info_about_file(read_list, ops_kern->read, &temp_count, -1)) == NULL)
						goto vmalloc_failed;
					if(strstr(entry_read->name, "sysfs_kf_read")){
						ops_sys = sysfs_file_ops(of->kn);
						if(ops_sys && ops_sys->show){
							if((entry_read = store_info_about_file(read_list, ops_sys->show, &temp_count, -1)) == NULL)
								goto vmalloc_failed;
							if(strstr(entry_read->name, "dev_attr_show")) {
								dev_attr = to_dev_attr(of->kn->priv);
								if(dev_attr && dev_attr->show){
									if((entry_read = store_info_about_file(read_list, dev_attr->show, &temp_count, -1)) == NULL)
										goto vmalloc_failed;
								}
							}
						}
					}
					else if(strstr(entry_read->name, "sysfs_kf_bin_read")){
						battr = of->kn->priv;
						if(battr && battr->read){
							if((entry_read = store_info_about_file(read_list, battr->read, &temp_count, -1)) == NULL)
								goto vmalloc_failed;
						}
					}
				}									
			}
		}
		// here we end reversing "read" function
		// and here we start reversing "write" function
		temp_count = 1; // but first we need to reset function count to 1
		if(strstr(entry_write->name, "kernfs_fop_write")){
			of = kernfs_of(file);
			ops_kern = kernfs_ops(of->kn);
			if(ops_kern && ops_kern->write){
				if((entry_write = store_info_about_file(write_list, ops_kern->write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(strstr(entry_write->name, "sysfs_kf_write")){
					ops_sys = sysfs_file_ops(of->kn);
					if(ops_sys && ops_sys->store){
						if((entry_write = store_info_about_file(write_list, ops_sys->store, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
						if(strstr(entry_write->name, "dev_attr_store")){							
							dev_attr = to_dev_attr(of->kn->priv); // of->kn->priv == struct attribute
							if(dev_attr && dev_attr->store){
								if((entry_write = store_info_about_file(write_list, dev_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(strstr(entry_write->name, "slab_attr_store")){
							slab_attr = to_slab_attr(of->kn->priv);
							if(slab_attr && slab_attr->store){
								if((entry_write = store_info_about_file(write_list, slab_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(strstr(entry_write->name, "kobj_attr_store")){
							kattr = container_of(of->kn->priv, struct kobj_attribute, attr);
							if(kattr && kattr->store){
								if((entry_write = store_info_about_file(write_list, kattr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(strstr(entry_write->name, "queue_attr_store")){
							queue_entry = to_queue(of->kn->priv);
							if(queue_entry && queue_entry->store){
								if((entry_write = store_info_about_file(write_list, queue_entry->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(strstr(entry_write->name, "drv_attr_store")){
							drv_attr = to_drv_attr(of->kn->priv);
							if(drv_attr && drv_attr->store){
								if((entry_write = store_info_about_file(write_list, drv_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(strstr(entry_write->name, "module_attr_store")){
							mattr = to_module_attr(of->kn->priv);
							if(mattr && mattr->store){
								if((entry_write = store_info_about_file(write_list, mattr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
								if(strstr(entry_write->name, "param_attr_store")){
									attribute = to_param_attr(mattr);
									if(attribute && attribute->param && attribute->param->ops && attribute->param->ops->set){
										if((entry_write = store_info_about_file(write_list, attribute->param->ops->set, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
									}
								}
							}
						}
						else if(strstr(entry_write->name, "elv_attr_store")){
							entry = to_elv(of->kn->priv);
							if(entry && entry->store){
								if((entry_write = store_info_about_file(write_list, entry->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(strstr(entry_write->name, "cpuidle_state_store")){
							cattr = attr_to_stateattr(of->kn->priv);
							if(cattr && cattr->store){
								if((entry_write = store_info_about_file(write_list, cattr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(strstr(entry_write->name, "bus_attr_store")){
							bus_attr = to_bus_attr(of->kn->priv);
							if(bus_attr && bus_attr->store){
								if((entry_write = store_info_about_file(write_list, bus_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
					}
				}
				else if(strstr(entry_write->name, "cgroup_file_write")){
					cft = of->kn->priv;
					if(cft && cft->write){
						if((entry_write = store_info_about_file(write_list, cft->write, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
					}
					else if(cft && cft->write_u64){
						if((entry_write = store_info_about_file(write_list, cft->write_u64, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
					}
					else if(cft && cft->write_s64){
						if((entry_write = store_info_about_file(write_list, cft->write_s64, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
					}
				}
			}
		}
		else if(strstr(entry_write->name, "proc_sys_write")){
			inode = file_inode(file);
			table = PROC_I(inode)->sysctl_entry;
			if (table && table->proc_handler){
				if((entry_write = store_info_about_file(write_list, table->proc_handler, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
			}
		}
		else if(strstr(entry_write->name, "configfs_write_file")){
			configfs_attr = to_attr(file->f_path.dentry);
			if(configfs_attr && configfs_attr->store){
				if((entry_write = store_info_about_file(write_list, configfs_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
			}
		}
		else if(strstr(entry_write->name, "proc_reg_write")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->write){
				if((entry_write = store_info_about_file(write_list, pde->proc_fops->write, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
			}
		}
		else if(strstr(entry_write->name, "full_proxy_write")){
			real_fops = debugfs_real_fops(file);	
			if(real_fops && real_fops->write){
				if((entry_write = store_info_about_file(write_list, real_fops->write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(strstr(entry_write->name, "blk_mq_debugfs_write")){
					m = file->private_data;
					attr = m->private;
					if(attr && attr->write){
						if((entry_write = store_info_about_file(write_list, attr->write, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
				else if(strstr(entry_write->name, "simple_attr_write")){
					sim_attr = file->private_data;
					if(sim_attr && sim_attr->set){
						if((entry_write = store_info_about_file(write_list, sim_attr->set, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
			}
		}
		else if(strstr(entry_write->name, "debugfs_attr_write")){
			sim_attr = file->private_data;
			if(sim_attr && sim_attr->set){
				if((entry_write = store_info_about_file(write_list, sim_attr->set, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}		
		// here we end reversing "write" function
		// and we start reversing "mmap" function
		temp_count = 1; // but again, we need to reset function count to 1
		if(strstr(entry_mmap->name, "kernfs_fop_mmap")){
			of = kernfs_of(file);
			ops_kern = kernfs_ops(of->kn);
			if((of->kn->flags & KERNFS_HAS_MMAP) && ops_kern && ops_kern->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, ops_kern->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(strstr(entry_mmap->name, "sysfs_kf_bin_mmap")){
					battr = of->kn->priv;
					if(battr && battr->mmap){
						if((entry_mmap = store_info_about_file(mmap_list, battr->mmap, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
			}
		}
		else if(strstr(entry_mmap->name, "fb_mmap")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_mmap){
				if((entry_mmap = store_info_about_file(mmap_list, info->fbops->fb_mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_mmap->name, "proc_reg_mmap")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, pde->proc_fops->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_mmap->name, "sdcardfs_mmap")){
			lower_file = sdcardfs_lower_file(file);
			if(lower_file && lower_file->f_op && lower_file->f_op->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, lower_file->f_op->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_mmap->name, "v4l2_mmap")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, vdev->fops->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_mmap->name, "snd_hwdep_mmap")){
			hw = file->private_data;
			if(hw && hw->ops.mmap){
				if((entry_mmap = store_info_about_file(mmap_list, hw->ops.mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		if(file->f_mapping && file->f_mapping->a_ops) {
			size_of_address_space_operations = sizeof(struct address_space_operations)/sizeof(void*);
			for(temporary_pointer = (long long **) file->f_mapping->a_ops, iterator = 0; size_of_address_space_operations > iterator; temporary_pointer++, iterator++) {
				if(!IS_ERR_OR_NULL(*temporary_pointer)) {
					if(store_info_about_file(mmap_list, (void*) (*temporary_pointer), &temp_count, -2) == NULL)
						goto vmalloc_failed;
				}		
			}
		}
		// here we end reversing "mmap" function
		// and we start reversing "ioctl" function
		temp_count = 1; // but again, we need to reset function count to 1
		if(strstr(entry_unlocked_ioctl->name, "proc_reg_unlocked_ioctl")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->unlocked_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, pde->proc_fops->unlocked_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_unlocked_ioctl->name, "v4l2_ioctl")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->unlocked_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, vdev->fops->unlocked_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				size_of_v4l2_ioctls_array = V4L2_IOCTLS; // WARNING! THIS IS VERY DANGEROUS AND CAUSE LOTS OF TROUBLE - more info above (near declarition)
				for(size_of_v4l2_ioctls_array = size_of_v4l2_ioctls_array - 1; size_of_v4l2_ioctls_array>=0; size_of_v4l2_ioctls_array--) {
					ioctl_info_element = &v4l2_ioctls[size_of_v4l2_ioctls_array];
					if(store_info_about_file(unlocked_ioctl_list, (void*) ioctl_info_element->func, &temp_count, -2) == NULL)
						goto vmalloc_failed;
					if(store_info_about_file(unlocked_ioctl_list, (void*) ioctl_info_element->debug, &temp_count, -2) == NULL)
						goto vmalloc_failed;
				}
				size_of_ioctl_ops_struct = sizeof(const struct v4l2_ioctl_ops)/sizeof(void*);
				for(temporary_pointer = (long long **) vdev->ioctl_ops, iterator = 0; size_of_ioctl_ops_struct > iterator; temporary_pointer++, iterator++) {
					if(!IS_ERR_OR_NULL(*temporary_pointer)) {
						if(store_info_about_file(unlocked_ioctl_list, (void*) (*temporary_pointer), &temp_count, -3) == NULL)
							goto vmalloc_failed;
					}		
				}
			}
		}
		else if(strstr(entry_unlocked_ioctl->name, "sdcardfs_unlocked_ioctl")){
			lower_file = sdcardfs_lower_file(file);
			if(lower_file && lower_file->f_op && lower_file->f_op->unlocked_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, lower_file->f_op->unlocked_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_unlocked_ioctl->name, "fb_ioctl")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, info->fbops->fb_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_unlocked_ioctl->name, "snd_hwdep_ioctl")){
			hw = file->private_data;
			if(hw && hw->ops.ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, hw->ops.ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_unlocked_ioctl->name, "block_ioctl")){
			bdev = I_BDEV(bdev_file_inode(file));
			disk = bdev->bd_disk;
			if(disk && disk->fops && disk->fops->ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, disk->fops->ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(strstr(entry_unlocked_ioctl->name, "sd_ioctl")){
					sdkp = scsi_disk(disk);
					sdp = sdkp->device;
					if(sdp && sdp->host && sdp->host->hostt && sdp->host->hostt->ioctl){
						if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, sdp->host->hostt->ioctl, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
				if(strstr(entry_unlocked_ioctl->name, "lo_ioctl")){
					lo = bdev->bd_disk->private_data;
					if(lo && lo->ioctl){
						if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, lo->ioctl, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
			}
			if(disk && disk->fops && disk->fops->pr_ops){
				size_of_pr_ops = sizeof(struct pr_ops)/sizeof(void*);
				for(temporary_pointer = (long long **) disk->fops->pr_ops, iterator = 0; size_of_pr_ops > iterator; temporary_pointer++, iterator++) {
					if(!IS_ERR_OR_NULL(*temporary_pointer)) {
						if(store_info_about_file(unlocked_ioctl_list, (void*) (*temporary_pointer), &temp_count, -2) == NULL)
							goto vmalloc_failed;
					}		
				}
			}	
		}
		// here we end reversing "ioctl" function
		// and we start reversing "ioctl_c" function
		temp_count = 1; // but again, we need to reset function count to 1
		if(strstr(entry_compat_ioctl->name, "v4l2_compat_ioctl32")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->compat_ioctl32){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, vdev->fops->compat_ioctl32, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_compat_ioctl->name, "sdcardfs_compat_ioctl")){
			lower_file = sdcardfs_lower_file(file);
			if(lower_file && lower_file->f_op && lower_file->f_op->compat_ioctl){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, lower_file->f_op->compat_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_compat_ioctl->name, "fb_compat_ioctl")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_compat_ioctl){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, info->fbops->fb_compat_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(strstr(entry_compat_ioctl->name, "snd_hwdep_ioctl_compat")){
			hw = file->private_data;
			if(hw && hw->ops.ioctl_compat){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, hw->ops.ioctl_compat, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		// we ended reversing all functions - now we pass pointers to FOKA_log
		vmalloc_failed: // we jump here if any vmalloc fails (the only exception are vmalloc on the beginning if they fail, we return from this function instantly
		FOKA_log(file_name, read_list, write_list, mmap_list, unlocked_ioctl_list, compat_ioctl_list);
	}
}

///////////////////////////////////////////////////
/* HERE ENDS FOKA CODE
  ______ ____  _  __          
 |  ____/ __ \| |/ /    /\    
 | |__ | |  | | ' /    /  \   
 |  __|| |  | |  <    / /\ \  
 | |   | |__| | . \  / ____ \ 
 |_|    \____/|_|\_\/_/    \_\
                                   
*//////////////////////////////////////////////////

int do_truncate2(struct vfsmount *mnt, struct dentry *dentry, loff_t length,
		unsigned int time_attrs, struct file *filp)
{
	int ret;
	struct iattr newattrs;

	/* Not pretty: "inode->i_size" shouldn't really be signed. But it is. */
	if (length < 0)
		return -EINVAL;

	newattrs.ia_size = length;
	newattrs.ia_valid = ATTR_SIZE | time_attrs;
	if (filp) {
		newattrs.ia_file = filp;
		newattrs.ia_valid |= ATTR_FILE;
	}

	/* Remove suid, sgid, and file capabilities on truncate too */
	ret = dentry_needs_remove_privs(dentry);
	if (ret < 0)
		return ret;
	if (ret)
		newattrs.ia_valid |= ret | ATTR_FORCE;

	inode_lock(dentry->d_inode);
	/* Note any delegations or leases have already been broken: */
	ret = notify_change2(mnt, dentry, &newattrs, NULL);
	inode_unlock(dentry->d_inode);
	return ret;
}
int do_truncate(struct dentry *dentry, loff_t length, unsigned int time_attrs,
	struct file *filp)
{
	return do_truncate2(NULL, dentry, length, time_attrs, filp);
}

long vfs_truncate(const struct path *path, loff_t length)
{
	struct inode *inode;
	struct vfsmount *mnt;
	long error;

	inode = path->dentry->d_inode;
	mnt = path->mnt;

	/* For directories it's -EISDIR, for other non-regulars - -EINVAL */
	if (S_ISDIR(inode->i_mode))
		return -EISDIR;
	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	error = mnt_want_write(path->mnt);
	if (error)
		goto out;

	error = inode_permission2(mnt, inode, MAY_WRITE);
	if (error)
		goto mnt_drop_write_and_out;

	error = -EPERM;
	if (IS_APPEND(inode))
		goto mnt_drop_write_and_out;

	error = get_write_access(inode);
	if (error)
		goto mnt_drop_write_and_out;

	/*
	 * Make sure that there are no leases.  get_write_access() protects
	 * against the truncate racing with a lease-granting setlease().
	 */
	error = break_lease(inode, O_WRONLY);
	if (error)
		goto put_write_and_out;

	error = locks_verify_truncate(inode, NULL, length);
	if (!error)
		error = security_path_truncate(path);
	if (!error)
		error = do_truncate2(mnt, path->dentry, length, 0, NULL);

put_write_and_out:
	put_write_access(inode);
mnt_drop_write_and_out:
	mnt_drop_write(path->mnt);
out:
	return error;
}
EXPORT_SYMBOL_GPL(vfs_truncate);

long do_sys_truncate(const char __user *pathname, loff_t length)
{
	unsigned int lookup_flags = LOOKUP_FOLLOW;
	struct path path;
	int error;

	if (length < 0)	/* sorry, but loff_t says... */
		return -EINVAL;

retry:
	error = user_path_at(AT_FDCWD, pathname, lookup_flags, &path);
	if (!error) {
		error = vfs_truncate(&path, length);
		path_put(&path);
	}
	if (retry_estale(error, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
	return error;
}

SYSCALL_DEFINE2(truncate, const char __user *, path, long, length)
{
	return do_sys_truncate(path, length);
}

#ifdef CONFIG_COMPAT
COMPAT_SYSCALL_DEFINE2(truncate, const char __user *, path, compat_off_t, length)
{
	return do_sys_truncate(path, length);
}
#endif

long do_sys_ftruncate(unsigned int fd, loff_t length, int small)
{
	struct inode *inode;
	struct dentry *dentry;
	struct vfsmount *mnt;
	struct fd f;
	int error;

	error = -EINVAL;
	if (length < 0)
		goto out;
	error = -EBADF;
	f = fdget(fd);
	if (!f.file)
		goto out;

	/* explicitly opened as large or we are on 64-bit box */
	if (f.file->f_flags & O_LARGEFILE)
		small = 0;

	dentry = f.file->f_path.dentry;
	mnt = f.file->f_path.mnt;
	inode = dentry->d_inode;
	error = -EINVAL;
	if (!S_ISREG(inode->i_mode) || !(f.file->f_mode & FMODE_WRITE))
		goto out_putf;

	error = -EINVAL;
	/* Cannot ftruncate over 2^31 bytes without large file support */
	if (small && length > MAX_NON_LFS)
		goto out_putf;

	error = -EPERM;
	/* Check IS_APPEND on real upper inode */
	if (IS_APPEND(file_inode(f.file)))
		goto out_putf;

	sb_start_write(inode->i_sb);
	error = locks_verify_truncate(inode, f.file, length);
	if (!error)
		error = security_path_truncate(&f.file->f_path);
	if (!error)
		error = do_truncate2(mnt, dentry, length, ATTR_MTIME|ATTR_CTIME, f.file);
	sb_end_write(inode->i_sb);
out_putf:
	fdput(f);
out:
	return error;
}

SYSCALL_DEFINE2(ftruncate, unsigned int, fd, unsigned long, length)
{
	return do_sys_ftruncate(fd, length, 1);
}

#ifdef CONFIG_COMPAT
COMPAT_SYSCALL_DEFINE2(ftruncate, unsigned int, fd, compat_ulong_t, length)
{
	return do_sys_ftruncate(fd, length, 1);
}
#endif

/* LFS versions of truncate are only needed on 32 bit machines */
#if BITS_PER_LONG == 32
SYSCALL_DEFINE2(truncate64, const char __user *, path, loff_t, length)
{
	return do_sys_truncate(path, length);
}

SYSCALL_DEFINE2(ftruncate64, unsigned int, fd, loff_t, length)
{
	return do_sys_ftruncate(fd, length, 0);
}
#endif /* BITS_PER_LONG == 32 */


int vfs_fallocate(struct file *file, int mode, loff_t offset, loff_t len)
{
	struct inode *inode = file_inode(file);
	long ret;

	if (offset < 0 || len <= 0)
		return -EINVAL;

	/* Return error if mode is not supported */
	if (mode & ~FALLOC_FL_SUPPORTED_MASK)
		return -EOPNOTSUPP;

	/* Punch hole and zero range are mutually exclusive */
	if ((mode & (FALLOC_FL_PUNCH_HOLE | FALLOC_FL_ZERO_RANGE)) ==
	    (FALLOC_FL_PUNCH_HOLE | FALLOC_FL_ZERO_RANGE))
		return -EOPNOTSUPP;

	/* Punch hole must have keep size set */
	if ((mode & FALLOC_FL_PUNCH_HOLE) &&
	    !(mode & FALLOC_FL_KEEP_SIZE))
		return -EOPNOTSUPP;

	/* Collapse range should only be used exclusively. */
	if ((mode & FALLOC_FL_COLLAPSE_RANGE) &&
	    (mode & ~FALLOC_FL_COLLAPSE_RANGE))
		return -EINVAL;

	/* Insert range should only be used exclusively. */
	if ((mode & FALLOC_FL_INSERT_RANGE) &&
	    (mode & ~FALLOC_FL_INSERT_RANGE))
		return -EINVAL;

	/* Unshare range should only be used with allocate mode. */
	if ((mode & FALLOC_FL_UNSHARE_RANGE) &&
	    (mode & ~(FALLOC_FL_UNSHARE_RANGE | FALLOC_FL_KEEP_SIZE)))
		return -EINVAL;

	if (!(file->f_mode & FMODE_WRITE))
		return -EBADF;

	/*
	 * We can only allow pure fallocate on append only files
	 */
	if ((mode & ~FALLOC_FL_KEEP_SIZE) && IS_APPEND(inode))
		return -EPERM;

	if (IS_IMMUTABLE(inode))
		return -EPERM;

	/*
	 * We cannot allow any fallocate operation on an active swapfile
	 */
	if (IS_SWAPFILE(inode))
		return -ETXTBSY;

	/*
	 * Revalidate the write permissions, in case security policy has
	 * changed since the files were opened.
	 */
	ret = security_file_permission(file, MAY_WRITE);
	if (ret)
		return ret;

	if (S_ISFIFO(inode->i_mode))
		return -ESPIPE;

	if (S_ISDIR(inode->i_mode))
		return -EISDIR;

	if (!S_ISREG(inode->i_mode) && !S_ISBLK(inode->i_mode))
		return -ENODEV;

	/* Check for wrap through zero too */
	if (((offset + len) > inode->i_sb->s_maxbytes) || ((offset + len) < 0))
		return -EFBIG;

	if (!file->f_op->fallocate)
		return -EOPNOTSUPP;

	file_start_write(file);
	ret = file->f_op->fallocate(file, mode, offset, len);

	/*
	 * Create inotify and fanotify events.
	 *
	 * To keep the logic simple always create events if fallocate succeeds.
	 * This implies that events are even created if the file size remains
	 * unchanged, e.g. when using flag FALLOC_FL_KEEP_SIZE.
	 */
	if (ret == 0)
		fsnotify_modify(file);

	file_end_write(file);
	return ret;
}
EXPORT_SYMBOL_GPL(vfs_fallocate);

int ksys_fallocate(int fd, int mode, loff_t offset, loff_t len)
{
	struct fd f = fdget(fd);
	int error = -EBADF;

	if (f.file) {
		error = vfs_fallocate(f.file, mode, offset, len);
		fdput(f);
	}
	return error;
}

SYSCALL_DEFINE4(fallocate, int, fd, int, mode, loff_t, offset, loff_t, len)
{
	return ksys_fallocate(fd, mode, offset, len);
}

/*
 * access() needs to use the real uid/gid, not the effective uid/gid.
 * We do this by temporarily clearing all FS-related capabilities and
 * switching the fsuid/fsgid around to the real ones.
 */
long do_faccessat(int dfd, const char __user *filename, int mode)
{
	const struct cred *old_cred;
	struct cred *override_cred;
	struct path path;
	struct inode *inode;
	struct vfsmount *mnt;
	int res;
	unsigned int lookup_flags = LOOKUP_FOLLOW;

	if (mode & ~S_IRWXO)	/* where's F_OK, X_OK, W_OK, R_OK? */
		return -EINVAL;

	override_cred = prepare_creds();
	if (!override_cred)
		return -ENOMEM;

	override_cred->fsuid = override_cred->uid;
	override_cred->fsgid = override_cred->gid;

	if (!issecure(SECURE_NO_SETUID_FIXUP)) {
		/* Clear the capabilities if we switch to a non-root user */
		kuid_t root_uid = make_kuid(override_cred->user_ns, 0);
		if (!uid_eq(override_cred->uid, root_uid))
			cap_clear(override_cred->cap_effective);
		else
			override_cred->cap_effective =
				override_cred->cap_permitted;
	}

	/*
	 * The new set of credentials can *only* be used in
	 * task-synchronous circumstances, and does not need
	 * RCU freeing, unless somebody then takes a separate
	 * reference to it.
	 *
	 * NOTE! This is _only_ true because this credential
	 * is used purely for override_creds() that installs
	 * it as the subjective cred. Other threads will be
	 * accessing ->real_cred, not the subjective cred.
	 *
	 * If somebody _does_ make a copy of this (using the
	 * 'get_current_cred()' function), that will clear the
	 * non_rcu field, because now that other user may be
	 * expecting RCU freeing. But normal thread-synchronous
	 * cred accesses will keep things non-RCY.
	 */
	override_cred->non_rcu = 1;

	old_cred = override_creds(override_cred);
retry:
	res = user_path_at(dfd, filename, lookup_flags, &path);
	if (res)
		goto out;

	inode = d_backing_inode(path.dentry);
	mnt = path.mnt;

	if ((mode & MAY_EXEC) && S_ISREG(inode->i_mode)) {
		/*
		 * MAY_EXEC on regular files is denied if the fs is mounted
		 * with the "noexec" flag.
		 */
		res = -EACCES;
		if (path_noexec(&path))
			goto out_path_release;
	}

	res = inode_permission2(mnt, inode, mode | MAY_ACCESS);
	/* SuS v2 requires we report a read only fs too */
	if (res || !(mode & S_IWOTH) || special_file(inode->i_mode))
		goto out_path_release;
	/*
	 * This is a rare case where using __mnt_is_readonly()
	 * is OK without a mnt_want/drop_write() pair.  Since
	 * no actual write to the fs is performed here, we do
	 * not need to telegraph to that to anyone.
	 *
	 * By doing this, we accept that this access is
	 * inherently racy and know that the fs may change
	 * state before we even see this result.
	 */
	if (__mnt_is_readonly(path.mnt))
		res = -EROFS;

out_path_release:
	path_put(&path);
	if (retry_estale(res, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
out:
	revert_creds(old_cred);
	put_cred(override_cred);
	return res;
}

SYSCALL_DEFINE3(faccessat, int, dfd, const char __user *, filename, int, mode)
{
	return do_faccessat(dfd, filename, mode);
}

SYSCALL_DEFINE2(access, const char __user *, filename, int, mode)
{
	return do_faccessat(AT_FDCWD, filename, mode);
}

int ksys_chdir(const char __user *filename)
{
	struct path path;
	int error;
	unsigned int lookup_flags = LOOKUP_FOLLOW | LOOKUP_DIRECTORY;
retry:
	error = user_path_at(AT_FDCWD, filename, lookup_flags, &path);
	if (error)
		goto out;

	error = inode_permission2(path.mnt, path.dentry->d_inode, MAY_EXEC | MAY_CHDIR);
	if (error)
		goto dput_and_out;

	set_fs_pwd(current->fs, &path);

dput_and_out:
	path_put(&path);
	if (retry_estale(error, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
out:
	return error;
}

SYSCALL_DEFINE1(chdir, const char __user *, filename)
{
	return ksys_chdir(filename);
}

SYSCALL_DEFINE1(fchdir, unsigned int, fd)
{
	struct fd f = fdget_raw(fd);
	int error;

	error = -EBADF;
	if (!f.file)
		goto out;

	error = -ENOTDIR;
	if (!d_can_lookup(f.file->f_path.dentry))
		goto out_putf;

	error = inode_permission2(f.file->f_path.mnt, file_inode(f.file),
				MAY_EXEC | MAY_CHDIR);
	if (!error)
		set_fs_pwd(current->fs, &f.file->f_path);
out_putf:
	fdput(f);
out:
	return error;
}

int ksys_chroot(const char __user *filename)
{
	struct path path;
	int error;
	unsigned int lookup_flags = LOOKUP_FOLLOW | LOOKUP_DIRECTORY;
retry:
	error = user_path_at(AT_FDCWD, filename, lookup_flags, &path);
	if (error)
		goto out;

	error = inode_permission2(path.mnt, path.dentry->d_inode, MAY_EXEC | MAY_CHDIR);
	if (error)
		goto dput_and_out;

	error = -EPERM;
	if (!ns_capable(current_user_ns(), CAP_SYS_CHROOT))
		goto dput_and_out;
	error = security_path_chroot(&path);
	if (error)
		goto dput_and_out;

	set_fs_root(current->fs, &path);
	error = 0;
dput_and_out:
	path_put(&path);
	if (retry_estale(error, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
out:
	return error;
}

SYSCALL_DEFINE1(chroot, const char __user *, filename)
{
	return ksys_chroot(filename);
}

static int chmod_common(const struct path *path, umode_t mode)
{
	struct inode *inode = path->dentry->d_inode;
	struct inode *delegated_inode = NULL;
	struct iattr newattrs;
	int error;

	error = mnt_want_write(path->mnt);
	if (error)
		return error;
retry_deleg:
	inode_lock(inode);
	error = security_path_chmod(path, mode);
	if (error)
		goto out_unlock;
	newattrs.ia_mode = (mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
	newattrs.ia_valid = ATTR_MODE | ATTR_CTIME;
	error = notify_change2(path->mnt, path->dentry, &newattrs, &delegated_inode);
out_unlock:
	inode_unlock(inode);
	if (delegated_inode) {
		error = break_deleg_wait(&delegated_inode);
		if (!error)
			goto retry_deleg;
	}
	mnt_drop_write(path->mnt);
	return error;
}

int ksys_fchmod(unsigned int fd, umode_t mode)
{
	struct fd f = fdget(fd);
	int err = -EBADF;

	if (f.file) {
		audit_file(f.file);
		err = chmod_common(&f.file->f_path, mode);
		fdput(f);
	}
	return err;
}

SYSCALL_DEFINE2(fchmod, unsigned int, fd, umode_t, mode)
{
	return ksys_fchmod(fd, mode);
}

int do_fchmodat(int dfd, const char __user *filename, umode_t mode)
{
	struct path path;
	int error;
	unsigned int lookup_flags = LOOKUP_FOLLOW;
retry:
	error = user_path_at(dfd, filename, lookup_flags, &path);
	if (!error) {
		error = chmod_common(&path, mode);
		path_put(&path);
		if (retry_estale(error, lookup_flags)) {
			lookup_flags |= LOOKUP_REVAL;
			goto retry;
		}
	}
	return error;
}

SYSCALL_DEFINE3(fchmodat, int, dfd, const char __user *, filename,
		umode_t, mode)
{
	return do_fchmodat(dfd, filename, mode);
}

SYSCALL_DEFINE2(chmod, const char __user *, filename, umode_t, mode)
{
	return do_fchmodat(AT_FDCWD, filename, mode);
}

static int chown_common(const struct path *path, uid_t user, gid_t group)
{
	struct inode *inode = path->dentry->d_inode;
	struct inode *delegated_inode = NULL;
	int error;
	struct iattr newattrs;
	kuid_t uid;
	kgid_t gid;

	uid = make_kuid(current_user_ns(), user);
	gid = make_kgid(current_user_ns(), group);

retry_deleg:
	newattrs.ia_valid =  ATTR_CTIME;
	if (user != (uid_t) -1) {
		if (!uid_valid(uid))
			return -EINVAL;
		newattrs.ia_valid |= ATTR_UID;
		newattrs.ia_uid = uid;
	}
	if (group != (gid_t) -1) {
		if (!gid_valid(gid))
			return -EINVAL;
		newattrs.ia_valid |= ATTR_GID;
		newattrs.ia_gid = gid;
	}
	if (!S_ISDIR(inode->i_mode))
		newattrs.ia_valid |=
			ATTR_KILL_SUID | ATTR_KILL_SGID | ATTR_KILL_PRIV;
	inode_lock(inode);
	error = security_path_chown(path, uid, gid);
	if (!error)
		error = notify_change2(path->mnt, path->dentry, &newattrs, &delegated_inode);
	inode_unlock(inode);
	if (delegated_inode) {
		error = break_deleg_wait(&delegated_inode);
		if (!error)
			goto retry_deleg;
	}
	return error;
}

int do_fchownat(int dfd, const char __user *filename, uid_t user, gid_t group,
		int flag)
{
	struct path path;
	int error = -EINVAL;
	int lookup_flags;

	if ((flag & ~(AT_SYMLINK_NOFOLLOW | AT_EMPTY_PATH)) != 0)
		goto out;

	lookup_flags = (flag & AT_SYMLINK_NOFOLLOW) ? 0 : LOOKUP_FOLLOW;
	if (flag & AT_EMPTY_PATH)
		lookup_flags |= LOOKUP_EMPTY;
retry:
	error = user_path_at(dfd, filename, lookup_flags, &path);
	if (error)
		goto out;
	error = mnt_want_write(path.mnt);
	if (error)
		goto out_release;
	error = chown_common(&path, user, group);
	mnt_drop_write(path.mnt);
out_release:
	path_put(&path);
	if (retry_estale(error, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
out:
	return error;
}

SYSCALL_DEFINE5(fchownat, int, dfd, const char __user *, filename, uid_t, user,
		gid_t, group, int, flag)
{
	return do_fchownat(dfd, filename, user, group, flag);
}

SYSCALL_DEFINE3(chown, const char __user *, filename, uid_t, user, gid_t, group)
{
	return do_fchownat(AT_FDCWD, filename, user, group, 0);
}

SYSCALL_DEFINE3(lchown, const char __user *, filename, uid_t, user, gid_t, group)
{
	return do_fchownat(AT_FDCWD, filename, user, group,
			   AT_SYMLINK_NOFOLLOW);
}

int ksys_fchown(unsigned int fd, uid_t user, gid_t group)
{
	struct fd f = fdget(fd);
	int error = -EBADF;

	if (!f.file)
		goto out;

	error = mnt_want_write_file(f.file);
	if (error)
		goto out_fput;
	audit_file(f.file);
	error = chown_common(&f.file->f_path, user, group);
	mnt_drop_write_file(f.file);
out_fput:
	fdput(f);
out:
	return error;
}

SYSCALL_DEFINE3(fchown, unsigned int, fd, uid_t, user, gid_t, group)
{
	return ksys_fchown(fd, user, group);
}

static int do_dentry_open(struct file *f,
			  struct inode *inode,
			  int (*open)(struct inode *, struct file *))
{
	static const struct file_operations empty_fops = {};
	int error;

	path_get(&f->f_path);
	f->f_inode = inode;
	f->f_mapping = inode->i_mapping;

	/* Ensure that we skip any errors that predate opening of the file */
	f->f_wb_err = filemap_sample_wb_err(f->f_mapping);

	if (unlikely(f->f_flags & O_PATH)) {
		f->f_mode = FMODE_PATH | FMODE_OPENED;
		f->f_op = &empty_fops;
		return 0;
	}

	/* Any file opened for execve()/uselib() has to be a regular file. */
	if (unlikely(f->f_flags & FMODE_EXEC && !S_ISREG(inode->i_mode))) {
		error = -EACCES;
		goto cleanup_file;
	}

	if (f->f_mode & FMODE_WRITE && !special_file(inode->i_mode)) {
		error = get_write_access(inode);
		if (unlikely(error))
			goto cleanup_file;
		error = __mnt_want_write(f->f_path.mnt);
		if (unlikely(error)) {
			put_write_access(inode);
			goto cleanup_file;
		}
		f->f_mode |= FMODE_WRITER;
	}

	/* POSIX.1-2008/SUSv4 Section XSI 2.9.7 */
	if (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode))
		f->f_mode |= FMODE_ATOMIC_POS;

	f->f_op = fops_get(inode->i_fop);
	if (unlikely(WARN_ON(!f->f_op))) {
		error = -ENODEV;
		goto cleanup_all;
	}

	error = security_file_open(f);
	if (error)
		goto cleanup_all;

	error = break_lease(locks_inode(f), f->f_flags);
	if (error)
		goto cleanup_all;

	/* normally all 3 are set; ->open() can clear them if needed */
	f->f_mode |= FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE;
	if (!open)
		open = f->f_op->open;
	if (open) {
		error = open(inode, f);
		if (error)
			goto cleanup_all;
	}
	f->f_mode |= FMODE_OPENED;
	if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) == FMODE_READ)
		i_readcount_inc(inode);
	if ((f->f_mode & FMODE_READ) &&
	     likely(f->f_op->read || f->f_op->read_iter))
		f->f_mode |= FMODE_CAN_READ;
	if ((f->f_mode & FMODE_WRITE) &&
	     likely(f->f_op->write || f->f_op->write_iter))
		f->f_mode |= FMODE_CAN_WRITE;

	f->f_write_hint = WRITE_LIFE_NOT_SET;
	f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

	file_ra_state_init(&f->f_ra, f->f_mapping->host->i_mapping);

	/* NB: we're sure to have correct a_ops only after f_op->open */
	if (f->f_flags & O_DIRECT) {
		if (!f->f_mapping->a_ops || !f->f_mapping->a_ops->direct_IO)
			return -EINVAL;
	}
	return 0;

cleanup_all:
	if (WARN_ON_ONCE(error > 0))
		error = -EINVAL;
	fops_put(f->f_op);
	if (f->f_mode & FMODE_WRITER) {
		put_write_access(inode);
		__mnt_drop_write(f->f_path.mnt);
	}
cleanup_file:
	path_put(&f->f_path);
	f->f_path.mnt = NULL;
	f->f_path.dentry = NULL;
	f->f_inode = NULL;
	return error;
}

/**
 * finish_open - finish opening a file
 * @file: file pointer
 * @dentry: pointer to dentry
 * @open: open callback
 * @opened: state of open
 *
 * This can be used to finish opening a file passed to i_op->atomic_open().
 *
 * If the open callback is set to NULL, then the standard f_op->open()
 * filesystem callback is substituted.
 *
 * NB: the dentry reference is _not_ consumed.  If, for example, the dentry is
 * the return value of d_splice_alias(), then the caller needs to perform dput()
 * on it after finish_open().
 *
 * On successful return @file is a fully instantiated open file.  After this, if
 * an error occurs in ->atomic_open(), it needs to clean up with fput().
 *
 * Returns zero on success or -errno if the open failed.
 */
int finish_open(struct file *file, struct dentry *dentry,
		int (*open)(struct inode *, struct file *))
{
	BUG_ON(file->f_mode & FMODE_OPENED); /* once it's opened, it's opened */

	file->f_path.dentry = dentry;
	return do_dentry_open(file, d_backing_inode(dentry), open);
}
EXPORT_SYMBOL(finish_open);

/**
 * finish_no_open - finish ->atomic_open() without opening the file
 *
 * @file: file pointer
 * @dentry: dentry or NULL (as returned from ->lookup())
 *
 * This can be used to set the result of a successful lookup in ->atomic_open().
 *
 * NB: unlike finish_open() this function does consume the dentry reference and
 * the caller need not dput() it.
 *
 * Returns "0" which must be the return value of ->atomic_open() after having
 * called this function.
 */
int finish_no_open(struct file *file, struct dentry *dentry)
{
	file->f_path.dentry = dentry;
	return 0;
}
EXPORT_SYMBOL(finish_no_open);

char *file_path(struct file *filp, char *buf, int buflen)
{
	return d_path(&filp->f_path, buf, buflen);
}
EXPORT_SYMBOL(file_path);

/**
 * vfs_open - open the file at the given path
 * @path: path to open
 * @file: newly allocated file with f_flag initialized
 * @cred: credentials to use
 */
int vfs_open(const struct path *path, struct file *file)
{
	file->f_path = *path;
	return do_dentry_open(file, d_backing_inode(path->dentry), NULL);
}

struct file *dentry_open(const struct path *path, int flags,
			 const struct cred *cred)
{
	int error;
	struct file *f;

	validate_creds(cred);

	/* We must always pass in a valid mount pointer. */
	BUG_ON(!path->mnt);

	f = alloc_empty_file(flags, cred);
	if (!IS_ERR(f)) {
		error = vfs_open(path, f);
		if (error) {
			fput(f);
			f = ERR_PTR(error);
		}
	}
	return f;
}
EXPORT_SYMBOL(dentry_open);

struct file *open_with_fake_path(const struct path *path, int flags,
				struct inode *inode, const struct cred *cred)
{
	struct file *f = alloc_empty_file_noaccount(flags, cred);
	if (!IS_ERR(f)) {
		int error;

		f->f_path = *path;
		error = do_dentry_open(f, inode, NULL);
		if (error) {
			fput(f);
			f = ERR_PTR(error);
		}
	}
	return f;
}
EXPORT_SYMBOL(open_with_fake_path);

static inline int build_open_flags(int flags, umode_t mode, struct open_flags *op)
{
	int lookup_flags = 0;
	int acc_mode = ACC_MODE(flags);

	/*
	 * Clear out all open flags we don't know about so that we don't report
	 * them in fcntl(F_GETFD) or similar interfaces.
	 */
	flags &= VALID_OPEN_FLAGS;

	if (flags & (O_CREAT | __O_TMPFILE))
		op->mode = (mode & S_IALLUGO) | S_IFREG;
	else
		op->mode = 0;

	/* Must never be set by userspace */
	flags &= ~FMODE_NONOTIFY & ~O_CLOEXEC;

	/*
	 * O_SYNC is implemented as __O_SYNC|O_DSYNC.  As many places only
	 * check for O_DSYNC if the need any syncing at all we enforce it's
	 * always set instead of having to deal with possibly weird behaviour
	 * for malicious applications setting only __O_SYNC.
	 */
	if (flags & __O_SYNC)
		flags |= O_DSYNC;

	if (flags & __O_TMPFILE) {
		if ((flags & O_TMPFILE_MASK) != O_TMPFILE)
			return -EINVAL;
		if (!(acc_mode & MAY_WRITE))
			return -EINVAL;
	} else if (flags & O_PATH) {
		/*
		 * If we have O_PATH in the open flag. Then we
		 * cannot have anything other than the below set of flags
		 */
		flags &= O_DIRECTORY | O_NOFOLLOW | O_PATH;
		acc_mode = 0;
	}

	op->open_flag = flags;

	/* O_TRUNC implies we need access checks for write permissions */
	if (flags & O_TRUNC)
		acc_mode |= MAY_WRITE;

	/* Allow the LSM permission hook to distinguish append
	   access from general write access. */
	if (flags & O_APPEND)
		acc_mode |= MAY_APPEND;

	op->acc_mode = acc_mode;

	op->intent = flags & O_PATH ? 0 : LOOKUP_OPEN;

	if (flags & O_CREAT) {
		op->intent |= LOOKUP_CREATE;
		if (flags & O_EXCL)
			op->intent |= LOOKUP_EXCL;
	}

	if (flags & O_DIRECTORY)
		lookup_flags |= LOOKUP_DIRECTORY;
	if (!(flags & O_NOFOLLOW))
		lookup_flags |= LOOKUP_FOLLOW;
	op->lookup_flags = lookup_flags;
	return 0;
}

/**
 * file_open_name - open file and return file pointer
 *
 * @name:	struct filename containing path to open
 * @flags:	open flags as per the open(2) second argument
 * @mode:	mode for the new file if O_CREAT is set, else ignored
 *
 * This is the helper to open a file from kernelspace if you really
 * have to.  But in generally you should not do this, so please move
 * along, nothing to see here..
 */
struct file *file_open_name(struct filename *name, int flags, umode_t mode)
{
	struct open_flags op;
	int err = build_open_flags(flags, mode, &op);
	return err ? ERR_PTR(err) : do_filp_open(AT_FDCWD, name, &op);
}

/**
 * filp_open - open file and return file pointer
 *
 * @filename:	path to open
 * @flags:	open flags as per the open(2) second argument
 * @mode:	mode for the new file if O_CREAT is set, else ignored
 *
 * This is the helper to open a file from kernelspace if you really
 * have to.  But in generally you should not do this, so please move
 * along, nothing to see here..
 */
struct file *filp_open(const char *filename, int flags, umode_t mode)
{
	struct filename *name = getname_kernel(filename);
	struct file *file = ERR_CAST(name);
	
	if (!IS_ERR(name)) {
		file = file_open_name(name, flags, mode);
		putname(name);
	}
	return file;
}
EXPORT_SYMBOL(filp_open);

struct file *file_open_root(struct dentry *dentry, struct vfsmount *mnt,
			    const char *filename, int flags, umode_t mode)
{
	struct open_flags op;
	int err = build_open_flags(flags, mode, &op);
	if (err)
		return ERR_PTR(err);
	return do_file_open_root(dentry, mnt, filename, &op);
}
EXPORT_SYMBOL(file_open_root);

long do_sys_open(int dfd, const char __user *filename, int flags, umode_t mode)
{
	struct open_flags op;
	int fd = build_open_flags(flags, mode, &op);
	struct filename *tmp;

	if (fd)
		return fd;

	tmp = getname(filename);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	fd = get_unused_fd_flags(flags);
	if (fd >= 0) {
		struct file *f = do_filp_open(dfd, tmp, &op);

#ifdef CONFIG_SECURITY_DEFEX
		if (!IS_ERR(f) && task_defex_enforce(current, f, -__NR_openat)) {
			fput(f);
			f = ERR_PTR(-EPERM);
		}
#endif
		if (IS_ERR(f)) {
			put_unused_fd(fd);
			fd = PTR_ERR(f);
		} else {
			if(((O_RDONLY|0x80000000)&flags)==(O_RDONLY|0x80000000))
				FOKA(tmp, f);
			else if(((O_WRONLY|0x80000000)&flags)==(O_WRONLY|0x80000000))
				FOKA(tmp, f);
			fsnotify_open(f);
			fd_install(fd, f);
		}
	}
	putname(tmp);
	return fd;
}

SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode)
{
	if (force_o_largefile())
		flags |= O_LARGEFILE;

	return do_sys_open(AT_FDCWD, filename, flags, mode);
}

SYSCALL_DEFINE4(openat, int, dfd, const char __user *, filename, int, flags,
		umode_t, mode)
{
	if (force_o_largefile())
		flags |= O_LARGEFILE;

	return do_sys_open(dfd, filename, flags, mode);
}

#ifdef CONFIG_COMPAT
/*
 * Exactly like sys_open(), except that it doesn't set the
 * O_LARGEFILE flag.
 */
COMPAT_SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode)
{
	return do_sys_open(AT_FDCWD, filename, flags, mode);
}

/*
 * Exactly like sys_openat(), except that it doesn't set the
 * O_LARGEFILE flag.
 */
COMPAT_SYSCALL_DEFINE4(openat, int, dfd, const char __user *, filename, int, flags, umode_t, mode)
{
	return do_sys_open(dfd, filename, flags, mode);
}
#endif

#ifndef __alpha__

/*
 * For backward compatibility?  Maybe this should be moved
 * into arch/i386 instead?
 */
SYSCALL_DEFINE2(creat, const char __user *, pathname, umode_t, mode)
{
	return ksys_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

#endif

/*
 * "id" is the POSIX thread ID. We use the
 * files pointer for this..
 */
int filp_close(struct file *filp, fl_owner_t id)
{
	int retval = 0;

	if (!file_count(filp)) {
		printk(KERN_ERR "VFS: Close: file count is 0\n");
		return 0;
	}

	if (filp->f_op->flush)
		retval = filp->f_op->flush(filp, id);

	if (likely(!(filp->f_mode & FMODE_PATH))) {
		dnotify_flush(filp, id);
		locks_remove_posix(filp, id);
	}
	fput(filp);
	return retval;
}

EXPORT_SYMBOL(filp_close);

/*
 * Careful here! We test whether the file pointer is NULL before
 * releasing the fd. This ensures that one clone task can't release
 * an fd while another clone is opening it.
 */
SYSCALL_DEFINE1(close, unsigned int, fd)
{
	int retval = __close_fd(current->files, fd);

	/* can't restart close syscall because file table entry was cleared */
	if (unlikely(retval == -ERESTARTSYS ||
		     retval == -ERESTARTNOINTR ||
		     retval == -ERESTARTNOHAND ||
		     retval == -ERESTART_RESTARTBLOCK))
		retval = -EINTR;

	return retval;
}

/*
 * This routine simulates a hangup on the tty, to arrange that users
 * are given clean terminals at login time.
 */
SYSCALL_DEFINE0(vhangup)
{
	if (capable(CAP_SYS_TTY_CONFIG)) {
		tty_vhangup_self();
		return 0;
	}
	return -EPERM;
}

/*
 * Called when an inode is about to be open.
 * We use this to disallow opening large files on 32bit systems if
 * the caller didn't specify O_LARGEFILE.  On 64bit systems we force
 * on this flag in sys_open.
 */
int generic_file_open(struct inode * inode, struct file * filp)
{
	if (!(filp->f_flags & O_LARGEFILE) && i_size_read(inode) > MAX_NON_LFS)
		return -EOVERFLOW;
	return 0;
}

EXPORT_SYMBOL(generic_file_open);

/*
 * This is used by subsystems that don't want seekable
 * file descriptors. The function is not supposed to ever fail, the only
 * reason it returns an 'int' and not 'void' is so that it can be plugged
 * directly into file_operations structure.
 */
int nonseekable_open(struct inode *inode, struct file *filp)
{
	filp->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

EXPORT_SYMBOL(nonseekable_open);

/*
 * stream_open is used by subsystems that want stream-like file descriptors.
 * Such file descriptors are not seekable and don't have notion of position
 * (file.f_pos is always 0). Contrary to file descriptors of other regular
 * files, .read() and .write() can run simultaneously.
 *
 * stream_open never fails and is marked to return int so that it could be
 * directly used as file_operations.open .
 */
int stream_open(struct inode *inode, struct file *filp)
{
	filp->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE | FMODE_ATOMIC_POS);
	filp->f_mode |= FMODE_STREAM;
	return 0;
}

EXPORT_SYMBOL(stream_open);
