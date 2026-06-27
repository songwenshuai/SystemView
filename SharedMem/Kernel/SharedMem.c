// SPDX-License-Identifier: GPL-2.0-only OR BSD-1-Clause
/*
 * Debug shared-memory mmap driver for heterogeneous SoCs.
 *
 * Copyright (c) 2023-2026 CineLogic.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/minmax.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include <asm/pgtable.h>

#include "SharedMem.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
#define sharedmem_vm_flags_set(vma, flags) ((vma)->vm_flags |= (flags))
#else
#define sharedmem_vm_flags_set(vma, flags) vm_flags_set((vma), (flags))
#endif

#define sharedmem_ioremap(addr, size) ioremap(addr, size)

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
#define sharedmem_class_create(name) class_create(THIS_MODULE, name)
#else
#define sharedmem_class_create(name) class_create(name)
#endif

#define VERSION_STRING			"3.2"
#define SHAREDMEM_DEVNAME		"shared_mem"
#define SHAREDMEM_MAX_DEVICES		4
#define SHAREDMEM_IOCTL_CHUNK_SIZE	PAGE_SIZE

MODULE_AUTHOR("CineLogic <wenshuaisong@gmail.com>");
MODULE_DESCRIPTION("Debug shared-memory mmap driver for heterogeneous SoCs");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(VERSION_STRING);

static int sharedmem_debug;
module_param_named(debug, sharedmem_debug, int, 0644);
MODULE_PARM_DESC(debug, "Enable debugging (0=off, 1=error, 2=info, 3=debug)");

/*
 * Debug parameters for one or more explicit shared memory regions.
 * Example:
 *   insmod SharedMem.ko phys_addrs=0x90000000,0x91000000 \
 *     mem_sizes=0x10000,0x20000
 *
 * If a single mem_sizes value is supplied, it is reused for all phys_addrs.
 */
static unsigned long long phys_addrs[SHAREDMEM_MAX_DEVICES];
static int phys_addrs_count;
module_param_array(phys_addrs, ullong, &phys_addrs_count, 0644);
MODULE_PARM_DESC(phys_addrs,
		 "Comma-separated physical base addresses for debug memory regions");

static unsigned long long mem_sizes[SHAREDMEM_MAX_DEVICES];
static int mem_sizes_count;
module_param_array(mem_sizes, ullong, &mem_sizes_count, 0644);
MODULE_PARM_DESC(mem_sizes,
		 "Comma-separated sizes for phys_addrs; one value may be reused for all regions");

static bool claim_mem_region = true;
module_param(claim_mem_region, bool, 0644);
MODULE_PARM_DESC(claim_mem_region,
		 "Call request_mem_region for debug ranges (disable if reserved-memory is already claimed)");

#define sharedmem_err(dev, fmt, ...) \
	do { \
		dev_err(dev, fmt, ##__VA_ARGS__); \
	} while (0)

#define sharedmem_info(dev, fmt, ...) \
	do { \
		if (sharedmem_debug >= 2) \
			dev_info(dev, fmt, ##__VA_ARGS__); \
	} while (0)

#define sharedmem_dbg(dev, fmt, ...) \
	do { \
		if (sharedmem_debug >= 3) \
			dev_dbg(dev, fmt, ##__VA_ARGS__); \
	} while (0)

#define sharedmem_warn(dev, fmt, ...) \
	do { \
		dev_warn(dev, fmt, ##__VA_ARGS__); \
	} while (0)

/**
 * struct sharedmem_region - mapped shared memory region
 * @phys_base: Physical base address of the region
 * @virt_base: Kernel virtual base returned by ioremap()
 * @size: Size of the region
 * @dev: Associated device
 * @lock: Serializes region access
 * @claimed: request_mem_region() ownership flag
 */
struct sharedmem_region {
	phys_addr_t phys_base;
	void __iomem *virt_base;
	size_t size;
	struct device *dev;
	struct mutex lock;
	bool claimed;
};

/**
 * struct sharedmem_region_desc - module parameter region descriptor
 * @phys_base: Physical base address
 * @size: Region size
 */
struct sharedmem_region_desc {
	phys_addr_t phys_base;
	size_t size;
};

/**
 * struct sharedmem_dev - character device instance
 * @dev: Platform device
 * @region: Memory region information
 * @cdev: Character device structure
 * @device: Device node
 * @minor: Minor number
 * @is_open: Device open status
 * @lock: Protects open state
 * @usage_count: Usage reference count
 */
struct sharedmem_dev {
	struct device *dev;
	struct sharedmem_region region;
	struct cdev cdev;
	struct device *device;
	int minor;
	bool is_open;
	spinlock_t lock;
	atomic_t usage_count;
};

static dev_t sharedmem_devt;
static struct class *sharedmem_class;
static struct sharedmem_dev *sharedmem_devices[SHAREDMEM_MAX_DEVICES];
static DEFINE_MUTEX(sharedmem_mutex);
static struct platform_device **sharedmem_pdevs;
static int sharedmem_pdev_count;

static bool sharedmem_range_valid(u64 offset, u64 size, size_t region_size)
{
	u64 region_size64 = (u64)region_size;

	if (size == 0)
		return false;

	if (offset > region_size64)
		return false;

	return size <= region_size64 - offset;
}

static int sharedmem_phys_from_param(unsigned long long value, phys_addr_t *paddr)
{
	phys_addr_t converted = (phys_addr_t)value;

	if ((unsigned long long)converted != value) {
		pr_err("Physical address 0x%llx does not fit phys_addr_t\n",
		       value);
		return -EINVAL;
	}

	*paddr = converted;
	return 0;
}

static int sharedmem_size_from_param(unsigned long long value, size_t *size)
{
	size_t converted = (size_t)value;

	if ((unsigned long long)converted != value) {
		pr_err("Memory size 0x%llx does not fit size_t\n", value);
		return -EINVAL;
	}

	*size = converted;
	return 0;
}

static int sharedmem_validate_region_params(phys_addr_t paddr, size_t size)
{
	phys_addr_t phys_limit = ~(phys_addr_t)0;

	if (paddr == 0) {
		pr_err("Physical address must be non-zero\n");
		return -EINVAL;
	}

	if (size == 0) {
		pr_err("Memory size must be non-zero\n");
		return -EINVAL;
	}

	if (!IS_ALIGNED((u64)paddr, PAGE_SIZE)) {
		pr_err("Physical address 0x%llx is not page-aligned\n",
		       (unsigned long long)paddr);
		return -EINVAL;
	}

	if (!IS_ALIGNED(size, PAGE_SIZE)) {
		pr_err("Memory size 0x%zx is not page-aligned\n", size);
		return -EINVAL;
	}

	if ((phys_addr_t)(size - 1) > phys_limit - paddr) {
		pr_err("Physical region overflows address space: base 0x%llx, size 0x%zx\n",
		       (unsigned long long)paddr, size);
		return -EINVAL;
	}

	return 0;
}

static bool sharedmem_regions_overlap(const struct sharedmem_region_desc *a,
				      const struct sharedmem_region_desc *b)
{
	u64 a_start = (u64)a->phys_base;
	u64 b_start = (u64)b->phys_base;
	u64 a_last = a_start + (u64)a->size - 1ULL;
	u64 b_last = b_start + (u64)b->size - 1ULL;

	return a_start <= b_last && b_start <= a_last;
}

static int sharedmem_validate_regions(const struct sharedmem_region_desc *regions,
				      int region_count)
{
	int i;
	int j;
	int ret;

	for (i = 0; i < region_count; i++) {
		ret = sharedmem_validate_region_params(regions[i].phys_base,
						       regions[i].size);
		if (ret) {
			pr_err("Invalid region %d\n", i);
			return ret;
		}
	}

	for (i = 0; i < region_count; i++) {
		for (j = i + 1; j < region_count; j++) {
			if (sharedmem_regions_overlap(&regions[i], &regions[j])) {
				pr_err("Regions %d and %d overlap: [0x%llx, +0x%zx) and [0x%llx, +0x%zx)\n",
				       i, j,
				       (unsigned long long)regions[i].phys_base,
				       regions[i].size,
				       (unsigned long long)regions[j].phys_base,
				       regions[j].size);
				return -EINVAL;
			}
		}
	}

	return 0;
}

/**
 * sharedmem_vma_open - account a new VMA reference
 * @vma: VMA opened for this device
 */
static void sharedmem_vma_open(struct vm_area_struct *vma)
{
	struct sharedmem_dev *dev = vma->vm_private_data;

	if (!dev) {
		pr_err("%s: Invalid VMA private data\n", __func__);
		return;
	}

	atomic_inc(&dev->usage_count);
	sharedmem_dbg(dev->dev, "VMA open, virt %lx, phys %lx\n",
		      vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

/**
 * sharedmem_vma_close - drop a VMA reference
 * @vma: VMA closed for this device
 */
static void sharedmem_vma_close(struct vm_area_struct *vma)
{
	struct sharedmem_dev *dev = vma->vm_private_data;

	if (!dev) {
		pr_err("%s: Invalid VMA private data\n", __func__);
		return;
	}

	atomic_dec(&dev->usage_count);
	sharedmem_dbg(dev->dev, "VMA close\n");
}

static const struct vm_operations_struct sharedmem_vm_ops = {
	.open = sharedmem_vma_open,
	.close = sharedmem_vma_close,
};

/**
 * sharedmem_open - open the character device
 * @inode: Device inode
 * @filp: Open file
 *
 * Return: 0 on success, negative error code on failure
 */
static int sharedmem_open(struct inode *inode, struct file *filp)
{
	struct sharedmem_dev *dev;
	unsigned long flags;
	int ret = 0;

	dev = container_of(inode->i_cdev, struct sharedmem_dev, cdev);
	if (!dev) {
		pr_err("%s: Failed to get device data\n", __func__);
		return -ENODEV;
	}

	filp->private_data = dev;

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->is_open) {
		ret = -EBUSY;
	} else {
		dev->is_open = true;
		atomic_set(&dev->usage_count, 1);
	}
	spin_unlock_irqrestore(&dev->lock, flags);

	if (ret) {
		sharedmem_err(dev->dev, "Device already opened\n");
		return ret;
	}

	sharedmem_info(dev->dev, "Device opened\n");
	return 0;
}

/**
 * sharedmem_release - release the character device
 * @inode: Device inode
 * @filp: Open file
 *
 * Return: 0 on success
 */
static int sharedmem_release(struct inode *inode, struct file *filp)
{
	struct sharedmem_dev *dev = filp->private_data;
	unsigned long flags;
	unsigned long timeout;

	if (!dev) {
		pr_err("%s: Invalid device data\n", __func__);
		return -ENODEV;
	}

	timeout = jiffies + msecs_to_jiffies(1000);
	while (atomic_read(&dev->usage_count) > 1) {
		if (time_after(jiffies, timeout)) {
			sharedmem_warn(dev->dev,
				       "Timeout waiting for VMA to close\n");
			break;
		}
		schedule_timeout_interruptible(HZ / 10);
	}

	spin_lock_irqsave(&dev->lock, flags);
	dev->is_open = false;
	spin_unlock_irqrestore(&dev->lock, flags);

	sharedmem_info(dev->dev, "Device released\n");
	return 0;
}

/**
 * sharedmem_mmap - map device memory into userspace
 * @filp: Open file
 * @vma: VMA describing the mapping
 *
 * Return: 0 on success, negative error code on failure
 */
static int sharedmem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct sharedmem_dev *dev = filp->private_data;
	unsigned long size = vma->vm_end - vma->vm_start;
	u64 offset;
	u64 pfn;
	int ret;

	if (!dev) {
		pr_err("%s: Invalid device data\n", __func__);
		return -ENODEV;
	}

	if ((u64)vma->vm_pgoff > (~0ULL >> PAGE_SHIFT)) {
		sharedmem_err(dev->dev, "Invalid mmap page offset: 0x%lx\n",
			      vma->vm_pgoff);
		return -EINVAL;
	}
	offset = (u64)vma->vm_pgoff << PAGE_SHIFT;

	if (size == 0) {
		sharedmem_err(dev->dev, "Invalid mmap size: 0\n");
		return -EINVAL;
	}

	if (!IS_ALIGNED(size, PAGE_SIZE)) {
		sharedmem_err(dev->dev,
			      "Invalid mmap size 0x%lx: must be page-aligned\n",
			      size);
		return -EINVAL;
	}

	if (!(vma->vm_flags & VM_SHARED)) {
		sharedmem_err(dev->dev, "mmap must use MAP_SHARED\n");
		return -EINVAL;
	}

	mutex_lock(&dev->region.lock);

	if (!sharedmem_range_valid(offset, size, dev->region.size)) {
		sharedmem_err(dev->dev,
			      "Invalid mmap range: offset 0x%llx, size 0x%lx\n",
			      (unsigned long long)offset, size);
		ret = -EINVAL;
		goto out_unlock;
	}

	pfn = ((u64)dev->region.phys_base + offset) >> PAGE_SHIFT;
	if (pfn > ULONG_MAX) {
		sharedmem_err(dev->dev,
			      "mmap PFN 0x%llx does not fit unsigned long\n",
			      (unsigned long long)pfn);
		ret = -ERANGE;
		goto out_unlock;
	}

	sharedmem_vm_flags_set(vma, VM_IO | VM_PFNMAP | VM_DONTEXPAND |
			       VM_DONTDUMP);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_ops = &sharedmem_vm_ops;
	vma->vm_private_data = dev;

	ret = remap_pfn_range(vma, vma->vm_start, (unsigned long)pfn, size,
			      vma->vm_page_prot);
	if (ret) {
		sharedmem_err(dev->dev, "remap_pfn_range failed: %d\n", ret);
		goto out_unlock;
	}

	sharedmem_vma_open(vma);
	sharedmem_dbg(dev->dev, "mmap: mapped 0x%lx bytes at offset 0x%llx\n",
		      size, (unsigned long long)offset);

out_unlock:
	mutex_unlock(&dev->region.lock);
	return ret;
}

/**
 * sharedmem_ioctl - handle device ioctls
 * @filp: Open file
 * @cmd: Ioctl command
 * @arg: Userspace argument
 *
 * Return: 0 on success, negative error code on failure
 */
static long sharedmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct sharedmem_dev *dev = filp->private_data;
	struct sharedmem_ioctl_data ioctl_data;
	void __user *user_data = NULL;
	void *kbuf = NULL;
	size_t chunk_size;
	size_t copied;
	size_t io_offset;
	size_t io_size;
	int ret = 0;

	if (!dev) {
		pr_err("%s: Invalid device data\n", __func__);
		return -ENODEV;
	}

	if (_IOC_TYPE(cmd) != SHAREDMEM_MAGIC) {
		sharedmem_err(dev->dev, "Invalid IOCTL command type\n");
		return -ENOTTY;
	}

	if (_IOC_NR(cmd) > SHAREDMEM_MAX_NR) {
		sharedmem_err(dev->dev, "Invalid IOCTL command number\n");
		return -ENOTTY;
	}

	mutex_lock(&dev->region.lock);

	switch (cmd) {
	case SHAREDMEM_GET_PHYS_ADDR: {
		sharedmem_addr_t phys_addr_val;

		phys_addr_val = (sharedmem_addr_t)dev->region.phys_base;
		if (copy_to_user((void __user *)arg, &phys_addr_val,
				 sizeof(phys_addr_val))) {
			ret = -EFAULT;
			goto out_unlock;
		}
		break;
	}
	case SHAREDMEM_GET_MEM_SIZE: {
		sharedmem_size_t size_val;

		size_val = (sharedmem_size_t)dev->region.size;
		if (copy_to_user((void __user *)arg, &size_val,
				 sizeof(size_val))) {
			ret = -EFAULT;
			goto out_unlock;
		}
		break;
	}
	case SHAREDMEM_IOCTL_READ:
		if (copy_from_user(&ioctl_data, (void __user *)arg,
				   sizeof(ioctl_data))) {
			ret = -EFAULT;
			goto out_unlock;
		}

		if (!sharedmem_range_valid(ioctl_data.address, ioctl_data.size,
					   dev->region.size)) {
			ret = -EINVAL;
			goto out_unlock;
		}
		io_offset = (size_t)ioctl_data.address;
		io_size = (size_t)ioctl_data.size;

		user_data = u64_to_user_ptr(ioctl_data.data);
		if (!user_data) {
			ret = -EINVAL;
			goto out_unlock;
		}

		chunk_size = min_t(size_t, io_size, SHAREDMEM_IOCTL_CHUNK_SIZE);
		kbuf = kmalloc(chunk_size, GFP_KERNEL);
		if (!kbuf) {
			ret = -ENOMEM;
			goto out_unlock;
		}

		for (copied = 0; copied < io_size; copied += chunk_size) {
			chunk_size = min_t(size_t, io_size - copied,
					   SHAREDMEM_IOCTL_CHUNK_SIZE);
			memcpy_fromio(kbuf,
				      (u8 __iomem *)dev->region.virt_base +
				      io_offset + copied, chunk_size);
			if (copy_to_user((u8 __user *)user_data + copied,
					 kbuf, chunk_size)) {
				ret = -EFAULT;
				goto out_unlock;
			}
		}
		break;
	case SHAREDMEM_IOCTL_WRITE:
		if (copy_from_user(&ioctl_data, (void __user *)arg,
				   sizeof(ioctl_data))) {
			ret = -EFAULT;
			goto out_unlock;
		}

		if (!sharedmem_range_valid(ioctl_data.address, ioctl_data.size,
					   dev->region.size)) {
			ret = -EINVAL;
			goto out_unlock;
		}
		io_offset = (size_t)ioctl_data.address;
		io_size = (size_t)ioctl_data.size;

		user_data = u64_to_user_ptr(ioctl_data.data);
		if (!user_data) {
			ret = -EINVAL;
			goto out_unlock;
		}

		chunk_size = min_t(size_t, io_size, SHAREDMEM_IOCTL_CHUNK_SIZE);
		kbuf = kmalloc(chunk_size, GFP_KERNEL);
		if (!kbuf) {
			ret = -ENOMEM;
			goto out_unlock;
		}

		for (copied = 0; copied < io_size; copied += chunk_size) {
			chunk_size = min_t(size_t, io_size - copied,
					   SHAREDMEM_IOCTL_CHUNK_SIZE);
			if (copy_from_user(kbuf, (u8 __user *)user_data + copied,
					   chunk_size)) {
				ret = -EFAULT;
				goto out_unlock;
			}
			memcpy_toio((u8 __iomem *)dev->region.virt_base +
				    io_offset + copied, kbuf, chunk_size);
		}
		break;
	default:
		ret = -ENOTTY;
		break;
	}

out_unlock:
	kfree(kbuf);
	mutex_unlock(&dev->region.lock);
	return ret;
}

#ifdef CONFIG_COMPAT
static long sharedmem_compat_ioctl(struct file *filp, unsigned int cmd,
				   unsigned long arg)
{
	return sharedmem_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations sharedmem_fops = {
	.owner = THIS_MODULE,
	.open = sharedmem_open,
	.release = sharedmem_release,
	.mmap = sharedmem_mmap,
	.unlocked_ioctl = sharedmem_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sharedmem_compat_ioctl,
#endif
};

/**
 * sharedmem_cleanup_device - release one device instance
 * @dev: Device instance to release
 */
static void sharedmem_cleanup_device(struct sharedmem_dev *dev)
{
	if (!dev)
		return;

	if (dev->device) {
		device_destroy(sharedmem_class,
			       MKDEV(MAJOR(sharedmem_devt), dev->minor));
		dev->device = NULL;
	}

	cdev_del(&dev->cdev);

	if (dev->region.virt_base)
		iounmap(dev->region.virt_base);

	if (dev->region.claimed)
		release_mem_region(dev->region.phys_base, dev->region.size);

	kfree(dev);
}

/**
 * sharedmem_find_free_minor - find an unused minor number
 *
 * Return: Available minor number, or -1 if none available
 */
static int sharedmem_find_free_minor(void)
{
	int i;

	for (i = 0; i < SHAREDMEM_MAX_DEVICES; i++) {
		if (!sharedmem_devices[i])
			return i;
	}

	return -1;
}

/**
 * sharedmem_setup_device - set up a single device instance
 * @paddr: Physical base address
 * @size: Size of memory region
 * @index: Device index
 *
 * Return: 0 on success, negative error code on failure
 */
static int sharedmem_setup_device(phys_addr_t paddr, size_t size, int index)
{
	struct sharedmem_dev *dev;
	int minor;
	int ret;

	ret = sharedmem_validate_region_params(paddr, size);
	if (ret)
		return ret;

	mutex_lock(&sharedmem_mutex);
	minor = sharedmem_find_free_minor();
	if (minor < 0) {
		mutex_unlock(&sharedmem_mutex);
		pr_err("No free device slots available\n");
		return -ENOSPC;
	}
	mutex_unlock(&sharedmem_mutex);

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		pr_err("Failed to allocate device structure\n");
		return -ENOMEM;
	}

	dev->dev = &sharedmem_pdevs[index]->dev;
	dev->minor = minor;
	spin_lock_init(&dev->lock);
	mutex_init(&dev->region.lock);
	atomic_set(&dev->usage_count, 0);

	dev->region.phys_base = paddr;
	dev->region.size = size;
	dev->region.dev = dev->dev;

	if (claim_mem_region) {
		if (!request_mem_region(dev->region.phys_base, dev->region.size,
					SHAREDMEM_DEVNAME)) {
			pr_err("Failed to request memory region at 0x%llx, size 0x%zx\n",
			       (unsigned long long)dev->region.phys_base,
			       dev->region.size);
			ret = -EBUSY;
			goto err_free_dev;
		}
		dev->region.claimed = true;
	} else {
		pr_warn("%s%d: mapping physical debug range without request_mem_region ownership\n",
			SHAREDMEM_DEVNAME, minor);
	}

	dev->region.virt_base = sharedmem_ioremap(dev->region.phys_base,
						  dev->region.size);
	if (!dev->region.virt_base) {
		pr_err("Failed to map memory region\n");
		ret = -ENOMEM;
		goto err_release_region;
	}

	cdev_init(&dev->cdev, &sharedmem_fops);
	dev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev->cdev, MKDEV(MAJOR(sharedmem_devt), minor), 1);
	if (ret) {
		pr_err("Failed to add character device\n");
		goto err_unmap;
	}

	dev->device = device_create(sharedmem_class, NULL,
				    MKDEV(MAJOR(sharedmem_devt), minor),
				    dev, "%s%d", SHAREDMEM_DEVNAME, minor);
	if (IS_ERR(dev->device)) {
		pr_err("Failed to create device node\n");
		ret = PTR_ERR(dev->device);
		goto err_del_cdev;
	}

	mutex_lock(&sharedmem_mutex);
	sharedmem_devices[minor] = dev;
	mutex_unlock(&sharedmem_mutex);

	pr_info("%s%d: Created with phys addr 0x%llx, size 0x%zx\n",
		SHAREDMEM_DEVNAME, minor,
		(unsigned long long)dev->region.phys_base, dev->region.size);

	return 0;

err_del_cdev:
	cdev_del(&dev->cdev);
err_unmap:
	iounmap(dev->region.virt_base);
err_release_region:
	if (dev->region.claimed)
		release_mem_region(dev->region.phys_base, dev->region.size);
err_free_dev:
	kfree(dev);
	return ret;
}

/**
 * sharedmem_init - initialize the module and create devices
 *
 * Return: 0 on success, negative error code on failure
 */
static int __init sharedmem_init(void)
{
	struct sharedmem_region_desc regions[SHAREDMEM_MAX_DEVICES];
	int devices_created = 0;
	int region_count;
	int ret;
	int i;

	pr_info("Initializing %s driver v%s\n", SHAREDMEM_DEVNAME,
		VERSION_STRING);

	memset(regions, 0, sizeof(regions));

	if (phys_addrs_count <= 0) {
		pr_err("phys_addrs must be specified\n");
		return -EINVAL;
	}

	if (mem_sizes_count <= 0) {
		pr_err("mem_sizes must be specified\n");
		return -EINVAL;
	}

	if (phys_addrs_count > SHAREDMEM_MAX_DEVICES) {
		pr_err("Too many regions: %d (max %d)\n", phys_addrs_count,
		       SHAREDMEM_MAX_DEVICES);
		return -EINVAL;
	}

	if (mem_sizes_count != 1 && mem_sizes_count != phys_addrs_count) {
		pr_err("mem_sizes must contain either one size or one size per phys_addrs entry\n");
		return -EINVAL;
	}

	region_count = phys_addrs_count;
	for (i = 0; i < region_count; i++) {
		unsigned long long region_size;

		if (mem_sizes_count == 1)
			region_size = mem_sizes[0];
		else
			region_size = mem_sizes[i];

		ret = sharedmem_phys_from_param(phys_addrs[i],
						&regions[i].phys_base);
		if (ret) {
			pr_err("Invalid phys_addrs[%d]\n", i);
			return ret;
		}

		ret = sharedmem_size_from_param(region_size, &regions[i].size);
		if (ret) {
			pr_err("Invalid mem_sizes[%d]\n",
			       mem_sizes_count == 1 ? 0 : i);
			return ret;
		}
	}

	ret = sharedmem_validate_regions(regions, region_count);
	if (ret)
		return ret;

	ret = alloc_chrdev_region(&sharedmem_devt, 0, SHAREDMEM_MAX_DEVICES,
				  SHAREDMEM_DEVNAME);
	if (ret < 0) {
		pr_err("Failed to allocate device numbers: %d\n", ret);
		return ret;
	}

	sharedmem_class = sharedmem_class_create(SHAREDMEM_DEVNAME);
	if (IS_ERR(sharedmem_class)) {
		ret = PTR_ERR(sharedmem_class);
		pr_err("Failed to create device class: %d\n", ret);
		goto err_unregister_chrdev;
	}

	sharedmem_pdevs = kcalloc(region_count, sizeof(*sharedmem_pdevs),
				  GFP_KERNEL);
	if (!sharedmem_pdevs) {
		ret = -ENOMEM;
		pr_err("Failed to allocate platform device array\n");
		goto err_destroy_class;
	}
	sharedmem_pdev_count = region_count;

	for (i = 0; i < region_count; i++) {
		sharedmem_pdevs[i] = platform_device_alloc(SHAREDMEM_DEVNAME,
							   i);
		if (!sharedmem_pdevs[i]) {
			pr_err("Failed to allocate platform device %d\n", i);
			ret = -ENOMEM;
			goto err_cleanup;
		}

		ret = platform_device_add(sharedmem_pdevs[i]);
		if (ret) {
			pr_err("Failed to add platform device %d: %d\n", i,
			       ret);
			platform_device_put(sharedmem_pdevs[i]);
			sharedmem_pdevs[i] = NULL;
			goto err_cleanup;
		}

		ret = sharedmem_setup_device(regions[i].phys_base,
					     regions[i].size, i);
		if (ret) {
			pr_err("Failed to set up device %d: %d\n", i, ret);
			goto err_cleanup;
		}

		devices_created++;
	}

	pr_info("%s driver initialized successfully with %d device(s)\n",
		SHAREDMEM_DEVNAME, devices_created);
	return 0;

err_cleanup:
	for (i = 0; i < SHAREDMEM_MAX_DEVICES; i++) {
		if (sharedmem_devices[i]) {
			sharedmem_cleanup_device(sharedmem_devices[i]);
			sharedmem_devices[i] = NULL;
		}
	}

	for (i = 0; i < sharedmem_pdev_count; i++) {
		if (sharedmem_pdevs[i])
			platform_device_unregister(sharedmem_pdevs[i]);
	}
	kfree(sharedmem_pdevs);
	sharedmem_pdevs = NULL;
	sharedmem_pdev_count = 0;

err_destroy_class:
	class_destroy(sharedmem_class);
err_unregister_chrdev:
	unregister_chrdev_region(sharedmem_devt, SHAREDMEM_MAX_DEVICES);
	return ret;
}

/**
 * sharedmem_exit - release module resources
 */
static void __exit sharedmem_exit(void)
{
	int i;

	pr_info("Unloading %s driver\n", SHAREDMEM_DEVNAME);

	for (i = 0; i < SHAREDMEM_MAX_DEVICES; i++) {
		if (sharedmem_devices[i]) {
			sharedmem_cleanup_device(sharedmem_devices[i]);
			sharedmem_devices[i] = NULL;
		}
	}

	for (i = 0; i < sharedmem_pdev_count; i++) {
		if (sharedmem_pdevs[i])
			platform_device_unregister(sharedmem_pdevs[i]);
	}
	kfree(sharedmem_pdevs);
	sharedmem_pdevs = NULL;
	sharedmem_pdev_count = 0;

	class_destroy(sharedmem_class);

	unregister_chrdev_region(sharedmem_devt, SHAREDMEM_MAX_DEVICES);

	pr_info("%s driver unloaded\n", SHAREDMEM_DEVNAME);
}

module_init(sharedmem_init);
module_exit(sharedmem_exit);
