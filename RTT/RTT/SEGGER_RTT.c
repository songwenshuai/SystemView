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
Purpose : Implementation of SEGGER real-time transfer (RTT) which
          allows real-time communication on targets which support
          debugger memory accesses while the CPU is running.

          SEGGER strongly recommends to not make any changes to or 
          modify the source code of this software in order to stay
          compatible with the RTT protocol and J-Link.

Additional information:
          Type "int" is assumed to be 32-bits in size
          H->T    Host to target communication
          T->H    Target to host communication

          RTT channel 0 is always present and reserved for Terminal usage.
          Name is fixed to "Terminal"

          Effective buffer size: SizeOfBuffer - 1

          WrOff == RdOff:       Buffer is empty
          WrOff == (RdOff - 1): Buffer is full
          WrOff >  RdOff:       Free space includes wrap-around
          WrOff <  RdOff:       Used space includes wrap-around
          (WrOff == (SizeOfBuffer - 1)) && (RdOff == 0):
                                Buffer full and wrap-around after next byte

----------------------------------------------------------------------
*/

#include "SEGGER_RTT.h"

#include <string.h>                 // for memcpy

/*********************************************************************
*
*       Configuration, default values
*
**********************************************************************
*/

#ifndef   BUFFER_SIZE_UP
  #define BUFFER_SIZE_UP                                  1024  // Size of the buffer for terminal output of target, up to host
#endif

#ifndef   BUFFER_SIZE_DOWN
  #define BUFFER_SIZE_DOWN                                16    // Size of the buffer for terminal input to target from host (Usually keyboard input)
#endif

#ifndef   SEGGER_RTT_MAX_NUM_UP_BUFFERS
  #define SEGGER_RTT_MAX_NUM_UP_BUFFERS                    2    // Number of up-buffers (T->H) available on this target
#endif

#ifndef   SEGGER_RTT_MAX_NUM_DOWN_BUFFERS
  #define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS                  2    // Number of down-buffers (H->T) available on this target
#endif

#ifndef   SEGGER_RTT_MODE_DEFAULT
  #define SEGGER_RTT_MODE_DEFAULT                         SEGGER_RTT_MODE_NO_BLOCK_SKIP
#endif

#ifndef   SEGGER_RTT_LOCK
  #define SEGGER_RTT_LOCK()
#endif

#ifndef   SEGGER_RTT_UNLOCK
  #define SEGGER_RTT_UNLOCK()
#endif

#ifndef   STRLEN
  #define STRLEN(a)                                       strlen((a))
#endif

#ifndef   STRCPY
  #define STRCPY(pDest, pSrc)                             strcpy((pDest), (pSrc))
#endif

#ifndef   SEGGER_RTT_MEMCPY_USE_BYTELOOP
  #define SEGGER_RTT_MEMCPY_USE_BYTELOOP                  0
#endif

#ifndef   SEGGER_RTT_MEMCPY
  #ifdef  MEMCPY
    #define SEGGER_RTT_MEMCPY(pDest, pSrc, NumBytes)      MEMCPY((pDest), (pSrc), (NumBytes))
  #else
    #define SEGGER_RTT_MEMCPY(pDest, pSrc, NumBytes)      memcpy((pDest), (pSrc), (NumBytes))
  #endif
#endif

#ifndef   MIN
  #define MIN(a, b)                                       (((a) < (b)) ? (a) : (b))
#endif

#ifndef   MAX
  #define MAX(a, b)                                       (((a) > (b)) ? (a) : (b))
#endif

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const unsigned char _aTerminalId[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
static const char _aRTTId[] = "SEGGER RTT";
static const char _aTerminalStr[] = "Terminal";  // Default terminal name copied into shared RTT memory

/*********************************************************************
*
*       Internal helper macros
*
**********************************************************************
*/

//
// Runtime down-buffer offset helpers for systems where another core
// initialized the control block with its own MaxNumUpBuffers value.
//
#define SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME(MaxNumUpBuffers)             (SEGGER_RTT__CB_OFF_A_UP + ((uintptr_t)(MaxNumUpBuffers) * SEGGER_RTT__BUFFER_SIZE))
#define SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex) \
                                                                      (SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME(MaxNumUpBuffers) + ((uintptr_t)(BufferIndex) * SEGGER_RTT__BUFFER_SIZE))
#define SEGGER_RTT_SPINLOCK_SW_MAGIC                                  (0x5254534Cu)
#define SEGGER_RTT_SPINLOCK_SW_OFF_MAGIC                              (0u)
#define SEGGER_RTT_SPINLOCK_SW_OFF_MAX_CORES                          (4u)
#define SEGGER_RTT_SPINLOCK_SW_OFF_ENTERING                           (SEGGER_RTT_SPINLOCK_SW_HEADER_SIZE)
#define SEGGER_RTT_SPINLOCK_SW_OFF_NUMBER                             (SEGGER_RTT_SPINLOCK_SW_OFF_ENTERING + (SEGGER_RTT_SPINLOCK_MAX_CORES * SEGGER_RTT_SPINLOCK_SW_WORD_SIZE))
#define SEGGER_RTT_SPINLOCK_SW_ENTERING_ADDR(Address, Id)             (SEGGER_RTT__ADDR((Address), SEGGER_RTT_SPINLOCK_SW_OFF_ENTERING + ((uintptr_t)(Id) * SEGGER_RTT_SPINLOCK_SW_WORD_SIZE)))
#define SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(Address, Id)               (SEGGER_RTT__ADDR((Address), SEGGER_RTT_SPINLOCK_SW_OFF_NUMBER + ((uintptr_t)(Id) * SEGGER_RTT_SPINLOCK_SW_WORD_SIZE)))

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _IsValidFlags()
*
*  Function description
*    Checks whether a public configuration flags value uses a known RTT
*    operating mode and no reserved bits.
*
*  Parameters
*    Flags  RTT buffer flags.
*
*  Return value
*    != 0  Valid.
*    == 0  Invalid.
*/
static int _IsValidFlags(unsigned Flags) {
  return (Flags <= SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL) ? 1 : 0;
}

/*********************************************************************
*
*       _GetRequiredMemSize()
*
*  Function description
*    Returns the shared-memory size required for the RTT control block,
*    the default channel, and the requested number of extra buffer pairs.
*
*  Parameters
*    NumBuffers  Number of up/down buffer pairs to support.
*
*  Return value
*    > 0  Required shared-memory size.
*    == 0 Invalid buffer count or size overflow.
*/
static size_t _GetRequiredMemSize(unsigned NumBuffers) {
  size_t NumExtraBuffers;

  if ((NumBuffers == 0u) ||
      (NumBuffers > SEGGER_RTT_MAX_NUM_UP_BUFFERS) ||
      (NumBuffers > SEGGER_RTT_MAX_NUM_DOWN_BUFFERS)) {
    return 0u;
  }
  NumExtraBuffers = (size_t)(NumBuffers - 1u);
  if (NumExtraBuffers > ((SIZE_MAX - SEGGER_RTT__REQUIRED_MEM_SIZE) / SEGGER_RTT__EXTRA_BUFFER_PAIR_SIZE)) {
    return 0u;
  }
  return SEGGER_RTT_REQUIRED_MEM_SIZE_FOR_BUFFER_PAIRS(NumBuffers);
}

/*********************************************************************
*
*       _CheckSpinLockRange()
*
*  Function description
*    Checks whether a software spinlock object fits into a directly
*    accessible memory range.
*/
static int _CheckSpinLockRange(uintptr_t Address, size_t Size) {
  if ((Address == 0u) || ((Address & (uintptr_t)(SEGGER_RTT_SPINLOCK_SW_WORD_SIZE - 1u)) != 0u)) {
    return -1;
  }
  if (Size < SEGGER_RTT_SPINLOCK_SW_SIZE) {
    return -1;
  }
  if (Size > (size_t)(UINTPTR_MAX - Address)) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       _SpinLockWaitStep()
*
*  Function description
*    Consume one software spinlock wait iteration.
*/
static int _SpinLockWaitStep(uint32_t* pRemainingSpins) {
  if (pRemainingSpins != NULL) {
    if (*pRemainingSpins == 0u) {
      return -1;
    }
    (*pRemainingSpins)--;
  }
  RTT__DMB();
  return 0;
}

/*********************************************************************
*
*       _SpinLockCancel()
*
*  Function description
*    Withdraw the current core from the software spinlock algorithm.
*/
static void _SpinLockCancel(uintptr_t Address, unsigned Id) {
  SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_ENTERING_ADDR(Address, Id), 0u);
  SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(  Address, Id), 0u);
  RTT__DMB();
}

/*********************************************************************
*
*       _SpinLockAcquire()
*
*  Function description
*    Acquires a software spinlock with optional bounded waiting.
*/
static int _SpinLockAcquire(uintptr_t Address, unsigned Id, uint32_t* pRemainingSpins) {
  uint32_t max;
  uint32_t current;
  uint32_t j;
  uint32_t n;
  uint32_t own_number;

  if ((Address == 0u) || (Id >= SEGGER_RTT_SPINLOCK_MAX_CORES)) {
    return -1;
  }
  if (SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAGIC)) != SEGGER_RTT_SPINLOCK_SW_MAGIC) {
    return -1;
  }
  if (SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAX_CORES)) != SEGGER_RTT_SPINLOCK_MAX_CORES) {
    return -1;
  }

  max = 0u;
  SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_ENTERING_ADDR(Address, Id), 1u);
  RTT__DMB();
  for (n = 0u; n < SEGGER_RTT_SPINLOCK_MAX_CORES; n++) {
    current = SEGGER_RTT__RD32(SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(Address, n));
    if (current > max) {
      max = current;
    }
  }
  if (max == UINT32_MAX) {
    _SpinLockCancel(Address, Id);
    return -1;
  }
  SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(Address, Id), max + 1u);
  RTT__DMB();
  SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_ENTERING_ADDR(Address, Id), 0u);
  RTT__DMB();
  for (j = 0u; j < SEGGER_RTT_SPINLOCK_MAX_CORES; j++) {
    while (SEGGER_RTT__RD32(SEGGER_RTT_SPINLOCK_SW_ENTERING_ADDR(Address, j)) != 0u) {
      if (_SpinLockWaitStep(pRemainingSpins) != 0) {
        _SpinLockCancel(Address, Id);
        return -1;
      }
    }
    for (;;) {
      current = SEGGER_RTT__RD32(SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(Address, j));
      own_number = SEGGER_RTT__RD32(SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(Address, Id));
      if ((current == 0u) ||
          (current > own_number) ||
          ((current == own_number) && (j >= Id))) {
        break;
      }
      if (_SpinLockWaitStep(pRemainingSpins) != 0) {
        _SpinLockCancel(Address, Id);
        return -1;
      }
    }
  }
  RTT__DMB();
  return 0;
}

/*********************************************************************
*
*       _IsRTTId()
*
*  Function description
*    Checks whether a control block contains the complete initialized
*    RTT ID string.
*
*  Parameters
*    Address  Base address of the RTT control block.
*
*  Return value
*    != 0  Complete RTT ID is present.
*    == 0  RTT ID is missing or address is invalid.
*/
static int _IsRTTId(uintptr_t Address) {
  unsigned i;

  if ((Address == 0u) || ((Address & SEGGER_RTT__CB_ALIGNMENT_MASK) != 0u)) {
    return 0;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0;
    }
  }
  RTT__DMB();                         // Force ID read before reading the rest of the control block
  return 1;
}

/*********************************************************************
*
*       _IsValidSharedName()
*
*  Function description
*    Checks whether an optional shared-memory string pointer can be
*    represented as a 64-bit offset from the control block base.
*
*  Parameters
*    Address  Base address of the RTT control block.
*    Ptr      Optional pointer to validate.
*
*  Return value
*    != 0  Pointer is NULL or valid.
*    == 0  Pointer cannot be encoded as a shared-memory offset.
*/
static int _IsValidSharedName(uintptr_t Address, const void* Ptr) {
  uintptr_t Addr;

  Addr = (uintptr_t)Ptr;
  if (Addr == 0u) {
    return 1;
  }
  if (Addr <= Address) {
    return 0;
  }
#if UINTPTR_MAX > UINT64_MAX
  if ((Addr - Address) > (uintptr_t)UINT64_MAX) {
    return 0;
  }
#endif
  return 1;
}

/*********************************************************************
*
*       _IsValidSharedBuffer()
*
*  Function description
*    Checks whether a shared-memory payload buffer can be represented as
*    a 64-bit offset and has enough bytes to operate as a ring buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    pBuffer     Buffer pointer to validate.
*    BufferSize  Buffer size in bytes.
*
*  Return value
*    != 0  Buffer pointer and size are valid.
*    == 0  Buffer cannot be encoded or is too small.
*/
static int _IsValidSharedBuffer(uintptr_t Address, const void* pBuffer, unsigned BufferSize) {
  uintptr_t Addr;
#if UINTPTR_MAX > UINT64_MAX
  uintptr_t Off;
#endif

  Addr = (uintptr_t)pBuffer;
  if ((BufferSize < 2u) || (Addr <= Address)) {
    return 0;
  }
  if ((uintptr_t)BufferSize > (UINTPTR_MAX - Addr)) {
    return 0;
  }
#if UINTPTR_MAX > UINT64_MAX
  Off = Addr - Address;
  if (Off > (uintptr_t)UINT64_MAX) {
    return 0;
  }
  if ((uintptr_t)BufferSize > ((uintptr_t)UINT64_MAX - Off)) {
    return 0;
  }
#endif
  return 1;
}

/*********************************************************************
*
*       _IsOffsetRangeInRegion()
*
*  Function description
*    Checks whether an offset range is fully inside a mapped shared
*    memory region.
*
*  Parameters
*    Off       Offset from RTT control block base.
*    NumBytes  Number of bytes in the range.
*    Size      Mapped shared-memory region size.
*
*  Return value
*    != 0  Range is inside the region.
*    == 0  Range is outside the region.
*/
static int _IsOffsetRangeInRegion(uint64_t Off, uint32_t NumBytes, size_t Size) {
  size_t Offset;

  if (Off > SIZE_MAX) {
    return 0;
  }
  Offset = (size_t)Off;
  if (Offset >= Size) {
    return 0;
  }
  if ((size_t)NumBytes > (Size - Offset)) {
    return 0;
  }
  return 1;
}

/*********************************************************************
*
*       _CheckRingInRegion()
*
*  Function description
*    Validates one ring descriptor against a known shared-memory region.
*
*  Parameters
*    pRing  Address of the descriptor to validate.
*    Size   Mapped shared-memory region size.
*
*  Return value
*    >= 0  Descriptor is valid.
*     < 0  Descriptor is invalid.
*/
static int _CheckRingInRegion(uintptr_t pRing, size_t Size) {
  uint64_t NameOff;
  uint64_t BufferOff;
  uint32_t SizeOfBuffer;
  uint32_t RdOff;
  uint32_t WrOff;
  uint32_t Flags;

  NameOff      = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_NAME));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  Flags        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS));
  RTT__DMB();                         // Force descriptor reads before validating offsets
  if ((NameOff != 0u) && !_IsOffsetRangeInRegion(NameOff, 1u, Size)) {
    return -1;
  }
  if (!_IsValidFlags(Flags)) {
    return -1;
  }
  if (BufferOff == 0u) {
    if ((SizeOfBuffer != 0u) || (RdOff != 0u) || (WrOff != 0u)) {
      return -1;
    }
    return 0;
  }
  if ((SizeOfBuffer < 2u) || !_IsOffsetRangeInRegion(BufferOff, SizeOfBuffer, Size)) {
    return -1;
  }
  if ((RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       _CheckRingConfigured()
*
*  Function description
*    Checks whether one ring descriptor is configured.
*
*  Parameters
*    pRing  Address of the descriptor to check.
*
*  Return value
*    >= 0  Descriptor is configured.
*     < 0  Descriptor is empty or invalid.
*/
static int _CheckRingConfigured(uintptr_t pRing) {
  uint64_t BufferOff;
  uint32_t SizeOfBuffer;

  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RTT__DMB();                         // Force descriptor reads before checking configured state
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u)) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       _IsExtraBufferPairConfigured()
*
*  Function description
*    Checks whether an extra up/down buffer pair is configured.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    BufferIndex  Index of the buffer pair to check.
*
*  Return value
*     1  Buffer pair is configured.
*     0  Buffer pair is empty.
*    <0  Buffer pair is invalid.
*/
static int _IsExtraBufferPairConfigured(uintptr_t Address, unsigned BufferIndex) {
  unsigned  MaxNumUpBuffers;
  unsigned  MaxNumDownBuffers;
  uint64_t  UpBufferOff;
  uint64_t  DownBufferOff;
  uintptr_t pUp;
  uintptr_t pDown;

  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                         // Force buffer count reads before calculating descriptor addresses
  if ((BufferIndex >= MaxNumUpBuffers) || (BufferIndex >= MaxNumDownBuffers)) {
    return -1;
  }
  pUp   = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  pDown = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
  UpBufferOff   = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pUp,   SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  DownBufferOff = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  RTT__DMB();                         // Force descriptor reads before checking pair state
  if ((UpBufferOff == 0u) && (DownBufferOff == 0u)) {
    return 0;
  }
  if ((UpBufferOff == 0u) || (DownBufferOff == 0u)) {
    return -1;
  }
  return 1;
}

/*********************************************************************
*
*       _ConfigureExtraBufferPair()
*
*  Function description
*    Configures one extra up/down buffer pair in the mapped RTT region.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    Size         Size of the mapped shared-memory region.
*    BufferIndex  Index of the buffer pair to configure.
*
*  Return value
*      0  O.K.
*    < 0  Error.
*/
static int _ConfigureExtraBufferPair(uintptr_t Address, size_t Size, unsigned BufferIndex) {
  size_t    BufferOff;
  size_t    RequiredSize;
  size_t    i;
  uintptr_t BufferBase;
  char*     sName;
  void*     pUpBuffer;
  void*     pDownBuffer;

  if (BufferIndex == 0u) {
    return 0;
  }
  RequiredSize = _GetRequiredMemSize(BufferIndex + 1u);
  if (RequiredSize == 0u) {
    return -1;
  }
  BufferOff = RequiredSize - SEGGER_RTT__EXTRA_BUFFER_PAIR_SIZE;
  if ((BufferOff > Size) || (SEGGER_RTT__EXTRA_BUFFER_PAIR_SIZE > (Size - BufferOff))) {
    return -1;
  }
  BufferBase  = SEGGER_RTT__ADDR(Address, BufferOff);
  sName       = (char*)BufferBase;
  pUpBuffer   = (void*)SEGGER_RTT__ADDR(BufferBase, SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED);
  pDownBuffer = (void*)SEGGER_RTT__ADDR(BufferBase, SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED + BUFFER_SIZE_UP);
  for (i = 0u; i < SEGGER_RTT__EXTRA_BUFFER_PAIR_SIZE; i++) {
    SEGGER_RTT__WR8(SEGGER_RTT__ADDR(BufferBase, i), 0u);
  }
  for (i = 0u; i < sizeof(_aTerminalStr); i++) {
    SEGGER_RTT__WR8(SEGGER_RTT__ADDR((uintptr_t)sName, i), (unsigned char)_aTerminalStr[i]);
  }
  if (SEGGER_RTT_ConfigUpBuffer(Address, BufferIndex, sName, pUpBuffer, BUFFER_SIZE_UP, SEGGER_RTT_MODE_DEFAULT) < 0) {
    return -1;
  }
  if (SEGGER_RTT_ConfigDownBuffer(Address, BufferIndex, sName, pDownBuffer, BUFFER_SIZE_DOWN, SEGGER_RTT_MODE_DEFAULT) < 0) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       _EnsureExtraBufferPairs()
*
*  Function description
*    Ensures all requested extra up/down buffer pairs are configured.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    Size        Size of the mapped shared-memory region.
*    NumBuffers  Number of buffer pairs to configure.
*
*  Return value
*      0  O.K.
*    < 0  Error.
*/
static int _EnsureExtraBufferPairs(uintptr_t Address, size_t Size, unsigned NumBuffers) {
  unsigned BufferIndex;
  int      Configured;

  if ((NumBuffers == 0u) ||
      (NumBuffers > SEGGER_RTT_MAX_NUM_UP_BUFFERS) ||
      (NumBuffers > SEGGER_RTT_MAX_NUM_DOWN_BUFFERS)) {
    return -1;
  }
  for (BufferIndex = 1u; BufferIndex < NumBuffers; BufferIndex++) {
    Configured = _IsExtraBufferPairConfigured(Address, BufferIndex);
    if (Configured < 0) {
      return -1;
    }
    if (Configured == 0) {
      if (_ConfigureExtraBufferPair(Address, Size, BufferIndex) != 0) {
        return -1;
      }
    }
  }
  return SEGGER_RTT_CheckRegion(Address, Size);
}

/*********************************************************************
*
*       _DoInit()
*
*  Function description
*    Initializes the control block and buffers.
*
*  Parameters
*    Address     Base address of the RTT control block.
*
*  Notes
*    (1) May only be called via INIT() to avoid overriding settings.
*        The only exception is SEGGER_RTT_Init(), to make an intentional override possible.
*/
#define INIT(Address)                                                  \
  do {                                                                 \
    uintptr_t _SEGGER_RTT__Address;                                    \
    _SEGGER_RTT__Address = (uintptr_t)(Address);                       \
    if (_SEGGER_RTT__Address != 0u) {                                  \
      if (_IsRTTId(_SEGGER_RTT__Address) == 0) {                       \
        _DoInit(_SEGGER_RTT__Address);                                 \
      }                                                                \
    }                                                                  \
  } while (0)

static void _DoInit(uintptr_t Address) {
  unsigned i;
  static const char _aInitStr[] = "\0\0\0\0\0\0TTR REGGES";  // Init complete ID string to make sure that things also work if RTT is linked to a no-init memory area
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
  unsigned       NumBytes;
  volatile char* pDst;
  const char*    pSrc;
#endif

  if ((Address == 0u) || ((Address & SEGGER_RTT__CB_ALIGNMENT_MASK) != 0u)) {
    return;
  }
  //
  // Initialize control block
  //
  for (i = 0u; i < SEGGER_RTT__REQUIRED_MEM_SIZE; i++) {     // Make sure that the RTT CB and default buffers are always zero initialized.
    SEGGER_RTT__WR8(Address + i, 0u);
  }
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS),   SEGGER_RTT_MAX_NUM_UP_BUFFERS);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS), SEGGER_RTT_MAX_NUM_DOWN_BUFFERS);
  //
  // Initialize up buffer 0
  //
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
  pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__UP_NAME_OFF);
  pSrc = _aTerminalStr;
  NumBytes = sizeof(_aTerminalStr);
  while (NumBytes--) {
    *pDst++ = *pSrc++;
  };
#else
  SEGGER_RTT_MEMCPY((void*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__UP_NAME_OFF), _aTerminalStr, sizeof(_aTerminalStr));
#endif
  SEGGER_RTT__WR64(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_NAME),           SEGGER_RTT__UP_NAME_OFF);
  SEGGER_RTT__WR64(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_P_BUFFER),       SEGGER_RTT__UP_BUFFER_OFF);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), BUFFER_SIZE_UP);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_RD_OFF),         0u);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_WR_OFF),         0u);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_FLAGS),          SEGGER_RTT_MODE_DEFAULT);
  //
  // Initialize down buffer 0
  //
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
  pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__DOWN_NAME_OFF);
  pSrc = _aTerminalStr;
  NumBytes = sizeof(_aTerminalStr);
  while (NumBytes--) {
    *pDst++ = *pSrc++;
  };
#else
  SEGGER_RTT_MEMCPY((void*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__DOWN_NAME_OFF), _aTerminalStr, sizeof(_aTerminalStr));
#endif
  SEGGER_RTT__WR64(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_NAME),           SEGGER_RTT__DOWN_NAME_OFF);
  SEGGER_RTT__WR64(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_P_BUFFER),       SEGGER_RTT__DOWN_BUFFER_OFF);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), BUFFER_SIZE_DOWN);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_RD_OFF),         0u);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_WR_OFF),         0u);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_INDEX(0u) + SEGGER_RTT__BUFFER_OFF_FLAGS),          SEGGER_RTT_MODE_DEFAULT);
  //
  // Finish initialization of the control block.
  // Copy Id string backwards to make sure that "SEGGER RTT" is not found in initializer memory (usually flash),
  // as this would cause J-Link to "find" the control block at a wrong address.
  //
  RTT__DMB();                       // Force order of memory accesses for cores that may perform out-of-order memory accesses
  for (i = 0; i < sizeof(_aInitStr) - 1; ++i) {
    SEGGER_RTT__WR8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i), _aInitStr[sizeof(_aInitStr) - 2u - i]);  // Skip terminating \0 at the end of the array
  }
  RTT__DMB();                       // Force order of memory accesses for cores that may perform out-of-order memory accesses
}

/*********************************************************************
*
*       _WriteBlocking()
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT ring buffer
*    and updates the associated write pointer which is periodically
*    read by the host.
*    The caller is responsible for managing the write chunk sizes as
*    _WriteBlocking() will block until all data has been posted successfully.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    pRing        Ring buffer to post to.
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Return value
*    >= 0 - Number of bytes written into buffer.
*/
static unsigned _WriteBlocking(uintptr_t Address, uintptr_t pRing, const char* pBuffer, unsigned NumBytes) {
  unsigned NumBytesToWrite;
  unsigned NumBytesWritten;
  unsigned RdOff;
  unsigned WrOff;
  volatile char* pDst;
  //
  // Write data to buffer and handle wrap-around if necessary
  //
  NumBytesWritten = 0u;
  WrOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  do {
    RdOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));  // May be changed by host (debug probe) in the meantime
    if (RdOff > WrOff) {
      NumBytesToWrite = RdOff - WrOff - 1u;
    } else {
      NumBytesToWrite = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER)) - (WrOff - RdOff + 1u);
    }
    NumBytesToWrite = MIN(NumBytesToWrite, (SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER)) - WrOff));      // Number of bytes that can be written until buffer wrap-around
    NumBytesToWrite = MIN(NumBytesToWrite, NumBytes);
    pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)) + WrOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
    NumBytesWritten += NumBytesToWrite;
    NumBytes        -= NumBytesToWrite;
    WrOff           += NumBytesToWrite;
    while (NumBytesToWrite--) {
      *pDst++ = *pBuffer++;
    };
#else
    SEGGER_RTT_MEMCPY((void*)pDst, pBuffer, NumBytesToWrite);
    NumBytesWritten += NumBytesToWrite;
    pBuffer         += NumBytesToWrite;
    NumBytes        -= NumBytesToWrite;
    WrOff           += NumBytesToWrite;
#endif
    if (WrOff == SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER))) {
      WrOff = 0u;
    }
    RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff);
  } while (NumBytes);
  return NumBytesWritten;
}

/*********************************************************************
*
*       _WriteNoCheck()
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT ring buffer
*    and updates the associated write pointer which is periodically
*    read by the host.
*    It is callers responsibility to make sure data actually fits in buffer.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    pRing        Ring buffer to post to.
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Notes
*    (1) If there might not be enough space in the "Up"-buffer, call _WriteBlocking
*/
static void _WriteNoCheck(uintptr_t Address, uintptr_t pRing, const char* pData, unsigned NumBytes) {
  unsigned NumBytesAtOnce;
  unsigned WrOff;
  unsigned Rem;
  volatile char* pDst;

  WrOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  Rem = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER)) - WrOff;
  if (Rem > NumBytes) {
    //
    // All data fits before wrap around
    //
    pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)) + WrOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
    WrOff += NumBytes;
    while (NumBytes--) {
      *pDst++ = *pData++;
    };
    RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff);
#else
    SEGGER_RTT_MEMCPY((void*)pDst, pData, NumBytes);
    RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff + NumBytes);
#endif
  } else {
    //
    // We reach the end of the buffer, so need to wrap around
    //
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
    pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)) + WrOff);
    NumBytesAtOnce = Rem;
    while (NumBytesAtOnce--) {
      *pDst++ = *pData++;
    };
    pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)));
    NumBytesAtOnce = NumBytes - Rem;
    while (NumBytesAtOnce--) {
      *pDst++ = *pData++;
    };
    RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), NumBytes - Rem);
#else
    NumBytesAtOnce = Rem;
    pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)) + WrOff);
    SEGGER_RTT_MEMCPY((void*)pDst, pData, NumBytesAtOnce);
    NumBytesAtOnce = NumBytes - Rem;
    pDst = (volatile char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)));
    SEGGER_RTT_MEMCPY((void*)pDst, pData + Rem, NumBytesAtOnce);
    RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), NumBytesAtOnce);
#endif
  }
}

/*********************************************************************
*
*       _PostTerminalSwitch()
*
*  Function description
*    Switch terminal to the given terminal ID.  It is the caller's
*    responsibility to ensure the terminal ID is correct and there is
*    enough space in the buffer for this to complete successfully.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    pRing        Ring buffer to post to.
*    TerminalId   Terminal ID to switch to.
*/
static void _PostTerminalSwitch(uintptr_t Address, uintptr_t pRing, unsigned char TerminalId) {
  unsigned char ac[2];

  ac[0] = 0xFFu;
  ac[1] = _aTerminalId[TerminalId];  // Caller made already sure that TerminalId does not exceed our terminal limit
  _WriteBlocking(Address, pRing, (const char*)ac, 2u);
}

/*********************************************************************
*
*       _GetAvailWriteSpace()
*
*  Function description
*    Returns the number of bytes that can be written to the ring
*    buffer without blocking.
*
*  Parameters
*    pRing        Ring buffer to check.
*
*  Return value
*    Number of bytes that are free in the buffer.
*/
static unsigned _GetAvailWriteSpace(uintptr_t pRing) {
  unsigned RdOff;
  unsigned WrOff;
  unsigned SizeOfBuffer;
  unsigned r;
  //
  // Avoid warnings regarding volatile access order.  It's not a problem
  // in this case, but dampen compiler enthusiasm.
  //
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force offset reads to be complete before calculating free space, in case CPU is allowed to change the order of memory accesses
  if ((SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  if (RdOff <= WrOff) {
    r = SizeOfBuffer - 1u - WrOff + RdOff;
  } else {
    r = RdOff - WrOff - 1u;
  }
  return r;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_RTT_CheckInit()
*
*  Function description
*    Checks whether the RTT control block contains the SEGGER RTT ID.
*    This function never initializes or modifies the control block.
*
*  Parameters
*    Address     Base address of the RTT control block.
*
*  Return value
*      0 - RTT control block is initialized.
*    < 0 - RTT control block is not initialized.
*/
int SEGGER_RTT_CheckInit(uintptr_t Address) {
  return _IsRTTId(Address) ? 0 : -1;
}

/*********************************************************************
*
*       SEGGER_RTT_CheckRegion()
*
*  Function description
*    Checks whether an initialized RTT control block and its currently
*    configured descriptors are fully contained in a mapped shared
*    memory region.  This function never initializes or modifies the
*    control block.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    Size        Size of the mapped shared-memory region.
*
*  Return value
*      0 - RTT control block and descriptors are valid for the region.
*    < 0 - RTT control block or descriptors are invalid.
*/
int SEGGER_RTT_CheckRegion(uintptr_t Address, size_t Size) {
  unsigned MaxNumUpBuffers;
  unsigned MaxNumDownBuffers;
  unsigned i;
  uintptr_t DownOff;
  uintptr_t pRing;

  if ((Address == 0u) || ((Address & SEGGER_RTT__CB_ALIGNMENT_MASK) != 0u) || (Size < SEGGER_RTT__CB_OFF_A_UP)) {
    return -1;
  }
  if (Size > (size_t)(UINTPTR_MAX - Address)) {
    return -1;
  }
  if (_IsRTTId(Address) == 0) {
    return -1;
  }
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before checking descriptor ranges
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u)) {
    return -1;
  }
  if ((uintptr_t)MaxNumUpBuffers > ((uintptr_t)(Size - SEGGER_RTT__CB_OFF_A_UP) / SEGGER_RTT__BUFFER_SIZE)) {
    return -1;
  }
  DownOff = SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME(MaxNumUpBuffers);
  if (DownOff > (uintptr_t)Size) {
    return -1;
  }
  if ((uintptr_t)MaxNumDownBuffers > (((uintptr_t)Size - DownOff) / SEGGER_RTT__BUFFER_SIZE)) {
    return -1;
  }
  for (i = 0u; i < MaxNumUpBuffers; i++) {
    pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(i));
    if (_CheckRingInRegion(pRing, Size) < 0) {
      return -1;
    }
  }
  for (i = 0u; i < MaxNumDownBuffers; i++) {
    pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, i));
    if (_CheckRingInRegion(pRing, Size) < 0) {
      return -1;
    }
  }
  return 0;
}

/*********************************************************************
*
*       SEGGER_RTT_CheckUpBuffer()
*
*  Function description
*    Checks whether an initialized RTT control block contains a valid
*    and configured up-buffer descriptor in a mapped shared-memory
*    region. This function never initializes or modifies the control
*    block.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    Size         Size of the mapped shared-memory region.
*    BufferIndex  Index of Up-buffer to check.
*
*  Return value
*      0 - Up-buffer descriptor is valid and configured.
*    < 0 - RTT control block or up-buffer descriptor is invalid.
*/
int SEGGER_RTT_CheckUpBuffer(uintptr_t Address, size_t Size, unsigned BufferIndex) {
  unsigned  MaxNumUpBuffers;
  uintptr_t pRing;

  if (SEGGER_RTT_CheckRegion(Address, Size) != 0) {
    return -1;
  }
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force buffer count read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return -1;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  return _CheckRingConfigured(pRing);
}

/*********************************************************************
*
*       SEGGER_RTT_CheckDownBuffer()
*
*  Function description
*    Checks whether an initialized RTT control block contains a valid
*    and configured down-buffer descriptor in a mapped shared-memory
*    region. This function never initializes or modifies the control
*    block.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    Size         Size of the mapped shared-memory region.
*    BufferIndex  Index of Down-buffer to check.
*
*  Return value
*      0 - Down-buffer descriptor is valid and configured.
*    < 0 - RTT control block or down-buffer descriptor is invalid.
*/
int SEGGER_RTT_CheckDownBuffer(uintptr_t Address, size_t Size, unsigned BufferIndex) {
  unsigned  MaxNumUpBuffers;
  unsigned  MaxNumDownBuffers;
  uintptr_t pRing;

  if (SEGGER_RTT_CheckRegion(Address, Size) != 0) {
    return -1;
  }
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
    return -1;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
  return _CheckRingConfigured(pRing);
}

/*********************************************************************
*
*       SEGGER_RTT_GetRequiredMemSize()
*
*  Function description
*    Returns the shared-memory size required by SEGGER RTT for the
*    requested number of up/down buffer pairs. The returned size does not
*    include application-defined memory placed after the RTT buffers.
*
*  Parameters
*    NumBuffers  Number of up/down buffer pairs to support.
*
*  Return value
*    > 0  Required shared-memory size.
*    == 0 Invalid buffer count or size overflow.
*/
size_t SEGGER_RTT_GetRequiredMemSize(unsigned NumBuffers) {
  return _GetRequiredMemSize(NumBuffers);
}

/*********************************************************************
*
*       SEGGER_RTT_SPINLOCK_SW_Create()
*
*  Function description
*    Creates a software-implementation spinlock in shared memory.
*
*  Parameters
*    Address  Base address of the spinlock object.
*    Size     Size of the accessible spinlock memory range.
*
*  Return value
*      0  O.K.
*    < 0  Error.
*
*  Additional information
*    After creation, the spinlock is not locked.
*/
int SEGGER_RTT_SPINLOCK_SW_Create(uintptr_t Address, size_t Size) {
  unsigned i;

  if (_CheckSpinLockRange(Address, Size) != 0) {
    return -1;
  }

  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAGIC), 0u);
  RTT__DMB();
  for (i = 0u; i < SEGGER_RTT_SPINLOCK_MAX_CORES; i++) {
    SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_ENTERING_ADDR(Address, i), 0u);
    SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(  Address, i), 0u);
  }
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAX_CORES), SEGGER_RTT_SPINLOCK_MAX_CORES);
  RTT__DMB();
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAGIC), SEGGER_RTT_SPINLOCK_SW_MAGIC);
  RTT__DMB();
  return 0;
}

/*********************************************************************
*
*       SEGGER_RTT_SPINLOCK_SW_Check()
*
*  Function description
*    Checks whether a software-implementation spinlock exists in the
*    supplied shared-memory range.
*
*  Parameters
*    Address  Base address of the spinlock object.
*    Size     Size of the accessible spinlock memory range.
*
*  Return value
*      0  O.K.
*    < 0  Error.
*/
int SEGGER_RTT_SPINLOCK_SW_Check(uintptr_t Address, size_t Size) {
  if (_CheckSpinLockRange(Address, Size) != 0) {
    return -1;
  }
  if (SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAGIC)) != SEGGER_RTT_SPINLOCK_SW_MAGIC) {
    return -1;
  }
  RTT__DMB();
  if (SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAX_CORES)) != SEGGER_RTT_SPINLOCK_MAX_CORES) {
    return -1;
  }
  return 0;
}

/*********************************************************************
*
*       SEGGER_RTT_SPINLOCK_SW_Lock()
*
*  Function description
*    Acquires a software-implementation spinlock.
*
*  Parameters
*    Address  Base address of the spinlock object.
*    Id       Unique core identifier, 0 <= Id < SEGGER_RTT_SPINLOCK_MAX_CORES.
*
*  Return value
*      0  O.K.
*    < 0  Error.
*
*  Additional information
*    A core that has acquired a spinlock must not call this function for
*    the same spinlock again. The spinlock must first be released by
*    SEGGER_RTT_SPINLOCK_SW_Unlock().
*/
int SEGGER_RTT_SPINLOCK_SW_Lock(uintptr_t Address, unsigned Id) {
  return _SpinLockAcquire(Address, Id, NULL);
}

/*********************************************************************
*
*       SEGGER_RTT_SPINLOCK_SW_LockWithLimit()
*
*  Function description
*    Acquires a software-implementation spinlock with bounded waiting.
*
*  Parameters
*    Address       Base address of the spinlock object.
*    Id            Unique core identifier, 0 <= Id < SEGGER_RTT_SPINLOCK_MAX_CORES.
*    MaxWaitSpins  Maximum wait-loop iterations before returning an error.
*
*  Return value
*      0  O.K.
*    < 0  Error.
*
*  Additional information
*    MaxWaitSpins limits only waits for other participants. If no wait is
*    needed, the lock can be acquired even when MaxWaitSpins is 0.
*/
int SEGGER_RTT_SPINLOCK_SW_LockWithLimit(uintptr_t Address, unsigned Id, uint32_t MaxWaitSpins) {
  return _SpinLockAcquire(Address, Id, &MaxWaitSpins);
}

/*********************************************************************
*
*       SEGGER_RTT_SPINLOCK_SW_Unlock()
*
*  Function description
*    Releases a software-implementation spinlock.
*
*  Parameters
*    Address  Base address of the spinlock object.
*    Id       Unique core identifier, 0 <= Id < SEGGER_RTT_SPINLOCK_MAX_CORES.
*
*  Return value
*      0  O.K.
*    < 0  Error.
*/
int SEGGER_RTT_SPINLOCK_SW_Unlock(uintptr_t Address, unsigned Id) {
  if ((Address == 0u) || (Id >= SEGGER_RTT_SPINLOCK_MAX_CORES)) {
    return -1;
  }
  if (SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAGIC)) != SEGGER_RTT_SPINLOCK_SW_MAGIC) {
    return -1;
  }
  if (SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT_SPINLOCK_SW_OFF_MAX_CORES)) != SEGGER_RTT_SPINLOCK_MAX_CORES) {
    return -1;
  }
  RTT__DMB();
  SEGGER_RTT__WR32(SEGGER_RTT_SPINLOCK_SW_NUMBER_ADDR(Address, Id), 0u);
  RTT__DMB();
  return 0;
}

/*********************************************************************
*
*       SEGGER_RTT_FindControlBlock()
*
*  Function description
*    Searches for the RTT control block ID string in a mapped memory
*    region. This function never initializes or modifies the control
*    block. The memory region must be directly readable by this core.
*
*  Parameters
*    pAddress   Pointer to the mapped memory base address.
*               Updated to the RTT control block address on success.
*    Size       Size of the mapped memory region to search.
*
*  Return value
*      0 - RTT control block found, pAddress updated.
*    < 0 - RTT control block not found.
*/
int SEGGER_RTT_FindControlBlock(uintptr_t* pAddress, size_t Size) {
  uintptr_t Address;
  uintptr_t Candidate;
  size_t    Off;
  unsigned  i;

  if ((pAddress == NULL) || (*pAddress == 0u) || (Size < SEGGER_RTT__CB_OFF_A_UP)) {
    return -1;
  }
  Address = *pAddress;
  if (Size > (size_t)(UINTPTR_MAX - Address)) {
    return -1;
  }
  for (Off = 0u; Off <= (Size - sizeof(_aRTTId)); Off++) {
    Candidate = SEGGER_RTT__ADDR(Address, Off);
    if (((Candidate & SEGGER_RTT__CB_ALIGNMENT_MASK) != 0u) || (Off > (Size - SEGGER_RTT__CB_OFF_A_UP))) {
      continue;
    }
    if (SEGGER_RTT__RD8(Candidate) != (unsigned char)_aRTTId[0]) {
      continue;
    }
    for (i = 1u; i < sizeof(_aRTTId); i++) {
      if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Candidate, i)) != (unsigned char)_aRTTId[i]) {
        break;
      }
    }
    if (i == sizeof(_aRTTId)) {
      RTT__DMB();                   // Force ID read before returning the control block address
      *pAddress = Candidate;
      return 0;
    }
  }
  return -1;
}

/*********************************************************************
*
*       SEGGER_RTT_FindValidControlBlock()
*
*  Function description
*    Searches for the first initialized RTT control block whose
*    descriptor table and configured payload buffers are valid for the
*    remaining mapped shared-memory region. This function never
*    initializes or modifies the control block.
*
*  Parameters
*    pAddress     Pointer to the mapped memory base address.
*                 Updated to the RTT control block address on success.
*    Size         Size of the mapped memory region to search.
*    pRegionSize  Optional pointer updated to the remaining mapped
*                 region size from the returned control block address.
*
*  Return value
*      0 - Valid RTT control block found, pAddress updated.
*    < 0 - Valid RTT control block not found.
*/
int SEGGER_RTT_FindValidControlBlock(uintptr_t* pAddress, size_t Size, size_t* pRegionSize) {
  uintptr_t SearchBase;
  uintptr_t SearchEnd;
  uintptr_t RemainingBase;
  uintptr_t Candidate;
  uintptr_t NextBase;
  size_t    RemainingSize;
  size_t    CandidateRegionSize;

  if ((pAddress == NULL) || (*pAddress == 0u) || (Size == 0u)) {
    return -1;
  }
  SearchBase = *pAddress;
  if (Size > (size_t)(UINTPTR_MAX - SearchBase)) {
    return -1;
  }
  SearchEnd     = SearchBase + Size;
  RemainingBase = SearchBase;
  RemainingSize = Size;
  while (RemainingSize >= SEGGER_RTT__CB_OFF_A_UP) {
    Candidate = RemainingBase;
    if (SEGGER_RTT_FindControlBlock(&Candidate, RemainingSize) != 0) {
      return -1;
    }
    if ((Candidate < RemainingBase) || (Candidate >= SearchEnd)) {
      return -1;
    }
    CandidateRegionSize = (size_t)(SearchEnd - Candidate);
    if (SEGGER_RTT_CheckRegion(Candidate, CandidateRegionSize) == 0) {
      *pAddress = Candidate;
      if (pRegionSize != NULL) {
        *pRegionSize = CandidateRegionSize;
      }
      return 0;
    }
    if (Candidate > (UINTPTR_MAX - SEGGER_RTT__CB_ALIGNMENT)) {
      return -1;
    }
    NextBase = Candidate + SEGGER_RTT__CB_ALIGNMENT;
    if (NextBase >= SearchEnd) {
      return -1;
    }
    RemainingBase = NextBase;
    RemainingSize = (size_t)(SearchEnd - RemainingBase);
  }
  return -1;
}

/*********************************************************************
*
*       SEGGER_RTT_ReadUpBufferNoLock()
*
*  Function description
*    Reads characters from SEGGER real-time-terminal control block
*    which have been previously stored by the application.
*    Do not lock against interrupts and multiple access.
*    Used to do the same operation that J-Link does, to transfer
*    RTT data via other channels, such as TCP/IP or UART.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of Up-buffer to be used.
*    pBuffer      Pointer to buffer provided by target application, to copy characters from RTT-up-buffer to.
*    BufferSize   Size of the target application buffer.
*
*  Return value
*    Number of bytes that have been read.
*
*  Additional information
*    This function must not be called when J-Link might also do RTT.
*/
unsigned SEGGER_RTT_ReadUpBufferNoLock(uintptr_t Address, unsigned BufferIndex, void* pData, unsigned BufferSize) {
  unsigned                NumBytesRem;
  unsigned                NumBytesRead;
  unsigned                RdOff;
  unsigned                WrOff;
  unsigned                i;
  unsigned                MaxNumUpBuffers;
  uint64_t                BufferOff;
  unsigned                SizeOfBuffer;
  unsigned char*          pBuffer;
  uintptr_t               pRing;
  volatile char*          pSrc;

  if ((Address == 0u) || (pData == NULL) || (BufferSize == 0u)) {
    return 0u;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  pBuffer = (unsigned char*)pData;
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before using offsets or buffer memory
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  NumBytesRead = 0u;
  //
  // Read from current read position to wrap-around of buffer, first
  //
  if (RdOff > WrOff) {
    NumBytesRem = SizeOfBuffer - RdOff;
    NumBytesRem = MIN(NumBytesRem, BufferSize);
    pSrc = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + RdOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
    NumBytesRead += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
    while (NumBytesRem--) {
      *pBuffer++ = *pSrc++;
    };
#else
    SEGGER_RTT_MEMCPY(pBuffer, (void*)pSrc, NumBytesRem);
    NumBytesRead += NumBytesRem;
    pBuffer      += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
#endif
    //
    // Handle wrap-around of buffer
    //
    if (RdOff == SizeOfBuffer) {
      RdOff = 0u;
    }
  }
  //
  // Read remaining items of buffer
  //
  NumBytesRem = WrOff - RdOff;
  NumBytesRem = MIN(NumBytesRem, BufferSize);
  if (NumBytesRem > 0u) {
    pSrc = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + RdOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
    NumBytesRead += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
    while (NumBytesRem--) {
      *pBuffer++ = *pSrc++;
    };
#else
    SEGGER_RTT_MEMCPY(pBuffer, (void*)pSrc, NumBytesRem);
    NumBytesRead += NumBytesRem;
    pBuffer      += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
#endif
  }
  //
  // Update read offset of buffer
  //
  if (NumBytesRead) {
    RTT__DMB();                     // Force data read to be complete before writing the <RdOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF), RdOff);
  }
  //
  return NumBytesRead;
}

/*********************************************************************
*
*       SEGGER_RTT_ReadNoLock()
*
*  Function description
*    Reads characters from SEGGER real-time-terminal control block
*    which have been previously stored by the host.
*    Do not lock against interrupts and multiple access.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of Down-buffer to be used (e.g. 0 for "Terminal").
*    pBuffer      Pointer to buffer provided by target application, to copy characters from RTT-down-buffer to.
*    BufferSize   Size of the target application buffer.
*
*  Return value
*    Number of bytes that have been read.
*/
unsigned SEGGER_RTT_ReadNoLock(uintptr_t Address, unsigned BufferIndex, void* pData, unsigned BufferSize) {
  unsigned                NumBytesRem;
  unsigned                NumBytesRead;
  unsigned                RdOff;
  unsigned                WrOff;
  unsigned                i;
  unsigned                MaxNumUpBuffers;
  unsigned                MaxNumDownBuffers;
  uint64_t                BufferOff;
  unsigned                SizeOfBuffer;
  unsigned char*          pBuffer;
  uintptr_t               pRing;
  volatile char*          pSrc;
  //
  if ((Address == 0u) || (pData == NULL) || (BufferSize == 0u)) {
    return 0u;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
  pBuffer = (unsigned char*)pData;
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before using offsets or buffer memory
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  NumBytesRead = 0u;
  //
  // Read from current read position to wrap-around of buffer, first
  //
  if (RdOff > WrOff) {
    NumBytesRem = SizeOfBuffer - RdOff;
    NumBytesRem = MIN(NumBytesRem, BufferSize);
    pSrc = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + RdOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
    NumBytesRead += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
    while (NumBytesRem--) {
      *pBuffer++ = *pSrc++;
    };
#else
    SEGGER_RTT_MEMCPY(pBuffer, (void*)pSrc, NumBytesRem);
    NumBytesRead += NumBytesRem;
    pBuffer      += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
#endif
    //
    // Handle wrap-around of buffer
    //
    if (RdOff == SizeOfBuffer) {
      RdOff = 0u;
    }
  }
  //
  // Read remaining items of buffer
  //
  NumBytesRem = WrOff - RdOff;
  NumBytesRem = MIN(NumBytesRem, BufferSize);
  if (NumBytesRem > 0u) {
    pSrc = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + RdOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
    NumBytesRead += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
    while (NumBytesRem--) {
      *pBuffer++ = *pSrc++;
    };
#else
    SEGGER_RTT_MEMCPY(pBuffer, (void*)pSrc, NumBytesRem);
    NumBytesRead += NumBytesRem;
    pBuffer      += NumBytesRem;
    BufferSize   -= NumBytesRem;
    RdOff        += NumBytesRem;
#endif
  }
  if (NumBytesRead) {
    RTT__DMB();                     // Force data read to be complete before writing the <RdOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF), RdOff);
  }
  //
  return NumBytesRead;
}

/*********************************************************************
*
*       SEGGER_RTT_ReadUpBuffer
*
*  Function description
*    Reads characters from SEGGER real-time-terminal control block
*    which have been previously stored by the application.
*    Used to do the same operation that J-Link does, to transfer
*    RTT data via other channels, such as TCP/IP or UART.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of Up-buffer to be used.
*    pBuffer      Pointer to buffer provided by target application, to copy characters from RTT-up-buffer to.
*    BufferSize   Size of the target application buffer.
*
*  Return value
*    Number of bytes that have been read.
*
*  Additional information
*    This function must not be called when J-Link might also do RTT.
*    This function locks against all other RTT operations. I.e. during
*    the read operation, writing is also locked.
*    If only one consumer reads from the up buffer,
*    call sEGGER_RTT_ReadUpBufferNoLock() instead.
*/
unsigned SEGGER_RTT_ReadUpBuffer(uintptr_t Address, unsigned BufferIndex, void* pBuffer, unsigned BufferSize) {
  unsigned NumBytesRead;

  INIT(Address);
  SEGGER_RTT_LOCK();
  //
  // Call the non-locking read function
  //
  NumBytesRead = SEGGER_RTT_ReadUpBufferNoLock(Address, BufferIndex, pBuffer, BufferSize);
  //
  // Finish up.
  //
  SEGGER_RTT_UNLOCK();
  //
  return NumBytesRead;
}

/*********************************************************************
*
*       SEGGER_RTT_Read
*
*  Function description
*    Reads characters from SEGGER real-time-terminal control block
*    which have been previously stored by the host.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of Down-buffer to be used (e.g. 0 for "Terminal").
*    pBuffer      Pointer to buffer provided by target application, to copy characters from RTT-down-buffer to.
*    BufferSize   Size of the target application buffer.
*
*  Return value
*    Number of bytes that have been read.
*/
unsigned SEGGER_RTT_Read(uintptr_t Address, unsigned BufferIndex, void* pBuffer, unsigned BufferSize) {
  unsigned NumBytesRead;

  INIT(Address);
  SEGGER_RTT_LOCK();
  //
  // Call the non-locking read function
  //
  NumBytesRead = SEGGER_RTT_ReadNoLock(Address, BufferIndex, pBuffer, BufferSize);
  //
  // Finish up.
  //
  SEGGER_RTT_UNLOCK();
  //
  return NumBytesRead;
}

/*********************************************************************
*
*       SEGGER_RTT_WriteWithOverwriteNoLock
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT
*    control block.
*    SEGGER_RTT_WriteWithOverwriteNoLock does not lock the application
*    and overwrites data if the data does not fit into the buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Notes
*    (1) If there is not enough space in the "Up"-buffer, data is overwritten.
*    (2) For performance reasons this function does not call Init()
*        and may only be called after RTT has been initialized.
*        Either by calling SEGGER_RTT_Init() or calling another RTT API function first.
*    (3) Do not use SEGGER_RTT_WriteWithOverwriteNoLock if a J-Link
*        connection reads RTT data.
*    (4) The shared-memory implementation validates Address and basic
*        arguments before accessing shared memory.
*/
void SEGGER_RTT_WriteWithOverwriteNoLock(uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  const char*    pData;
  uintptr_t      pRing;
  unsigned       Avail;
  unsigned       RdOff;
  unsigned       WrOff;
  unsigned       i;
  unsigned       MaxNumUpBuffers;
  unsigned       SizeOfBuffer;
  uint64_t       BufferOff;
  volatile char* pDst;

  if ((Address == 0u) || (pBuffer == NULL) || (NumBytes == 0u)) {
    return;
  }
  //
  // Get "to-host" ring buffer and copy some elements into local variables.
  //
  pData = (const char *)pBuffer;
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before using offsets or buffer memory
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return;
  }
  //
  // Check if we will overwrite data and need to adjust the RdOff.
  //
  if (WrOff == RdOff) {
    Avail = SizeOfBuffer - 1u;
  } else if (WrOff < RdOff) {
    Avail = RdOff - WrOff - 1u;
  } else {
    Avail = RdOff - WrOff - 1u + SizeOfBuffer;
  }
  if (NumBytes > Avail) {
    RdOff += (NumBytes - Avail);
    while (RdOff >= SizeOfBuffer) {
      RdOff -= SizeOfBuffer;
    }
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF), RdOff);
  }
  //
  // Write all data, no need to check the RdOff, but possibly handle multiple wrap-arounds
  //
  Avail = SizeOfBuffer - WrOff;
  do {
    if (Avail > NumBytes) {
      //
      // Last round
      //
      pDst = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + WrOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
      Avail = NumBytes;
      while (NumBytes--) {
        *pDst++ = *pData++;
      };
      RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff + Avail);
#else
      SEGGER_RTT_MEMCPY((void*)pDst, pData, NumBytes);
      RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff + NumBytes);
#endif
      break;
    } else {
      //
      //  Wrap-around necessary, write until wrap-around and reset WrOff
      //
      pDst = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + WrOff);
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
      NumBytes -= Avail;
      while (Avail--) {
        *pDst++ = *pData++;
      };
      RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
      WrOff = 0u;
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff);
#else
      SEGGER_RTT_MEMCPY((void*)pDst, pData, Avail);
      pData += Avail;
      RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
      WrOff = 0u;
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff);
      NumBytes -= Avail;
#endif
      Avail = SizeOfBuffer - 1u;
    }
  } while (NumBytes);
}

/*********************************************************************
*
*       SEGGER_RTT_WriteSkipNoLock
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT
*    control block which is then read by the host.
*    SEGGER_RTT_WriteSkipNoLock does not lock the application and
*    skips all data, if the data does not fit into the buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Return value
*    1: Data has been copied
*    0: No space, data has not been copied
*
*  Notes
*    (1) If there is not enough space in the "Up"-buffer, all data is dropped.
*    (2) For performance reasons this function does not call Init()
*        and may only be called after RTT has been initialized.
*        Either by calling SEGGER_RTT_Init() or calling another RTT API function first.
*    (3) The shared-memory implementation validates Address and basic
*        arguments before accessing shared memory.
*/
#if (RTT_USE_ASM == 0)
unsigned SEGGER_RTT_WriteSkipNoLock(uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  const char*    pData;
  uintptr_t      pRing;
  unsigned       Avail;
  unsigned       RdOff;
  unsigned       WrOff;
  unsigned       Rem;
  unsigned       i;
  unsigned       MaxNumUpBuffers;
  uint64_t       BufferOff;
  unsigned       SizeOfBuffer;
  volatile char* pDst;

  if ((Address == 0u) || (pBuffer == NULL) || (NumBytes == 0u)) {
    return 0u;
  }
  //
  // Cases:
  //   1) RdOff <= WrOff => Space until wrap-around is sufficient
  //   2) RdOff <= WrOff => Space after wrap-around needed (copy in 2 chunks)
  //   3) RdOff <  WrOff => No space in buf
  //   4) RdOff >  WrOff => Space is sufficient
  //   5) RdOff >  WrOff => No space in buf
  //
  // 1) is the most common case for large buffers and assuming that J-Link reads the data fast enough
  //
  pData = (const char *)pBuffer;
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  RdOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RTT__DMB();                       // Force descriptor reads before using offsets or buffer memory
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  pDst = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + WrOff);
  if (RdOff <= WrOff) {                                 // Case 1), 2) or 3)
    Avail = SizeOfBuffer - WrOff - 1u;                  // Space until wrap-around (assume 1 byte not usable for case that RdOff == 0)
    if (Avail >= NumBytes) {                            // Case 1)?
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
      Rem = NumBytes;
      while (Rem--) {
        *pDst++ = *pData++;
      };
#else
      SEGGER_RTT_MEMCPY((void*)pDst, pData, NumBytes);
#endif
      RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff + NumBytes);
      return 1;
    }
    Avail += RdOff;                                     // Space incl. wrap-around
    if (Avail >= NumBytes) {                            // Case 2? => If not, we have case 3) (does not fit)
      Rem = SizeOfBuffer - WrOff;                       // Space until end of buffer
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
      NumBytes -= Rem;
      while (Rem--) {
        *pDst++ = *pData++;
      };
      if (NumBytes) {
        pDst = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff);
        Rem = NumBytes;
        while (Rem--) {
          *pDst++ = *pData++;
        };
      }
#else
      SEGGER_RTT_MEMCPY((void*)pDst, pData, Rem);        // Copy 1st chunk
      NumBytes -= Rem;
      //
      // Special case: First check that assumed RdOff == 0 calculated that last element before wrap-around could not be used
      // But 2nd check (considering space until wrap-around and until RdOff) revealed that RdOff is not 0, so we can use the last element
      // In this case, we may use a copy straight until buffer end anyway without needing to copy 2 chunks
      // Therefore, check if 2nd memcpy is necessary at all
      //
      if (NumBytes) {
        pDst = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff);
        SEGGER_RTT_MEMCPY((void*)pDst, pData + Rem, NumBytes);
      }
#endif
      RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), NumBytes);
      return 1;
    }
  } else {                                             // Potential case 4)
    Avail = RdOff - WrOff - 1u;
    if (Avail >= NumBytes) {                           // Case 4)? => If not, we have case 5) (does not fit)
#if SEGGER_RTT_MEMCPY_USE_BYTELOOP
      Rem = NumBytes;
      while (Rem--) {
        *pDst++ = *pData++;
      };
#else
      SEGGER_RTT_MEMCPY((void*)pDst, pData, NumBytes);
#endif
      RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff + NumBytes);
      return 1;
    }
  }
  return 0;     // No space in buffer
}
#endif

/*********************************************************************
*
*       SEGGER_RTT_WriteDownBufferNoLock
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT
*    control block inside a <Down> buffer.
*    SEGGER_RTT_WriteDownBufferNoLock does not lock the application.
*    Used to do the same operation that J-Link does, to transfer
*    RTT data from other channels, such as TCP/IP or UART.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Down"-buffer to be used.
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Return value
*    Number of bytes which have been stored in the "Down"-buffer.
*
*  Notes
*    (1) Data is stored according to buffer flags.
*    (2) For performance reasons this function does not call Init()
*        and may only be called after RTT has been initialized.
*        Either by calling SEGGER_RTT_Init() or calling another RTT API function first.
*    (3) The shared-memory implementation validates Address and basic
*        arguments before accessing shared memory.
*
*  Additional information
*    This function must not be called when J-Link might also do RTT.
*/
unsigned SEGGER_RTT_WriteDownBufferNoLock(uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  unsigned                Status;
  unsigned                Avail;
  unsigned                i;
  unsigned                MaxNumUpBuffers;
  unsigned                MaxNumDownBuffers;
  uint64_t                BufferOff;
  unsigned                SizeOfBuffer;
  unsigned                RdOff;
  unsigned                WrOff;
  const char*             pData;
  uintptr_t               pRing;
  //
  // Get "to-target" ring buffer.
  // It is save to cast that to a "to-host" buffer. Up and Down buffer differ in volatility of offsets that might be modified by J-Link.
  //
  if ((Address == 0u) || (pBuffer == NULL) || (NumBytes == 0u)) {
    return 0u;
  }
  pData = (const char *)pBuffer;
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before using offsets or buffer memory
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  //
  // How we output depends upon the mode...
  //
  switch (SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS)) & SEGGER_RTT_MODE_MASK) {
  case SEGGER_RTT_MODE_NO_BLOCK_SKIP:
    //
    // If we are in skip mode and there is no space for the whole
    // of this output, don't bother.
    //
    Avail = _GetAvailWriteSpace(pRing);
    if (Avail < NumBytes) {
      Status = 0u;
    } else {
      Status = NumBytes;
      _WriteNoCheck(Address, pRing, pData, NumBytes);
    }
    break;
  case SEGGER_RTT_MODE_NO_BLOCK_TRIM:
    //
    // If we are in trim mode, trim to what we can output without blocking.
    //
    Avail = _GetAvailWriteSpace(pRing);
    Status = Avail < NumBytes ? Avail : NumBytes;
    _WriteNoCheck(Address, pRing, pData, Status);
    break;
  case SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL:
    //
    // If we are in blocking mode, output everything.
    //
    Status = _WriteBlocking(Address, pRing, pData, NumBytes);
    break;
  default:
    Status = 0u;
    break;
  }
  //
  // Finish up.
  //
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_WriteNoLock
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT
*    control block which is then read by the host.
*    SEGGER_RTT_WriteNoLock does not lock the application.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Return value
*    Number of bytes which have been stored in the "Up"-buffer.
*
*  Notes
*    (1) Data is stored according to buffer flags.
*    (2) For performance reasons this function does not call Init()
*        and may only be called after RTT has been initialized.
*        Either by calling SEGGER_RTT_Init() or calling another RTT API function first.
*    (3) The shared-memory implementation validates Address and basic
*        arguments before accessing shared memory.
*/
unsigned SEGGER_RTT_WriteNoLock(uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  unsigned              Status;
  unsigned              Avail;
  unsigned              i;
  unsigned              MaxNumUpBuffers;
  uint64_t              BufferOff;
  unsigned              SizeOfBuffer;
  unsigned              RdOff;
  unsigned              WrOff;
  const char*           pData;
  uintptr_t             pRing;
  if ((Address == 0u) || (pBuffer == NULL) || (NumBytes == 0u)) {
    return 0u;
  }
  //
  // Get "to-host" ring buffer.
  //
  pData = (const char *)pBuffer;
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before using offsets or buffer memory
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  //
  // How we output depends upon the mode...
  //
  switch (SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS)) & SEGGER_RTT_MODE_MASK) {
  case SEGGER_RTT_MODE_NO_BLOCK_SKIP:
    //
    // If we are in skip mode and there is no space for the whole
    // of this output, don't bother.
    //
    Avail = _GetAvailWriteSpace(pRing);
    if (Avail < NumBytes) {
      Status = 0u;
    } else {
      Status = NumBytes;
      _WriteNoCheck(Address, pRing, pData, NumBytes);
    }
    break;
  case SEGGER_RTT_MODE_NO_BLOCK_TRIM:
    //
    // If we are in trim mode, trim to what we can output without blocking.
    //
    Avail = _GetAvailWriteSpace(pRing);
    Status = Avail < NumBytes ? Avail : NumBytes;
    _WriteNoCheck(Address, pRing, pData, Status);
    break;
  case SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL:
    //
    // If we are in blocking mode, output everything.
    //
    Status = _WriteBlocking(Address, pRing, pData, NumBytes);
    break;
  default:
    Status = 0u;
    break;
  }
  //
  // Finish up.
  //
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_WriteDownBuffer
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT control block in a <Down> buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Return value
*    Number of bytes which have been stored in the "Down"-buffer.
*
*  Notes
*    (1) Data is stored according to buffer flags.
*
*  Additional information
*    This function must not be called when J-Link might also do RTT.
*    This function locks against all other RTT operations. I.e. during
*    the write operation, writing from the application is also locked.
*    If only one consumer writes to the down buffer,
*    call SEGGER_RTT_WriteDownBufferNoLock() instead.
*/
unsigned SEGGER_RTT_WriteDownBuffer(uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  unsigned Status;

  INIT(Address);
  SEGGER_RTT_LOCK();
  Status = SEGGER_RTT_WriteDownBufferNoLock(Address, BufferIndex, pBuffer, NumBytes);  // Call the non-locking write function
  SEGGER_RTT_UNLOCK();
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_Write
*
*  Function description
*    Stores a specified number of characters in SEGGER RTT
*    control block which is then read by the host.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    pBuffer      Pointer to character array. Does not need to point to a \0 terminated string.
*    NumBytes     Number of bytes to be stored in the SEGGER RTT control block.
*
*  Return value
*    Number of bytes which have been stored in the "Up"-buffer.
*
*  Notes
*    (1) Data is stored according to buffer flags.
*/
unsigned SEGGER_RTT_Write(uintptr_t Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  unsigned Status;

  INIT(Address);
  SEGGER_RTT_LOCK();
  Status = SEGGER_RTT_WriteNoLock(Address, BufferIndex, pBuffer, NumBytes);  // Call the non-locking write function
  SEGGER_RTT_UNLOCK();
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_WriteString
*
*  Function description
*    Stores string in SEGGER RTT control block.
*    This data is read by the host.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    s            Pointer to string.
*
*  Return value
*    Number of bytes which have been stored in the "Up"-buffer.
*
*  Notes
*    (1) Data is stored according to buffer flags.
*    (2) String passed to this function has to be \0 terminated
*    (3) \0 termination character is *not* stored in RTT buffer
*/
unsigned SEGGER_RTT_WriteString(uintptr_t Address, unsigned BufferIndex, const char* s) {
  unsigned Len;

  if (s == NULL) {
    return 0u;
  }
  Len = STRLEN(s);
  return SEGGER_RTT_Write(Address, BufferIndex, s, Len);
}

/*********************************************************************
*
*       SEGGER_RTT_PutCharSkipNoLock
*
*  Function description
*    Stores a single character/byte in SEGGER RTT buffer.
*    SEGGER_RTT_PutCharSkipNoLock does not lock the application and
*    skips the byte, if it does not fit into the buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    c            Byte to be stored.
*
*  Return value
*    Number of bytes which have been stored in the "Up"-buffer.
*
*  Notes
*    (1) If there is not enough space in the "Up"-buffer, the character is dropped.
*    (2) For performance reasons this function does not call Init()
*        and may only be called after RTT has been initialized.
*        Either by calling SEGGER_RTT_Init() or calling another RTT API function first.
*    (3) The shared-memory implementation validates Address before
*        accessing shared memory.
*/

unsigned SEGGER_RTT_PutCharSkipNoLock(uintptr_t Address, unsigned BufferIndex, char c) {
  uintptr_t      pRing;
  unsigned       WrOff;
  unsigned       WrOffNext;
  unsigned       RdOff;
  unsigned       i;
  unsigned       MaxNumUpBuffers;
  uint64_t       BufferOff;
  unsigned       SizeOfBuffer;
  unsigned       Status;
  volatile char* pDst;

  if (Address == 0u) {
    return 0u;
  }
  //
  // Get "to-host" ring buffer.
  //
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before using offsets or buffer memory
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  //
  // Get write position and handle wrap-around if necessary
  //
  WrOffNext = WrOff + 1u;
  if (WrOffNext == SizeOfBuffer) {
    WrOffNext = 0u;
  }
  //
  // Output byte if free space is available
  //
  if (WrOffNext != RdOff) {
    pDst  = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + WrOff);
    *pDst = c;
    RTT__DMB();                     // Force data write to be complete before writing the <WrOff>, in case CPU is allowed to change the order of memory accesses
    SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOffNext);
    Status = 1;
  } else {
    Status = 0;
  }
  //
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_PutCharSkip
*
*  Function description
*    Stores a single character/byte in SEGGER RTT buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    c            Byte to be stored.
*
*  Return value
*    Number of bytes which have been stored in the "Up"-buffer.
*
*  Notes
*    (1) If there is not enough space in the "Up"-buffer, the character is dropped.
*/

unsigned SEGGER_RTT_PutCharSkip(uintptr_t Address, unsigned BufferIndex, char c) {
  uintptr_t      pRing;
  unsigned       WrOff;
  unsigned       WrOffNext;
  unsigned       RdOff;
  unsigned       MaxNumUpBuffers;
  uint64_t       BufferOff;
  unsigned       SizeOfBuffer;
  unsigned       Status;
  volatile char* pDst;

  if (Address == 0u) {
    return 0u;
  }
  INIT(Address);
  SEGGER_RTT_LOCK();
  //
  // Get "to-host" ring buffer from the runtime descriptor.
  //
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    Status = 0u;
  } else {
    pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
    BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
    SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
    RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
    WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
    RTT__DMB();                     // Force descriptor reads before using offsets or buffer memory
    if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
      Status = 0u;
    } else {
      //
      // Get write position and handle wrap-around if necessary.
      //
      WrOffNext = WrOff + 1u;
      if (WrOffNext == SizeOfBuffer) {
        WrOffNext = 0u;
      }
      //
      // Output byte if free space is available.
      //
      if (WrOffNext != RdOff) {
        pDst  = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + WrOff);
        *pDst = c;
        RTT__DMB();                 // Force data write to be complete before writing the <WrOff>
        SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOffNext);
        Status = 1u;
      } else {
        Status = 0u;
      }
    }
  }
  SEGGER_RTT_UNLOCK();
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_PutChar
*
*  Function description
*    Stores a single character/byte in SEGGER RTT buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of "Up"-buffer to be used (e.g. 0 for "Terminal").
*    c            Byte to be stored.
*
*  Return value
*    Number of bytes which have been stored in the "Up"-buffer.
*
*  Notes
*    (1) Data is stored according to buffer flags.
*/

unsigned SEGGER_RTT_PutChar(uintptr_t Address, unsigned BufferIndex, char c) {
  uintptr_t      pRing;
  unsigned       WrOff;
  unsigned       WrOffNext;
  unsigned       RdOff;
  unsigned       MaxNumUpBuffers;
  uint64_t       BufferOff;
  unsigned       SizeOfBuffer;
  unsigned       Flags;
  unsigned       Status;
  volatile char* pDst;

  if (Address == 0u) {
    return 0u;
  }
  INIT(Address);
  SEGGER_RTT_LOCK();
  //
  // Get "to-host" ring buffer from the runtime descriptor.
  //
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    Status = 0u;
  } else {
    pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
    BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
    SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
    RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
    WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
    Flags        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS));
    RTT__DMB();                     // Force descriptor reads before using offsets or buffer memory
    if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
      Status = 0u;
    } else {
      //
      // Get write position and handle wrap-around if necessary.
      //
      WrOffNext = WrOff + 1u;
      if (WrOffNext == SizeOfBuffer) {
        WrOffNext = 0u;
      }
      //
      // Wait for free space if mode is set to blocking.
      //
      if ((Flags & SEGGER_RTT_MODE_MASK) == SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL) {
        while (WrOffNext == SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF))) {
          ;
        }
      }
      //
      // Output byte if free space is available.
      //
      if (WrOffNext != SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF))) {
        pDst  = (volatile char*)SEGGER_RTT__ADDR(Address, BufferOff + WrOff);
        *pDst = c;
        RTT__DMB();                 // Force data write to be complete before writing the <WrOff>
        SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOffNext);
        Status = 1u;
      } else {
        Status = 0u;
      }
    }
  }
  SEGGER_RTT_UNLOCK();
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_GetKey
*
*  Function description
*    Reads one character from the SEGGER RTT buffer.
*    Host has previously stored data there.
*
*  Parameters
*    Address     Base address of the RTT control block.
*
*  Return value
*    <  0 -   No character available (buffer empty).
*    >= 0 -   Character which has been read. (Possible values: 0 - 255)
*
*  Notes
*    (1) This function is only specified for accesses to RTT buffer 0.
*/
int SEGGER_RTT_GetKey(uintptr_t Address) {
  char c;
  int r;

  r = (int)SEGGER_RTT_Read(Address, 0u, &c, 1u);
  if (r == 1) {
    r = (int)(unsigned char)c;
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_WaitKey
*
*  Function description
*    Waits until at least one character is avaible in the SEGGER RTT buffer.
*    Once a character is available, it is read and this function returns.
*
*  Parameters
*    Address     Base address of the RTT control block.
*
*  Return value
*    >=0 -   Character which has been read.
*
*  Notes
*    (1) This function is only specified for accesses to RTT buffer 0
*    (2) This function is blocking if no character is present in RTT buffer
*/
int SEGGER_RTT_WaitKey(uintptr_t Address) {
  int r;

  do {
    r = SEGGER_RTT_GetKey(Address);
  } while (r < 0);
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_HasKey
*
*  Function description
*    Checks if at least one character for reading is available in the SEGGER RTT buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*
*  Return value
*    == 0 -     No characters are available to read.
*    == 1 -     At least one character is available.
*
*  Notes
*    (1) This function is only specified for accesses to RTT buffer 0
*/
int SEGGER_RTT_HasKey(uintptr_t Address) {
  unsigned RdOff;
  unsigned WrOff;
  unsigned i;
  unsigned MaxNumUpBuffers;
  unsigned MaxNumDownBuffers;
  uint64_t BufferOff;
  unsigned SizeOfBuffer;
  uintptr_t pRing;

  if (Address == 0u) {
    return 0;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u)) {
    return 0;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, 0u));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before checking data availability
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0;
  }
  return (RdOff != WrOff) ? 1 : 0;
}

/*********************************************************************
*
*       SEGGER_RTT_HasData
*
*  Function description
*    Check if there is data from the host in the given buffer.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    BufferIndex  Index of Down-buffer to be used.
*
*  Return value:
*  ==0:  No data
*  !=0:  Data in buffer
*
*/
unsigned SEGGER_RTT_HasData(uintptr_t Address, unsigned BufferIndex) {
  unsigned RdOff;
  unsigned WrOff;
  unsigned i;
  unsigned MaxNumUpBuffers;
  unsigned MaxNumDownBuffers;
  uint64_t BufferOff;
  unsigned SizeOfBuffer;
  uintptr_t pRing;

  if (Address == 0u) {
    return 0u;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before calculating bytes in buffer
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  if (RdOff <= WrOff) {
    return WrOff - RdOff;
  }
  return SizeOfBuffer - RdOff + WrOff;
}

/*********************************************************************
*
*       SEGGER_RTT_HasDataUp
*
*  Function description
*    Check if there is data remaining to be sent in the given buffer.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    BufferIndex  Index of Up-buffer to be used.
*
*  Return value:
*  ==0:  No data
*  !=0:  Data in buffer
*
*/
unsigned SEGGER_RTT_HasDataUp(uintptr_t Address, unsigned BufferIndex) {
  unsigned RdOff;
  unsigned WrOff;
  unsigned i;
  unsigned MaxNumUpBuffers;
  uint64_t BufferOff;
  unsigned SizeOfBuffer;
  uintptr_t pRing;

  if (Address == 0u) {
    return 0u;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before calculating bytes in buffer
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  if (RdOff <= WrOff) {
    return WrOff - RdOff;
  }
  return SizeOfBuffer - RdOff + WrOff;
}

/*********************************************************************
*
*       SEGGER_RTT_AllocDownBuffer
*
*  Function description
*    Run-time configuration of the next down-buffer (H->T).
*    The next buffer, which is not used yet is configured.
*    This includes: Buffer address, size, name, flags, ...
*
*  Parameters
*    Address     Base address of the RTT control block.
*    sName        Pointer to a constant name string.
*    pBuffer      Pointer to a buffer to be used.
*    BufferSize   Size of the buffer.
*    Flags        Operating modes. Define behavior if buffer is full (not enough space for entire message).
*                 Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
*
*  Return value
*    >= 0 - O.K. Buffer Index
*     < 0 - Error
*/
int SEGGER_RTT_AllocDownBuffer(uintptr_t Address, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags) {
  int       BufferIndex;
  unsigned  MaxNumUpBuffers;
  unsigned  MaxNumDownBuffers;
  uintptr_t Addr;
  uintptr_t pRing;

  if ((Address == 0u) || !_IsValidFlags(Flags) || !_IsValidSharedName(Address, sName) || !_IsValidSharedBuffer(Address, pBuffer, BufferSize)) {
    return -1;
  }
  INIT(Address);
  SEGGER_RTT_LOCK();
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u)) {
    BufferIndex = -1;
  } else {
    BufferIndex = 0;
    do {
      pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, (unsigned)BufferIndex));
      if (SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)) == 0u) {
        break;
      }
      BufferIndex++;
    } while ((unsigned)BufferIndex < MaxNumDownBuffers);
    if ((unsigned)BufferIndex < MaxNumDownBuffers) {
      Addr = (uintptr_t)sName;
      SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_NAME),           Addr ? (uint64_t)(Addr - Address) : 0u);
      Addr = (uintptr_t)pBuffer;
      SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER),       Addr ? (uint64_t)(Addr - Address) : 0u);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), BufferSize);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF),         0u);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF),         0u);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS),          Flags);
      RTT__DMB();                     // Force configuration writes to be complete before returning, in case CPU is allowed to change the order of memory accesses
    } else {
      BufferIndex = -1;
    }
  }
  SEGGER_RTT_UNLOCK();
  return BufferIndex;
}

/*********************************************************************
*
*       SEGGER_RTT_AllocUpBuffer
*
*  Function description
*    Run-time configuration of the next up-buffer (T->H).
*    The next buffer, which is not used yet is configured.
*    This includes: Buffer address, size, name, flags, ...
*
*  Parameters
*    Address     Base address of the RTT control block.
*    sName        Pointer to a constant name string.
*    pBuffer      Pointer to a buffer to be used.
*    BufferSize   Size of the buffer.
*    Flags        Operating modes. Define behavior if buffer is full (not enough space for entire message).
*                 Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
*
*  Return value
*    >= 0 - O.K. Buffer Index
*     < 0 - Error
*/
int SEGGER_RTT_AllocUpBuffer(uintptr_t Address, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags) {
  int       BufferIndex;
  unsigned  MaxNumUpBuffers;
  uintptr_t Addr;
  uintptr_t pRing;

  if ((Address == 0u) || !_IsValidFlags(Flags) || !_IsValidSharedName(Address, sName) || !_IsValidSharedBuffer(Address, pBuffer, BufferSize)) {
    return -1;
  }
  INIT(Address);
  SEGGER_RTT_LOCK();
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if (MaxNumUpBuffers == 0u) {
    BufferIndex = -1;
  } else {
    BufferIndex = 0;
    do {
      pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX((unsigned)BufferIndex));
      if (SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)) == 0u) {
        break;
      }
      BufferIndex++;
    } while ((unsigned)BufferIndex < MaxNumUpBuffers);
    if ((unsigned)BufferIndex < MaxNumUpBuffers) {
      Addr = (uintptr_t)sName;
      SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_NAME),           Addr ? (uint64_t)(Addr - Address) : 0u);
      Addr = (uintptr_t)pBuffer;
      SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER),       Addr ? (uint64_t)(Addr - Address) : 0u);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), BufferSize);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF),         0u);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF),         0u);
      SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS),          Flags);
      RTT__DMB();                     // Force configuration writes to be complete before returning, in case CPU is allowed to change the order of memory accesses
    } else {
      BufferIndex = -1;
    }
  }
  SEGGER_RTT_UNLOCK();
  return BufferIndex;
}

/*********************************************************************
*
*       SEGGER_RTT_ConfigUpBuffer
*
*  Function description
*    Run-time configuration of a specific up-buffer (T->H).
*    Buffer to be configured is specified by index.
*    This includes: Buffer address, size, name, flags, ...
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the buffer to configure.
*    sName        Pointer to a constant name string.
*    pBuffer      Pointer to a buffer to be used.
*    BufferSize   Size of the buffer.
*    Flags        Operating modes. Define behavior if buffer is full (not enough space for entire message).
*                 Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
*
*  Return value
*    >= 0 - O.K.
*     < 0 - Error
*
*  Additional information
*    Buffer 0 is configured on compile-time.
*    May only be called once per buffer.
*    Buffer name and flags can be reconfigured using the appropriate functions.
*/
int SEGGER_RTT_ConfigUpBuffer(uintptr_t Address, unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags) {
  int r;
  unsigned MaxNumUpBuffers;
  uintptr_t Addr;
  uintptr_t pUp;

  if (Address != 0u) {
    r = _IsValidFlags(Flags) ? 0 : -1;
    if (BufferIndex) {
      if (!_IsValidSharedName(Address, sName)) {
        r = -1;
      }
      if ((r == 0) && !_IsValidSharedBuffer(Address, pBuffer, BufferSize)) {
        r = -1;
      }
    }
    if (r == 0) {
      INIT(Address);
      SEGGER_RTT_LOCK();
      MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
      RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
      if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
        r = -1;
      } else {
        pUp = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
        if (BufferIndex) {
          Addr = (uintptr_t)sName;
          SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_NAME),           Addr ? (uint64_t)(Addr - Address) : 0u);
          Addr = (uintptr_t)pBuffer;
          SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_P_BUFFER),       Addr ? (uint64_t)(Addr - Address) : 0u);
          SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), BufferSize);
          SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_RD_OFF),         0u);
          SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_WR_OFF),         0u);
        }
        SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_FLAGS), Flags);
        RTT__DMB();                     // Force configuration writes to be complete before returning, in case CPU is allowed to change the order of memory accesses
      }
      SEGGER_RTT_UNLOCK();
    }
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_ConfigDownBuffer
*
*  Function description
*    Run-time configuration of a specific down-buffer (H->T).
*    Buffer to be configured is specified by index.
*    This includes: Buffer address, size, name, flags, ...
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the buffer to configure.
*    sName        Pointer to a constant name string.
*    pBuffer      Pointer to a buffer to be used.
*    BufferSize   Size of the buffer.
*    Flags        Operating modes. Define behavior if buffer is full (not enough space for entire message).
*                 Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
*
*  Return value
*    >= 0  O.K.
*     < 0  Error
*
*  Additional information
*    Buffer 0 is configured on compile-time.
*    May only be called once per buffer.
*    Buffer name and flags can be reconfigured using the appropriate functions.
*/
int SEGGER_RTT_ConfigDownBuffer(uintptr_t Address, unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags) {
  int r;
  unsigned MaxNumUpBuffers;
  unsigned MaxNumDownBuffers;
  uintptr_t Addr;
  uintptr_t pDown;

  if (Address != 0u) {
    r = _IsValidFlags(Flags) ? 0 : -1;
    if (BufferIndex) {
      if (!_IsValidSharedName(Address, sName)) {
        r = -1;
      }
      if ((r == 0) && !_IsValidSharedBuffer(Address, pBuffer, BufferSize)) {
        r = -1;
      }
    }
    if (r == 0) {
      INIT(Address);
      SEGGER_RTT_LOCK();
      MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
      MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
      RTT__DMB();                       // Force buffer count reads before calculating the ring address
      if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
        r = -1;
      } else {
        pDown = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
        if (BufferIndex) {
          Addr = (uintptr_t)sName;
          SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_NAME),           Addr ? (uint64_t)(Addr - Address) : 0u);
          Addr = (uintptr_t)pBuffer;
          SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_P_BUFFER),       Addr ? (uint64_t)(Addr - Address) : 0u);
          SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), BufferSize);
          SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_RD_OFF),         0u);
          SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_WR_OFF),         0u);
        }
        SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_FLAGS), Flags);
        RTT__DMB();                     // Force configuration writes to be complete before returning, in case CPU is allowed to change the order of memory accesses
      }
      SEGGER_RTT_UNLOCK();
    }
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_SetNameUpBuffer
*
*  Function description
*    Run-time configuration of a specific up-buffer name (T->H).
*    Buffer to be configured is specified by index.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the buffer to renamed.
*    sName        Pointer to a constant name string.
*
*  Return value
*    >= 0  O.K.
*     < 0  Error
*/
int SEGGER_RTT_SetNameUpBuffer(uintptr_t Address, unsigned BufferIndex, const char* sName) {
  uintptr_t Addr;
  unsigned MaxNumUpBuffers;
  int r;
  uintptr_t pUp;

  if (Address != 0u) {
    Addr = (uintptr_t)sName;
    if (!_IsValidSharedName(Address, sName)) {
      r = -1;
    } else {
      INIT(Address);
      SEGGER_RTT_LOCK();
      MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
      RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
      if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
        r = -1;
      } else {
        pUp = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
        SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_NAME), Addr ? (uint64_t)(Addr - Address) : 0u);
        r =  0;
      }
      SEGGER_RTT_UNLOCK();
    }
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_SetNameDownBuffer
*
*  Function description
*    Run-time configuration of a specific Down-buffer name (T->H).
*    Buffer to be configured is specified by index.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the buffer to renamed.
*    sName        Pointer to a constant name string.
*
*  Return value
*    >= 0  O.K.
*     < 0  Error
*/
int SEGGER_RTT_SetNameDownBuffer(uintptr_t Address, unsigned BufferIndex, const char* sName) {
  uintptr_t Addr;
  unsigned MaxNumUpBuffers;
  unsigned MaxNumDownBuffers;
  int r;
  uintptr_t pDown;

  if (Address != 0u) {
    Addr = (uintptr_t)sName;
    if (!_IsValidSharedName(Address, sName)) {
      r = -1;
    } else {
      INIT(Address);
      SEGGER_RTT_LOCK();
      MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
      MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
      RTT__DMB();                       // Force buffer count reads before calculating the ring address
      if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
        r = -1;
      } else {
        pDown = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
        SEGGER_RTT__WR64(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_NAME), Addr ? (uint64_t)(Addr - Address) : 0u);
        r =  0;
      }
      SEGGER_RTT_UNLOCK();
    }
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_SetFlagsUpBuffer
*
*  Function description
*    Run-time configuration of specific up-buffer flags (T->H).
*    Buffer to be configured is specified by index.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the buffer.
*    Flags        Flags to set for the buffer.
*                 Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
*
*  Return value
*    >= 0  O.K.
*     < 0  Error
*/
int SEGGER_RTT_SetFlagsUpBuffer(uintptr_t Address, unsigned BufferIndex, unsigned Flags) {
  int r;
  unsigned MaxNumUpBuffers;
  uintptr_t pUp;

  if (Address != 0u) {
    if (!_IsValidFlags(Flags)) {
      r = -1;
    } else {
      INIT(Address);
      SEGGER_RTT_LOCK();
      MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
      RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
      if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
        r = -1;
      } else {
        pUp = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
        SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pUp, SEGGER_RTT__BUFFER_OFF_FLAGS), Flags);
        r =  0;
      }
      SEGGER_RTT_UNLOCK();
    }
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_SetFlagsDownBuffer
*
*  Function description
*    Run-time configuration of specific Down-buffer flags (T->H).
*    Buffer to be configured is specified by index.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the buffer to renamed.
*    Flags        Flags to set for the buffer.
*                 Flags[31:24] are used for validity check and must be zero. Flags[23:2] are reserved for future use. Flags[1:0] = RTT operating mode.
*
*  Return value
*    >= 0  O.K.
*     < 0  Error
*/
int SEGGER_RTT_SetFlagsDownBuffer(uintptr_t Address, unsigned BufferIndex, unsigned Flags) {
  int r;
  unsigned MaxNumUpBuffers;
  unsigned MaxNumDownBuffers;
  uintptr_t pDown;

  if (Address != 0u) {
    if (!_IsValidFlags(Flags)) {
      r = -1;
    } else {
      INIT(Address);
      SEGGER_RTT_LOCK();
      MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
      MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
      RTT__DMB();                       // Force buffer count reads before calculating the ring address
      if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
        r = -1;
      } else {
        pDown = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
        SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pDown, SEGGER_RTT__BUFFER_OFF_FLAGS), Flags);
        r =  0;
      }
      SEGGER_RTT_UNLOCK();
    }
  } else {
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_Init
*
*  Function description
*    Initializes the RTT Control Block.
*    Should be used in RAM targets, at start of the application.
*
*  Parameters
*    Address     Base address of the RTT control block.
*/
void SEGGER_RTT_Init (uintptr_t Address) {
  _DoInit(Address);
}

/*********************************************************************
*
*       SEGGER_RTT_InitEx
*
*  Function description
*    Initializes the RTT Control Block after verifying that the mapped
*    shared-memory region is large enough for the default control block
*    and buffers.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    Size        Size of the mapped shared-memory region.
*
*  Return value
*      0 - O.K.
*    < 0 - Error
*/
int SEGGER_RTT_InitEx (uintptr_t Address, size_t Size) {
  if ((Address == 0u) || ((Address & SEGGER_RTT__CB_ALIGNMENT_MASK) != 0u) || (Size < SEGGER_RTT__REQUIRED_MEM_SIZE)) {
    return -1;
  }
  if (Size > (size_t)(UINTPTR_MAX - Address)) {
    return -1;
  }
  _DoInit(Address);
  return SEGGER_RTT_CheckRegion(Address, Size);
}

/*********************************************************************
*
*       SEGGER_RTT_EnsureInitEx
*
*  Function description
*    Initializes the RTT Control Block if it is not initialized,
*    validates it if it already exists, and configures the requested
*    up/down buffer pairs in the mapped shared-memory region.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    Size        Size of the mapped shared-memory region.
*    NumBuffers  Number of up/down buffer pairs to configure.
*
*  Return value
*      0 - O.K.
*    < 0 - Error
*/
int SEGGER_RTT_EnsureInitEx (uintptr_t Address, size_t Size, unsigned NumBuffers) {
  if ((Address == 0u) || (Size == 0u)) {
    return -1;
  }
  if ((NumBuffers == 0u) ||
      (NumBuffers > SEGGER_RTT_MAX_NUM_UP_BUFFERS) ||
      (NumBuffers > SEGGER_RTT_MAX_NUM_DOWN_BUFFERS)) {
    return -1;
  }
  if (Size > (size_t)(UINTPTR_MAX - Address)) {
    return -1;
  }
  if (SEGGER_RTT_CheckInit(Address) != 0) {
    if (SEGGER_RTT_InitEx(Address, Size) != 0) {
      return -1;
    }
  } else if (SEGGER_RTT_CheckRegion(Address, Size) != 0) {
    return -1;
  }
  return _EnsureExtraBufferPairs(Address, Size, NumBuffers);
}

/*********************************************************************
*
*       SEGGER_RTT_SetTerminal
*
*  Function description
*    Sets the terminal to be used for output on channel 0.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    TerminalId  Index of the terminal.
*
*  Return value
*    >= 0  O.K.
*     < 0  Error (e.g. if RTT is configured for non-blocking mode and there was no space in the buffer to set the new terminal Id)
*
*  Notes
*    (1) Buffer 0 is always reserved for terminal I/O, so we can use index 0 here, fixed
*/
int SEGGER_RTT_SetTerminal (uintptr_t Address, unsigned char TerminalId) {
  unsigned char         ac[2];
  uintptr_t             pRing;
  unsigned Avail;
  int r;

  if ((Address == 0u) || (TerminalId >= sizeof(_aTerminalId))) {
    return -1;
  }
  INIT(Address);
  r = 0;
  ac[0] = 0xFFu;
  ac[1] = _aTerminalId[TerminalId];
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u));
  SEGGER_RTT_LOCK();                     // Lock to make sure that no other task is writing into buffer, while we are and number of free bytes in buffer does not change downwards after checking and before writing
  if ((SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS)) & SEGGER_RTT_MODE_MASK) == SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL) {
    SEGGER_RTT__WR8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__ACTIVE_TERMINAL_OFF), TerminalId);
    _WriteBlocking(Address, pRing, (const char*)ac, 2u);
  } else {                                                                            // Skipping mode or trim mode? => We cannot trim this command so handling is the same for both modes
    Avail = _GetAvailWriteSpace(pRing);
    if (Avail >= 2) {
      SEGGER_RTT__WR8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__ACTIVE_TERMINAL_OFF), TerminalId);    // Only change active terminal in case of success
      _WriteNoCheck(Address, pRing, (const char*)ac, 2u);
    } else {
      r = -1;
    }
  }
  SEGGER_RTT_UNLOCK();
  return r;
}

/*********************************************************************
*
*       SEGGER_RTT_TerminalOut
*
*  Function description
*    Writes a string to the given terminal
*     without changing the terminal for channel 0.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    TerminalId   Index of the terminal.
*    s            String to be printed on the terminal.
*
*  Return value
*    >= 0 - Number of bytes written.
*     < 0 - Error.
*
*/
int SEGGER_RTT_TerminalOut (uintptr_t Address, unsigned char TerminalId, const char* s) {
  int                   Status;
  unsigned              FragLen;
  unsigned              Avail;
  uintptr_t             pRing;
  unsigned char         ActiveTerminal;
  //
  if ((Address == 0u) || (s == NULL) || (TerminalId >= sizeof(_aTerminalId))) {
    return -1;
  }
  INIT(Address);
  //
  // Get "to-host" ring buffer.
  //
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(0u));
  ActiveTerminal = SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__ACTIVE_TERMINAL_OFF));
  //
  // Need to be able to change terminal, write data, change back.
  // Compute the fixed and variable sizes.
  //
  FragLen = STRLEN(s);
  //
  // How we output depends upon the mode...
  //
  SEGGER_RTT_LOCK();
  Avail = _GetAvailWriteSpace(pRing);
  switch (SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS)) & SEGGER_RTT_MODE_MASK) {
  case SEGGER_RTT_MODE_NO_BLOCK_SKIP:
    //
    // If we are in skip mode and there is no space for the whole
    // of this output, don't bother switching terminals at all.
    //
    if (Avail < (FragLen + 4u)) {
      Status = 0;
    } else {
      _PostTerminalSwitch(Address, pRing, TerminalId);
      Status = (int)_WriteBlocking(Address, pRing, s, FragLen);
      _PostTerminalSwitch(Address, pRing, ActiveTerminal);
    }
    break;
  case SEGGER_RTT_MODE_NO_BLOCK_TRIM:
    //
    // If we are in trim mode and there is not enough space for everything,
    // trim the output but always include the terminal switch.  If no room
    // for terminal switch, skip that totally.
    //
    if (Avail < 4u) {
      Status = -1;
    } else {
      _PostTerminalSwitch(Address, pRing, TerminalId);
      Status = (int)_WriteBlocking(Address, pRing, s, (FragLen < (Avail - 4u)) ? FragLen : (Avail - 4u));
      _PostTerminalSwitch(Address, pRing, ActiveTerminal);
    }
    break;
  case SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL:
    //
    // If we are in blocking mode, output everything.
    //
    _PostTerminalSwitch(Address, pRing, TerminalId);
    Status = (int)_WriteBlocking(Address, pRing, s, FragLen);
    _PostTerminalSwitch(Address, pRing, ActiveTerminal);
    break;
  default:
    Status = -1;
    break;
  }
  //
  // Finish up.
  //
  SEGGER_RTT_UNLOCK();
  return Status;
}

/*********************************************************************
*
*       SEGGER_RTT_GetAvailWriteSpace
*
*  Function description
*    Returns the number of bytes available in the ring buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the up buffer.
*
*  Return value
*    Number of bytes that are free in the selected up buffer.
*/
unsigned SEGGER_RTT_GetAvailWriteSpace (uintptr_t Address, unsigned BufferIndex) {
  unsigned RdOff;
  unsigned WrOff;
  unsigned i;
  unsigned MaxNumUpBuffers;
  uint64_t BufferOff;
  unsigned SizeOfBuffer;
  uintptr_t pRing;

  if (Address == 0u) {
    return 0u;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before calculating free space
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  if (RdOff <= WrOff) {
    return SizeOfBuffer - 1u - WrOff + RdOff;
  }
  return RdOff - WrOff - 1u;
}


/*********************************************************************
*
*       SEGGER_RTT_GetBytesInBuffer()
*
*  Function description
*    Returns the number of bytes currently used in the up buffer.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the up buffer.
*
*  Return value
*    Number of bytes that are used in the buffer.
*/
unsigned SEGGER_RTT_GetBytesInBuffer(uintptr_t Address, unsigned BufferIndex) {
  unsigned RdOff;
  unsigned WrOff;
  unsigned i;
  unsigned MaxNumUpBuffers;
  uint64_t BufferOff;
  unsigned SizeOfBuffer;
  uintptr_t pRing;

  if (Address == 0u) {
    return 0u;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  RTT__DMB();                       // Force MaxNumUpBuffers read before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (BufferIndex >= MaxNumUpBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before calculating bytes in buffer
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  if (RdOff <= WrOff) {
    return WrOff - RdOff;
  }
  return SizeOfBuffer - RdOff + WrOff;
}

/*********************************************************************
*
*       SEGGER_RTT_GetBytesDownInBuffer()
*
*  Function description
*    Returns the number of bytes currently used in the down buffer.
*    This function never initializes or modifies the control block.
*
*  Parameters
*    Address     Base address of the RTT control block.
*    BufferIndex  Index of the down buffer.
*
*  Return value
*    Number of bytes that are used in the buffer.
*/
unsigned SEGGER_RTT_GetBytesDownInBuffer(uintptr_t Address, unsigned BufferIndex) {
  unsigned RdOff;
  unsigned WrOff;
  unsigned i;
  unsigned MaxNumUpBuffers;
  unsigned MaxNumDownBuffers;
  uint64_t BufferOff;
  unsigned SizeOfBuffer;
  uintptr_t pRing;

  if (Address == 0u) {
    return 0u;
  }
  for (i = 0u; i < sizeof(_aRTTId); i++) {
    if (SEGGER_RTT__RD8(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_AC_ID + i)) != (unsigned char)_aRTTId[i]) {
      return 0u;
    }
  }
  RTT__DMB();                       // Force ID read before reading the rest of the control block
  MaxNumUpBuffers   = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  MaxNumDownBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS));
  RTT__DMB();                       // Force buffer count reads before calculating the ring address
  if ((MaxNumUpBuffers == 0u) || (MaxNumDownBuffers == 0u) || (BufferIndex >= MaxNumDownBuffers)) {
    return 0u;
  }
  pRing = SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_DOWN_RUNTIME_INDEX(MaxNumUpBuffers, BufferIndex));
  BufferOff    = SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER));
  SizeOfBuffer = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER));
  RdOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF));
  WrOff        = SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF));
  RTT__DMB();                       // Force descriptor reads before calculating bytes in buffer
  if ((BufferOff == 0u) || (SizeOfBuffer < 2u) || (RdOff >= SizeOfBuffer) || (WrOff >= SizeOfBuffer)) {
    return 0u;
  }
  if (RdOff <= WrOff) {
    return WrOff - RdOff;
  }
  return SizeOfBuffer - RdOff + WrOff;
}

/*************************** End of file ****************************/
