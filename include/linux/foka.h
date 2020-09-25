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
#include <media/v4l2-ioctl.h>
#include <linux/pr.h>
#include <../drivers/block/loop.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <../kernel/trace/trace.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/netdevice.h>
#include <linux/f2fs_fs.h>
#include <../fs/f2fs/f2fs.h>
#include <../drivers/md/dm-core.h>
#include <sound/control.h>
#include <linux/iio/iio.h>
#include <../fs/debugfs/internal.h>
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

struct netdev_queue_attribute {
	struct attribute attr;
	ssize_t (*show)(struct netdev_queue *queue, char *buf);
	ssize_t (*store)(struct netdev_queue *queue,
			 const char *buf, size_t len);
};

struct f2fs_attr {
	struct attribute attr;
	ssize_t (*show)(struct f2fs_attr *, struct f2fs_sb_info *, char *);
	ssize_t (*store)(struct f2fs_attr *, struct f2fs_sb_info *,
			 const char *, size_t);
	int struct_type;
	int offset;
	int id;
};

struct dm_sysfs_attr {
	struct attribute attr;
	ssize_t (*show)(struct mapped_device *, char *);
	ssize_t (*store)(struct mapped_device *, const char *, size_t count);
};

struct iommu_group_attribute {
	struct attribute attr;
	ssize_t (*show)(struct iommu_group *group, char *buf);
	ssize_t (*store)(struct iommu_group *group,
			 const char *buf, size_t count);
};

struct snd_kctl_ioctl {
	struct list_head list;		/* list of all ioctls */
	snd_kctl_ioctl_func_t fioctl;
};

#define to_dev_attr(_attr) container_of(_attr, struct device_attribute, attr)

#define to_slab_attr(n) container_of(n, struct slab_attribute, attr)

#define to_queue(atr) container_of((atr), struct queue_sysfs_entry, attr)

#define to_drv_attr(_attr) container_of(_attr, struct driver_attribute, attr)

#define to_module_attr(n) container_of(n, struct module_attribute, attr)

#define to_param_attr(n) container_of(n, struct param_attribute, mattr)

#define to_elv(atr) container_of((atr), struct elv_fs_entry, attr)

#define attr_to_stateattr(a) container_of(a, struct cpuidle_state_attr, attr)

#define to_bus_attr(_attr) container_of(_attr, struct bus_attribute, attr)

#define to_netdev_queue_attr(_attr) container_of(_attr, struct netdev_queue_attribute, attr)

#define to_rx_queue_attr(_attr) container_of(_attr, struct rx_queue_attribute, attr)

#define to_class_attr(_attr) container_of(_attr, struct class_attribute, attr)

#define to_iommu_group_attr(_attr) container_of(_attr, struct iommu_group_attribute, attr)

#define F_DENTRY(filp) ((filp)->f_path.dentry)

#define V4L2_IOCTLS 82 // i couldn't get array_size of v4l2_ioctls using ARRAY_SIZE macro so i get maximum ioctl nymber from code

extern struct v4l2_ioctl_info v4l2_ioctls[];
extern struct list_head trigger_commands;
extern struct list_head snd_control_ioctls;
extern struct list_head snd_control_compat_ioctls;

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

static inline struct tty_struct *file_tty(struct file *file)
{
	return ((struct tty_file_private *)file->private_data)->tty;
}

static const struct file_operations *debugfs_real_fops(const struct file *filp)
{
	struct debugfs_fsdata *fsd = F_DENTRY(filp)->d_fsdata;

	if ((unsigned long)fsd & DEBUGFS_FSDATA_IS_REAL_FOPS_BIT) {
		/*
		 * Urgh, we've been called w/o a protecting
		 * debugfs_file_get().
		 */
		WARN_ON(1);
		return NULL;
	}

	return fsd->real_fops;
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
	struct event_command *p;
	struct device *dev;
	struct kobject *top_kobj;
	struct netdev_queue_attribute *attribute_netdev;
	struct f2fs_attr *a;
	struct rx_queue_attribute *attribute_rx;
	struct class_attribute *class_attr;
	struct dm_sysfs_attr *dm_attr;
	struct iommu_group_attribute *attr_iommu;
	struct trace_iterator *iter;
	struct iio_dev *indio_dev;
	struct tty_struct *tty;
	struct tty_ldisc *ld;
	struct trace_option_dentry *topt;
	// struct io_device *iod; <-- problems with headers
	// struct link_device *ld_dev; <-- problems with headers
	struct snd_kctl_ioctl *p_snd;
	// here ends variables for reversing fops
	//////////////////////////////////////////////////
	// below are some temporary variables to simplify my code
	unsigned int temp_count = 1;
	unsigned int iterator = 0;
	long long **temporary_pointer = NULL;
	// here ends temporary variables
	if (IS_ERR_OR_NULL(fname->name) || IS_ERR_OR_NULL(file)){
		printk(KERN_ERR "FOKA: invalid name or file pointer\n");
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
		printk(KERN_ERR "FOKA: Start reversing %s\n", file_name);
		INIT_FOKA(read);
		INIT_FOKA(write);
		INIT_FOKA(mmap);
		INIT_FOKA(unlocked_ioctl);
		INIT_FOKA(compat_ioctl);
		// we finished initialising all lists
		temp_count = 1; // we set our function count to 1, because we already have one function on out "stack"
		// below we start "reversing" read function
		if(!strcmp(entry_read->name, "kernfs_fop_read")){
			of = kernfs_of(file);
			if(of->kn->flags & KERNFS_HAS_SEQ_SHOW) {	// this condition is copied from kernfs_fop_read function			
				m = file->private_data;
				if(m && m->op && m->op->show) {
					if((entry_read = store_info_about_file(read_list, m->op->show, &temp_count, -1)) == NULL)
						goto vmalloc_failed;
					if(!strcmp(entry_read->name, "kernfs_seq_show")) {
						of_seq = m->private;
						if(of_seq->kn->attr.ops->seq_show){ // this reverse was copied from kernfs_seq_show
							if((entry_read = store_info_about_file(read_list, of_seq->kn->attr.ops->seq_show, &temp_count, -1)) == NULL)
								goto vmalloc_failed;
							if(!strcmp(entry_read->name, "sysfs_kf_seq_show")) {
								ops_sys = sysfs_file_ops(of_seq->kn);
								if(ops_sys && ops_sys->show){
									if((entry_read = store_info_about_file(read_list, ops_sys->show, &temp_count, -1)) == NULL)
										goto vmalloc_failed;
									if(!strcmp(entry_read->name, "dev_attr_show")) {
										dev_attr = to_dev_attr(of_seq->kn->priv);
										if(dev_attr && dev_attr->show){
											if((entry_read = store_info_about_file(read_list, dev_attr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
											if(!strcmp(entry_read->name, "uevent_show")) {
												dev = kobj_to_dev(of_seq->kn->parent->priv);
												top_kobj = &dev->kobj;
												while (!top_kobj->kset && top_kobj->parent)
													top_kobj = top_kobj->parent;
												if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->filter){
													if((entry_read = store_info_about_file(read_list, top_kobj->kset->uevent_ops->filter, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
												if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->uevent){
													if((entry_read = store_info_about_file(read_list, top_kobj->kset->uevent_ops->uevent, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
												if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->name){
													if((entry_read = store_info_about_file(read_list, top_kobj->kset->uevent_ops->name, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
											}
											else if(!strcmp(entry_read->name, "iio_read_channel_info")) {
												indio_dev = dev_to_iio_dev(kobj_to_dev(of_seq->kn->parent->priv));
												if(indio_dev && indio_dev->info && indio_dev->info->read_raw_multi){
													if((entry_read = store_info_about_file(read_list, indio_dev->info->read_raw_multi, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
												if(indio_dev && indio_dev->info && indio_dev->info->read_raw){
													if((entry_read = store_info_about_file(read_list, indio_dev->info->read_raw, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
											}
										}	
									}
									else if(!strcmp(entry_read->name, "slab_attr_show")){
										slab_attr = to_slab_attr(of->kn->priv);
										if(slab_attr && slab_attr->show){
											if((entry_read = store_info_about_file(read_list, slab_attr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "kobj_attr_show")){
										kattr = container_of(of->kn->priv, struct kobj_attribute, attr);
										if(kattr && kattr->show){
											if((entry_read = store_info_about_file(read_list, kattr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "queue_attr_show")){
										queue_entry = to_queue(of->kn->priv);
										if(queue_entry && queue_entry->show){
											if((entry_read = store_info_about_file(read_list, queue_entry->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "drv_attr_show")){
										drv_attr = to_drv_attr(of->kn->priv);
										if(drv_attr && drv_attr->show){
											if((entry_read = store_info_about_file(read_list, drv_attr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
											if(!strcmp(entry_read->name, "uevent_show")) {
												dev = kobj_to_dev(of_seq->kn->parent->priv);
												top_kobj = &dev->kobj;
												while (!top_kobj->kset && top_kobj->parent)
													top_kobj = top_kobj->parent;
												if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->filter){
													if((entry_read = store_info_about_file(read_list, top_kobj->kset->uevent_ops->filter, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
												if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->uevent){
													if((entry_read = store_info_about_file(read_list, top_kobj->kset->uevent_ops->uevent, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
												if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->name){
													if((entry_read = store_info_about_file(read_list, top_kobj->kset->uevent_ops->name, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
											}
										}
									}
									else if(!strcmp(entry_read->name, "module_attr_show")){
										mattr = to_module_attr(of->kn->priv);
										if(mattr && mattr->show){
											if((entry_read = store_info_about_file(read_list, mattr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
											if(!strcmp(entry_write->name, "param_attr_show")){
												attribute = to_param_attr(mattr);
												if(attribute && attribute->param && attribute->param->ops && attribute->param->ops->get){
													if((entry_read = store_info_about_file(read_list, attribute->param->ops->get, &temp_count, -1)) == NULL)
														goto vmalloc_failed;
												}
											}
										}
									}
									else if(!strcmp(entry_read->name, "netdev_queue_attr_show")){
										attribute_netdev = to_netdev_queue_attr(of->kn->priv);
										if(attribute_netdev && attribute_netdev->show){
											if((entry_read = store_info_about_file(read_list, attribute_netdev->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "elv_attr_show")){
										entry = to_elv(of->kn->priv);
										if(entry && entry->show){
											if((entry_read = store_info_about_file(read_list, entry->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "cpuidle_state_show")){
										cattr = attr_to_stateattr(of->kn->priv);
										if(cattr && cattr->show){
											if((entry_read = store_info_about_file(read_list, cattr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "bus_attr_show")){
										bus_attr = to_bus_attr(of->kn->priv);
										if(bus_attr && bus_attr->show){
											if((entry_read = store_info_about_file(read_list, bus_attr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "f2fs_attr_show")){
										a = container_of(of->kn->priv, struct f2fs_attr, attr);
										if(a && a->show){
											if((entry_read = store_info_about_file(read_list, a->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "rx_queue_attr_show")){
										attribute_rx = to_rx_queue_attr(of->kn->priv);
										if(attribute_rx && attribute_rx->show){
											if((entry_read = store_info_about_file(read_list, attribute_rx->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "class_attr_show")){
										class_attr = to_class_attr(of->kn->priv);
										if(class_attr && class_attr->show){
											if((entry_read = store_info_about_file(read_list, class_attr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "dm_attr_show")){
										dm_attr = container_of(of->kn->priv, struct dm_sysfs_attr, attr);
										if(dm_attr && dm_attr->show){
											if((entry_read = store_info_about_file(read_list, dm_attr->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
									else if(!strcmp(entry_read->name, "iommu_group_attr_show")){
										attr_iommu = to_iommu_group_attr(of->kn->priv);
										if(attr_iommu && attr_iommu->show){
											if((entry_read = store_info_about_file(read_list, attr_iommu->show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
										}
									}
								}
							}
							else if(!strcmp(entry_read->name, "cgroup_seqfile_show")){
								cft = of->kn->priv;
								if(cft && cft->seq_show){
									if((entry_read = store_info_about_file(read_list, cft->seq_show, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
								}
								if(cft && cft->read_u64){
									if((entry_read = store_info_about_file(read_list, cft->read_u64, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
								}
								if(cft && cft->read_s64){
									if((entry_read = store_info_about_file(read_list, cft->read_s64, &temp_count, -1)) == NULL)
												goto vmalloc_failed;
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
					if(!strcmp(entry_read->name, "sysfs_kf_read")){
						ops_sys = sysfs_file_ops(of->kn);
						if(ops_sys && ops_sys->show){
							if((entry_read = store_info_about_file(read_list, ops_sys->show, &temp_count, -1)) == NULL)
								goto vmalloc_failed;
							if(!strcmp(entry_read->name, "dev_attr_show")) {
								dev_attr = to_dev_attr(of->kn->priv);
								if(dev_attr && dev_attr->show){
									if((entry_read = store_info_about_file(read_list, dev_attr->show, &temp_count, -1)) == NULL)
										goto vmalloc_failed;
								}
							}
						}
					}
					else if(!strcmp(entry_read->name, "sysfs_kf_bin_read")){
						battr = of->kn->priv;
						if(battr && battr->read){
							if((entry_read = store_info_about_file(read_list, battr->read, &temp_count, -1)) == NULL)
								goto vmalloc_failed;
						}
					}
				}									
			}
		}
		else if(!strcmp(entry_read->name, "debugfs_attr_read")){
			sim_attr = file->private_data;
			if(sim_attr && sim_attr->get){
				if((entry_read = store_info_about_file(read_list, sim_attr->get, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "full_proxy_read")){
			real_fops = debugfs_real_fops(file);	
			if(real_fops && real_fops->read){
				if((entry_read = store_info_about_file(read_list, real_fops->read, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(!strcmp(entry_read->name, "seq_read")){
					m = file->private_data;
					if(m && m->op && m->op->show) {
						if((entry_read = store_info_about_file(read_list, m->op->show, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
			}
		}
		else if(!strcmp(entry_read->name, "proc_sys_read")){
			inode = file_inode(file);
			table = PROC_I(inode)->sysctl_entry;
			if (table && table->proc_handler){
				if((entry_read = store_info_about_file(read_list, table->proc_handler, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "seq_read")){
			m = file->private_data;
			if(m && m->op && m->op->show) {
				if((entry_read = store_info_about_file(read_list, m->op->show, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "proc_reg_read")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->read){
				if((entry_read = store_info_about_file(read_list, pde->proc_fops->read, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "configfs_read_file")){
			configfs_attr = to_attr(file->f_path.dentry);
			if(configfs_attr && configfs_attr->show){
				if((entry_read = store_info_about_file(read_list, configfs_attr->show, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "v4l2_read")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->read){
				if((entry_read = store_info_about_file(read_list, vdev->fops->read, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "sdcardfs_read")){
			if(sdcardfs_lower_file(file) && sdcardfs_lower_file(file)->f_op && sdcardfs_lower_file(file)->f_op->read){
				if((entry_read = store_info_about_file(read_list, sdcardfs_lower_file(file)->f_op->read, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
			if(sdcardfs_lower_file(file) && sdcardfs_lower_file(file)->f_op && sdcardfs_lower_file(file)->f_op->read_iter){
				if((entry_read = store_info_about_file(read_list, sdcardfs_lower_file(file)->f_op->read_iter, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "snd_hwdep_read")){
			hw = file->private_data;
			if(hw && hw->ops.read){
				if((entry_read = store_info_about_file(read_list, hw->ops.read, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "tracing_read_pipe")){
			iter = file->private_data;
			if(iter && iter->trace && iter->trace->read){
				if((entry_read = store_info_about_file(read_list, iter->trace->read, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "tty_read")){
			tty = file_tty(file);
			if(tty){
				ld = tty->ldisc;
				if(ld && ld->ops && ld->ops->read){
					if((entry_read = store_info_about_file(read_list, ld->ops->read, &temp_count, -1)) == NULL)
						goto vmalloc_failed;
				}
			}
		}
		else if(!strcmp(entry_read->name, "fb_read")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_read){
				if((entry_read = store_info_about_file(read_list, info->fbops->fb_read, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_read->name, "pstore_file_read")){
			m = file->private_data;
			if(m && m->op && m->op->show) {
				if((entry_read = store_info_about_file(read_list, m->op->show, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		if(file->f_mapping && file->f_mapping->a_ops) {
			size_of_address_space_operations = sizeof(struct address_space_operations)/sizeof(void*);
			for(temporary_pointer = (long long **) file->f_mapping->a_ops, iterator = 0; size_of_address_space_operations > iterator; temporary_pointer++, iterator++) {
				if(temporary_pointer && *temporary_pointer) {
					if(store_info_about_file(read_list, (void*) (*temporary_pointer), &temp_count, -10) == NULL)
						goto vmalloc_failed;
				}		
			}
		}
		// here we end reversing "read" function
		// and here we start reversing "write" function
		temp_count = 1; // but first we need to reset function count to 1
		if(!strcmp(entry_write->name, "kernfs_fop_write")){
			of = kernfs_of(file);
			ops_kern = kernfs_ops(of->kn);
			if(ops_kern && ops_kern->write){
				if((entry_write = store_info_about_file(write_list, ops_kern->write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(!strcmp(entry_write->name, "sysfs_kf_write")){
					ops_sys = sysfs_file_ops(of->kn);
					if(ops_sys && ops_sys->store){
						if((entry_write = store_info_about_file(write_list, ops_sys->store, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
						if(!strcmp(entry_write->name, "dev_attr_store")){							
							dev_attr = to_dev_attr(of->kn->priv); // of->kn->priv == struct attribute
							if(dev_attr && dev_attr->store){
								if((entry_write = store_info_about_file(write_list, dev_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
								if(!strcmp(entry_write->name, "uevent_store")) {
									dev = kobj_to_dev(of_seq->kn->parent->priv);
									top_kobj = &dev->kobj;
									while (!top_kobj->kset && top_kobj->parent)
										top_kobj = top_kobj->parent;
									if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->filter){
										if((entry_write = store_info_about_file(write_list, top_kobj->kset->uevent_ops->filter, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
									if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->uevent){
										if((entry_write = store_info_about_file(write_list, top_kobj->kset->uevent_ops->uevent, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
									if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->name){
										if((entry_write = store_info_about_file(write_list, top_kobj->kset->uevent_ops->name, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
								}
								else if(!strcmp(entry_write->name, "iio_write_channel_info")) {
									indio_dev = dev_to_iio_dev(kobj_to_dev(of_seq->kn->parent->priv));
									if(indio_dev && indio_dev->info && indio_dev->info->write_raw){
										if((entry_write = store_info_about_file(write_list, indio_dev->info->write_raw, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
									if(indio_dev && indio_dev->info && indio_dev->info->write_raw_get_fmt){
										if((entry_write = store_info_about_file(write_list, indio_dev->info->write_raw_get_fmt, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
								}
							}
						}
						else if(!strcmp(entry_write->name, "slab_attr_store")){
							slab_attr = to_slab_attr(of->kn->priv);
							if(slab_attr && slab_attr->store){
								if((entry_write = store_info_about_file(write_list, slab_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "kobj_attr_store")){
							kattr = container_of(of->kn->priv, struct kobj_attribute, attr);
							if(kattr && kattr->store){
								if((entry_write = store_info_about_file(write_list, kattr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "queue_attr_store")){
							queue_entry = to_queue(of->kn->priv);
							if(queue_entry && queue_entry->store){
								if((entry_write = store_info_about_file(write_list, queue_entry->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "drv_attr_store")){
							drv_attr = to_drv_attr(of->kn->priv);
							if(drv_attr && drv_attr->store){
								if((entry_write = store_info_about_file(write_list, drv_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
								if(!strcmp(entry_write->name, "uevent_store")) {
									dev = kobj_to_dev(of_seq->kn->parent->priv);
									top_kobj = &dev->kobj;
									while (!top_kobj->kset && top_kobj->parent)
										top_kobj = top_kobj->parent;
									if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->filter){
										if((entry_write = store_info_about_file(write_list, top_kobj->kset->uevent_ops->filter, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
									if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->uevent){
										if((entry_write = store_info_about_file(write_list, top_kobj->kset->uevent_ops->uevent, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
									if(top_kobj->kset && top_kobj->kset->uevent_ops && top_kobj->kset->uevent_ops->name){
										if((entry_write = store_info_about_file(write_list, top_kobj->kset->uevent_ops->name, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
								}
							}
						}
						else if(!strcmp(entry_write->name, "module_attr_store")){
							mattr = to_module_attr(of->kn->priv);
							if(mattr && mattr->store){
								if((entry_write = store_info_about_file(write_list, mattr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
								if(!strcmp(entry_write->name, "param_attr_store")){
									attribute = to_param_attr(mattr);
									if(attribute && attribute->param && attribute->param->ops && attribute->param->ops->set){
										if((entry_write = store_info_about_file(write_list, attribute->param->ops->set, &temp_count, -1)) == NULL)
											goto vmalloc_failed;
									}
								}
							}
						}
						else if(!strcmp(entry_write->name, "elv_attr_store")){
							entry = to_elv(of->kn->priv);
							if(entry && entry->store){
								if((entry_write = store_info_about_file(write_list, entry->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "cpuidle_state_store")){
							cattr = attr_to_stateattr(of->kn->priv);
							if(cattr && cattr->store){
								if((entry_write = store_info_about_file(write_list, cattr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "bus_attr_store")){
							bus_attr = to_bus_attr(of->kn->priv);
							if(bus_attr && bus_attr->store){
								if((entry_write = store_info_about_file(write_list, bus_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "netdev_queue_attr_store")){
							attribute_netdev = to_netdev_queue_attr(of->kn->priv);
							if(attribute_netdev && attribute_netdev->store){
								if((entry_write = store_info_about_file(write_list, attribute_netdev->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "f2fs_attr_store")){
							a = container_of(of->kn->priv, struct f2fs_attr, attr);
							if(a && a->store){
								if((entry_write = store_info_about_file(write_list, a->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "rx_queue_attr_store")){
							attribute_rx = to_rx_queue_attr(of->kn->priv);
							if(attribute_rx && attribute_rx->store){
								if((entry_write = store_info_about_file(write_list, attribute_rx->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "class_attr_store")){
							class_attr = to_class_attr(of->kn->priv);
							if(class_attr && class_attr->store){
								if((entry_write = store_info_about_file(write_list, class_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "dm_attr_store")){
							dm_attr = container_of(of->kn->priv, struct dm_sysfs_attr, attr);
							if(dm_attr && dm_attr->store){
								if((entry_write = store_info_about_file(write_list, dm_attr->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
						else if(!strcmp(entry_write->name, "iommu_group_attr_store")){
							attr_iommu = to_iommu_group_attr(of->kn->priv);
							if(attr_iommu && attr_iommu->store){
								if((entry_write = store_info_about_file(write_list, attr_iommu->store, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
							}
						}
					}
				}
				else if(!strcmp(entry_write->name, "cgroup_file_write")){
					cft = of->kn->priv;
					if(cft && cft->write){
						if((entry_write = store_info_about_file(write_list, cft->write, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
					}
					if(cft && cft->write_u64){
						if((entry_write = store_info_about_file(write_list, cft->write_u64, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
					}
					if(cft && cft->write_s64){
						if((entry_write = store_info_about_file(write_list, cft->write_s64, &temp_count, -1)) == NULL)
									goto vmalloc_failed;
					}
				}
				else if(!strcmp(entry_write->name, "sysfs_kf_bin_write")){
					battr = of->kn->priv;
					if(battr && battr->write){
						if((entry_write = store_info_about_file(write_list, battr->write, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
			}
		}
		else if(!strcmp(entry_write->name, "proc_sys_write")){
			inode = file_inode(file);
			table = PROC_I(inode)->sysctl_entry;
			if (table && table->proc_handler){
				if((entry_write = store_info_about_file(write_list, table->proc_handler, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "configfs_write_file")){
			configfs_attr = to_attr(file->f_path.dentry);
			if(configfs_attr && configfs_attr->store){
				if((entry_write = store_info_about_file(write_list, configfs_attr->store, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "proc_reg_write")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->write){
				if((entry_write = store_info_about_file(write_list, pde->proc_fops->write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "full_proxy_write")){
			real_fops = debugfs_real_fops(file);	
			if(real_fops && real_fops->write){
				if((entry_write = store_info_about_file(write_list, real_fops->write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(!strcmp(entry_write->name, "blk_mq_debugfs_write")){
					m = file->private_data;
					attr = m->private;
					if(attr && attr->write){
						if((entry_write = store_info_about_file(write_list, attr->write, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
				else if(!strcmp(entry_write->name, "simple_attr_write")){
					sim_attr = file->private_data;
					if(sim_attr && sim_attr->set){
						if((entry_write = store_info_about_file(write_list, sim_attr->set, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
			}
		}
		else if(!strcmp(entry_write->name, "debugfs_attr_write")){
			sim_attr = file->private_data;
			if(sim_attr && sim_attr->set){
				if((entry_write = store_info_about_file(write_list, sim_attr->set, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "event_trigger_write")){
			list_for_each_entry(p, &trigger_commands, list) {
				if((entry_write = store_info_about_file(write_list, p->func, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "v4l2_write")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->write){
				if((entry_write = store_info_about_file(write_list, vdev->fops->write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "sdcardfs_write")){
			if(sdcardfs_lower_file(file) && sdcardfs_lower_file(file)->f_op && sdcardfs_lower_file(file)->f_op->write){
				if((entry_write = store_info_about_file(write_list, sdcardfs_lower_file(file)->f_op->write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
			if(sdcardfs_lower_file(file) && sdcardfs_lower_file(file)->f_op && sdcardfs_lower_file(file)->f_op->write_iter){
				if((entry_write = store_info_about_file(write_list, sdcardfs_lower_file(file)->f_op->write_iter, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "snd_hwdep_write")){
			hw = file->private_data;
			if(hw && hw->ops.write){
				if((entry_write = store_info_about_file(write_list, hw->ops.write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_write->name, "tty_write")){
			tty = file_tty(file);
			if(tty){
				ld = tty->ldisc;
				if(ld && ld->ops && ld->ops->write){
					if((entry_write = store_info_about_file(write_list, ld->ops->write, &temp_count, -1)) == NULL)
						goto vmalloc_failed;
				}
			}
		}
		else if(!strcmp(entry_write->name, "trace_options_write")){
			topt = file->private_data;
			if(topt && topt->flags && topt->flags->trace && topt->flags->trace->set_flag){
				if((entry_write = store_info_about_file(write_list, topt->flags->trace->set_flag, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
        else if(!strcmp(entry_write->name, "fb_write")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_write){
				if((entry_write = store_info_about_file(write_list, info->fbops->fb_write, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}

		if(file->f_mapping && file->f_mapping->a_ops) {
			size_of_address_space_operations = sizeof(struct address_space_operations)/sizeof(void*);
			for(temporary_pointer = (long long **) file->f_mapping->a_ops, iterator = 0; size_of_address_space_operations > iterator; temporary_pointer++, iterator++) {
				if(temporary_pointer && *temporary_pointer) {
					if(store_info_about_file(write_list, (void*) (*temporary_pointer), &temp_count, -10) == NULL)
						goto vmalloc_failed;
				}		
			}
		}
		// here we end reversing "write" function
		// and we start reversing "mmap" function
		temp_count = 1; // but again, we need to reset function count to 1
		if(!strcmp(entry_mmap->name, "kernfs_fop_mmap")){
			of = kernfs_of(file);
			ops_kern = kernfs_ops(of->kn);
			if((of->kn->flags & KERNFS_HAS_MMAP) && ops_kern && ops_kern->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, ops_kern->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(!strcmp(entry_mmap->name, "sysfs_kf_bin_mmap")){
					battr = of->kn->priv;
					if(battr && battr->mmap){
						if((entry_mmap = store_info_about_file(mmap_list, battr->mmap, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
			}
		}
		else if(!strcmp(entry_mmap->name, "fb_mmap")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_mmap){
				if((entry_mmap = store_info_about_file(mmap_list, info->fbops->fb_mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_mmap->name, "proc_reg_mmap")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, pde->proc_fops->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_mmap->name, "sdcardfs_mmap")){
			lower_file = sdcardfs_lower_file(file);
			if(lower_file && lower_file->f_op && lower_file->f_op->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, lower_file->f_op->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_mmap->name, "v4l2_mmap")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->mmap){
				if((entry_mmap = store_info_about_file(mmap_list, vdev->fops->mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_mmap->name, "snd_hwdep_mmap")){
			hw = file->private_data;
			if(hw && hw->ops.mmap){
				if((entry_mmap = store_info_about_file(mmap_list, hw->ops.mmap, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		if(file->f_mapping && file->f_mapping->a_ops) {
			size_of_address_space_operations = sizeof(struct address_space_operations)/sizeof(void*);
			for(temporary_pointer = (long long **) file->f_mapping->a_ops, iterator = 0; size_of_address_space_operations > iterator; temporary_pointer++, iterator++) {
				if(temporary_pointer && *temporary_pointer) {
					if(store_info_about_file(mmap_list, (void*) (*temporary_pointer), &temp_count, -10) == NULL)
						goto vmalloc_failed;
				}		
			}
		}
		// here we end reversing "mmap" function
		// and we start reversing "ioctl" function
		temp_count = 1; // but again, we need to reset function count to 1
		if(!strcmp(entry_unlocked_ioctl->name, "proc_reg_unlocked_ioctl")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->unlocked_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, pde->proc_fops->unlocked_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_unlocked_ioctl->name, "v4l2_ioctl")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->unlocked_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, vdev->fops->unlocked_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				size_of_v4l2_ioctls_array = V4L2_IOCTLS; // WARNING! THIS IS VERY DANGEROUS AND CAUSE LOTS OF TROUBLE - more info above (near declarition)
				for(size_of_v4l2_ioctls_array = size_of_v4l2_ioctls_array - 1; size_of_v4l2_ioctls_array>=0; size_of_v4l2_ioctls_array--) {
					ioctl_info_element = &v4l2_ioctls[size_of_v4l2_ioctls_array];
					if(ioctl_info_element && ioctl_info_element->func){
						if(store_info_about_file(unlocked_ioctl_list, (void*) ioctl_info_element->func, &temp_count, -2) == NULL)
							goto vmalloc_failed;
					}
					if(ioctl_info_element && ioctl_info_element->debug){
						if(store_info_about_file(unlocked_ioctl_list, (void*) ioctl_info_element->debug, &temp_count, -2) == NULL)
							goto vmalloc_failed;
					}
				}
				size_of_ioctl_ops_struct = sizeof(const struct v4l2_ioctl_ops)/sizeof(void*);
				if(vdev->ioctl_ops){	
					for(temporary_pointer = (long long **) vdev->ioctl_ops, iterator = 0; size_of_ioctl_ops_struct > iterator; temporary_pointer++, iterator++) {
						if(temporary_pointer && *temporary_pointer) {
							if(store_info_about_file(unlocked_ioctl_list, (void*) (*temporary_pointer), &temp_count, -3) == NULL)
								goto vmalloc_failed;
						}		
					}
				}
			}
		}
		else if(!strcmp(entry_unlocked_ioctl->name, "sdcardfs_unlocked_ioctl")){
			lower_file = sdcardfs_lower_file(file);
			if(lower_file && lower_file->f_op && lower_file->f_op->unlocked_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, lower_file->f_op->unlocked_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_unlocked_ioctl->name, "fb_ioctl")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, info->fbops->fb_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_unlocked_ioctl->name, "snd_hwdep_ioctl")){
			hw = file->private_data;
			if(hw && hw->ops.ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, hw->ops.ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_unlocked_ioctl->name, "block_ioctl")){
			bdev = I_BDEV(bdev_file_inode(file));
			disk = bdev->bd_disk;
			if(disk && disk->fops && disk->fops->ioctl){
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, disk->fops->ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
				if(!strcmp(entry_unlocked_ioctl->name, "sd_ioctl")){
					sdkp = scsi_disk(disk);
					sdp = sdkp->device;
					if(sdp && sdp->host && sdp->host->hostt && sdp->host->hostt->ioctl){
						if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, sdp->host->hostt->ioctl, &temp_count, -1)) == NULL)
							goto vmalloc_failed;
					}
				}
				else if(!strcmp(entry_unlocked_ioctl->name, "lo_ioctl")){
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
					if(temporary_pointer && *temporary_pointer) {
						if(store_info_about_file(unlocked_ioctl_list, (void*) (*temporary_pointer), &temp_count, -2) == NULL)
							goto vmalloc_failed;
					}		
				}
			}	
		}
		else if(!strcmp(entry_unlocked_ioctl->name, "tty_ioctl")){
			tty = file_tty(file);
			if(tty){
				ld = tty->ldisc;
				if(ld && ld->ops && ld->ops->ioctl){
					if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, ld->ops->ioctl, &temp_count, -1)) == NULL)
						goto vmalloc_failed;
				}
			}
		}
		else if(!strcmp(entry_unlocked_ioctl->name, "snd_ctl_ioctl")){
			list_for_each_entry(p_snd, &snd_control_ioctls, list) {
				if((entry_unlocked_ioctl = store_info_about_file(unlocked_ioctl_list, p_snd->fioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}

		if(file->f_mapping && file->f_mapping->a_ops) {
			size_of_address_space_operations = sizeof(struct address_space_operations)/sizeof(void*);
			for(temporary_pointer = (long long **) file->f_mapping->a_ops, iterator = 0; size_of_address_space_operations > iterator; temporary_pointer++, iterator++) {
				if(temporary_pointer && *temporary_pointer) {
					if(store_info_about_file(unlocked_ioctl_list, (void*) (*temporary_pointer), &temp_count, -10) == NULL)
						goto vmalloc_failed;
				}		
			}
		}
		// here we end reversing "ioctl" function
		// and we start reversing "ioctl_c" function
		temp_count = 1; // but again, we need to reset function count to 1
		if(!strcmp(entry_compat_ioctl->name, "v4l2_compat_ioctl32")){
			vdev = video_devdata(file);
			if(vdev && vdev->fops && vdev->fops->compat_ioctl32){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, vdev->fops->compat_ioctl32, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_compat_ioctl->name, "sdcardfs_compat_ioctl")){
			lower_file = sdcardfs_lower_file(file);
			if(lower_file && lower_file->f_op && lower_file->f_op->compat_ioctl){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, lower_file->f_op->compat_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_compat_ioctl->name, "fb_compat_ioctl")){
			info = file_fb_info(file);
			fb = info->fbops;
			if(info && info->fbops && info->fbops->fb_compat_ioctl){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, info->fbops->fb_compat_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_compat_ioctl->name, "snd_hwdep_ioctl_compat")){
			hw = file->private_data;
			if(hw && hw->ops.ioctl_compat){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, hw->ops.ioctl_compat, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_compat_ioctl->name, "proc_reg_unlocked_ioctl")){
			pde = PDE(file_inode(file));
			if(pde && pde->proc_fops && pde->proc_fops->compat_ioctl){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, pde->proc_fops->compat_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}
		else if(!strcmp(entry_compat_ioctl->name, "compat_blkdev_ioctl")){
			bdev = I_BDEV(bdev_file_inode(file));
			disk = bdev->bd_disk;
			if(disk && disk->fops && disk->fops->compat_ioctl){
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, disk->fops->compat_ioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
			if(disk && disk->fops && disk->fops->pr_ops){
				size_of_pr_ops = sizeof(struct pr_ops)/sizeof(void*);
				for(temporary_pointer = (long long **) disk->fops->pr_ops, iterator = 0; size_of_pr_ops > iterator; temporary_pointer++, iterator++) {
					if(temporary_pointer && *temporary_pointer) {
						if(store_info_about_file(compat_ioctl_list, (void*) (*temporary_pointer), &temp_count, -2) == NULL)
							goto vmalloc_failed;
					}		
				}
			}	
		}
		else if(!strcmp(entry_compat_ioctl->name, "tty_compat_ioctl")){
			tty = file_tty(file);
			if(tty){
				ld = tty->ldisc;
				if(ld && ld->ops && ld->ops->compat_ioctl){
					if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, ld->ops->compat_ioctl, &temp_count, -1)) == NULL)
						goto vmalloc_failed;
				}
			}
		}
		else if(!strcmp(entry_compat_ioctl->name, "snd_ctl_ioctl_compat")){
			list_for_each_entry(p_snd, &snd_control_compat_ioctls, list) {
				if((entry_compat_ioctl = store_info_about_file(compat_ioctl_list, p_snd->fioctl, &temp_count, -1)) == NULL)
					goto vmalloc_failed;
			}
		}

		if(file->f_mapping && file->f_mapping->a_ops) {
			size_of_address_space_operations = sizeof(struct address_space_operations)/sizeof(void*);
			for(temporary_pointer = (long long **) file->f_mapping->a_ops, iterator = 0; size_of_address_space_operations > iterator; temporary_pointer++, iterator++) {
				if(temporary_pointer && *temporary_pointer) {
					if(store_info_about_file(compat_ioctl_list, (void*) (*temporary_pointer), &temp_count, -10) == NULL)
						goto vmalloc_failed;
				}		
			}
		}
		// we ended reversing all functions - now we pass pointers to FOKA_log
		vmalloc_failed: // we jump here if any vmalloc fails (the only exception are vmalloc on the beginning if they fail, we return from this function instantly
		FOKA_log(file_name, read_list, write_list, mmap_list, unlocked_ioctl_list, compat_ioctl_list);
	}
}