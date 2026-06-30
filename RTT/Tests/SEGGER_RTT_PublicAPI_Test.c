/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*        SEGGER RTT * Real Time Transfer for embedded targets        *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------
Purpose : Unit tests for the shared-memory RTT public API.

          These tests use a local byte array as the shared memory
          mapping.  Descriptor fields and payload buffers are accessed
          through the public RTT API exactly as an application would do.

----------------------------------------------------------------------
*/

#include "SEGGER_RTT.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define TEST_MEM_SIZE                         (2048u)
#define TEST_EXTRA_OFF                        (SEGGER_RTT__REQUIRED_MEM_SIZE + 32u)
#define TEST_EXTRA_STR0_OFF                   (TEST_EXTRA_OFF + 0u)
#define TEST_EXTRA_STR1_OFF                   (TEST_EXTRA_OFF + 32u)
#define TEST_EXTRA_UP_BUF_OFF                 (TEST_EXTRA_OFF + 96u)
#define TEST_EXTRA_DOWN_BUF_OFF               (TEST_EXTRA_OFF + 160u)
#define TEST_EXTRA_PRINTF_BUF_OFF             (TEST_EXTRA_OFF + 256u)
#define TEST_EXTRA_PRINTF_BUF_SIZE            (384u)
#define TEST_ENSURE_EXTRA_PAIR_SIZE           (SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED + BUFFER_SIZE_UP + BUFFER_SIZE_DOWN)
#define TEST_RUNTIME_DOWN_OFF(NumUp)          (SEGGER_RTT__CB_OFF_A_UP + ((PTR_ADDR)(NumUp) * SEGGER_RTT__BUFFER_SIZE))
#define TEST_RUNTIME_DOWN_INDEX(NumUp, Index) (TEST_RUNTIME_DOWN_OFF(NumUp) + ((PTR_ADDR)(Index) * SEGGER_RTT__BUFFER_SIZE))

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef union {
  uint64_t Align;
  unsigned char ac[TEST_MEM_SIZE];
} TEST_MEM;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static TEST_MEM _TestMem;
static unsigned _NumFailures;

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _Fail()
*
*  Function description
*    Records a failed assertion and prints the source location.  Tests
*    continue after a failure so one run can report multiple broken API
*    contracts.
*
*  Parameters
*    sFile       Source file that contains the failed assertion.
*    Line        Source line that contains the failed assertion.
*    sExpr       Assertion expression text.
*
*  Return value
*    None.
*/
static void _Fail(const char* sFile, int Line, const char* sExpr) {
  _NumFailures++;
  printf("%s:%d: assertion failed: %s\n", sFile, Line, sExpr);
}

//
// Minimal assertion helpers.  The macros follow the simple style used
// by SEGGER sample code: no external unit-test framework and no dynamic
// allocation.
//
#define TEST_ASSERT(Expr)                         \
  do {                                            \
    if (!(Expr)) {                                \
      _Fail(__FILE__, __LINE__, #Expr);           \
    }                                             \
  } while (0)

#define TEST_ASSERT_EQ_U(Expected, Actual)        \
  do {                                            \
    unsigned _Expected;                           \
    unsigned _Actual;                             \
    _Expected = (unsigned)(Expected);             \
    _Actual   = (unsigned)(Actual);               \
    if (_Expected != _Actual) {                   \
      _NumFailures++;                             \
      printf("%s:%d: expected %u, got %u\n",      \
             __FILE__, __LINE__, _Expected, _Actual); \
    }                                             \
  } while (0)

#define TEST_ASSERT_EQ_I(Expected, Actual)        \
  do {                                            \
    int _Expected;                                \
    int _Actual;                                  \
    _Expected = (int)(Expected);                  \
    _Actual   = (int)(Actual);                    \
    if (_Expected != _Actual) {                   \
      _NumFailures++;                             \
      printf("%s:%d: expected %d, got %d\n",      \
             __FILE__, __LINE__, _Expected, _Actual); \
    }                                             \
  } while (0)

/*********************************************************************
*
*       _Base()
*
*  Function description
*    Returns the base address of the simulated shared memory mapping.
*
*  Parameters
*    None.
*
*  Return value
*    Base address of the simulated shared memory mapping.
*/
static PTR_ADDR _Base(void) {
  return (PTR_ADDR)&_TestMem.ac[0];
}

/*********************************************************************
*
*       _Reset()
*
*  Function description
*    Clears the simulated shared memory and creates a fresh RTT control
*    block at the mapping base.  Each test starts from this state unless
*    it intentionally tests search behavior at a different offset.
*
*  Parameters
*    None.
*
*  Return value
*    Base address of the initialized RTT control block.
*/
static PTR_ADDR _Reset(void) {
  PTR_ADDR Address;

  memset(&_TestMem, 0, sizeof(_TestMem));
  Address = _Base();
  SEGGER_RTT_Init(Address);
  return Address;
}

/*********************************************************************
*
*       _UpRing()
*
*  Function description
*    Returns the descriptor address for an up-buffer using the fixed
*    up-buffer array location in the RTT control block.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    BufferIndex  Index of the up-buffer descriptor.
*
*  Return value
*    Address of the selected up-buffer descriptor.
*/
static PTR_ADDR _UpRing(PTR_ADDR Address, unsigned BufferIndex) {
  return SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_A_UP_INDEX(BufferIndex));
}

/*********************************************************************
*
*       _DownRing()
*
*  Function description
*    Returns the descriptor address for a down-buffer using the runtime
*    MaxNumUpBuffers field.  This mirrors the shared-memory requirement
*    that a reader must not assume its own compile-time down-buffer
*    offset.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    BufferIndex  Index of the down-buffer descriptor.
*
*  Return value
*    Address of the selected down-buffer descriptor.
*/
static PTR_ADDR _DownRing(PTR_ADDR Address, unsigned BufferIndex) {
  unsigned MaxNumUpBuffers;

  MaxNumUpBuffers = SEGGER_RTT__RD32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS));
  return SEGGER_RTT__ADDR(Address, TEST_RUNTIME_DOWN_INDEX(MaxNumUpBuffers, BufferIndex));
}

/*********************************************************************
*
*       _ExpectMem()
*
*  Function description
*    Compares a byte sequence returned from a ring buffer with the
*    expected payload.
*
*  Parameters
*    pData        Pointer to the data returned by the RTT API.
*    sExpected    Pointer to the expected byte sequence.
*    NumBytes     Number of bytes to compare.
*
*  Return value
*    None.
*/
static void _ExpectMem(const void* pData, const char* sExpected, unsigned NumBytes) {
  TEST_ASSERT(memcmp(pData, sExpected, NumBytes) == 0);
}

/*********************************************************************
*
*       _RingName()
*
*  Function description
*    Resolves a descriptor name offset to a local test-process pointer.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    pRing        Address of the ring-buffer descriptor.
*
*  Return value
*    Pointer to the descriptor name string in the simulated mapping.
*/
static const char* _RingName(PTR_ADDR Address, PTR_ADDR pRing) {
  return (const char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_NAME)));
}

/*********************************************************************
*
*       _RingBuffer()
*
*  Function description
*    Resolves a descriptor payload-buffer offset to a local test-process
*    pointer.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    pRing        Address of the ring-buffer descriptor.
*
*  Return value
*    Pointer to the descriptor payload buffer in the simulated mapping.
*/
static char* _RingBuffer(PTR_ADDR Address, PTR_ADDR pRing) {
  return (char*)SEGGER_RTT__ADDR(Address, SEGGER_RTT__RD64(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_P_BUFFER)));
}

/*********************************************************************
*
*       _SetRingOffsets()
*
*  Function description
*    Writes ring-buffer offsets directly into the simulated shared
*    memory so boundary and wrap-around branches can be exercised.
*
*  Parameters
*    pRing        Address of the ring-buffer descriptor.
*    RdOff        Read offset to write into the descriptor.
*    WrOff        Write offset to write into the descriptor.
*
*  Return value
*    None.
*/
static void _SetRingOffsets(PTR_ADDR pRing, unsigned RdOff, unsigned WrOff) {
  SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF), RdOff);
  SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF), WrOff);
}

/*********************************************************************
*
*       _SetRingFlags()
*
*  Function description
*    Writes ring-buffer mode flags directly into a descriptor.
*
*  Parameters
*    pRing        Address of the ring-buffer descriptor.
*    Flags        Ring-buffer mode flags.
*
*  Return value
*    None.
*/
static void _SetRingFlags(PTR_ADDR pRing, unsigned Flags) {
  SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS), Flags);
}

/*********************************************************************
*
*       _SetRingSize()
*
*  Function description
*    Writes a descriptor size field directly into the simulated shared
*    memory.
*
*  Parameters
*    pRing         Address of the ring-buffer descriptor.
*    SizeOfBuffer  Size field value to write into the descriptor.
*
*  Return value
*    None.
*/
static void _SetRingSize(PTR_ADDR pRing, unsigned SizeOfBuffer) {
  SEGGER_RTT__WR32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), SizeOfBuffer);
}

/*********************************************************************
*
*       _SharedString()
*
*  Function description
*    Places a NUL-terminated string inside the simulated shared mapping
*    and returns its local address for configuration APIs.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    Off          Offset inside the simulated shared mapping.
*    s            String to copy into the simulated shared mapping.
*
*  Return value
*    Pointer to the copied string in the simulated shared mapping.
*/
static char* _SharedString(PTR_ADDR Address, unsigned Off, const char* s) {
  char* p;

  p = (char*)SEGGER_RTT__ADDR(Address, Off);
  strcpy(p, s);
  return p;
}

/*********************************************************************
*
*       _SharedBuffer()
*
*  Function description
*    Reserves a payload buffer inside the simulated shared mapping and
*    clears it before it is installed in a descriptor.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    Off          Offset inside the simulated shared mapping.
*    NumBytes     Number of bytes to clear.
*
*  Return value
*    Pointer to the cleared buffer in the simulated shared mapping.
*/
static void* _SharedBuffer(PTR_ADDR Address, unsigned Off, unsigned NumBytes) {
  void* p;

  p = (void*)SEGGER_RTT__ADDR(Address, Off);
  memset(p, 0, NumBytes);
  return p;
}

/*********************************************************************
*
*       _CallVPrintf()
*
*  Function description
*    Small adapter used to exercise SEGGER_RTT_vprintf(), whose public
*    API receives a va_list pointer.
*
*  Parameters
*    Address      Base address of the RTT control block.
*    sFormat      Format string passed to SEGGER_RTT_vprintf().
*    ...          Format arguments.
*
*  Return value
*    Return value from SEGGER_RTT_vprintf().
*/
static int _CallVPrintf(PTR_ADDR Address, const char* sFormat, ...) {
  int r;
  va_list ParamList;

  va_start(ParamList, sFormat);
  r = SEGGER_RTT_vprintf(Address, 0u, sFormat, &ParamList);
  va_end(ParamList);
  return r;
}

/*********************************************************************
*
*       Test cases
*
**********************************************************************
*/

/*********************************************************************
*
*       _TestInitAndFind()
*
*  Function description
*    Verifies initialization detection and range-limited control block
*    search.  The test covers aligned and unaligned search bases while
*    requiring the returned control block address to be aligned.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestInitAndFind(void) {
  PTR_ADDR Address;
  PTR_ADDR Search;
  size_t DescriptorRegionSize;
  size_t RegionSize;
  size_t TruncatedRegionSize;

  memset(&_TestMem, 0, sizeof(_TestMem));
  Address = _Base();
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckInit(0u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckInit(Address + 1u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_InitEx(0u, TEST_MEM_SIZE));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_InitEx(Address + 1u, TEST_MEM_SIZE));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_InitEx(Address + 4u, TEST_MEM_SIZE - 4u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_InitEx(Address, SEGGER_RTT__REQUIRED_MEM_SIZE - 1u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_FindControlBlock(NULL, sizeof(_TestMem.ac)));

  TEST_ASSERT_EQ_I(0, SEGGER_RTT_InitEx(Address, TEST_MEM_SIZE));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckInit(Address));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckRegion(Address, TEST_MEM_SIZE));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckUpBuffer(Address, TEST_MEM_SIZE, 0u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckUpBuffer(Address, TEST_MEM_SIZE, 1u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckDownBuffer(Address, TEST_MEM_SIZE, 0u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckDownBuffer(Address, TEST_MEM_SIZE, 1u));
  DescriptorRegionSize = SEGGER_RTT__DOWN_BUFFER_OFF + BUFFER_SIZE_DOWN;
  TruncatedRegionSize  = DescriptorRegionSize - 1u;
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckRegion(Address, DescriptorRegionSize));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckUpBuffer(Address, DescriptorRegionSize, 0u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckDownBuffer(Address, DescriptorRegionSize, 0u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckRegion(Address, TruncatedRegionSize));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckUpBuffer(Address, TruncatedRegionSize, 0u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckDownBuffer(Address, TruncatedRegionSize, 0u));
  Search = Address;
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_FindControlBlock(&Search, sizeof(_TestMem.ac)));
  TEST_ASSERT(Search == Address);
  Search = Address;
  RegionSize = 0u;
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_FindValidControlBlock(&Search, sizeof(_TestMem.ac), &RegionSize));
  TEST_ASSERT(Search == Address);
  TEST_ASSERT_EQ_U(TEST_MEM_SIZE, RegionSize);

  memset(&_TestMem, 0, sizeof(_TestMem));
  Address = _Base() + SEGGER_RTT__CB_ALIGNMENT;
  SEGGER_RTT_Init(Address);
  Search = _Base();
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_FindControlBlock(&Search, sizeof(_TestMem.ac) - SEGGER_RTT__CB_ALIGNMENT));
  TEST_ASSERT(Search == Address);

  Search = _Base() + 1u;
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_FindControlBlock(&Search, sizeof(_TestMem.ac) - 1u));
  TEST_ASSERT(Search == Address);
  Search = _Base();
  RegionSize = 0u;
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_FindValidControlBlock(&Search, sizeof(_TestMem.ac), &RegionSize));
  TEST_ASSERT(Search == Address);
  TEST_ASSERT_EQ_U(TEST_MEM_SIZE - SEGGER_RTT__CB_ALIGNMENT, RegionSize);

  Address = _Reset();
  SEGGER_RTT__WR64(SEGGER_RTT__FIELD(_UpRing(Address, 0u), SEGGER_RTT__BUFFER_OFF_P_BUFFER), TEST_MEM_SIZE - 4u);
  SEGGER_RTT__WR32(SEGGER_RTT__FIELD(_UpRing(Address, 0u), SEGGER_RTT__BUFFER_OFF_SIZE_OF_BUFFER), 8u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckRegion(Address, TEST_MEM_SIZE));
  Search = Address;
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_FindValidControlBlock(&Search, TEST_MEM_SIZE, NULL));

  Address = _Reset();
  SEGGER_RTT__WR32(SEGGER_RTT__FIELD(_UpRing(Address, 0u), SEGGER_RTT__BUFFER_OFF_FLAGS), 3u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckRegion(Address, TEST_MEM_SIZE));

  Address = _Reset();
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS), TEST_MEM_SIZE);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckRegion(Address, TEST_MEM_SIZE));

  memset(&_TestMem, 0, sizeof(_TestMem));
  Address = _Base();
  SEGGER_RTT__WR8(Address, 'S');
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckInit(Address));
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutChar(Address, 0u, 'Z'));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckInit(Address));
}

/*********************************************************************
*
*       _TestEnsureInitExAPI()
*
*  Function description
*    Verifies the owner-side initialize-or-reuse API and its configured
*    extra up/down buffer pairs.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestEnsureInitExAPI(void) {
  PTR_ADDR Address;
  PTR_ADDR ExpectedPairBase;
  PTR_ADDR Search;
  PTR_ADDR pRing;
  size_t RegionSize;
  size_t FoundRegionSize;
  char ac[64];

  memset(&_TestMem, 0, sizeof(_TestMem));
  Address = _Base();
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_EnsureInitEx(0u, TEST_MEM_SIZE, 1u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_EnsureInitEx(Address + 1u, TEST_MEM_SIZE - 1u, 1u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_EnsureInitEx(Address, 0u, 1u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_EnsureInitEx(Address, TEST_MEM_SIZE, 0u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_EnsureInitEx(Address,
                                               TEST_MEM_SIZE,
                                               SEGGER_RTT_MAX_NUM_UP_BUFFERS + 1u));

  RegionSize = SEGGER_RTT__REQUIRED_MEM_SIZE + TEST_ENSURE_EXTRA_PAIR_SIZE - 1u;
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_EnsureInitEx(Address, RegionSize, 2u));

  memset(&_TestMem, 0, sizeof(_TestMem));
  Address = _Base();
  RegionSize = SEGGER_RTT__REQUIRED_MEM_SIZE + (2u * TEST_ENSURE_EXTRA_PAIR_SIZE);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_EnsureInitEx(Address, RegionSize, 3u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckRegion(Address, RegionSize));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckUpBuffer(Address, RegionSize, 0u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckUpBuffer(Address, RegionSize, 1u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckUpBuffer(Address, RegionSize, 2u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckDownBuffer(Address, RegionSize, 0u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckDownBuffer(Address, RegionSize, 1u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckDownBuffer(Address, RegionSize, 2u));

  Search = Address;
  FoundRegionSize = 0u;
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_FindValidControlBlock(&Search, RegionSize, &FoundRegionSize));
  TEST_ASSERT(Search == Address);
  TEST_ASSERT_EQ_U(RegionSize, FoundRegionSize);

  ExpectedPairBase = SEGGER_RTT__ADDR(Address, SEGGER_RTT__REQUIRED_MEM_SIZE);
  pRing = _UpRing(Address, 1u);
  TEST_ASSERT(strcmp(_RingName(Address, pRing), "Terminal") == 0);
  TEST_ASSERT(_RingBuffer(Address, pRing) == (char*)SEGGER_RTT__ADDR(ExpectedPairBase, SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED));
  pRing = _DownRing(Address, 1u);
  TEST_ASSERT(strcmp(_RingName(Address, pRing), "Terminal") == 0);
  TEST_ASSERT(_RingBuffer(Address, pRing) == (char*)SEGGER_RTT__ADDR(ExpectedPairBase, SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED + BUFFER_SIZE_UP));

  ExpectedPairBase = SEGGER_RTT__ADDR(Address, SEGGER_RTT__REQUIRED_MEM_SIZE + TEST_ENSURE_EXTRA_PAIR_SIZE);
  pRing = _UpRing(Address, 2u);
  TEST_ASSERT(strcmp(_RingName(Address, pRing), "Terminal") == 0);
  TEST_ASSERT(_RingBuffer(Address, pRing) == (char*)SEGGER_RTT__ADDR(ExpectedPairBase, SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED));
  pRing = _DownRing(Address, 2u);
  TEST_ASSERT(strcmp(_RingName(Address, pRing), "Terminal") == 0);
  TEST_ASSERT(_RingBuffer(Address, pRing) == (char*)SEGGER_RTT__ADDR(ExpectedPairBase, SEGGER_RTT__TERMINAL_NAME_SIZE_ALIGNED + BUFFER_SIZE_UP));

  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteNoLock(Address, 1u, "u1", 2u));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteDownBufferNoLock(Address, 2u, "d2", 2u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_EnsureInitEx(Address, RegionSize, 3u));

  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_ReadUpBufferNoLock(Address, 1u, ac, sizeof(ac)));
  _ExpectMem(ac, "u1", 2u);
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_ReadNoLock(Address, 2u, ac, sizeof(ac)));
  _ExpectMem(ac, "d2", 2u);
}

/*********************************************************************
*
*       _TestUpBufferAPI()
*
*  Function description
*    Exercises the target-to-host public API surface.  The test covers
*    locked and non-locking writes, direct character writes, string
*    writes, skip mode, trim mode, overwrite mode, readback, and byte
*    counters.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestUpBufferAPI(void) {
  PTR_ADDR Address;
  char ac[64];
  unsigned r;

  Address = _Reset();
  TEST_ASSERT_EQ_U(BUFFER_SIZE_UP - 1u, SEGGER_RTT_GetAvailWriteSpace(Address, 0u));

  r = SEGGER_RTT_WriteNoLock(Address, 0u, "ABC", 3u);
  TEST_ASSERT_EQ_U(3u, r);
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT_HASDATA_UP(Address, 0u));
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT_HasDataUp(Address, 0u));
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT_GetBytesInBuffer(Address, 0u));
  memset(ac, 0, sizeof(ac));
  r = SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac));
  TEST_ASSERT_EQ_U(3u, r);
  _ExpectMem(ac, "ABC", 3u);

  r = SEGGER_RTT_Write(Address, 0u, "DE", 2u);
  TEST_ASSERT_EQ_U(2u, r);
  memset(ac, 0, sizeof(ac));
  r = SEGGER_RTT_ReadUpBuffer(Address, 0u, ac, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, r);
  _ExpectMem(ac, "DE", 2u);

  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutCharSkipNoLock(Address, 0u, 'F'));
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutCharSkip(Address, 0u, 'G'));
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutChar(Address, 0u, 'H'));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteString(Address, 0u, "IJ"));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteString(Address, 0u, NULL));
  memset(ac, 0, sizeof(ac));
  r = SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac));
  TEST_ASSERT_EQ_U(5u, r);
  _ExpectMem(ac, "FGHIJ", 5u);

  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "KL", 2u));
  memset(ac, 0, sizeof(ac));
  r = SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, r);
  _ExpectMem(ac, "KL", 2u);

  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "0123456789ABCDEF", BUFFER_SIZE_UP));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetFlagsUpBuffer(Address, 0u, SEGGER_RTT_MODE_NO_BLOCK_TRIM));
  r = SEGGER_RTT_WriteNoLock(Address, 0u, "0123456789ABCDEF", BUFFER_SIZE_UP);
  TEST_ASSERT_EQ_U(BUFFER_SIZE_UP - 1u, r);
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(BUFFER_SIZE_UP - 1u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));

  SEGGER_RTT_WriteWithOverwriteNoLock(Address, 0u, "MN", 2u);
  memset(ac, 0, sizeof(ac));
  r = SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, r);
  _ExpectMem(ac, "MN", 2u);
}

/*********************************************************************
*
*       _TestDownBufferAPI()
*
*  Function description
*    Exercises the host-to-target public API surface.  The test covers
*    locked and non-locking writes, direct key reads, blocking wait in a
*    pre-filled buffer, data availability checks, trim mode, and byte
*    counters.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestDownBufferAPI(void) {
  PTR_ADDR Address;
  char ac[64];
  unsigned r;

  Address = _Reset();
  r = SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "ab", 2u);
  TEST_ASSERT_EQ_U(2u, r);
  TEST_ASSERT_EQ_I(1, SEGGER_RTT_HasKey(Address));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_HASDATA(Address, 0u));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_HasData(Address, 0u));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_GetBytesDownInBuffer(Address, 0u));

  memset(ac, 0, sizeof(ac));
  r = SEGGER_RTT_ReadNoLock(Address, 0u, ac, 1u);
  TEST_ASSERT_EQ_U(1u, r);
  _ExpectMem(ac, "a", 1u);
  TEST_ASSERT_EQ_I('b', SEGGER_RTT_GetKey(Address));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_GetKey(Address));

  r = SEGGER_RTT_WriteDownBuffer(Address, 0u, "cd", 2u);
  TEST_ASSERT_EQ_U(2u, r);
  memset(ac, 0, sizeof(ac));
  r = SEGGER_RTT_Read(Address, 0u, ac, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, r);
  _ExpectMem(ac, "cd", 2u);

  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "z", 1u));
  TEST_ASSERT_EQ_I('z', SEGGER_RTT_WaitKey(Address));

  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetFlagsDownBuffer(Address, 0u, SEGGER_RTT_MODE_NO_BLOCK_TRIM));
  r = SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "0123456789ABCDEF", BUFFER_SIZE_DOWN);
  TEST_ASSERT_EQ_U(BUFFER_SIZE_DOWN - 1u, r);
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(BUFFER_SIZE_DOWN - 1u, SEGGER_RTT_ReadNoLock(Address, 0u, ac, sizeof(ac)));
}

/*********************************************************************
*
*       _TestConfigAndAllocAPI()
*
*  Function description
*    Verifies descriptor configuration APIs.  Names and payload buffers
*    are located in the simulated shared mapping so the RTT code stores
*    and later resolves relative offsets, matching the heterogeneous
*    shared-memory contract.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestConfigAndAllocAPI(void) {
  PTR_ADDR Address;
  PTR_ADDR pRing;
  char* sName;
  void* pBuffer;
  char ac[64];
  int BufferIndex;

  Address = _Reset();
  sName   = _SharedString(Address, TEST_EXTRA_STR0_OFF, "Up1");
  pBuffer = _SharedBuffer(Address, TEST_EXTRA_UP_BUF_OFF, 24u);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_ConfigUpBuffer(Address, 1u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckUpBuffer(Address, TEST_MEM_SIZE, 1u));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_Write(Address, 1u, "u1", 2u));
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_ReadUpBufferNoLock(Address, 1u, ac, sizeof(ac)));
  _ExpectMem(ac, "u1", 2u);

  sName   = _SharedString(Address, TEST_EXTRA_STR1_OFF, "Down1");
  pBuffer = _SharedBuffer(Address, TEST_EXTRA_DOWN_BUF_OFF, 24u);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_ConfigDownBuffer(Address, 1u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckDownBuffer(Address, TEST_MEM_SIZE, 1u));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteDownBuffer(Address, 1u, "d1", 2u));
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_ReadNoLock(Address, 1u, ac, sizeof(ac)));
  _ExpectMem(ac, "d1", 2u);

  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetNameUpBuffer(Address, 1u, _SharedString(Address, TEST_EXTRA_STR0_OFF, "UpRenamed")));
  pRing = _UpRing(Address, 1u);
  TEST_ASSERT(strcmp(_RingName(Address, pRing), "UpRenamed") == 0);

  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetNameDownBuffer(Address, 1u, _SharedString(Address, TEST_EXTRA_STR1_OFF, "DownRenamed")));
  pRing = _DownRing(Address, 1u);
  TEST_ASSERT(strcmp(_RingName(Address, pRing), "DownRenamed") == 0);

  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetFlagsUpBuffer(Address, 1u, SEGGER_RTT_MODE_NO_BLOCK_TRIM));
  pRing = _UpRing(Address, 1u);
  TEST_ASSERT_EQ_U(SEGGER_RTT_MODE_NO_BLOCK_TRIM, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS)));

  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetFlagsDownBuffer(Address, 1u, SEGGER_RTT_MODE_NO_BLOCK_TRIM));
  pRing = _DownRing(Address, 1u);
  TEST_ASSERT_EQ_U(SEGGER_RTT_MODE_NO_BLOCK_TRIM, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_FLAGS)));

  Address = _Reset();
  sName   = _SharedString(Address, TEST_EXTRA_STR0_OFF, "AllocUp");
  pBuffer = _SharedBuffer(Address, TEST_EXTRA_UP_BUF_OFF, 20u);
  BufferIndex = SEGGER_RTT_AllocUpBuffer(Address, sName, pBuffer, 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  TEST_ASSERT_EQ_I(1, BufferIndex);
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteNoLock(Address, (unsigned)BufferIndex, "AU", 2u));

  sName   = _SharedString(Address, TEST_EXTRA_STR1_OFF, "AllocDown");
  pBuffer = _SharedBuffer(Address, TEST_EXTRA_DOWN_BUF_OFF, 20u);
  BufferIndex = SEGGER_RTT_AllocDownBuffer(Address, sName, pBuffer, 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  TEST_ASSERT_EQ_I(1, BufferIndex);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_CheckDownBuffer(Address, TEST_MEM_SIZE, (unsigned)BufferIndex));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteDownBufferNoLock(Address, (unsigned)BufferIndex, "AD", 2u));
}

/*********************************************************************
*
*       _TestInvalidDescriptorAPI()
*
*  Function description
*    Exercises error paths that must fail without touching payload
*    memory when the address, ID, buffer count, or ring descriptor is
*    invalid.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestInvalidDescriptorAPI(void) {
  PTR_ADDR Address;
  PTR_ADDR Search;
  PTR_ADDR pRing;
  char ac[8];

  memset(&_TestMem, 0, sizeof(_TestMem));
  Address = _Base() + 1u;
  SEGGER_RTT_Init(Address);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckInit(Address));
  Search = _Base();
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_FindControlBlock(&Search, SEGGER_RTT__CB_OFF_A_UP - 1u));
  memset(&_TestMem, 0, sizeof(_TestMem));
  _TestMem.ac[4] = 'S';
  Search = _Base();
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_FindControlBlock(&Search, sizeof(_TestMem.ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadUpBufferNoLock(0u, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadNoLock(0u, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteNoLock(0u, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteSkipNoLock(0u, 0u, "A", 1u));
  SEGGER_RTT_WriteWithOverwriteNoLock(0u, 0u, "A", 1u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteDownBufferNoLock(0u, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkipNoLock(0u, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkip(0u, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutChar(0u, 0u, 'A'));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_HasKey(0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasData(0u, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasDataUp(0u, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetAvailWriteSpace(0u, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesInBuffer(0u, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesDownInBuffer(0u, 0u));

  Address = _Reset();
  SEGGER_RTT__WR8(Address, 'X');
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_CheckInit(Address));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteNoLock(Address, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "A", 1u));
  SEGGER_RTT_WriteWithOverwriteNoLock(Address, 0u, "A", 1u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkipNoLock(Address, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_HasKey(Address));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasData(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasDataUp(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetAvailWriteSpace(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesInBuffer(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesDownInBuffer(Address, 0u));

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingSize(pRing, 1u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetAvailWriteSpace(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasDataUp(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesInBuffer(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteNoLock(Address, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkipNoLock(Address, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkip(Address, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutChar(Address, 0u, 'A'));
  SEGGER_RTT_WriteWithOverwriteNoLock(Address, 0u, "A", 1u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetTerminal(Address, 1u));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_TerminalOut(Address, 1u, "A"));

  Address = _Reset();
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS), 0u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteNoLock(Address, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "A", 1u));
  SEGGER_RTT_WriteWithOverwriteNoLock(Address, 0u, "A", 1u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "A", 1u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkipNoLock(Address, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkip(Address, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutChar(Address, 0u, 'A'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasDataUp(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetAvailWriteSpace(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesInBuffer(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesDownInBuffer(Address, 0u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetFlagsUpBuffer(Address, 0u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));

  Address = _Reset();
  pRing = _DownRing(Address, 0u);
  _SetRingSize(pRing, 1u);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_HasKey(Address));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasData(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesDownInBuffer(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "A", 1u));

  Address = _Reset();
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS), 0u);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_HasKey(Address));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_HasData(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_GetBytesDownInBuffer(Address, 0u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_ReadNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "A", 1u));
}

/*********************************************************************
*
*       _TestRingBoundaryAPI()
*
*  Function description
*    Exercises wrap-around, blocking-mode success, full-buffer skip,
*    and overwrite branches using descriptor offsets crafted in shared
*    memory.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestRingBoundaryAPI(void) {
  PTR_ADDR Address;
  PTR_ADDR pRing;
  char* pBuffer;
  char ac[32];

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  pBuffer = _RingBuffer(Address, pRing);

  _SetRingOffsets(pRing, 8u, 4u);
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT_GetAvailWriteSpace(Address, 0u));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteNoLock(Address, 0u, "rs", 2u));

  _SetRingOffsets(pRing, 3u, 14u);
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "WXYZ", 4u));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));
  TEST_ASSERT(pBuffer[14] == 'W');
  TEST_ASSERT(pBuffer[15] == 'X');
  TEST_ASSERT(pBuffer[0] == 'Y');
  TEST_ASSERT(pBuffer[1] == 'Z');

  _SetRingOffsets(pRing, 10u, 4u);
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "abc", 3u));
  TEST_ASSERT_EQ_U(7u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));

  _SetRingOffsets(pRing, 6u, 4u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteSkipNoLock(Address, 0u, "abc", 3u));

  _SetRingFlags(pRing, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
  _SetRingOffsets(pRing, 4u, 14u);
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteNoLock(Address, 0u, "BC", 2u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));

  _SetRingOffsets(pRing, 8u, 4u);
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteNoLock(Address, 0u, "DE", 2u));
  TEST_ASSERT_EQ_U(6u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));

  _SetRingFlags(pRing, 3u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteNoLock(Address, 0u, "F", 1u));

  _SetRingFlags(pRing, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  _SetRingOffsets(pRing, 4u, 15u);
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutCharSkipNoLock(Address, 0u, 'G'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));
  _SetRingOffsets(pRing, 4u, 3u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkipNoLock(Address, 0u, 'H'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutCharSkip(Address, 0u, 'I'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_PutChar(Address, 0u, 'J'));

  _SetRingOffsets(pRing, 4u, 15u);
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutCharSkip(Address, 0u, 'K'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));
  _SetRingOffsets(pRing, 4u, 15u);
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutChar(Address, 0u, 'L'));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));
  _SetRingFlags(pRing, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
  _SetRingOffsets(pRing, 4u, 1u);
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT_PutChar(Address, 0u, 'M'));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));
  _SetRingFlags(pRing, SEGGER_RTT_MODE_NO_BLOCK_SKIP);

  _SetRingOffsets(pRing, 1u, 14u);
  SEGGER_RTT_WriteWithOverwriteNoLock(Address, 0u, "QRST", 4u);
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF)));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));
  TEST_ASSERT(pBuffer[14] == 'Q');
  TEST_ASSERT(pBuffer[15] == 'R');
  TEST_ASSERT(pBuffer[0] == 'S');
  TEST_ASSERT(pBuffer[1] == 'T');

  _SetRingOffsets(pRing, 14u, 4u);
  SEGGER_RTT_WriteWithOverwriteNoLock(Address, 0u, "abcdefghijkl", 12u);
  TEST_ASSERT_EQ_U(1u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_RD_OFF)));

  memset(pBuffer, 0, BUFFER_SIZE_UP);
  pBuffer[14] = '1';
  pBuffer[15] = '2';
  pBuffer[0]  = '3';
  pBuffer[1]  = '4';
  _SetRingOffsets(pRing, 14u, 2u);
  TEST_ASSERT_EQ_U(4u, SEGGER_RTT_HasDataUp(Address, 0u));
  TEST_ASSERT_EQ_U(4u, SEGGER_RTT_GetBytesInBuffer(Address, 0u));
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(4u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  _ExpectMem(ac, "1234", 4u);

  Address = _Reset();
  pRing = _DownRing(Address, 0u);
  pBuffer = _RingBuffer(Address, pRing);
  memset(pBuffer, 0, BUFFER_SIZE_DOWN);
  pBuffer[14] = '5';
  pBuffer[15] = '6';
  pBuffer[0]  = '7';
  _SetRingOffsets(pRing, 14u, 1u);
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT_HasData(Address, 0u));
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT_GetBytesDownInBuffer(Address, 0u));
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(3u, SEGGER_RTT_ReadNoLock(Address, 0u, ac, sizeof(ac)));
  _ExpectMem(ac, "567", 3u);

  _SetRingFlags(pRing, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
  _SetRingOffsets(pRing, 0u, BUFFER_SIZE_DOWN - 1u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "kl", 2u));

  _SetRingFlags(pRing, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
  _SetRingOffsets(pRing, 4u, 14u);
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "kl", 2u));
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));
  _SetRingFlags(pRing, 3u);
  TEST_ASSERT_EQ_U(0u, SEGGER_RTT_WriteDownBufferNoLock(Address, 0u, "m", 1u));
}

/*********************************************************************
*
*       _TestConfigErrorAPI()
*
*  Function description
*    Exercises configuration and allocation failures for bad addresses,
*    bad runtime buffer counts, invalid indexes, and exhausted
*    descriptors.
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestConfigErrorAPI(void) {
  PTR_ADDR Address;
  char* sName;
  void* pBuffer;

  Address = _Reset();
  sName = _SharedString(Address, TEST_EXTRA_STR0_OFF, "CfgErr");
  pBuffer = _SharedBuffer(Address, TEST_EXTRA_UP_BUF_OFF, 24u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigUpBuffer(0u, 1u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigUpBuffer(Address, 9u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigUpBuffer(Address, 1u, (const char*)Address, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigUpBuffer(Address, 1u, sName, (void*)Address, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigUpBuffer(Address, 1u, sName, pBuffer, 1u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigUpBuffer(Address, 1u, sName, pBuffer, 24u, 3u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigDownBuffer(Address, 1u, (const char*)Address, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigDownBuffer(Address, 1u, sName, (void*)Address, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigDownBuffer(Address, 1u, sName, pBuffer, 1u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigDownBuffer(Address, 1u, sName, pBuffer, 24u, 3u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigDownBuffer(0u, 1u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_ConfigDownBuffer(Address, 9u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetNameUpBuffer(0u, 0u, sName));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetNameUpBuffer(Address, 0u, (const char*)Address));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetNameUpBuffer(Address, 9u, sName));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetNameDownBuffer(0u, 0u, sName));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetNameDownBuffer(Address, 1u, (const char*)Address));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetNameDownBuffer(Address, 9u, sName));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetFlagsUpBuffer(0u, 0u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetFlagsUpBuffer(Address, 9u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetFlagsUpBuffer(Address, 0u, 3u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetFlagsDownBuffer(0u, 0u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetFlagsDownBuffer(Address, 9u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetFlagsDownBuffer(Address, 0u, 3u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocUpBuffer(0u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocUpBuffer(Address, sName, (void*)Address, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocUpBuffer(Address, sName, pBuffer, 1u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocUpBuffer(Address, sName, pBuffer, 24u, 3u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocDownBuffer(0u, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocDownBuffer(Address, (const char*)Address, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocUpBuffer(Address, (const char*)Address, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocDownBuffer(Address, sName, (void*)Address, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocDownBuffer(Address, sName, pBuffer, 1u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocDownBuffer(Address, sName, pBuffer, 24u, 3u));

  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS), 0u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocUpBuffer(Address, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_UP_BUFFERS), SEGGER_RTT_MAX_NUM_UP_BUFFERS);
  SEGGER_RTT__WR32(SEGGER_RTT__ADDR(Address, SEGGER_RTT__CB_OFF_MAX_NUM_DOWN_BUFFERS), 0u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocDownBuffer(Address, sName, pBuffer, 24u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));

  Address = _Reset();
  TEST_ASSERT_EQ_I(1, SEGGER_RTT_AllocUpBuffer(Address, _SharedString(Address, TEST_EXTRA_STR0_OFF, "FullUp1"), _SharedBuffer(Address, TEST_EXTRA_UP_BUF_OFF, 20u), 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(2, SEGGER_RTT_AllocUpBuffer(Address, _SharedString(Address, TEST_EXTRA_STR1_OFF, "FullUp2"), _SharedBuffer(Address, TEST_EXTRA_UP_BUF_OFF + 32u, 20u), 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocUpBuffer(Address, _SharedString(Address, TEST_EXTRA_STR0_OFF, "FullUp3"), _SharedBuffer(Address, TEST_EXTRA_UP_BUF_OFF + 64u, 20u), 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(1, SEGGER_RTT_AllocDownBuffer(Address, _SharedString(Address, TEST_EXTRA_STR0_OFF, "FullDown1"), _SharedBuffer(Address, TEST_EXTRA_DOWN_BUF_OFF, 20u), 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(2, SEGGER_RTT_AllocDownBuffer(Address, _SharedString(Address, TEST_EXTRA_STR1_OFF, "FullDown2"), _SharedBuffer(Address, TEST_EXTRA_DOWN_BUF_OFF + 32u, 20u), 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_AllocDownBuffer(Address, _SharedString(Address, TEST_EXTRA_STR0_OFF, "FullDown3"), _SharedBuffer(Address, TEST_EXTRA_DOWN_BUF_OFF + 64u, 20u), 20u, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
}

/*********************************************************************
*
*       _TestTerminalAndPrintfAPI()
*
*  Function description
*    Exercises terminal switching and formatted output.  The test checks
*    the terminal escape bytes as well as the data produced by
*    SEGGER_RTT_printf() and SEGGER_RTT_vprintf().
*
*  Parameters
*    None.
*
*  Return value
*    None.
*/
static void _TestTerminalAndPrintfAPI(void) {
  PTR_ADDR Address;
  PTR_ADDR pRing;
  char ac[512];
  char acLong[180];
  unsigned NumBytesRead;
  int r;

  Address = _Reset();
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetTerminal(Address, 16u));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_TerminalOut(0u, 1u, "Bad"));
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_TerminalOut(Address, 1u, NULL));
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetTerminal(Address, 1u));
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0xFFu, (unsigned char)ac[0]);
  TEST_ASSERT_EQ_U('1', (unsigned char)ac[1]);

  r = SEGGER_RTT_TerminalOut(Address, 2u, "Hi");
  TEST_ASSERT_EQ_I(2, r);
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(6u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0xFFu, (unsigned char)ac[0]);
  TEST_ASSERT_EQ_U('2', (unsigned char)ac[1]);
  TEST_ASSERT_EQ_U('H', (unsigned char)ac[2]);
  TEST_ASSERT_EQ_U('i', (unsigned char)ac[3]);
  TEST_ASSERT_EQ_U(0xFFu, (unsigned char)ac[4]);
  TEST_ASSERT_EQ_U('1', (unsigned char)ac[5]);

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingOffsets(pRing, 0u, BUFFER_SIZE_UP - 1u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_SetTerminal(Address, 2u));

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingFlags(pRing, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_SetTerminal(Address, 3u));
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(2u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0xFFu, (unsigned char)ac[0]);
  TEST_ASSERT_EQ_U('3', (unsigned char)ac[1]);

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingOffsets(pRing, 0u, BUFFER_SIZE_UP - 2u);
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_TerminalOut(Address, 2u, "TooLarge"));

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingFlags(pRing, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
  _SetRingOffsets(pRing, 0u, BUFFER_SIZE_UP - 3u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_TerminalOut(Address, 2u, "TooLarge"));

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingFlags(pRing, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
  _SetRingOffsets(pRing, 0u, BUFFER_SIZE_UP - 8u);
  TEST_ASSERT_EQ_I(3, SEGGER_RTT_TerminalOut(Address, 2u, "abcdef"));
  TEST_ASSERT_EQ_U(BUFFER_SIZE_UP - 1u, SEGGER_RTT__RD32(SEGGER_RTT__FIELD(pRing, SEGGER_RTT__BUFFER_OFF_WR_OFF)));

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingFlags(pRing, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
  TEST_ASSERT_EQ_I(2, SEGGER_RTT_TerminalOut(Address, 4u, "XY"));
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(6u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  TEST_ASSERT_EQ_U(0xFFu, (unsigned char)ac[0]);
  TEST_ASSERT_EQ_U('4', (unsigned char)ac[1]);
  _ExpectMem(&ac[2], "XY", 2u);
  TEST_ASSERT_EQ_U(0xFFu, (unsigned char)ac[4]);
  TEST_ASSERT_EQ_U('0', (unsigned char)ac[5]);

  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingFlags(pRing, 3u);
  TEST_ASSERT_EQ_I(-1, SEGGER_RTT_TerminalOut(Address, 1u, "BadMode"));

  Address = _Reset();
  r = SEGGER_RTT_printf(Address, 0u, "P:%d:%s", 7, "ok");
  TEST_ASSERT(r >= 6);
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(6u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  _ExpectMem(ac, "P:7:ok", 6u);

  r = _CallVPrintf(Address, "V:%c:%X", 'A', 0x2A);
  TEST_ASSERT(r >= 6);
  memset(ac, 0, sizeof(ac));
  TEST_ASSERT_EQ_U(6u, SEGGER_RTT_ReadUpBufferNoLock(Address, 0u, ac, sizeof(ac)));
  _ExpectMem(ac, "V:A:2A", 6u);

  Address = _Reset();
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_ConfigUpBuffer(Address, 1u, _SharedString(Address, TEST_EXTRA_STR0_OFF, "Printf"), _SharedBuffer(Address, TEST_EXTRA_PRINTF_BUF_OFF, TEST_EXTRA_PRINTF_BUF_SIZE), TEST_EXTRA_PRINTF_BUF_SIZE, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  r = SEGGER_RTT_printf(Address, 1u, "[%05d][%-4u][%+d][%.3u][%.2s][%s][%p][%%][%#X][%hd][%ld][%q]", -12, 7, 5, 9, "abcdef", NULL, 0x1234, 0x2A, 3, 4);
  TEST_ASSERT(r > 0);
  r = SEGGER_RTT_printf(Address, 1u, "[%.*s]", 3, "abcdef");
  TEST_ASSERT(r > 0);
  memset(ac, 0, sizeof(ac));
  NumBytesRead = SEGGER_RTT_ReadUpBufferNoLock(Address, 1u, ac, sizeof(ac) - 1u);
  ac[NumBytesRead] = '\0';
  TEST_ASSERT(strstr(ac, "[-0012]") != NULL);
  TEST_ASSERT(strstr(ac, "[7   ]") != NULL);
  TEST_ASSERT(strstr(ac, "[+5]") != NULL);
  TEST_ASSERT(strstr(ac, "[009]") != NULL);
  TEST_ASSERT(strstr(ac, "[ab]") != NULL);
  TEST_ASSERT(strstr(ac, "[(NULL)]") != NULL);
  TEST_ASSERT(strstr(ac, "[00001234]") != NULL);
  TEST_ASSERT(strstr(ac, "[abc]") != NULL);

  Address = _Reset();
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_ConfigUpBuffer(Address, 1u, _SharedString(Address, TEST_EXTRA_STR0_OFF, "Flush"), _SharedBuffer(Address, TEST_EXTRA_PRINTF_BUF_OFF, TEST_EXTRA_PRINTF_BUF_SIZE), TEST_EXTRA_PRINTF_BUF_SIZE, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  memset(acLong, 'L', sizeof(acLong) - 1u);
  acLong[sizeof(acLong) - 1u] = '\0';
  r = SEGGER_RTT_printf(Address, 1u, "%s", acLong);
  TEST_ASSERT(r > 0);
  memset(ac, 0, sizeof(ac));
  NumBytesRead = SEGGER_RTT_ReadUpBufferNoLock(Address, 1u, ac, sizeof(ac));
  TEST_ASSERT_EQ_U(sizeof(acLong) - 1u, NumBytesRead);
  TEST_ASSERT(ac[0] == 'L');
  TEST_ASSERT(ac[sizeof(acLong) - 2u] == 'L');

  Address = _Reset();
  r = SEGGER_RTT_printf(Address, 0u, "%s", acLong);
  TEST_ASSERT(r < 0);

  Address = _Reset();
  TEST_ASSERT(SEGGER_RTT_printf(Address, 0u, "%200u", 1u) < 0);
  Address = _Reset();
  TEST_ASSERT(SEGGER_RTT_printf(Address, 0u, "%-200u", 1u) < 0);
  Address = _Reset();
  TEST_ASSERT(SEGGER_RTT_printf(Address, 0u, "%200d", 1) < 0);
  Address = _Reset();
  TEST_ASSERT(SEGGER_RTT_printf(Address, 0u, "%0200d", 1) < 0);

#if defined(TEST_RTT_PRINTF_SMALL_BUFFER)
  Address = _Reset();
  pRing = _UpRing(Address, 0u);
  _SetRingOffsets(pRing, 0u, BUFFER_SIZE_UP - 4u);
  TEST_ASSERT(SEGGER_RTT_printf(Address, 0u, "%u", 123456u) < 0);
#endif

  Address = _Reset();
  TEST_ASSERT_EQ_I(0, SEGGER_RTT_ConfigUpBuffer(Address, 1u, _SharedString(Address, TEST_EXTRA_STR0_OFF, "Precision"), _SharedBuffer(Address, TEST_EXTRA_PRINTF_BUF_OFF, TEST_EXTRA_PRINTF_BUF_SIZE), TEST_EXTRA_PRINTF_BUF_SIZE, SEGGER_RTT_MODE_NO_BLOCK_SKIP));
  TEST_ASSERT(SEGGER_RTT_printf(Address, 1u, "[%.3d]", 1) > 0);
}

/*********************************************************************
*
*       main()
*
*  Function description
*    Runs all SEGGER RTT public API unit tests and returns a process
*    status suitable for CTest.
*
*  Parameters
*    None.
*
*  Return value
*      0 - All tests passed.
*    !=0 - At least one assertion failed.
*
**********************************************************************
*/

int main(void) {
  _TestInitAndFind();
  _TestEnsureInitExAPI();
  _TestUpBufferAPI();
  _TestDownBufferAPI();
  _TestConfigAndAllocAPI();
  _TestInvalidDescriptorAPI();
  _TestRingBoundaryAPI();
  _TestConfigErrorAPI();
  _TestTerminalAndPrintfAPI();

  if (_NumFailures != 0u) {
    printf("SEGGER_RTT_PublicAPI_Test failed: %u failure(s)\n", _NumFailures);
    return 1;
  }
  printf("SEGGER_RTT_PublicAPI_Test passed\n");
  return 0;
}

/*************************** End of file ****************************/
