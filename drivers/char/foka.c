/*
 *  FOKA
 *  Copyright (C) Samsung 2019 by interns
 */

#define FOKA_VERSION "1.0"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

struct function_entry {
	char name[128];  // for now it is enough length, but it is possible that in the future function name can be longer then 128
	void *ptr;
	int branch_nr; // sometimes function can call more than one function, so to track all possible paths i added branch_nr to know in which path i've gone
	unsigned int count; // this field of structure is now unused, but in future i will maybe need it
	struct list_head list;
};

struct foka_entry {
	char *dev_name;
	struct list_head *read_backtrace;
	struct list_head *write_backtrace;
	struct list_head *mmap_backtrace;
	struct list_head *ioctl_backtrace;
	struct list_head *ioctl_c_backtrace;
	struct list_head list;
};

static unsigned int foka_count;
static struct list_head FOKA_list = LIST_HEAD_INIT(FOKA_list);
static int FOKA_open(struct inode *inode, struct file *file);
static int FOKA_close(struct inode *inode, struct file *file);
static int FOKA_show(struct seq_file *seq, void *v);
static void free_list_elems(void);

static const struct file_operations foka_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= FOKA_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= FOKA_close,
};

static int FOKA_open(struct inode *inode, struct file *file)
{
	return single_open(file, FOKA_show, NULL);
}

static int FOKA_close(struct inode *inode, struct file *file)
{
	free_list_elems();
	return single_release(inode, file);
}

static int FOKA_show(struct seq_file *seq, void *v)
{
	struct foka_entry *current_foka;
	struct function_entry *current_backtrace;
	unsigned int backtrace_count;
	list_for_each_entry(current_foka, &FOKA_list, list) {
		seq_printf(seq, "%s\n", "FOKA_new_file");
		seq_printf(seq, "%s\n", current_foka->dev_name);
		////////////// - i added this to make reading of this code easier
		backtrace_count = 0;
		list_for_each_entry(current_backtrace, current_foka->read_backtrace, list) {
			backtrace_count = current_backtrace->count;
		}
		seq_printf(seq, "%s\n", "FOKA_read_backtrace");
		seq_printf(seq, "%d\n", backtrace_count);
		list_for_each_entry(current_backtrace, current_foka->read_backtrace, list) {
			seq_printf(seq, "%s\n", current_backtrace->name);
			seq_printf(seq, "%px\n", current_backtrace->ptr);
			seq_printf(seq, "%d\n", current_backtrace->branch_nr);
		}
		//////////////
		backtrace_count = 0;
		list_for_each_entry(current_backtrace, current_foka->write_backtrace, list) {
			backtrace_count = current_backtrace->count;
		}
		seq_printf(seq, "%s\n", "FOKA_write_backtrace");
		seq_printf(seq, "%d\n", backtrace_count);
		list_for_each_entry(current_backtrace, current_foka->write_backtrace, list) {
			seq_printf(seq, "%s\n", current_backtrace->name);
			seq_printf(seq, "%px\n", current_backtrace->ptr);
			seq_printf(seq, "%d\n", current_backtrace->branch_nr);
		}
		//////////////
		backtrace_count = 0;
		list_for_each_entry(current_backtrace, current_foka->mmap_backtrace, list) {
			backtrace_count = current_backtrace->count;
		}
		seq_printf(seq, "%s\n", "FOKA_mmap_backtrace");
		seq_printf(seq, "%d\n", backtrace_count);
		list_for_each_entry(current_backtrace, current_foka->mmap_backtrace, list) {
			seq_printf(seq, "%s\n", current_backtrace->name);
			seq_printf(seq, "%px\n", current_backtrace->ptr);
			seq_printf(seq, "%d\n", current_backtrace->branch_nr);
		}
		//////////////
		backtrace_count = 0;
		list_for_each_entry(current_backtrace, current_foka->ioctl_backtrace, list) {
			backtrace_count = current_backtrace->count;
		}
		seq_printf(seq, "%s\n", "FOKA_ioctl_backtrace");
		seq_printf(seq, "%d\n", backtrace_count);
		list_for_each_entry(current_backtrace, current_foka->ioctl_backtrace, list) {
			seq_printf(seq, "%s\n", current_backtrace->name);
			seq_printf(seq, "%px\n", current_backtrace->ptr);
			seq_printf(seq, "%d\n", current_backtrace->branch_nr);
		}
		//////////////
		backtrace_count = 0;
		list_for_each_entry(current_backtrace, current_foka->ioctl_c_backtrace, list) {
			backtrace_count = current_backtrace->count;
		}
		seq_printf(seq, "%s\n", "FOKA_ioctl_c_backtrace");
		seq_printf(seq, "%d\n", backtrace_count);
		list_for_each_entry(current_backtrace, current_foka->ioctl_c_backtrace, list) {
			seq_printf(seq, "%s\n", current_backtrace->name);
			seq_printf(seq, "%px\n", current_backtrace->ptr);
			seq_printf(seq, "%d\n", current_backtrace->branch_nr);
		}
	}
	return 0;
}

static void free_list_elems(void)
{
	struct foka_entry *current_foka;
	struct foka_entry *current_foka_temp;
	struct function_entry *current_backtrace;
	struct function_entry *current_backtrace_temp;	
	list_for_each_entry_safe(current_foka, current_foka_temp, &FOKA_list, list) {
		if(current_foka->dev_name)
			vfree(current_foka->dev_name);
		if(current_foka->read_backtrace) {
			list_for_each_entry_safe(current_backtrace, current_backtrace_temp, current_foka->read_backtrace, list) {
				list_del(&(current_backtrace->list));
				vfree(current_backtrace);				
			}
			vfree(current_foka->read_backtrace);
		}
		if(current_foka->write_backtrace) {
			list_for_each_entry_safe(current_backtrace, current_backtrace_temp, current_foka->write_backtrace, list) {
				list_del(&(current_backtrace->list));
				vfree(current_backtrace);				
			}
			vfree(current_foka->write_backtrace);
		}
		if(current_foka->mmap_backtrace) {
			list_for_each_entry_safe(current_backtrace, current_backtrace_temp, current_foka->mmap_backtrace, list) {
				list_del(&(current_backtrace->list));
				vfree(current_backtrace);				
			}
			vfree(current_foka->mmap_backtrace);
		}
		if(current_foka->ioctl_backtrace) {
			list_for_each_entry_safe(current_backtrace, current_backtrace_temp, current_foka->ioctl_backtrace, list) {
				list_del(&(current_backtrace->list));
				vfree(current_backtrace);				
			}
			vfree(current_foka->ioctl_backtrace);
		}
		if(current_foka->ioctl_c_backtrace) {
			list_for_each_entry_safe(current_backtrace, current_backtrace_temp, current_foka->ioctl_c_backtrace, list) {
				list_del(&(current_backtrace->list));
				vfree(current_backtrace);				
			}
			vfree(current_foka->ioctl_c_backtrace);
		}
		list_del(&current_foka->list);
		vfree(current_foka);
	}
}

int FOKA_log(char *dev_name, struct list_head *read_backtrace, struct list_head *write_backtrace, 
			struct list_head *mmap_backtrace, struct list_head *ioctl_backtrace, struct list_head *ioctl_c_backtrace)
{
	int ret = 0;
	struct foka_entry *device_entry = NULL;
	device_entry = vmalloc(sizeof(struct foka_entry));
	if (!device_entry) {
		printk(KERN_ERR "FOKA: cannot allocate device_entry\n");
		ret = -ENOMEM;
		goto err;
	}
	device_entry->dev_name = dev_name;
	device_entry->read_backtrace = read_backtrace;
	device_entry->write_backtrace = write_backtrace;
	device_entry->mmap_backtrace = mmap_backtrace;
	device_entry->ioctl_backtrace = ioctl_backtrace;
	device_entry->ioctl_c_backtrace = ioctl_c_backtrace;
	list_add_tail(&device_entry->list, &FOKA_list);
	foka_count++;
	return 0;
err:
	return ret;
}

EXPORT_SYMBOL(FOKA_log);

static int __init foka_init(void)
{
	int err = 0;
	if (!proc_create("FOKA", 0, NULL, &foka_proc_fops)) {
		printk(KERN_ERR "FOKA: failed to register procfs entry\n");
		err = -ENOMEM;
		goto error;
	}
	printk(KERN_INFO "File Operation Kernel Analyzer v" FOKA_VERSION "\n");
	return 0;
	error:
	return err;
}

static void __exit foka_exit(void)
{
	remove_proc_entry("FOKA", NULL);
	free_list_elems();
}

module_init(foka_init);
module_exit(foka_exit);

MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL");