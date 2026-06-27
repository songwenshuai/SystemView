/* SPDX-License-Identifier: GPL-2.0-only OR BSD-1-Clause */
/*
 * SharedMem userspace ABI.
 *
 * Copyright (c) 2023-2026 CineLogic.
 */

#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <linux/ioctl.h>
#include <linux/types.h>

#ifdef __KERNEL__
typedef __u64 sharedmem_addr_t;
typedef __u64 sharedmem_size_t;
typedef __u64 sharedmem_user_ptr_t;
#else
#include <stdint.h>

typedef uint64_t sharedmem_addr_t;
typedef uint64_t sharedmem_size_t;
typedef uint64_t sharedmem_user_ptr_t;
#endif

/**
 * struct sharedmem_ioctl_data - shared memory ioctl transfer descriptor
 * @address: Offset from the mapped region base
 * @size: Size of data to read or write
 * @data: User-space pointer to data buffer, encoded as a 64-bit value
 */
struct sharedmem_ioctl_data {
	sharedmem_addr_t address;
	sharedmem_size_t size;
	sharedmem_user_ptr_t data;
};

#define SHAREDMEM_MAGIC		'S'

#define SHAREDMEM_IOCTL_READ \
	_IOWR(SHAREDMEM_MAGIC, 0, struct sharedmem_ioctl_data)
#define SHAREDMEM_IOCTL_WRITE \
	_IOW(SHAREDMEM_MAGIC, 1, struct sharedmem_ioctl_data)
#define SHAREDMEM_GET_PHYS_ADDR \
	_IOR(SHAREDMEM_MAGIC, 2, sharedmem_addr_t)
#define SHAREDMEM_GET_MEM_SIZE \
	_IOR(SHAREDMEM_MAGIC, 3, sharedmem_size_t)

#define SHAREDMEM_MAX_NR	3

#endif /* SHAREDMEM_H */
