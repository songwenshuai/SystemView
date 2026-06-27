/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*        SEGGER RTT * Real Time Transfer for embedded targets        *
*                  https://github.com/SEGGERMicro/RTT                *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------
Purpose : Implementation of SEGGER real-time transfer which allows
          real-time communication on targets which support debugger
          memory accesses while the CPU is running.

          SEGGER strongly recommends to not make any changes to or 
          modify the source code of this software in order to stay
          compatible with the RTT protocol and J-Link.

----------------------------------------------------------------------
*/

#ifndef SEGGER_RTT_H
#define SEGGER_RTT_H

#include "SEGGER_RTT_ConfDefaults.h"

/*********************************************************************
*
*       Defines, defaults
*
**********************************************************************
*/

#ifndef RTT_USE_ASM
  //
  // Some cores support out-of-order memory accesses (reordering of memory accesses in the core)
  // For such cores, we need to define a memory barrier to guarantee the order of certain accesses to the RTT ring buffers.
  // Needed for:
  //   Cortex-M7 (ARMv7-M)
  //   Cortex-M23 (ARM-v8M)
  //   Cortex-M33 (ARM-v8M)
  //   Cortex-A/R (ARM-v7A/R)
  //
  // We do not explicitly check for "Embedded Studio" as the compiler in use determines what we support.
  // You can use an external toolchain like IAR inside ES. So there is no point in checking for "Embedded Studio"
  //
  #if (defined __CROSSWORKS_ARM)                  // Rowley Crossworks
    #define _CC_HAS_RTT_ASM_SUPPORT 1
    #if (defined __ARM_ARCH_7M__)                 // Cortex-M3
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
    #elif (defined __ARM_ARCH_7EM__)              // Cortex-M4/M7
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_BASE__)          // Cortex-M23
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_MAIN__)          // Cortex-M33
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined(__ARM_ARCH_8_1M_MAIN__))       // Cortex-M85
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #else
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
    #endif
  #elif (defined __ARMCC_VERSION)
    //
    // ARM compiler
    // ARM compiler V6.0 and later is clang based.
    // Our ASM part is compatible to clang.
    //
    #if (__ARMCC_VERSION >= 6000000)
      #define _CC_HAS_RTT_ASM_SUPPORT 1
    #else
      #define _CC_HAS_RTT_ASM_SUPPORT 0
    #endif
    #if (defined __ARM_ARCH_6M__)                 // Cortex-M0 / M1
      #define _CORE_HAS_RTT_ASM_SUPPORT 0         // No ASM support for this architecture
    #elif (defined __ARM_ARCH_7M__)               // Cortex-M3
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
    #elif (defined __ARM_ARCH_7EM__)              // Cortex-M4/M7
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_BASE__)          // Cortex-M23
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_MAIN__)          // Cortex-M33
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8_1M_MAIN__)        // Cortex-M85
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif \
    ((defined __ARM_ARCH_7A__) || (defined __ARM_ARCH_7R__)) || \
    ((defined __ARM_ARCH_8A__) || (defined __ARM_ARCH_8R__))
      //
      // Cortex-A/R ARMv7-A/R & ARMv8-A/R
      //
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #else
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
    #endif
  #elif ((defined __GNUC__) || (defined __clang__))
    //
    // GCC / Clang
    //
    #define _CC_HAS_RTT_ASM_SUPPORT 1
    // ARM 7/9: __ARM_ARCH_5__ / __ARM_ARCH_5E__ / __ARM_ARCH_5T__ / __ARM_ARCH_5T__ / __ARM_ARCH_5TE__
    #if (defined __ARM_ARCH_7M__)                 // Cortex-M3
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
    #elif (defined __ARM_ARCH_7EM__)              // Cortex-M4/M7
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1         // Only Cortex-M7 needs a DMB but we cannot distinguish M4 and M7 here...
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_BASE__)          // Cortex-M23
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8M_MAIN__)          // Cortex-M33
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif (defined __ARM_ARCH_8_1M_MAIN__)        // Cortex-M85
      #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #elif \
    (defined __ARM_ARCH_7A__) || (defined __ARM_ARCH_7R__) || \
    (defined __ARM_ARCH_8A__) || (defined __ARM_ARCH_8R__)
      //
      // Cortex-A/R ARMv7-A/R & ARMv8-A/R
      //
      #define _CORE_NEEDS_DMB           1
      #define RTT__DMB() __asm volatile ("dmb\n" : : :);
    #else
      #define _CORE_HAS_RTT_ASM_SUPPORT 0
    #endif
  #elif ((defined __IASMARM__) || (defined __ICCARM__))
    //
    // IAR assembler/compiler
    //
    #define _CC_HAS_RTT_ASM_SUPPORT 1
    #if (__VER__ < 6300000)
      #define VOLATILE
    #else
      #define VOLATILE volatile
    #endif
    #if (defined __ARM7M__)                            // Needed for old versions that do not know the define yet
      #if (__CORE__ == __ARM7M__)                      // Cortex-M3
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
      #endif
    #endif
    #if (defined __ARM7EM__)
      #if (__CORE__ == __ARM7EM__)                     // Cortex-M4/M7
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8M_BASELINE__)
      #if (__CORE__ == __ARM8M_BASELINE__)             // Cortex-M23
        #define _CORE_HAS_RTT_ASM_SUPPORT 0
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8M_MAINLINE__)
      #if (__CORE__ == __ARM8M_MAINLINE__)             // Cortex-M33
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if (defined __ARM8EM_MAINLINE__)
      #if (__CORE__ == __ARM8EM_MAINLINE__)            // Cortex-???
        #define _CORE_HAS_RTT_ASM_SUPPORT 1
        #define _CORE_NEEDS_DMB 1
        #define RTT__DMB() asm VOLATILE ("DMB");
      #endif
    #endif
    #if\
    ((defined __ARM7A__) && (__CORE__ == __ARM7A__)) || \
    ((defined __ARM7R__) && (__CORE__ == __ARM7R__)) || \
    ((defined __ARM8A__) && (__CORE__ == __ARM8A__)) || \
    ((defined __ARM8R__) && (__CORE__ == __ARM8R__))
      //
      // Cortex-A/R ARMv7-A/R & ARMv8-A/R
      //
       #define _CORE_NEEDS_DMB 1
      #define RTT__DMB() asm VOLATILE ("DMB");
    #endif
  #else
    //
    // Other compilers
    //
    #define _CC_HAS_RTT_ASM_SUPPORT   0
    #define _CORE_HAS_RTT_ASM_SUPPORT 0
  #endif
  //
  // If IDE and core support the ASM version, enable ASM version by default
  //
  #ifndef _CORE_HAS_RTT_ASM_SUPPORT
    #define _CORE_HAS_RTT_ASM_SUPPORT 0              // Default for unknown cores
  #endif
  #if SEGGER_RTT_USE_SHARED_MEMORY
    #define RTT_USE_ASM                           (0)
  #elif (_CC_HAS_RTT_ASM_SUPPORT && _CORE_HAS_RTT_ASM_SUPPORT)
    #define RTT_USE_ASM                           (1)
  #else
    #define RTT_USE_ASM                           (0)
  #endif
#endif

#if SEGGER_RTT_USE_SHARED_MEMORY && RTT_USE_ASM
  #undef  RTT_USE_ASM
  #define RTT_USE_ASM                           (0)
#endif

#ifndef _CORE_NEEDS_DMB
  #define _CORE_NEEDS_DMB 0
#endif

#ifndef RTT__DMB
  #if SEGGER_RTT_USE_SHARED_MEMORY
    #if defined(SEGGER_RTT_SHARED_MEMORY_BARRIER)
      #define RTT__DMB() SEGGER_RTT_SHARED_MEMORY_BARRIER()
    #elif ((defined __GNUC__) || (defined __clang__))
      #define RTT__DMB() __sync_synchronize()
    #elif defined(__ICCARM__)
      #include <intrinsics.h>
      #define RTT__DMB() __DMB()
    #elif defined(__CC_ARM)
      #define RTT__DMB() do { __schedule_barrier(); __asm { DMB }; __schedule_barrier(); } while (0)
    #else
      #error "Don't know how to place shared-memory barrier"
    #endif
  #elif _CORE_NEEDS_DMB
    #error "Don't know how to place inline assembly for DMB"
  #else
    #define RTT__DMB()
  #endif
#endif

#ifndef SEGGER_RTT_CPU_CACHE_LINE_SIZE
  #define SEGGER_RTT_CPU_CACHE_LINE_SIZE (0)   // On most target systems where RTT is used, we do not have a CPU cache, therefore 0 is a good default here
#endif

#ifndef SEGGER_RTT_UNCACHED_OFF
  #if SEGGER_RTT_CPU_CACHE_LINE_SIZE
    #error "SEGGER_RTT_UNCACHED_OFF must be defined when setting SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #else
    #define SEGGER_RTT_UNCACHED_OFF (0)
  #endif
#endif
#if RTT_USE_ASM
  #if SEGGER_RTT_CPU_CACHE_LINE_SIZE
    #error "RTT_USE_ASM is not available if SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
#endif

#ifndef SEGGER_RTT_ASM  // defined when SEGGER_RTT.h is included from assembly file
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

//
// Determine how much we must pad the control block to make it a multiple of a cache line in size
// Assuming: U8 = 1B
//           U16 = 2B
//           U32 = 4B
//           U8/U16/U32* = 4B
//
#if SEGGER_RTT_CPU_CACHE_LINE_SIZE    // Avoid division by zero in case we do not have any cache
  #define SEGGER_RTT__ROUND_UP_2_CACHE_LINE_SIZE(NumBytes) (((NumBytes + SEGGER_RTT_CPU_CACHE_LINE_SIZE - 1) / SEGGER_RTT_CPU_CACHE_LINE_SIZE) * SEGGER_RTT_CPU_CACHE_LINE_SIZE)
#else
  #define SEGGER_RTT__ROUND_UP_2_CACHE_LINE_SIZE(NumBytes) (NumBytes)
#endif
#define SEGGER_RTT__AC_ID_SIZE                           (16u)
#define SEGGER_RTT__CB_FIELD_SIZE                        (4u)
#define SEGGER_RTT__OFFSET_FIELD_SIZE                    (8u)
#define SEGGER_RTT__CB_ALIGNMENT                         (8u)
#define SEGGER_RTT__CB_ALIGNMENT_MASK                    (SEGGER_RTT__CB_ALIGNMENT - 1u)
#define SEGGER_RTT__BUFFER_OFF_NAME                      (0u)
#define SEGGER_RTT__BUFFER_OFF_P_BUFFER                  (SEGGER_RTT__BUFFER_OFF_NAME + SEGGER_RTT__OFFSET_FIELD_SIZE)
#define SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER            (SEGGER_RTT__BUFFER_OFF_P_BUFFER + SEGGER_RTT__OFFSET_FIELD_SIZE)
#define SEGGER_RTT__BUFFER_OFF_WR_OFF                    (SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER + SEGGER_RTT__CB_FIELD_SIZE)
#define SEGGER_RTT__BUFFER_OFF_RD_OFF                    (SEGGER_RTT__BUFFER_OFF_WR_OFF + SEGGER_RTT__CB_FIELD_SIZE)
#define SEGGER_RTT__BUFFER_OFF_FLAGS                     (SEGGER_RTT__BUFFER_OFF_RD_OFF + SEGGER_RTT__CB_FIELD_SIZE)
#define SEGGER_RTT__BUFFER_SIZE                          (SEGGER_RTT__BUFFER_OFF_FLAGS + SEGGER_RTT__CB_FIELD_SIZE)
#define SEGGER_RTT__CB_OFF_AC_ID                         (0u)
#define SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS            (SEGGER_RTT__CB_OFF_AC_ID + SEGGER_RTT__AC_ID_SIZE)
#define SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS          (SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS + SEGGER_RTT__CB_FIELD_SIZE)
#define SEGGER_RTT__CB_OFF_A_UP                          (SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS + SEGGER_RTT__CB_FIELD_SIZE)
#define SEGGER_RTT__CB_OFF_A_DOWN                        (SEGGER_RTT__CB_OFF_A_UP + (SEGGER_RTT_MAX_NUM_UP_BUFFERS * SEGGER_RTT__BUFFER_SIZE))
#define SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex)       (SEGGER_RTT__CB_OFF_A_UP + ((BufferIndex) * SEGGER_RTT__BUFFER_SIZE))
#define SEGGER_RTT__CB_OFF_A_DOWN_INDEX(BufferIndex)     (SEGGER_RTT__CB_OFF_A_DOWN + ((BufferIndex) * SEGGER_RTT__BUFFER_SIZE))
#define SEGGER_RTT__CB_SIZE                              (SEGGER_RTT__CB_OFF_A_DOWN + (SEGGER_RTT_MAX_NUM_DOWN_BUFFERS * SEGGER_RTT__BUFFER_SIZE))
#define SEGGER_RTT__CB_SIZE_ALIGNED                      (SEGGER_RTT__ROUND_UP_2_CACHE_LINE_SIZE(SEGGER_RTT__CB_SIZE))
#define SEGGER_RTT__CB_PADDING                           (SEGGER_RTT__CB_SIZE_ALIGNED - SEGGER_RTT__CB_SIZE)
#define SEGGER_RTT__ROUND_UP_4(NumBytes)                 (((NumBytes) + 3u) & ~3u)
#define SEGGER_RTT__TERMINAL_NAME_SIZE                   (9u)
#define SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED           (SEGGER_RTT__ROUND_UP_4(SEGGER_RTT__TERMINAL_NAME_SIZE))
#define SEGGER_RTT__UP_NAME_OFF                          (SEGGER_RTT__CB_SIZE_ALIGNED)
#define SEGGER_RTT__UP_BUFFER_OFF                        (SEGGER_RTT__UP_NAME_OFF + SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED)
#define SEGGER_RTT__DOWN_NAME_OFF                        (SEGGER_RTT__UP_BUFFER_OFF + BUFFER_SIZE_UP)
#define SEGGER_RTT__DOWN_BUFFER_OFF                      (SEGGER_RTT__DOWN_NAME_OFF + SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED)
#define SEGGER_RTT__ACTIVE_TERMINAL_OFF                  (SEGGER_RTT__DOWN_BUFFER_OFF + BUFFER_SIZE_DOWN)
#define SEGGER_RTT__REQUIRED_MEM_SIZE                    (SEGGER_RTT__ROUND_UP_4(SEGGER_RTT__ACTIVE_TERMINAL_OFF + 1u))
#define SEGGER_RTT__EXTRA_BUFFER_PAIR_SIZE               (SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED + BUFFER_SIZE_UP + BUFFER_SIZE_DOWN)
#define SEGGER_RTT_REQUIRED_MEM_SIZE_FOR_BUFFER_PAIRS(NumBuffers) \
        (((NumBuffers) == 0u) ? ((size_t)0u) : \
         (SEGGER_RTT__REQUIRED_MEM_SIZE + (((size_t)((NumBuffers) - 1u)) * SEGGER_RTT__EXTRA_BUFFER_PAIR_SIZE)))
#define SEGGER_RTT_SPINLOCK_SW_WORD_SIZE                 (4u)
#define SEGGER_RTT_SPINLOCK_SW_HEADER_SIZE               (8u)
#define SEGGER_RTT_SPINLOCK_SW_SIZE                      (SEGGER_RTT_SPINLOCK_SW_HEADER_SIZE + (SEGGER_RTT_SPINLOCK_MAX_CORES * SEGGER_RTT_SPINLOCK_SW_WORD_SIZE * 2u))
#define SEGGER_RTT__RD8(Address)                         (*(volatile unsigned char*)(uintptr_t)(Address))
#define SEGGER_RTT__WR8(Address, Data)                   (*(volatile unsigned char*)(uintptr_t)(Address) = (unsigned char)(Data))
#define SEGGER_RTT__RD32(Address)                        (*(volatile uint32_t*)(uintptr_t)(Address))
#define SEGGER_RTT__WR32(Address, Data)                  (*(volatile uint32_t*)(uintptr_t)(Address) = (uint32_t)(Data))
#define SEGGER_RTT__RD64(Address)                        (*(volatile uint64_t*)(uintptr_t)(Address))
#define SEGGER_RTT__WR64(Address, Data)                  (*(volatile uint64_t*)(uintptr_t)(Address) = (uint64_t)(Data))
#define SEGGER_RTT__ADDR(Address, Off)                   ((uintptr_t)(Address) + (uintptr_t)(Off))
#define SEGGER_RTT__FIELD(pRing, Off)                    ((uintptr_t)(pRing) + (uintptr_t)(Off))
#define SEGGER_RTT__STATIC_ASSERT(Name, Condition)       typedef char SEGGER_RTT__static_assert_##Name[(Condition) ? 1 : -1]

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

//
// Description for a circular buffer (also called "ring buffer")
// which is used as up-buffer (T->H)
//
typedef struct {
            uint64_t sName;         // 64-bit offset of optional name from RTT control block base. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
            uint64_t pBuffer;       // 64-bit offset of buffer from RTT control block base
            uint32_t SizeOfBuffer;  // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
            uint32_t WrOff;         // Position of next item to be written by either target.
  volatile  uint32_t RdOff;         // Position of next item to be read by host. Must be volatile since it may be modified by host.
            uint32_t Flags;         // Contains configuration flags. Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
} SEGGER_RTT_BUFFER_UP;

//
// Description for a circular buffer (also called "ring buffer")
// which is used as down-buffer (H->T)
//
typedef struct {
            uint64_t sName;         // 64-bit offset of optional name from RTT control block base. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
            uint64_t pBuffer;       // 64-bit offset of buffer from RTT control block base
            uint32_t SizeOfBuffer;  // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
  volatile  uint32_t WrOff;         // Position of next item to be written by host. Must be volatile since it may be modified by host.
            uint32_t RdOff;         // Position of next item to be read by target (down-buffer).
            uint32_t Flags;         // Contains configuration flags. Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
} SEGGER_RTT_BUFFER_DOWN;

//
// RTT control block which describes the number of buffers available
// as well as the configuration for each buffer
//
//
typedef struct {
  char                    acID[16];                                 // Initialized to "SEGGER RTT"
  uint32_t                MaxNumUpBuffers;                          // Initialized to SEGGER_RTT_MAX_NUM_UP_BUFFERS (type. 2)
  uint32_t                MaxNumDownBuffers;                        // Initialized to SEGGER_RTT_MAX_NUM_DOWN_BUFFERS (type. 2)
  SEGGER_RTT_BUFFER_UP    aUp[SEGGER_RTT_MAX_NUM_UP_BUFFERS];       // Up buffers, transferring information up from target via debug probe to host
  SEGGER_RTT_BUFFER_DOWN  aDown[SEGGER_RTT_MAX_NUM_DOWN_BUFFERS];   // Down buffers, transferring information down from host via debug probe to target
#if SEGGER_RTT__CB_PADDING
  unsigned char           aDummy[SEGGER_RTT__CB_PADDING];
#endif
} SEGGER_RTT_CB;

SEGGER_RTT__STATIC_ASSERT(buffer_up_size,          sizeof(SEGGER_RTT_BUFFER_UP) == SEGGER_RTT__BUFFER_SIZE);
SEGGER_RTT__STATIC_ASSERT(buffer_up_name_off,      offsetof(SEGGER_RTT_BUFFER_UP, sName) == SEGGER_RTT__BUFFER_OFF_NAME);
SEGGER_RTT__STATIC_ASSERT(buffer_up_buffer_off,    offsetof(SEGGER_RTT_BUFFER_UP, pBuffer) == SEGGER_RTT__BUFFER_OFF_P_BUFFER);
SEGGER_RTT__STATIC_ASSERT(buffer_up_size_off,      offsetof(SEGGER_RTT_BUFFER_UP, SizeOfBuffer) == SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER);
SEGGER_RTT__STATIC_ASSERT(buffer_up_wr_off,        offsetof(SEGGER_RTT_BUFFER_UP, WrOff) == SEGGER_RTT__BUFFER_OFF_WR_OFF);
SEGGER_RTT__STATIC_ASSERT(buffer_up_rd_off,        offsetof(SEGGER_RTT_BUFFER_UP, RdOff) == SEGGER_RTT__BUFFER_OFF_RD_OFF);
SEGGER_RTT__STATIC_ASSERT(buffer_up_flags_off,     offsetof(SEGGER_RTT_BUFFER_UP, Flags) == SEGGER_RTT__BUFFER_OFF_FLAGS);
SEGGER_RTT__STATIC_ASSERT(buffer_down_size,        sizeof(SEGGER_RTT_BUFFER_DOWN) == SEGGER_RTT__BUFFER_SIZE);
SEGGER_RTT__STATIC_ASSERT(buffer_down_name_off,    offsetof(SEGGER_RTT_BUFFER_DOWN, sName) == SEGGER_RTT__BUFFER_OFF_NAME);
SEGGER_RTT__STATIC_ASSERT(buffer_down_buffer_off,  offsetof(SEGGER_RTT_BUFFER_DOWN, pBuffer) == SEGGER_RTT__BUFFER_OFF_P_BUFFER);
SEGGER_RTT__STATIC_ASSERT(buffer_down_size_off,    offsetof(SEGGER_RTT_BUFFER_DOWN, SizeOfBuffer) == SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER);
SEGGER_RTT__STATIC_ASSERT(buffer_down_wr_off,      offsetof(SEGGER_RTT_BUFFER_DOWN, WrOff) == SEGGER_RTT__BUFFER_OFF_WR_OFF);
SEGGER_RTT__STATIC_ASSERT(buffer_down_rd_off,      offsetof(SEGGER_RTT_BUFFER_DOWN, RdOff) == SEGGER_RTT__BUFFER_OFF_RD_OFF);
SEGGER_RTT__STATIC_ASSERT(buffer_down_flags_off,   offsetof(SEGGER_RTT_BUFFER_DOWN, Flags) == SEGGER_RTT__BUFFER_OFF_FLAGS);
SEGGER_RTT__STATIC_ASSERT(cb_id_off,               offsetof(SEGGER_RTT_CB, acID) == SEGGER_RTT__CB_OFF_AC_ID);
SEGGER_RTT__STATIC_ASSERT(cb_max_up_off,           offsetof(SEGGER_RTT_CB, MaxNumUpBuffers) == SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS);
SEGGER_RTT__STATIC_ASSERT(cb_max_down_off,         offsetof(SEGGER_RTT_CB, MaxNumDownBuffers) == SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS);
SEGGER_RTT__STATIC_ASSERT(cb_a_up_off,             offsetof(SEGGER_RTT_CB, aUp) == SEGGER_RTT__CB_OFF_A_UP);
SEGGER_RTT__STATIC_ASSERT(cb_a_down_off,           offsetof(SEGGER_RTT_CB, aDown) == SEGGER_RTT__CB_OFF_A_DOWN);
SEGGER_RTT__STATIC_ASSERT(spinlock_max_cores,      SEGGER_RTT_SPINLOCK_MAX_CORES > 0u);

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/
//
// RTT control block data is provided by the application in shared memory.
// No default SEGGER_RTT_CB object is allocated by the RTT module.
//

/*********************************************************************
*
*       RTT API functions
*
**********************************************************************
*/
//
// Address parameters are local mapped addresses that are directly
// accessible by the current process or core. Shared RTT descriptors store
// fixed-width 64-bit offsets from this address, not backend or physical addresses.
//
#ifdef __cplusplus
  extern "C" {
#endif
int          SEGGER_RTT_AllocDownBuffer         (uintptr_t Address, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          SEGGER_RTT_AllocUpBuffer           (uintptr_t Address, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          SEGGER_RTT_CheckInit               (uintptr_t Address);
int          SEGGER_RTT_CheckRegion             (uintptr_t Address, size_t Size);
int          SEGGER_RTT_CheckUpBuffer           (uintptr_t Address, size_t Size, unsigned BufferIndex);
int          SEGGER_RTT_CheckDownBuffer         (uintptr_t Address, size_t Size, unsigned BufferIndex);
int          SEGGER_RTT_FindControlBlock        (uintptr_t* pAddress, size_t Size);
int          SEGGER_RTT_FindValidControlBlock   (uintptr_t* pAddress, size_t Size, size_t* pRegionSize);
int          SEGGER_RTT_ConfigUpBuffer          (uintptr_t Address, unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          SEGGER_RTT_ConfigDownBuffer        (uintptr_t Address, unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
size_t       SEGGER_RTT_GetRequiredMemSize      (unsigned NumBuffers);
int          SEGGER_RTT_GetKey                  (uintptr_t Address);
unsigned     SEGGER_RTT_HasData                 (uintptr_t Address, unsigned BufferIndex);
int          SEGGER_RTT_HasKey                  (uintptr_t Address);
unsigned     SEGGER_RTT_HasDataUp               (uintptr_t Address, unsigned BufferIndex);
void         SEGGER_RTT_Init                    (uintptr_t Address);
int          SEGGER_RTT_InitEx                  (uintptr_t Address, size_t Size);
int          SEGGER_RTT_EnsureInitEx            (uintptr_t Address, size_t Size, unsigned NumBuffers);
unsigned     SEGGER_RTT_Read                    (uintptr_t Address, unsigned BufferIndex,       void* pBuffer, unsigned BufferSize);
unsigned     SEGGER_RTT_ReadNoLock              (uintptr_t Address, unsigned BufferIndex,       void* pData,   unsigned BufferSize);
int          SEGGER_RTT_SetNameDownBuffer       (uintptr_t Address, unsigned BufferIndex, const char* sName);
int          SEGGER_RTT_SetNameUpBuffer         (uintptr_t Address, unsigned BufferIndex, const char* sName);
int          SEGGER_RTT_SetFlagsDownBuffer      (uintptr_t Address, unsigned BufferIndex, unsigned Flags);
int          SEGGER_RTT_SetFlagsUpBuffer        (uintptr_t Address, unsigned BufferIndex, unsigned Flags);
int          SEGGER_RTT_WaitKey                 (uintptr_t Address);
unsigned     SEGGER_RTT_Write                   (uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_WriteNoLock             (uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_WriteSkipNoLock         (uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_WriteString             (uintptr_t Address, unsigned BufferIndex, const char* s);
void         SEGGER_RTT_WriteWithOverwriteNoLock(uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_PutChar                 (uintptr_t Address, unsigned BufferIndex, char c);
unsigned     SEGGER_RTT_PutCharSkip             (uintptr_t Address, unsigned BufferIndex, char c);
unsigned     SEGGER_RTT_PutCharSkipNoLock       (uintptr_t Address, unsigned BufferIndex, char c);
unsigned     SEGGER_RTT_GetAvailWriteSpace      (uintptr_t Address, unsigned BufferIndex);
unsigned     SEGGER_RTT_GetBytesInBuffer        (uintptr_t Address, unsigned BufferIndex);
unsigned     SEGGER_RTT_GetBytesDownInBuffer    (uintptr_t Address, unsigned BufferIndex);
int          SEGGER_RTT_SPINLOCK_SW_Create      (uintptr_t Address, size_t Size);
int          SEGGER_RTT_SPINLOCK_SW_Check       (uintptr_t Address, size_t Size);
int          SEGGER_RTT_SPINLOCK_SW_Lock        (uintptr_t Address, unsigned Id);
int          SEGGER_RTT_SPINLOCK_SW_LockWithLimit(uintptr_t Address, unsigned Id, uint32_t MaxWaitSpins);
int          SEGGER_RTT_SPINLOCK_SW_Unlock      (uintptr_t Address, unsigned Id);
//
// Function macro for performance optimization
//
#define      SEGGER_RTT_HASDATA(Address, n)       (SEGGER_RTT_HasData((Address), (n)))

/*********************************************************************
*
*       RTT transfer functions to send RTT data via other channels.
*
**********************************************************************
*/
unsigned     SEGGER_RTT_ReadUpBuffer            (uintptr_t Address, unsigned BufferIndex, void* pBuffer, unsigned BufferSize);
unsigned     SEGGER_RTT_ReadUpBufferNoLock      (uintptr_t Address, unsigned BufferIndex, void* pData, unsigned BufferSize);
unsigned     SEGGER_RTT_WriteDownBuffer         (uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_WriteDownBufferNoLock   (uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);

#define      SEGGER_RTT_HASDATA_UP(Address, n)    (SEGGER_RTT_HasDataUp((Address), (n)))

/*********************************************************************
*
*       RTT "Terminal" API functions
*
**********************************************************************
*/
int     SEGGER_RTT_SetTerminal        (uintptr_t Address, unsigned char TerminalId);
int     SEGGER_RTT_TerminalOut        (uintptr_t Address, unsigned char TerminalId, const char* s);

/*********************************************************************
*
*       RTT printf functions (require SEGGER_RTT_printf.c)
*
**********************************************************************
*/
int SEGGER_RTT_printf(uintptr_t Address, unsigned BufferIndex, const char * sFormat, ...);
int SEGGER_RTT_vprintf(uintptr_t Address, unsigned BufferIndex, const char * sFormat, va_list * pParamList);

#ifdef __cplusplus
  }
#endif

#endif // ifndef(SEGGER_RTT_ASM)

//
// For some environments, NULL may not be defined until certain headers are included
//
#ifndef NULL
  #define NULL  ((void*)0)
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

//
// Operating modes. Define behavior if buffer is full (not enough space for entire message)
//
#define SEGGER_RTT_MODE_NO_BLOCK_SKIP         (0)     // Skip. Do not block, output nothing. (Default)
#define SEGGER_RTT_MODE_NO_BLOCK_TRIM         (1)     // Trim: Do not block, output as much as fits.
#define SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL    (2)     // Block: Wait until there is space in the buffer.
#define SEGGER_RTT_MODE_MASK                  (3)

//
// Control sequences, based on ANSI.
// Can be used to control color, and clear the screen
//
#define RTT_CTRL_RESET                "\x1B[0m"         // Reset to default colors
#define RTT_CTRL_CLEAR                "\x1B[2J"         // Clear screen, reposition cursor to top left

#define RTT_CTRL_TEXT_BLACK           "\x1B[2;30m"
#define RTT_CTRL_TEXT_RED             "\x1B[2;31m"
#define RTT_CTRL_TEXT_GREEN           "\x1B[2;32m"
#define RTT_CTRL_TEXT_YELLOW          "\x1B[2;33m"
#define RTT_CTRL_TEXT_BLUE            "\x1B[2;34m"
#define RTT_CTRL_TEXT_MAGENTA         "\x1B[2;35m"
#define RTT_CTRL_TEXT_CYAN            "\x1B[2;36m"
#define RTT_CTRL_TEXT_WHITE           "\x1B[2;37m"

#define RTT_CTRL_TEXT_BRIGHT_BLACK    "\x1B[1;30m"
#define RTT_CTRL_TEXT_BRIGHT_RED      "\x1B[1;31m"
#define RTT_CTRL_TEXT_BRIGHT_GREEN    "\x1B[1;32m"
#define RTT_CTRL_TEXT_BRIGHT_YELLOW   "\x1B[1;33m"
#define RTT_CTRL_TEXT_BRIGHT_BLUE     "\x1B[1;34m"
#define RTT_CTRL_TEXT_BRIGHT_MAGENTA  "\x1B[1;35m"
#define RTT_CTRL_TEXT_BRIGHT_CYAN     "\x1B[1;36m"
#define RTT_CTRL_TEXT_BRIGHT_WHITE    "\x1B[1;37m"

#define RTT_CTRL_BG_BLACK             "\x1B[24;40m"
#define RTT_CTRL_BG_RED               "\x1B[24;41m"
#define RTT_CTRL_BG_GREEN             "\x1B[24;42m"
#define RTT_CTRL_BG_YELLOW            "\x1B[24;43m"
#define RTT_CTRL_BG_BLUE              "\x1B[24;44m"
#define RTT_CTRL_BG_MAGENTA           "\x1B[24;45m"
#define RTT_CTRL_BG_CYAN              "\x1B[24;46m"
#define RTT_CTRL_BG_WHITE             "\x1B[24;47m"

#define RTT_CTRL_BG_BRIGHT_BLACK      "\x1B[4;40m"
#define RTT_CTRL_BG_BRIGHT_RED        "\x1B[4;41m"
#define RTT_CTRL_BG_BRIGHT_GREEN      "\x1B[4;42m"
#define RTT_CTRL_BG_BRIGHT_YELLOW     "\x1B[4;43m"
#define RTT_CTRL_BG_BRIGHT_BLUE       "\x1B[4;44m"
#define RTT_CTRL_BG_BRIGHT_MAGENTA    "\x1B[4;45m"
#define RTT_CTRL_BG_BRIGHT_CYAN       "\x1B[4;46m"
#define RTT_CTRL_BG_BRIGHT_WHITE      "\x1B[4;47m"


#endif

/*************************** End of file ****************************/
