/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : SharedMem.h
Purpose : User API for SharedMem driver
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef SHAREDMEM_H
#define SHAREDMEM_H

#include <linux/ioctl.h>
#include <linux/types.h>

/* Type compatibility for user-space and kernel-space */
#ifdef __KERNEL__
    /* Kernel-space: use fixed-width UAPI types */
    typedef __u64 sharedmem_addr_t;
    typedef __u64 sharedmem_size_t;
    typedef __u64 sharedmem_user_ptr_t;
#else
    /* User-space: use standard uint64_t */
    #include <stdint.h>
    typedef uint64_t sharedmem_addr_t;
    typedef uint64_t sharedmem_size_t;
    typedef uint64_t sharedmem_user_ptr_t;
#endif

/**
 * struct sharedmem_ioctl_data - Data structure for IOCTL operations
 * @address: Offset from the mapped region base
 * @size:    Size of data to read/write
 * @data:    User-space pointer to data buffer, encoded as a 64-bit value
 *
 * This structure is used for read/write operations through IOCTLs
 */
struct sharedmem_ioctl_data {
    sharedmem_addr_t address;    /* Offset from the mapped region base */
    sharedmem_size_t size;       /* Size of data to read/write */
    sharedmem_user_ptr_t data;   /* User-space pointer encoded as u64 */
};

/* IOCTL command definitions */
#define SHAREDMEM_MAGIC       'S'

/* Read data from the mapped memory region */
#define SHAREDMEM_IOCTL_READ  _IOWR(SHAREDMEM_MAGIC, 0, struct sharedmem_ioctl_data)

/* Write data to the mapped memory region */
#define SHAREDMEM_IOCTL_WRITE _IOW(SHAREDMEM_MAGIC, 1, struct sharedmem_ioctl_data)

/* Get physical base address of the mapped region */
#define SHAREDMEM_GET_PHYS_ADDR _IOR(SHAREDMEM_MAGIC, 2, sharedmem_addr_t)

/* Get size of the mapped region */
#define SHAREDMEM_GET_MEM_SIZE  _IOR(SHAREDMEM_MAGIC, 3, sharedmem_size_t)

/* Maximum number of IOCTL commands (0-3 = 4 commands) */
#define SHAREDMEM_MAX_NR         3

#endif /* SHAREDMEM_H */
