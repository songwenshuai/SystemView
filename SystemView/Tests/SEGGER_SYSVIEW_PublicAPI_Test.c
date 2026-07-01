/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*         SEGGER SystemView  * Real-time application analysis        *
*              https://github.com/SEGGERMicro/SystemView             *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------

Purpose : Unit tests for the SEGGER SystemView public API.

          The test links the production SystemView implementation with
          a host-side RTT stub.  The stub captures packets and verifies
          that SystemView calls the shared-memory RTT API with the
          configured control-block base address.
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_Int.h"
#include "SEGGER_RTT.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define TEST_MAX_WRITES             512u
#define TEST_MAX_WRITE_SIZE         512u
#define TEST_DOWN_BUFFER_SIZE       64u
#define TEST_EXPECTED_RTT_ADDRESS   ((PTR_ADDR)SEGGER_SYSVIEW_RTT_CB_ADDRESS)
#define TEST_EXPECTED_RTT_NAME      ((const char*)(PTR_ADDR)SEGGER_SYSVIEW_RTT_NAME_ADDRESS)

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
  PTR_ADDR Address;
  unsigned  Channel;
  unsigned  NumBytes;
  U8        Data[TEST_MAX_WRITE_SIZE];
} TEST_RTT_WRITE;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static TEST_RTT_WRITE _Writes[TEST_MAX_WRITES];
static unsigned       _NumWrites;
static unsigned       _NumFailures;
static unsigned       _BadRTTAddressCount;
static unsigned       _BadRTTNameCount;
static unsigned       _BadRTTBufferCount;
static unsigned       _RTTAllocUpCount;
static unsigned       _RTTConfigUpCount;
static unsigned       _RTTConfigDownCount;
static unsigned       _RTTWriteSkipFailCount;
static unsigned       _NextRTTChannel;
static PTR_ADDR      _ExpectedUpBufferAddress;
static PTR_ADDR      _ExpectedDownBufferAddress;
static unsigned       _ExpectedUpBufferSize;
static unsigned       _ExpectedDownBufferSize;
static U8             _DownBytes[TEST_DOWN_BUFFER_SIZE];
static unsigned       _DownRdOff;
static unsigned       _DownWrOff;
static U32            _Timestamp;
static U32            _InterruptId;
static unsigned       _StartCallbackCount;
static unsigned       _StopCallbackCount;
static unsigned       _SysDescCallbackCount;
static unsigned       _TaskListCallbackCount;
static unsigned       _EventRecordedCount;
static unsigned       _StartCommCount;
static unsigned       _ConfCount;
static unsigned       _ModuleDescCallbackCount;

static SEGGER_SYSVIEW_MODULE _ModuleA;
static SEGGER_SYSVIEW_MODULE _ModuleB;

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
*    Records a failed assertion and prints the failing source location.
*
*  Parameters
*    sFile  Source file that contains the failed assertion.
*    Line   Source line that contains the failed assertion.
*    sExpr  Assertion expression text.
*/
static void _Fail(const char* sFile, int Line, const char* sExpr) {
  _NumFailures++;
  printf("%s:%d: assertion failed: %s\n", sFile, Line, sExpr);
}

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

#define TEST_ASSERT_EQ_ADDR(Expected, Actual)     \
  do {                                            \
    PTR_ADDR _Expected;                           \
    PTR_ADDR _Actual;                             \
    _Expected = (PTR_ADDR)(Expected);             \
    _Actual   = (PTR_ADDR)(Actual);               \
    if (_Expected != _Actual) {                   \
      _NumFailures++;                             \
      printf("%s:%d: expected 0x%llx, got 0x%llx\n", \
             __FILE__, __LINE__,                  \
             (unsigned long long)_Expected,       \
             (unsigned long long)_Actual);        \
    }                                             \
  } while (0)

#define TEST_EXPECT_EVENT(ExpectedEvent, ...)     \
  do {                                            \
    unsigned _BeforeWrites;                       \
    _BeforeWrites = _NumWrites;                   \
    __VA_ARGS__;                                  \
    _ExpectLastEventFrom(_BeforeWrites, (ExpectedEvent)); \
  } while (0)

#define TEST_EXPECT_EXT_EVENT(ExpectedExtEvent, ...) \
  do {                                               \
    unsigned _BeforeWrites;                          \
    _BeforeWrites = _NumWrites;                      \
    __VA_ARGS__;                                     \
    _ExpectLastExtEventFrom(_BeforeWrites, (ExpectedExtEvent)); \
  } while (0)

/*********************************************************************
*
*       RTT stub
*
**********************************************************************
*/

/*********************************************************************
*
*       _CheckRTTAddress()
*
*  Function description
*    Verifies that SystemView passed the configured RTT control-block
*    base address into a stubbed RTT API call.
*
*  Parameters
*    Address  RTT control-block base address supplied by SystemView.
*/
static void _CheckRTTAddress(PTR_ADDR Address) {
  if (Address != TEST_EXPECTED_RTT_ADDRESS) {
    _BadRTTAddressCount++;
  }
}

/*********************************************************************
*
*       _CheckRTTName()
*
*  Function description
*    Verifies that SystemView passed the configured RTT channel-name
*    address into a stubbed RTT configuration API call.
*
*  Parameters
*    sName  RTT buffer name pointer supplied by SystemView.
*/
static void _CheckRTTName(const char* sName) {
  if (sName != TEST_EXPECTED_RTT_NAME) {
    _BadRTTNameCount++;
  }
}

/*********************************************************************
*
*       _CheckRTTBuffer()
*
*  Function description
*    Verifies that SystemView passed an expected shared-memory RTT
*    buffer address and size into a stubbed RTT configuration API call.
*
*  Parameters
*    pBuffer          RTT buffer pointer supplied by SystemView.
*    BufferSize       RTT buffer size supplied by SystemView.
*    ExpectedAddress  Expected RTT buffer address.
*    ExpectedSize     Expected RTT buffer size.
*/
static void _CheckRTTBuffer(void* pBuffer, unsigned BufferSize, PTR_ADDR ExpectedAddress, unsigned ExpectedSize) {
  if (((PTR_ADDR)pBuffer != ExpectedAddress) || (BufferSize != ExpectedSize)) {
    _BadRTTBufferCount++;
  }
}

/*********************************************************************
*
*       _CaptureWrite()
*
*  Function description
*    Stores a packet written by SystemView through the RTT transport
*    stub so the test can inspect its event id and payload.
*
*  Parameters
*    Address   RTT control-block base address supplied by SystemView.
*    Channel   RTT channel selected by SystemView.
*    pBuffer   Pointer to the packet data to capture.
*    NumBytes  Number of packet bytes to capture.
*/
static void _CaptureWrite(PTR_ADDR Address, unsigned Channel, const void* pBuffer, unsigned NumBytes) {
  TEST_ASSERT(_NumWrites < TEST_MAX_WRITES);
  TEST_ASSERT(NumBytes <= TEST_MAX_WRITE_SIZE);
  if ((_NumWrites < TEST_MAX_WRITES) && (NumBytes <= TEST_MAX_WRITE_SIZE)) {
    _Writes[_NumWrites].Address  = Address;
    _Writes[_NumWrites].Channel  = Channel;
    _Writes[_NumWrites].NumBytes = NumBytes;
    memcpy(_Writes[_NumWrites].Data, pBuffer, NumBytes);
    _NumWrites++;
  }
}

/*********************************************************************
*
*       SEGGER_RTT_AllocUpBuffer()
*
*  Function description
*    RTT transport stub used by SEGGER_SYSVIEW_Init_Ex() to allocate an
*    up-buffer channel.
*
*  Return value
*    Allocated channel id.
*/
int SEGGER_RTT_AllocUpBuffer(PTR_ADDR Address, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags) {
  TEST_ASSERT_EQ_U(SEGGER_RTT_MODE_NO_BLOCK_SKIP, Flags);
  _CheckRTTAddress(Address);
  _CheckRTTName(sName);
  _CheckRTTBuffer(pBuffer, BufferSize, _ExpectedUpBufferAddress, _ExpectedUpBufferSize);
  _RTTAllocUpCount++;
  return (int)_NextRTTChannel++;
}

/*********************************************************************
*
*       SEGGER_RTT_ConfigUpBuffer()
*
*  Function description
*    RTT transport stub used when SystemView configures an explicit
*    up-buffer channel.
*
*  Return value
*    0 on success.
*/
int SEGGER_RTT_ConfigUpBuffer(PTR_ADDR Address, unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags) {
  (void)BufferIndex;
  TEST_ASSERT_EQ_U(SEGGER_RTT_MODE_NO_BLOCK_SKIP, Flags);
  _CheckRTTAddress(Address);
  _CheckRTTName(sName);
  _CheckRTTBuffer(pBuffer, BufferSize, _ExpectedUpBufferAddress, _ExpectedUpBufferSize);
  _RTTConfigUpCount++;
  return 0;
}

/*********************************************************************
*
*       SEGGER_RTT_ConfigDownBuffer()
*
*  Function description
*    RTT transport stub used when SystemView configures a down-buffer
*    channel for host commands.
*
*  Return value
*    0 on success.
*/
int SEGGER_RTT_ConfigDownBuffer(PTR_ADDR Address, unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags) {
  (void)BufferIndex;
  TEST_ASSERT_EQ_U(SEGGER_RTT_MODE_NO_BLOCK_SKIP, Flags);
  _CheckRTTAddress(Address);
  _CheckRTTName(sName);
  _CheckRTTBuffer(pBuffer, BufferSize, _ExpectedDownBufferAddress, _ExpectedDownBufferSize);
  _RTTConfigDownCount++;
  return 0;
}

/*********************************************************************
*
*       SEGGER_RTT_HasData()
*
*  Function description
*    Reports whether the test has queued host command bytes for the
*    SystemView down-channel.
*
*  Return value
*    Number of queued bytes.
*/
unsigned SEGGER_RTT_HasData(PTR_ADDR Address, unsigned BufferIndex) {
  (void)BufferIndex;
  _CheckRTTAddress(Address);
  return _DownWrOff - _DownRdOff;
}

/*********************************************************************
*
*       SEGGER_RTT_ReadNoLock()
*
*  Function description
*    Supplies queued host command bytes to SystemView.
*
*  Return value
*    Number of bytes copied to pData.
*/
unsigned SEGGER_RTT_ReadNoLock(PTR_ADDR Address, unsigned BufferIndex, void* pData, unsigned BufferSize) {
  unsigned NumBytes;

  (void)BufferIndex;
  _CheckRTTAddress(Address);
  NumBytes = _DownWrOff - _DownRdOff;
  if (NumBytes > BufferSize) {
    NumBytes = BufferSize;
  }
  memcpy(pData, &_DownBytes[_DownRdOff], NumBytes);
  _DownRdOff += NumBytes;
  if (_DownRdOff == _DownWrOff) {
    _DownRdOff = 0u;
    _DownWrOff = 0u;
  }
  return NumBytes;
}

/*********************************************************************
*
*       SEGGER_RTT_WriteSkipNoLock()
*
*  Function description
*    Captures a packet that SystemView attempted to send in skip mode.
*
*  Return value
*    Number of bytes accepted by the stub.
*/
unsigned SEGGER_RTT_WriteSkipNoLock(PTR_ADDR Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  _CheckRTTAddress(Address);
  if (_RTTWriteSkipFailCount > 0u) {
    _RTTWriteSkipFailCount--;
    return 0u;
  }
  _CaptureWrite(Address, BufferIndex, pBuffer, NumBytes);
  return NumBytes;
}

/*********************************************************************
*
*       SEGGER_RTT_WriteWithOverwriteNoLock()
*
*  Function description
*    Captures a packet that SystemView attempted to send in overwrite
*    mode.
*/
void SEGGER_RTT_WriteWithOverwriteNoLock(PTR_ADDR Address, unsigned BufferIndex, const void* pBuffer, unsigned NumBytes) {
  _CheckRTTAddress(Address);
  _CaptureWrite(Address, BufferIndex, pBuffer, NumBytes);
}

/*********************************************************************
*
*       Application-provided SystemView hooks
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_GetTimestamp()
*
*  Function description
*    Test timestamp provider used by SEGGER_SYSVIEW_GET_TIMESTAMP().
*
*  Return value
*    Monotonic test timestamp.
*/
U32 SEGGER_SYSVIEW_X_GetTimestamp(void) {
  _Timestamp += 10u;
  return _Timestamp;
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_GetInterruptId()
*
*  Function description
*    Test interrupt-id provider used by SEGGER_SYSVIEW_GET_INTERRUPT_ID().
*
*  Return value
*    Configured test interrupt id.
*/
U32 SEGGER_SYSVIEW_X_GetInterruptId(void) {
  return _InterruptId;
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_StartComm()
*
*  Function description
*    Test communication-start hook.
*/
void SEGGER_SYSVIEW_X_StartComm(void) {
  _StartCommCount++;
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_X_OnEventRecorded()
*
*  Function description
*    Test event-recorded hook used by SEGGER_SYSVIEW_ON_EVENT_RECORDED().
*
*  Parameters
*    NumBytes  Number of bytes reported by SystemView.
*/
void SEGGER_SYSVIEW_X_OnEventRecorded(unsigned NumBytes) {
  (void)NumBytes;
  _EventRecordedCount++;
}

/*********************************************************************
*
*       SEGGER_SYSVIEW_Conf()
*
*  Function description
*    Test configuration hook.
*/
void SEGGER_SYSVIEW_Conf(void) {
  _ConfCount++;
}

/*********************************************************************
*
*       _OSGetTime()
*
*  Function description
*    Test OS time provider.
*
*  Return value
*    Constant 64-bit OS time used by systime tests.
*/
static U64 _OSGetTime(void) {
  return 0x0000000200000001ULL;
}

/*********************************************************************
*
*       _SendTaskListCallback()
*
*  Function description
*    Test OS task-list callback.  Emits one task descriptor through the
*    public SystemView API.
*/
static void _SendTaskListCallback(void) {
  SEGGER_SYSVIEW_TASKINFO Info;

  _TaskListCallbackCount++;
  memset(&Info, 0, sizeof(Info));
  Info.TaskID = 0x100u;
  Info.sName  = "TaskA";
  Info.Prio   = 5u;
  SEGGER_SYSVIEW_SendTaskInfo(&Info);
}

/*********************************************************************
*
*       _SendSysDescCallback()
*
*  Function description
*    Test system-description callback.  Emits one SystemView system
*    description string through the public API.
*/
static void _SendSysDescCallback(void) {
  _SysDescCallbackCount++;
  SEGGER_SYSVIEW_SendSysDesc("N=UnitTest,D=Host");
}

/*********************************************************************
*
*       _StartCallback()
*
*  Function description
*    Records that the SystemView start callback was called.
*/
static void _StartCallback(void) {
  _StartCallbackCount++;
}

/*********************************************************************
*
*       _StopCallback()
*
*  Function description
*    Records that the SystemView stop callback was called.
*/
static void _StopCallback(void) {
  _StopCallbackCount++;
}

static const SEGGER_SYSVIEW_OS_API _OSAPI = {
  _OSGetTime,
  _SendTaskListCallback
};

/*********************************************************************
*
*       Packet helpers
*
**********************************************************************
*/

/*********************************************************************
*
*       _ClearWrites()
*
*  Function description
*    Drops all captured RTT writes without changing the stub
*    configuration.
*/
static void _ClearWrites(void) {
  _NumWrites = 0u;
}

/*********************************************************************
*
*       _ResetRTTStub()
*
*  Function description
*    Resets the RTT transport stub to a known state before a test case.
*    Cumulative validation failures are preserved for the final result.
*/
static void _ResetRTTStub(void) {
  _ClearWrites();
  _RTTAllocUpCount    = 0u;
  _RTTConfigUpCount   = 0u;
  _RTTConfigDownCount = 0u;
  _RTTWriteSkipFailCount = 0u;
  _NextRTTChannel     = 1u;
  _ExpectedUpBufferAddress   = (PTR_ADDR)SEGGER_SYSVIEW_RTT_UP_BUFFER_ADDRESS;
  _ExpectedDownBufferAddress = (PTR_ADDR)SEGGER_SYSVIEW_RTT_DOWN_BUFFER_ADDRESS;
  _ExpectedUpBufferSize      = SEGGER_SYSVIEW_RTT_BUFFER_SIZE;
  _ExpectedDownBufferSize    = SEGGER_SYSVIEW_RTT_DOWN_BUFFER_SIZE;
  _DownRdOff          = 0u;
  _DownWrOff          = 0u;
  _Timestamp          = 100u;
  _InterruptId        = 33u;
  _EventRecordedCount = 0u;
}

/*********************************************************************
*
*       _ResetCallbacks()
*
*  Function description
*    Clears all callback counters.
*/
static void _ResetCallbacks(void) {
  _StartCallbackCount      = 0u;
  _StopCallbackCount       = 0u;
  _SysDescCallbackCount    = 0u;
  _TaskListCallbackCount   = 0u;
  _StartCommCount          = 0u;
  _ConfCount               = 0u;
  _ModuleDescCallbackCount = 0u;
}

/*********************************************************************
*
*       _QueueDownByte()
*
*  Function description
*    Queues one byte that will be returned by SEGGER_RTT_ReadNoLock().
*
*  Parameters
*    Data  Byte to queue for the SystemView down-channel.
*/
static void _QueueDownByte(U8 Data) {
  TEST_ASSERT(_DownWrOff < TEST_DOWN_BUFFER_SIZE);
  if (_DownWrOff < TEST_DOWN_BUFFER_SIZE) {
    _DownBytes[_DownWrOff++] = Data;
  }
}

/*********************************************************************
*
*       _DecodeU32()
*
*  Function description
*    Decodes a SystemView variable-length U32 value.
*
*  Parameters
*    pData     Pointer to encoded data.
*    NumBytes  Number of available encoded bytes.
*    pNumUsed  Receives the number of consumed bytes.
*
*  Return value
*    Decoded value.
*/
static U32 _DecodeU32(const U8* pData, unsigned NumBytes, unsigned* pNumUsed) {
  U32 Value;
  unsigned Shift;
  unsigned NumUsed;
  U8 Data;

  Value = 0u;
  Shift = 0u;
  NumUsed = 0u;
  do {
    TEST_ASSERT(NumUsed < NumBytes);
    if (NumUsed >= NumBytes) {
      break;
    }
    Data = pData[NumUsed++];
    Value |= ((U32)(Data & 0x7Fu) << Shift);
    Shift += 7u;
  } while ((Data & 0x80u) != 0u);
  *pNumUsed = NumUsed;
  return Value;
}

/*********************************************************************
*
*       _DecodeAddr()
*
*  Function description
*    Decodes a SystemView variable-length address-sized value.
*
*  Parameters
*    pData     Pointer to encoded data.
*    NumBytes  Number of available encoded bytes.
*    pNumUsed  Receives the number of consumed bytes.
*
*  Return value
*    Decoded value.
*/
static PTR_ADDR _DecodeAddr(const U8* pData, unsigned NumBytes, unsigned* pNumUsed) {
  PTR_ADDR Value;
  unsigned Shift;
  unsigned NumUsed;
  U8 Data;

  Value = 0u;
  Shift = 0u;
  NumUsed = 0u;
  do {
    TEST_ASSERT(NumUsed < NumBytes);
    if (NumUsed >= NumBytes) {
      break;
    }
    Data = pData[NumUsed++];
    Value |= ((PTR_ADDR)(Data & 0x7Fu) << Shift);
    Shift += 7u;
  } while ((Data & 0x80u) != 0u);
  *pNumUsed = NumUsed;
  return Value;
}

/*********************************************************************
*
*       _WriteEventId()
*
*  Function description
*    Reads the event id from a captured RTT write.
*
*  Parameters
*    WriteIndex  Index into the captured write table.
*
*  Return value
*    Decoded SystemView event id.
*/
static U32 _WriteEventId(unsigned WriteIndex) {
  unsigned NumUsed;

  TEST_ASSERT(WriteIndex < _NumWrites);
  return _DecodeU32(_Writes[WriteIndex].Data, _Writes[WriteIndex].NumBytes, &NumUsed);
}

/*********************************************************************
*
*       _WritePayload()
*
*  Function description
*    Returns a pointer to the payload bytes of a captured SystemView
*    packet, skipping event id and optional length fields.
*
*  Parameters
*    WriteIndex   Index into the captured write table.
*    pPayloadLen  Receives the number of payload bytes.
*
*  Return value
*    Pointer to the first payload byte.
*/
static const U8* _WritePayload(unsigned WriteIndex, unsigned* pPayloadLen) {
  const U8* pData;
  unsigned NumBytes;
  unsigned NumUsed;
  unsigned HeaderBytes;
  U32 EventId;

  pData = _Writes[WriteIndex].Data;
  NumBytes = _Writes[WriteIndex].NumBytes;
  EventId = _DecodeU32(pData, NumBytes, &NumUsed);
  HeaderBytes = NumUsed;
  if (EventId >= 24u) {
    (void)_DecodeU32(pData + HeaderBytes, NumBytes - HeaderBytes, &NumUsed);
    HeaderBytes += NumUsed;
  }
  TEST_ASSERT(HeaderBytes <= NumBytes);
  *pPayloadLen = NumBytes - HeaderBytes;
  return pData + HeaderBytes;
}

/*********************************************************************
*
*       _ExpectPayloadFieldsConsumed()
*
*  Function description
*    Verifies that all event payload fields were decoded and only the
*    trailing timestamp delta remains in the captured packet.
*/
static void _ExpectPayloadFieldsConsumed(unsigned PayloadLen, unsigned PayloadOff) {
  TEST_ASSERT(PayloadOff <= PayloadLen);
  if (PayloadOff <= PayloadLen) {
    TEST_ASSERT((PayloadLen - PayloadOff) <= SEGGER_SYSVIEW_QUANTA_U32);
  }
}

/*********************************************************************
*
*       _FindLastWriteFrom()
*
*  Function description
*    Finds the last non-empty captured write after a given index.
*
*  Parameters
*    StartIndex  First captured write index to consider.
*
*  Return value
*    Write index, or -1 if no write was captured.
*/
static int _FindLastWriteFrom(unsigned StartIndex) {
  unsigned i;

  i = _NumWrites;
  while (i > StartIndex) {
    i--;
    if (_Writes[i].NumBytes > 0u) {
      return (int)i;
    }
  }
  return -1;
}

/*********************************************************************
*
*       _FindEventFrom()
*
*  Function description
*    Finds the first captured write after a given index with the
*    expected SystemView event id.
*
*  Parameters
*    StartIndex     First captured write index to consider.
*    ExpectedEvent  SystemView event id to find.
*
*  Return value
*    Write index, or -1 if no matching write was captured.
*/
static int _FindEventFrom(unsigned StartIndex, unsigned ExpectedEvent) {
  unsigned i;

  for (i = StartIndex; i < _NumWrites; i++) {
    if (_WriteEventId(i) == ExpectedEvent) {
      return (int)i;
    }
  }
  return -1;
}

/*********************************************************************
*
*       _ExpectLastEventFrom()
*
*  Function description
*    Verifies that the last captured write after StartIndex has the
*    expected SystemView event id.
*
*  Return value
*    Index of the verified captured write.
*/
static unsigned _ExpectLastEventFrom(unsigned StartIndex, unsigned ExpectedEvent) {
  int WriteIndex;

  WriteIndex = _FindLastWriteFrom(StartIndex);
  TEST_ASSERT(WriteIndex >= 0);
  if (WriteIndex >= 0) {
    TEST_ASSERT_EQ_U(ExpectedEvent, _WriteEventId((unsigned)WriteIndex));
    return (unsigned)WriteIndex;
  }
  return StartIndex;
}

/*********************************************************************
*
*       _ExpectEventInRange()
*
*  Function description
*    Verifies that at least one captured write after StartIndex has the
*    expected SystemView event id.
*/
static void _ExpectEventInRange(unsigned StartIndex, unsigned ExpectedEvent) {
  int WriteIndex;

  WriteIndex = _FindEventFrom(StartIndex, ExpectedEvent);
  TEST_ASSERT(WriteIndex >= 0);
}

/*********************************************************************
*
*       _ExpectLastExtEventFrom()
*
*  Function description
*    Verifies that the last captured write is an extended SystemView
*    packet with the expected extended event id.
*
*  Return value
*    Index of the verified captured write.
*/
static unsigned _ExpectLastExtEventFrom(unsigned StartIndex, unsigned ExpectedExtEvent) {
  const U8* pPayload;
  unsigned PayloadLen;
  unsigned NumUsed;
  unsigned WriteIndex;
  U32 ExtEvent;

  WriteIndex = _ExpectLastEventFrom(StartIndex, SYSVIEW_EVTID_EX);
  pPayload = _WritePayload(WriteIndex, &PayloadLen);
  ExtEvent = _DecodeU32(pPayload, PayloadLen, &NumUsed);
  TEST_ASSERT_EQ_U(ExpectedExtEvent, ExtEvent);
  return WriteIndex;
}

/*********************************************************************
*
*       _ExpectRecordIdPayload()
*
*  Function description
*    Verifies that a captured packet payload starts with an
*    address-sized ID followed by the expected U32 values.
*/
static void _ExpectRecordIdPayload(unsigned WriteIndex, PTR_ADDR ExpectedId, const U32* pExpectedU32, unsigned NumExpectedU32) {
  const U8* pPayload;
  unsigned PayloadLen;
  unsigned PayloadOff;
  unsigned NumUsed;
  unsigned i;
  PTR_ADDR DecodedAddr;
  U32 DecodedU32;

  pPayload = _WritePayload(WriteIndex, &PayloadLen);
  PayloadOff = 0u;
  DecodedAddr = _DecodeAddr(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_ADDR(ExpectedId, DecodedAddr);
  for (i = 0u; i < NumExpectedU32; i++) {
    DecodedU32 = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
    PayloadOff += NumUsed;
    TEST_ASSERT_EQ_U(pExpectedU32[i], DecodedU32);
  }
  _ExpectPayloadFieldsConsumed(PayloadLen, PayloadOff);
}

/*********************************************************************
*
*       _ExpectNoNewWrites()
*
*  Function description
*    Verifies that no new RTT writes were captured after StartIndex.
*/
static void _ExpectNoNewWrites(unsigned StartIndex) {
  TEST_ASSERT_EQ_U(StartIndex, _NumWrites);
}

/*********************************************************************
*
*       _ExpectMem()
*
*  Function description
*    Compares an actual byte sequence with an expected byte sequence.
*/
static void _ExpectMem(const void* pActual, const void* pExpected, unsigned NumBytes) {
  TEST_ASSERT(memcmp(pActual, pExpected, NumBytes) == 0);
}

/*********************************************************************
*
*       _ExpectEncodedString()
*
*  Function description
*    Verifies one SystemView length-prefixed string in a packet payload.
*/
static void _ExpectEncodedString(const U8* pPayload, unsigned PayloadLen, unsigned* pPayloadOff, const char* sExpected, unsigned MaxLen) {
  unsigned ExpectedLen;
  unsigned Len;

  ExpectedLen = (sExpected != NULL) ? (unsigned)strlen(sExpected) : 0u;
  if (ExpectedLen > MaxLen) {
    ExpectedLen = MaxLen;
  }
  TEST_ASSERT(*pPayloadOff < PayloadLen);
  if (*pPayloadOff >= PayloadLen) {
    return;
  }
  Len = pPayload[*pPayloadOff];
  (*pPayloadOff)++;
  TEST_ASSERT_EQ_U(ExpectedLen, Len);
  TEST_ASSERT((*pPayloadOff + Len) <= PayloadLen);
  if (((*pPayloadOff + Len) <= PayloadLen) && (Len > 0u) && (sExpected != NULL)) {
    _ExpectMem(pPayload + *pPayloadOff, sExpected, Len);
  }
  *pPayloadOff += Len;
}

/*********************************************************************
*
*       _ExpectTaskInfoPayload()
*
*  Function description
*    Verifies the payload emitted by SEGGER_SYSVIEW_SendTaskInfo() for
*    the task descriptor packet.
*/
static void _ExpectTaskInfoPayload(unsigned WriteIndex, const SEGGER_SYSVIEW_TASKINFO* pExpected) {
  const U8* pPayload;
  unsigned PayloadLen;
  unsigned PayloadOff;
  unsigned NumUsed;
  PTR_ADDR DecodedAddr;
  U32 DecodedU32;

  pPayload = _WritePayload(WriteIndex, &PayloadLen);
  PayloadOff = 0u;
  DecodedAddr = _DecodeAddr(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_ADDR(SEGGER_SYSVIEW_ShrinkId(pExpected->TaskID), DecodedAddr);
  DecodedU32 = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_U(pExpected->Prio, DecodedU32);
  _ExpectEncodedString(pPayload, PayloadLen, &PayloadOff, pExpected->sName, 32u);
  _ExpectPayloadFieldsConsumed(PayloadLen, PayloadOff);
}

/*********************************************************************
*
*       _ExpectStackInfoPayload()
*
*  Function description
*    Verifies the payload emitted by stack-info packets.
*/
static void _ExpectStackInfoPayload(unsigned WriteIndex, PTR_ADDR TaskID, PTR_ADDR StackBase, U32 StackSize, U32 StackUsage) {
  const U8* pPayload;
  unsigned PayloadLen;
  unsigned PayloadOff;
  unsigned NumUsed;
  PTR_ADDR DecodedAddr;
  U32 DecodedU32;

  pPayload = _WritePayload(WriteIndex, &PayloadLen);
  PayloadOff = 0u;
  DecodedAddr = _DecodeAddr(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_ADDR(SEGGER_SYSVIEW_ShrinkId(TaskID), DecodedAddr);
  DecodedAddr = _DecodeAddr(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_ADDR(StackBase, DecodedAddr);
  DecodedU32 = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_U(StackSize, DecodedU32);
  DecodedU32 = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_U(StackUsage, DecodedU32);
  _ExpectPayloadFieldsConsumed(PayloadLen, PayloadOff);
}

/*********************************************************************
*
*       _ExpectPrintElfPayload()
*
*  Function description
*    Verifies the common payload emitted by PrintElf APIs and macros.
*/
static void _ExpectPrintElfPayload(unsigned WriteIndex, U32 ExpectedOptions, const U32* pExpectedU32, unsigned NumExpectedU32, const char* const* psExpectedStr, unsigned NumExpectedStr) {
  const U8* pPayload;
  unsigned PayloadLen;
  unsigned PayloadOff;
  unsigned NumUsed;
  unsigned i;
  PTR_ADDR DecodedAddr;
  U32 DecodedU32;

  pPayload = _WritePayload(WriteIndex, &PayloadLen);
  PayloadOff = 0u;
  DecodedU32 = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_U(SYSVIEW_EVTID_EX_PRINT_ELF, DecodedU32);
  DecodedAddr = _DecodeAddr(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT(DecodedAddr != 0u);
  DecodedU32 = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
  PayloadOff += NumUsed;
  TEST_ASSERT_EQ_U(ExpectedOptions, DecodedU32);
  for (i = 0u; i < NumExpectedU32; i++) {
    DecodedU32 = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
    PayloadOff += NumUsed;
    TEST_ASSERT_EQ_U(pExpectedU32[i], DecodedU32);
  }
  for (i = 0u; i < NumExpectedStr; i++) {
    _ExpectEncodedString(pPayload, PayloadLen, &PayloadOff, psExpectedStr[i], SEGGER_SYSVIEW_MAX_STRING_LEN);
  }
  _ExpectPayloadFieldsConsumed(PayloadLen, PayloadOff);
}

/*********************************************************************
*
*       _ExpectLastPrintElfFrom()
*
*  Function description
*    Verifies the last captured extended PrintElf packet after a given
*    write index.
*/
static void _ExpectLastPrintElfFrom(unsigned StartIndex, U32 ExpectedOptions, const U32* pExpectedU32, unsigned NumExpectedU32, const char* const* psExpectedStr, unsigned NumExpectedStr) {
  unsigned WriteIndex;

  WriteIndex = _ExpectLastExtEventFrom(StartIndex, SYSVIEW_EVTID_EX_PRINT_ELF);
  _ExpectPrintElfPayload(WriteIndex, ExpectedOptions, pExpectedU32, NumExpectedU32, psExpectedStr, NumExpectedStr);
}

/*********************************************************************
*
*       _InitNoStart()
*
*  Function description
*    Initializes SystemView for a test case but leaves tracing stopped.
*/
static void _InitNoStart(const SEGGER_SYSVIEW_OS_API* pOSAPI) {
  _ResetRTTStub();
  SEGGER_SYSVIEW_Init_Ex(1000u, 2000u, pOSAPI, _SendSysDescCallback, _StartCallback, _StopCallback);
  SEGGER_SYSVIEW_EnableEvents(0xFFFFFFFFu);
}

/*********************************************************************
*
*       _InitAndStart()
*
*  Function description
*    Initializes SystemView, starts tracing, and clears startup packets
*    so the test can inspect only the packets produced by the API under
*    test.
*/
static void _InitAndStart(const SEGGER_SYSVIEW_OS_API* pOSAPI) {
  _InitNoStart(pOSAPI);
  SEGGER_SYSVIEW_Start();
  TEST_ASSERT_EQ_I(1, SEGGER_SYSVIEW_IsStarted());
  _ClearWrites();
}

/*********************************************************************
*
*       Vararg adapters
*
**********************************************************************
*/

/*********************************************************************
*
*       _CallVPrintfHostEx()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VPrintfHostEx(), whose public
*    API receives a va_list pointer.
*/
static void _CallVPrintfHostEx(const char* s, U32 Options, ...) {
  va_list ParamList;

  va_start(ParamList, Options);
  SEGGER_SYSVIEW_VPrintfHostEx(s, Options, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       _CallVPrintfTargetEx()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VPrintfTargetEx().
*/
static void _CallVPrintfTargetEx(const char* s, U32 Options, ...) {
  va_list ParamList;

  va_start(ParamList, Options);
  SEGGER_SYSVIEW_VPrintfTargetEx(s, Options, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       _CallVPrintfHost()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VPrintfHost().
*/
static void _CallVPrintfHost(const char* s, ...) {
  va_list ParamList;

  va_start(ParamList, s);
  SEGGER_SYSVIEW_VPrintfHost(s, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       _CallVPrintfTarget()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VPrintfTarget().
*/
static void _CallVPrintfTarget(const char* s, ...) {
  va_list ParamList;

  va_start(ParamList, s);
  SEGGER_SYSVIEW_VPrintfTarget(s, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       _CallVWarnfHost()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VWarnfHost().
*/
static void _CallVWarnfHost(const char* s, ...) {
  va_list ParamList;

  va_start(ParamList, s);
  SEGGER_SYSVIEW_VWarnfHost(s, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       _CallVWarnfTarget()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VWarnfTarget().
*/
static void _CallVWarnfTarget(const char* s, ...) {
  va_list ParamList;

  va_start(ParamList, s);
  SEGGER_SYSVIEW_VWarnfTarget(s, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       _CallVErrorfHost()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VErrorfHost().
*/
static void _CallVErrorfHost(const char* s, ...) {
  va_list ParamList;

  va_start(ParamList, s);
  SEGGER_SYSVIEW_VErrorfHost(s, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       _CallVErrorfTarget()
*
*  Function description
*    Adapter to exercise SEGGER_SYSVIEW_VErrorfTarget().
*/
static void _CallVErrorfTarget(const char* s, ...) {
  va_list ParamList;

  va_start(ParamList, s);
  SEGGER_SYSVIEW_VErrorfTarget(s, &ParamList);
  va_end(ParamList);
}

/*********************************************************************
*
*       Module callbacks
*
**********************************************************************
*/

/*********************************************************************
*
*       _ModuleADesc()
*
*  Function description
*    Sends the detailed description for module A.
*/
static void _ModuleADesc(void) {
  _ModuleDescCallbackCount++;
  SEGGER_SYSVIEW_RecordModuleDescription(&_ModuleA, "ModuleA.Detail");
}

/*********************************************************************
*
*       _ModuleBDesc()
*
*  Function description
*    Sends the detailed description for module B.
*/
static void _ModuleBDesc(void) {
  _ModuleDescCallbackCount++;
  SEGGER_SYSVIEW_RecordModuleDescription(&_ModuleB, "ModuleB.Detail");
}

/*********************************************************************
*
*       Test cases
*
**********************************************************************
*/

/*********************************************************************
*
*       _TestApplicationHooks()
*
*  Function description
*    Exercises the application-provided hook functions required by
*    SystemView.
*/
static void _TestApplicationHooks(void) {
  _ResetCallbacks();
  SEGGER_SYSVIEW_Conf();
  SEGGER_SYSVIEW_X_StartComm();
  SEGGER_SYSVIEW_X_OnEventRecorded(7u);
  TEST_ASSERT_EQ_U(1u, _ConfCount);
  TEST_ASSERT_EQ_U(1u, _StartCommCount);
  TEST_ASSERT_EQ_U(1u, _EventRecordedCount);
}

/*********************************************************************
*
*       _TestEncodeAPI()
*
*  Function description
*    Verifies public data, string, id, and U32 encoding helpers.
*/
static void _TestEncodeAPI(void) {
  U8 ac[320];
  char acLong[260];
  U8* p;
  unsigned i;
  static const U8 _abU32_500[] = { 0xF4u, 0x03u };
  static const U8 _abU32_Max[] = { 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x0Fu };

  memset(ac, 0, sizeof(ac));
  p = SEGGER_SYSVIEW_EncodeU32(ac, 500u);
  TEST_ASSERT_EQ_U(2u, (unsigned)(p - ac));
  _ExpectMem(ac, _abU32_500, sizeof(_abU32_500));

  p = SEGGER_SYSVIEW_EncodeU32(ac, 0xFFFFFFFFu);
  TEST_ASSERT_EQ_U(5u, (unsigned)(p - ac));
  _ExpectMem(ac, _abU32_Max, sizeof(_abU32_Max));

  p = SEGGER_SYSVIEW_EncodeData(ac, "abc", 3u);
  TEST_ASSERT_EQ_U(4u, (unsigned)(p - ac));
  TEST_ASSERT_EQ_U(3u, ac[0]);
  _ExpectMem(&ac[1], "abc", 3u);

  for (i = 0u; i < sizeof(acLong); i++) {
    acLong[i] = (char)('A' + (i % 26u));
  }
  p = SEGGER_SYSVIEW_EncodeData(ac, acLong, sizeof(acLong));
  TEST_ASSERT_EQ_U(sizeof(acLong) + 3u, (unsigned)(p - ac));
  TEST_ASSERT_EQ_U(255u, ac[0]);
  TEST_ASSERT_EQ_U(1u, ac[1]);
  TEST_ASSERT_EQ_U(4u, ac[2]);
  _ExpectMem(&ac[3], acLong, sizeof(acLong));

  p = SEGGER_SYSVIEW_EncodeString(ac, "abcdef", 3u);
  TEST_ASSERT_EQ_U(4u, (unsigned)(p - ac));
  TEST_ASSERT_EQ_U(3u, ac[0]);
  _ExpectMem(&ac[1], "abc", 3u);

  for (i = 0u; i < SEGGER_SYSVIEW_MAX_STRING_LEN + 8u; i++) {
    acLong[i] = (char)('a' + (i % 26u));
  }
  acLong[SEGGER_SYSVIEW_MAX_STRING_LEN + 8u] = '\0';
  p = SEGGER_SYSVIEW_EncodeString(ac, acLong, SEGGER_SYSVIEW_MAX_STRING_LEN + 8u);
  TEST_ASSERT_EQ_U(SEGGER_SYSVIEW_MAX_STRING_LEN + 1u, (unsigned)(p - ac));
  TEST_ASSERT_EQ_U(SEGGER_SYSVIEW_MAX_STRING_LEN, ac[0]);

  p = SEGGER_SYSVIEW_EncodeString(ac, NULL, 10u);
  TEST_ASSERT_EQ_U(1u, (unsigned)(p - ac));
  TEST_ASSERT_EQ_U(0u, ac[0]);

  SEGGER_SYSVIEW_SetRAMBase(0x1000u);
  TEST_ASSERT_EQ_U(0x24u, SEGGER_SYSVIEW_ShrinkId(0x1024u));
  p = SEGGER_SYSVIEW_EncodeId(ac, 0x1024u);
  TEST_ASSERT_EQ_U(1u, (unsigned)(p - ac));
  TEST_ASSERT_EQ_U(0x24u, ac[0]);
  SEGGER_SYSVIEW_SetRAMBase(0u);
}

/*********************************************************************
*
*       _TestInitAndControlAPI()
*
*  Function description
*    Verifies initialization, channel selection, start/stop, callback,
*    and additional-buffer APIs.
*/
static void _TestInitAndControlAPI(void) {
  SEGGER_SYSVIEW_CORE_CONTEXT* pMainContext;
  SEGGER_SYSVIEW_CORE_CONTEXT ExtraContext;
  PTR_ADDR ExtraUpBufferAddress;
  PTR_ADDR ExtraDownBufferAddress;

  _ResetCallbacks();
  _ResetRTTStub();
  SEGGER_SYSVIEW_Init(111u, 222u, NULL, NULL);
  pMainContext = SEGGER_SYSVIEW_GetMainContext();
  TEST_ASSERT_EQ_U(111u, pMainContext->SysFreq);
  TEST_ASSERT_EQ_U(222u, pMainContext->CPUFreq);
  TEST_ASSERT_EQ_U(1u, pMainContext->UpChannel);
  TEST_ASSERT_EQ_U(1u, pMainContext->DownChannel);
  TEST_ASSERT_EQ_I(1, SEGGER_SYSVIEW_GetChannelID());
  TEST_ASSERT_EQ_I(0, SEGGER_SYSVIEW_IsStarted());

  _ResetRTTStub();
  SEGGER_SYSVIEW_Init_Ex(1000u, 2000u, &_OSAPI, _SendSysDescCallback, _StartCallback, _StopCallback);
  SEGGER_SYSVIEW_EnableEvents(0xFFFFFFFFu);
  pMainContext = SEGGER_SYSVIEW_GetMainContext();
  TEST_ASSERT_EQ_U(1000u, pMainContext->SysFreq);
  TEST_ASSERT_EQ_U(2000u, pMainContext->CPUFreq);
  TEST_ASSERT_EQ_U(1u, pMainContext->UpChannel);
  TEST_ASSERT_EQ_U(1u, pMainContext->DownChannel);
  TEST_ASSERT_EQ_U(1u, _RTTAllocUpCount);
  TEST_ASSERT_EQ_U(1u, _RTTConfigDownCount);

  ExtraUpBufferAddress = 0x12340400u;
  ExtraDownBufferAddress = 0x12340500u;
  _ExpectedUpBufferAddress = ExtraUpBufferAddress;
  _ExpectedDownBufferAddress = ExtraDownBufferAddress;
  _ExpectedUpBufferSize = 32u;
  _ExpectedDownBufferSize = 8u;
  memset(&ExtraContext, 0, sizeof(ExtraContext));
  SEGGER_SYSVIEW_InitAdditionalBuffer(&ExtraContext, ExtraUpBufferAddress, 32u, ExtraDownBufferAddress, 8u);
  TEST_ASSERT_EQ_U(2u, ExtraContext.UpChannel);
  TEST_ASSERT_EQ_U(2u, _RTTAllocUpCount);
  TEST_ASSERT_EQ_U(2u, _RTTConfigDownCount);

  SEGGER_SYSVIEW_Start();
  TEST_ASSERT_EQ_I(1, SEGGER_SYSVIEW_IsStarted());
  TEST_ASSERT_EQ_U(1u, _StartCallbackCount);
  TEST_ASSERT(_SysDescCallbackCount > 0u);
  TEST_ASSERT(_TaskListCallbackCount > 0u);
  TEST_ASSERT(_EventRecordedCount > 0u);

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TRACE_STOP, SEGGER_SYSVIEW_Stop());
  TEST_ASSERT_EQ_I(0, SEGGER_SYSVIEW_IsStarted());
  TEST_ASSERT_EQ_U(1u, _StopCallbackCount);

  ExtraContext.SysFreq = 500u;
  ExtraContext.CPUFreq = 600u;
  ExtraContext.RAMBaseAddress = 0u;
  ExtraContext.EnableState = 0u;
  _ClearWrites();
  SEGGER_SYSVIEW_Start_Ex(&ExtraContext, 77u);
  TEST_ASSERT_EQ_U(1u, ExtraContext.EnableState);
  TEST_ASSERT(_NumWrites > 0u);
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TRACE_STOP, SEGGER_SYSVIEW_Stop_Ex(&ExtraContext));
  TEST_ASSERT_EQ_U(0u, ExtraContext.EnableState);
}

/*********************************************************************
*
*       _TestHostCommandAPI()
*
*  Function description
*    Exercises host command handling through the RTT down-channel and
*    SEGGER_SYSVIEW_IsStarted().
*/
static void _TestHostCommandAPI(void) {
  _ResetCallbacks();
  _InitNoStart(&_OSAPI);

  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_START);
  TEST_ASSERT_EQ_I(1, SEGGER_SYSVIEW_IsStarted());
  TEST_ASSERT_EQ_U(1u, _StartCallbackCount);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_GET_SYSTIME);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectLastEventFrom(0u, SYSVIEW_EVTID_SYSTIME_US);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_GET_TASKLIST);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectLastEventFrom(0u, SYSVIEW_EVTID_STACK_INFO);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_GET_SYSDESC);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectLastEventFrom(0u, SYSVIEW_EVTID_SYSDESC);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_GET_NUMMODULES);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectLastEventFrom(0u, SYSVIEW_EVTID_NUMMODULES);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_GET_MODULEDESC);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectNoNewWrites(0u);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_GET_MODULE);
  _QueueDownByte(0u);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectNoNewWrites(0u);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_HEARTBEAT);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectNoNewWrites(0u);

  _ClearWrites();
  _QueueDownByte(200u);
  _QueueDownByte(1u);
  (void)SEGGER_SYSVIEW_IsStarted();
  _ExpectNoNewWrites(0u);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_GET_NUMMODULES);
  SEGGER_SYSVIEW_RecordVoid(60u);
  _ExpectLastEventFrom(0u, SYSVIEW_EVTID_NUMMODULES);

  _ClearWrites();
  _QueueDownByte(SEGGER_SYSVIEW_COMMAND_ID_STOP);
  TEST_ASSERT_EQ_I(0, SEGGER_SYSVIEW_IsStarted());
  _ExpectLastEventFrom(0u, SYSVIEW_EVTID_TRACE_STOP);
}

/*********************************************************************
*
*       _TestTaskAndDescriptionAPI()
*
*  Function description
*    Verifies task, stack, system-description, task-list, and data
*    sampling APIs.
*/
static void _TestTaskAndDescriptionAPI(void) {
  SEGGER_SYSVIEW_TASKINFO TaskInfo;
  SEGGER_SYSVIEW_STACKINFO StackInfo;
  SEGGER_SYSVIEW_DATA_SAMPLE Sample;
  U32 SampleValue;

  _InitAndStart(&_OSAPI);

  memset(&TaskInfo, 0, sizeof(TaskInfo));
  TaskInfo.TaskID = 0x120u;
  TaskInfo.sName = "TaskB";
  TaskInfo.Prio = 3u;
  TaskInfo.StackBase = 0x300u;
  TaskInfo.StackSize = 0x90u;
  TaskInfo.StackUsage = 0x30u;
  {
    unsigned Before;
    int TaskInfoWrite;
    unsigned StackInfoWrite;

    Before = _NumWrites;
    SEGGER_SYSVIEW_SendTaskInfo(&TaskInfo);
    TaskInfoWrite = _FindEventFrom(Before, SYSVIEW_EVTID_TASK_INFO);
    TEST_ASSERT(TaskInfoWrite >= 0);
    if (TaskInfoWrite >= 0) {
      _ExpectTaskInfoPayload((unsigned)TaskInfoWrite, &TaskInfo);
    }
    StackInfoWrite = _ExpectLastEventFrom(Before, SYSVIEW_EVTID_STACK_INFO);
    _ExpectStackInfoPayload(StackInfoWrite, TaskInfo.TaskID, TaskInfo.StackBase, TaskInfo.StackSize, TaskInfo.StackUsage);
  }

  memset(&StackInfo, 0, sizeof(StackInfo));
  StackInfo.TaskID = 0x120u;
  StackInfo.StackBase = 0x200u;
  StackInfo.StackSize = 0x80u;
  StackInfo.StackUsage = 0x20u;
  {
    unsigned Before;
    unsigned WriteIndex;

    Before = _NumWrites;
    SEGGER_SYSVIEW_SendStackInfo(&StackInfo);
    WriteIndex = _ExpectLastEventFrom(Before, SYSVIEW_EVTID_STACK_INFO);
    _ExpectStackInfoPayload(WriteIndex, StackInfo.TaskID, StackInfo.StackBase, StackInfo.StackSize, StackInfo.StackUsage);
  }

  {
    unsigned Before;

    Before = _NumWrites;
    SEGGER_SYSVIEW_GetSysDesc();
    _ExpectEventInRange(Before, SYSVIEW_EVTID_INIT);
    _ExpectLastEventFrom(Before, SYSVIEW_EVTID_SYSDESC);
  }
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_SYSDESC, SEGGER_SYSVIEW_SendSysDesc("I#1=Task"));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_STACK_INFO, SEGGER_SYSVIEW_SendTaskList());

  SampleValue = 0x3F800000u;
  Sample.ID = 9u;
  Sample.pValue.pFloat = (float*)&SampleValue;
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_DATA_SAMPLE, SEGGER_SYSVIEW_SampleData(&Sample));
}

/*********************************************************************
*
*       _TestRecordAPI()
*
*  Function description
*    Verifies generic event recording APIs and fixed SystemView event
*    recording helpers.
*/
static void _TestRecordAPI(void) {
  SEGGER_SYSVIEW_CORE_CONTEXT ExtraContext;

  _InitAndStart(&_OSAPI);

  TEST_EXPECT_EVENT(40u, SEGGER_SYSVIEW_RecordVoid(40u));
  TEST_EXPECT_EVENT(41u, SEGGER_SYSVIEW_RecordU32(41u, 1u));
  TEST_EXPECT_EVENT(42u, SEGGER_SYSVIEW_RecordU32x2(42u, 1u, 2u));
  TEST_EXPECT_EVENT(43u, SEGGER_SYSVIEW_RecordU32x3(43u, 1u, 2u, 3u));
  TEST_EXPECT_EVENT(44u, SEGGER_SYSVIEW_RecordU32x4(44u, 1u, 2u, 3u, 4u));
  TEST_EXPECT_EVENT(45u, SEGGER_SYSVIEW_RecordU32x5(45u, 1u, 2u, 3u, 4u, 5u));
  TEST_EXPECT_EVENT(46u, SEGGER_SYSVIEW_RecordU32x6(46u, 1u, 2u, 3u, 4u, 5u, 6u));
  TEST_EXPECT_EVENT(47u, SEGGER_SYSVIEW_RecordU32x7(47u, 1u, 2u, 3u, 4u, 5u, 6u, 7u));
  TEST_EXPECT_EVENT(48u, SEGGER_SYSVIEW_RecordU32x8(48u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u));
  TEST_EXPECT_EVENT(49u, SEGGER_SYSVIEW_RecordU32x9(49u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u));
  TEST_EXPECT_EVENT(50u, SEGGER_SYSVIEW_RecordU32x10(50u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u));
  TEST_EXPECT_EVENT(51u, SEGGER_SYSVIEW_RecordString(51u, "payload"));
  {
    const U32 aRecordIdxU32Args[1]   = { 0xF0000077u };
    const U32 aRecordIdxU32x2Args[2] = { 0xF0000088u, 0xF0000089u };
    const U32 aRecordIdxU32x3Args[3] = { 0xF0000099u, 0xF000009Au, 0xF000009Bu };
    const U32 aRecordIdxU32x4Args[4] = { 0xF00000AAu, 0xF00000ABu, 0xF00000ACu, 0xF00000ADu };
    unsigned Before;
    unsigned WriteIndex;

    Before = _NumWrites;
    SEGGER_SYSVIEW_RecordId(52u, (PTR_ADDR)0x100000055u);
    WriteIndex = _ExpectLastEventFrom(Before, 52u);
    _ExpectRecordIdPayload(WriteIndex, (PTR_ADDR)0x100000055u, NULL, 0u);

    Before = _NumWrites;
    SEGGER_SYSVIEW_RecordIdxU32(53u, (PTR_ADDR)0x100000066u, 0xF0000077u);
    WriteIndex = _ExpectLastEventFrom(Before, 53u);
    _ExpectRecordIdPayload(WriteIndex, (PTR_ADDR)0x100000066u, aRecordIdxU32Args, SEGGER_COUNTOF(aRecordIdxU32Args));

    Before = _NumWrites;
    SEGGER_SYSVIEW_RecordIdxU32x2(54u, (PTR_ADDR)0x100000088u, 0xF0000088u, 0xF0000089u);
    WriteIndex = _ExpectLastEventFrom(Before, 54u);
    _ExpectRecordIdPayload(WriteIndex, (PTR_ADDR)0x100000088u, aRecordIdxU32x2Args, SEGGER_COUNTOF(aRecordIdxU32x2Args));

    Before = _NumWrites;
    SEGGER_SYSVIEW_RecordIdxU32x3(55u, (PTR_ADDR)0x100000099u, 0xF0000099u, 0xF000009Au, 0xF000009Bu);
    WriteIndex = _ExpectLastEventFrom(Before, 55u);
    _ExpectRecordIdPayload(WriteIndex, (PTR_ADDR)0x100000099u, aRecordIdxU32x3Args, SEGGER_COUNTOF(aRecordIdxU32x3Args));

    Before = _NumWrites;
    SEGGER_SYSVIEW_RecordIdxU32x4(56u, (PTR_ADDR)0x1000000AAu, 0xF00000AAu, 0xF00000ABu, 0xF00000ACu, 0xF00000ADu);
    WriteIndex = _ExpectLastEventFrom(Before, 56u);
    _ExpectRecordIdPayload(WriteIndex, (PTR_ADDR)0x1000000AAu, aRecordIdxU32x4Args, SEGGER_COUNTOF(aRecordIdxU32x4Args));
  }
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_SYSTIME_US, SEGGER_SYSVIEW_RecordSystime());
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_ISR_ENTER, SEGGER_SYSVIEW_RecordEnterISR());
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_ISR_EXIT, SEGGER_SYSVIEW_RecordExitISR());
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_ISR_TO_SCHEDULER, SEGGER_SYSVIEW_RecordExitISRToScheduler());
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TIMER_ENTER, SEGGER_SYSVIEW_RecordEnterTimer(0x88u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TIMER_EXIT, SEGGER_SYSVIEW_RecordExitTimer());
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_END_CALL, SEGGER_SYSVIEW_RecordEndCall(60u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_END_CALL, SEGGER_SYSVIEW_RecordEndCallU32(60u, 0x77u));
  {
    const U8* pPayload;
    unsigned Before;
    unsigned NumUsed;
    unsigned PayloadLen;
    unsigned PayloadOff;
    unsigned WriteIndex;
    PTR_ADDR DecodedAddr;
    U32 DecodedEventId;

    Before = _NumWrites;
    SEGGER_SYSVIEW_RecordEndCallId(60u, (PTR_ADDR)0x100000077u);
    WriteIndex = _ExpectLastEventFrom(Before, SYSVIEW_EVTID_END_CALL);
    pPayload = _WritePayload(WriteIndex, &PayloadLen);
    PayloadOff = 0u;
    DecodedEventId = _DecodeU32(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
    PayloadOff += NumUsed;
    DecodedAddr = _DecodeAddr(pPayload + PayloadOff, PayloadLen - PayloadOff, &NumUsed);
    PayloadOff += NumUsed;
    TEST_ASSERT_EQ_U(60u, DecodedEventId);
    TEST_ASSERT_EQ_ADDR((PTR_ADDR)0x100000077u, DecodedAddr);
    _ExpectPayloadFieldsConsumed(PayloadLen, PayloadOff);
  }

  _InitAndStart(NULL);
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_SYSTIME_CYCLES, SEGGER_SYSVIEW_RecordSystime());

  memset(&ExtraContext, 0, sizeof(ExtraContext));
  ExtraContext.SysFreq = 2000u;
  ExtraContext.CPUFreq = 2000u;
  ExtraContext.RAMBaseAddress = 0u;
  ExtraContext.UpChannel = (U8)SEGGER_SYSVIEW_GetChannelID();
  _ClearWrites();
  SEGGER_SYSVIEW_Start_Ex(&ExtraContext, 77u);
  _ExpectEventInRange(0u, SYSVIEW_EVTID_SYSTIME_CYCLES);
}

/*********************************************************************
*
*       _TestConvenienceEventAPI()
*
*  Function description
*    Verifies task lifecycle, markers, heap, resource naming, and data
*    registration convenience APIs.
*/
static void _TestConvenienceEventAPI(void) {
  SEGGER_SYSVIEW_DATA_REGISTER DataReg;

  _InitAndStart(&_OSAPI);

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_IDLE, SEGGER_SYSVIEW_OnIdle());
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TASK_CREATE, SEGGER_SYSVIEW_OnTaskCreate(0x100u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TASK_TERMINATE, SEGGER_SYSVIEW_OnTaskTerminate(0x100u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TASK_START_EXEC, SEGGER_SYSVIEW_OnTaskStartExec(0x100u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TASK_STOP_EXEC, SEGGER_SYSVIEW_OnTaskStopExec());
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TASK_START_READY, SEGGER_SYSVIEW_OnTaskStartReady(0x100u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_TASK_STOP_READY, SEGGER_SYSVIEW_OnTaskStopReady(0x100u, 2u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MARK_START, SEGGER_SYSVIEW_MarkStart(3u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MARK_STOP, SEGGER_SYSVIEW_MarkStop(3u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MARK_START, SEGGER_SYSVIEW_OnUserStart(4u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MARK_STOP, SEGGER_SYSVIEW_OnUserStop(4u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_MARK, SEGGER_SYSVIEW_Mark(5u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_NAME_MARKER, SEGGER_SYSVIEW_NameMarker(5u, "marker"));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_NAME_RESOURCE, SEGGER_SYSVIEW_NameResource(0x200u, "resource"));

  memset(&DataReg, 0, sizeof(DataReg));
  DataReg.ID = 1u;
  DataReg.DataType = SEGGER_SYSVIEW_TYPE_FLOAT;
  DataReg.Offset = 2;
  DataReg.RangeMin = -10;
  DataReg.RangeMax = 10;
  DataReg.ScalingFactor = 1.5f;
  DataReg.sName = "data";
  DataReg.sUnit = "u";
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_REGISTER_DATA, SEGGER_SYSVIEW_RegisterData(&DataReg));

  DataReg.sUnit = NULL;
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_REGISTER_DATA, SEGGER_SYSVIEW_RegisterData(&DataReg));

  DataReg.ScalingFactor = 0.0f;
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_REGISTER_DATA, SEGGER_SYSVIEW_RegisterData(&DataReg));

  DataReg.RangeMax = 0;
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_REGISTER_DATA, SEGGER_SYSVIEW_RegisterData(&DataReg));

  DataReg.RangeMin = 0;
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_REGISTER_DATA, SEGGER_SYSVIEW_RegisterData(&DataReg));

  DataReg.Offset = 0;
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_REGISTER_DATA, SEGGER_SYSVIEW_RegisterData(&DataReg));

  DataReg.DataType = SEGGER_SYSVIEW_TYPE_U32;
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_REGISTER_DATA, SEGGER_SYSVIEW_RegisterData(&DataReg));

  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_HEAP_DEFINE, SEGGER_SYSVIEW_HeapDefine((void*)(PTR_ADDR)0x100u, (void*)(PTR_ADDR)0x200u, 64u, 8u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_HEAP_ALLOC, SEGGER_SYSVIEW_HeapAlloc((void*)(PTR_ADDR)0x100u, (void*)(PTR_ADDR)0x220u, 32u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_HEAP_ALLOC_EX, SEGGER_SYSVIEW_HeapAllocEx((void*)(PTR_ADDR)0x100u, (void*)(PTR_ADDR)0x240u, 32u, 9u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_HEAP_FREE, SEGGER_SYSVIEW_HeapFree((void*)(PTR_ADDR)0x100u, (void*)(PTR_ADDR)0x240u));
}

/*********************************************************************
*
*       _TestDisableEnableEventsAPI()
*
*  Function description
*    Verifies runtime enable/disable masks for standard events.
*/
static void _TestDisableEnableEventsAPI(void) {
  unsigned Before;

  _InitAndStart(&_OSAPI);
  SEGGER_SYSVIEW_DisableEvents(SYSVIEW_EVTMASK_IDLE);
  Before = _NumWrites;
  SEGGER_SYSVIEW_OnIdle();
  _ExpectNoNewWrites(Before);
  SEGGER_SYSVIEW_EnableEvents(SYSVIEW_EVTMASK_IDLE);
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_IDLE, SEGGER_SYSVIEW_OnIdle());
}

/*********************************************************************
*
*       _TestOverflowAPI()
*
*  Function description
*    Verifies the buffer-full path, overflow packet retry path, and
*    repeated overflow failure handling.
*/
static void _TestOverflowAPI(void) {
  SEGGER_SYSVIEW_CORE_CONTEXT* pContext;
  unsigned Before;

  _InitAndStart(&_OSAPI);
  pContext = SEGGER_SYSVIEW_GetMainContext();

  _RTTWriteSkipFailCount = 1u;
  Before = _NumWrites;
  SEGGER_SYSVIEW_RecordVoid(70u);
  _ExpectNoNewWrites(Before);
  TEST_ASSERT_EQ_U(2u, pContext->EnableState);

  _ClearWrites();
  SEGGER_SYSVIEW_RecordVoid(71u);
  _ExpectEventInRange(0u, SYSVIEW_EVTID_OVERFLOW);
  _ExpectLastEventFrom(0u, 71u);
  TEST_ASSERT_EQ_U(1u, pContext->EnableState);

  _RTTWriteSkipFailCount = 1u;
  Before = _NumWrites;
  SEGGER_SYSVIEW_RecordVoid(72u);
  _ExpectNoNewWrites(Before);
  TEST_ASSERT_EQ_U(2u, pContext->EnableState);

  _ClearWrites();
  _RTTWriteSkipFailCount = 1u;
  SEGGER_SYSVIEW_RecordVoid(73u);
  _ExpectNoNewWrites(0u);
  TEST_ASSERT_EQ_U(2u, pContext->EnableState);

  pContext->EnableState = 1u;
}

/*********************************************************************
*
*       _TestSendPacketAPI()
*
*  Function description
*    Verifies public packet send APIs and their return values.
*/
static void _TestSendPacketAPI(void) {
  SEGGER_SYSVIEW_CORE_CONTEXT* pContext;
  U8 aPacket[320];
  U8* pPayload;
  int Status;

  _InitNoStart(&_OSAPI);
  pContext = SEGGER_SYSVIEW_GetMainContext();
  _ClearWrites();
  aPacket[0] = 77u;
  Status = SEGGER_SYSVIEW_SendPacket_Ex(pContext, 555u, aPacket, &aPacket[1]);
  TEST_ASSERT_EQ_I(0, Status);
  _ExpectNoNewWrites(0u);

  _InitAndStart(&_OSAPI);
  pContext = SEGGER_SYSVIEW_GetMainContext();

  pPayload = &aPacket[4];
  pPayload = SEGGER_SYSVIEW_EncodeU32(pPayload, 0x55u);
  Status = SEGGER_SYSVIEW_SendPacket(aPacket, pPayload, 100u);
  TEST_ASSERT(Status != 0);
  _ExpectLastEventFrom(0u, 100u);

  _ClearWrites();
  memset(&aPacket[4], 0xA5, 130u);
  Status = SEGGER_SYSVIEW_SendPacket(aPacket, &aPacket[134], 101u);
  TEST_ASSERT(Status != 0);
  _ExpectLastEventFrom(0u, 101u);

  TEST_EXPECT_EVENT(128u, SEGGER_SYSVIEW_RecordVoid(128u));
  TEST_EXPECT_EVENT((1u << 14), SEGGER_SYSVIEW_RecordVoid(1u << 14));
  TEST_EXPECT_EVENT((1u << 21), SEGGER_SYSVIEW_RecordVoid(1u << 21));
  TEST_EXPECT_EVENT((1u << 28), SEGGER_SYSVIEW_RecordVoid(1u << 28));

  _ClearWrites();
  aPacket[0] = 77u;
  aPacket[1] = 0xAAu;
  Status = SEGGER_SYSVIEW_SendPacket_Ex(pContext, 555u, aPacket, &aPacket[2]);
  TEST_ASSERT(Status != 0);
  _ExpectLastEventFrom(0u, 77u);
}

/*********************************************************************
*
*       _TestPrintfAPI()
*
*  Function description
*    Verifies host-side and target-side printf-style public APIs.
*/
static void _TestPrintfAPI(void) {
  char acLong[SEGGER_SYSVIEW_MAX_STRING_LEN + 8u];
  unsigned i;

  _InitAndStart(&_OSAPI);

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_PrintfHostEx("host %d", SEGGER_SYSVIEW_LOG, 1));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVPrintfHostEx("vhost %d", SEGGER_SYSVIEW_WARNING, 2));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_PrintfTargetEx("target %d", SEGGER_SYSVIEW_ERROR, 3));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVPrintfTargetEx("vtarget %d", SEGGER_SYSVIEW_LOG, 4));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_PrintfHost("host %d", 5));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVPrintfHost("vhost %d", 6));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_PrintfTarget("target %d", 7));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVPrintfTarget("vtarget %d", 8));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_WarnfHost("warn %d", 9));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVWarnfHost("vwarn %d", 10));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_WarnfTarget("warn %d", 11));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVWarnfTarget("vwarn %d", 12));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_ErrorfHost("error %d", 13));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVErrorfHost("verror %d", 14));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_ErrorfTarget("error %d", 15));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, _CallVErrorfTarget("verror %d", 16));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_Print("print"));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_Warn("warn"));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_Error("error"));

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED,
                    SEGGER_SYSVIEW_PrintfHost("%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",
                                              1, 2, 3, 4, 5, 6, 7, 8, 9,
                                              10, 11, 12, 13, 14, 15, 16, 17));

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED,
                    SEGGER_SYSVIEW_PrintfTarget("fmt:%-8u:%08u:%8u:%.4u:%+8d:%08d:%-8d:%c:%u:%x:%s:%p:%%:%q:%ld",
                                                1, 2, 3, 4, 5, -6, 7, 'A', 8, 0x1A, "str", 0x1234, 9));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_PrintfTarget("fmt2:%.4d:%#x:%s", 5, 0x1A, NULL));

  for (i = 0u; i < sizeof(acLong) - 1u; i++) {
    acLong[i] = (char)('a' + (i % 26u));
  }
  acLong[sizeof(acLong) - 1u] = '\0';
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_PRINT_FORMATTED, SEGGER_SYSVIEW_PrintfTarget("%s", acLong));

  {
    unsigned Before;

    Before = _NumWrites;
    SEGGER_SYSVIEW_PrintfTarget("");
    _ExpectNoNewWrites(Before);
  }
}

/*********************************************************************
*
*       _TestPrintElfAPI()
*
*  Function description
*    Verifies ELF-based print event APIs with zero to ten integer
*    arguments and mixed format arguments.
*/
static void _TestPrintElfAPI(void) {
  U32 aIntArgs[2];
  const char* asStrArgs[2];

  _InitAndStart(&_OSAPI);

  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf(0x1000u, SEGGER_SYSVIEW_LOG));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32(0x1000u, SEGGER_SYSVIEW_LOG, 1u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x2(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x3(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x4(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u, 4u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x5(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u, 4u, 5u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x6(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u, 4u, 5u, 6u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x7(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u, 4u, 5u, 6u, 7u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x8(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x9(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u));
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_U32x10(0x1000u, SEGGER_SYSVIEW_LOG, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u));

  aIntArgs[0] = 1u;
  aIntArgs[1] = 2u;
  asStrArgs[0] = "a";
  asStrArgs[1] = "b";
  TEST_EXPECT_EXT_EVENT(SYSVIEW_EVTID_EX_PRINT_ELF, SEGGER_SYSVIEW__PrintElf_Fmt(0x1000u, SEGGER_SYSVIEW_WARNING, 2u, aIntArgs, 2u, asStrArgs));
}

/*********************************************************************
*
*       _TestPrintElfMacroAPI()
*
*  Function description
*    Verifies the public PrintElf convenience macros, including fixed
*    integer argument counts, variable integer/string arguments, and
*    SV_INT_ARGS/SV_STR_ARGS format argument helpers.
*/
static void _TestPrintElfMacroAPI(void) {
  const U32 aExpectedU32[10] = { 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u };
  const U32 aVarU32[3] = { 11u, 12u, 13u };
  const U32 aFmtU32[2] = { 21u, 22u };
  const char* asVarStr[2] = { "left", "right" };
  const char* asFmtStr[2] = { "fmt-a", "fmt-b" };
  unsigned Before;

  _InitAndStart(&_OSAPI);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF(SEGGER_SYSVIEW_LOG, "macro0");
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_LOG, NULL, 0u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32(SEGGER_SYSVIEW_WARNING, "macro1", 1u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_WARNING, aExpectedU32, 1u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X2(SEGGER_SYSVIEW_ERROR, "macro2", 1u, 2u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_ERROR, aExpectedU32, 2u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X3(SEGGER_SYSVIEW_LOG, "macro3", 1u, 2u, 3u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_LOG, aExpectedU32, 3u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X4(SEGGER_SYSVIEW_WARNING, "macro4", 1u, 2u, 3u, 4u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_WARNING, aExpectedU32, 4u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X5(SEGGER_SYSVIEW_ERROR, "macro5", 1u, 2u, 3u, 4u, 5u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_ERROR, aExpectedU32, 5u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X6(SEGGER_SYSVIEW_LOG, "macro6", 1u, 2u, 3u, 4u, 5u, 6u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_LOG, aExpectedU32, 6u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X7(SEGGER_SYSVIEW_WARNING, "macro7", 1u, 2u, 3u, 4u, 5u, 6u, 7u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_WARNING, aExpectedU32, 7u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X8(SEGGER_SYSVIEW_ERROR, "macro8", 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_ERROR, aExpectedU32, 8u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X9(SEGGER_SYSVIEW_LOG, "macro9", 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_LOG, aExpectedU32, 9u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_X10(SEGGER_SYSVIEW_WARNING, "macro10", 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_WARNING, aExpectedU32, 10u, NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_U32_VAR(SEGGER_SYSVIEW_ERROR, "macro-var-u32", 11u, 12u, 13u);
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_ERROR, aVarU32, SEGGER_COUNTOF(aVarU32), NULL, 0u);

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_STR_VAR(SEGGER_SYSVIEW_LOG, "macro-var-str", "left", "right");
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_LOG, NULL, 0u, asVarStr, SEGGER_COUNTOF(asVarStr));

  Before = _NumWrites;
  SEGGER_SYSVIEW_PRINT_ELF_FMT(SEGGER_SYSVIEW_WARNING, "macro-fmt", SV_INT_ARGS(21u, 22u), SV_STR_ARGS("fmt-a", "fmt-b"));
  _ExpectLastPrintElfFrom(Before, SEGGER_SYSVIEW_WARNING, aFmtU32, SEGGER_COUNTOF(aFmtU32), asFmtStr, SEGGER_COUNTOF(asFmtStr));
}

/*********************************************************************
*
*       _TestModuleAPI()
*
*  Function description
*    Verifies middleware module registration and module-description
*    transmission APIs.
*/
static void _TestModuleAPI(void) {
  _InitAndStart(&_OSAPI);

  memset(&_ModuleA, 0, sizeof(_ModuleA));
  memset(&_ModuleB, 0, sizeof(_ModuleB));
  _ModuleA.sModule = "ModuleA";
  _ModuleA.NumEvents = 3u;
  _ModuleA.pfSendModuleDesc = _ModuleADesc;
  _ModuleB.sModule = "ModuleB";
  _ModuleB.NumEvents = 2u;
  _ModuleB.pfSendModuleDesc = _ModuleBDesc;

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MODULEDESC, SEGGER_SYSVIEW_RegisterModule(&_ModuleA));
  TEST_ASSERT_EQ_U(512u, _ModuleA.EventOffset);
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MODULEDESC, SEGGER_SYSVIEW_RegisterModule(&_ModuleB));
  TEST_ASSERT_EQ_U(515u, _ModuleB.EventOffset);

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MODULEDESC, SEGGER_SYSVIEW_SendModule(0u));
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MODULEDESC, SEGGER_SYSVIEW_SendModule(1u));
  {
    unsigned Before;

    Before = _NumWrites;
    SEGGER_SYSVIEW_SendModule(9u);
    _ExpectNoNewWrites(Before);
  }
  TEST_EXPECT_EVENT(SYSVIEW_EVTID_MODULEDESC, SEGGER_SYSVIEW_RecordModuleDescription(&_ModuleA, "ModuleA.Direct"));

  _ClearWrites();
  SEGGER_SYSVIEW_SendModuleDescription();
  TEST_ASSERT(_ModuleDescCallbackCount >= 2u);
  _ExpectLastEventFrom(0u, SYSVIEW_EVTID_MODULEDESC);

  TEST_EXPECT_EVENT(SYSVIEW_EVTID_NUMMODULES, SEGGER_SYSVIEW_SendNumModules());
}

/*********************************************************************
*
*       main()
*
**********************************************************************
*/

int main(void) {
  _TestApplicationHooks();
  _TestEncodeAPI();
  _TestInitAndControlAPI();
  _TestHostCommandAPI();
  _TestTaskAndDescriptionAPI();
  _TestRecordAPI();
  _TestConvenienceEventAPI();
  _TestDisableEnableEventsAPI();
  _TestOverflowAPI();
  _TestSendPacketAPI();
  _TestPrintfAPI();
  _TestPrintElfAPI();
  _TestPrintElfMacroAPI();
  _TestModuleAPI();

  TEST_ASSERT_EQ_U(0u, _BadRTTAddressCount);
  TEST_ASSERT_EQ_U(0u, _BadRTTNameCount);
  TEST_ASSERT_EQ_U(0u, _BadRTTBufferCount);

  if (_NumFailures != 0u) {
    printf("SEGGER_SYSVIEW_PublicAPI_Test: %u failure(s)\n", _NumFailures);
    return 1;
  }
  printf("SEGGER_SYSVIEW_PublicAPI_Test: all tests passed\n");
  return 0;
}

/*************************** End of file ****************************/
