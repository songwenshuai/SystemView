/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2026 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS-Classic * Real time operating system                   *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.22.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : RTOSInit.c
Purpose : Initializes and handles the hardware for embOS.
*/

#if !defined(EMBOS_SIM_HOST_WINDOWS) || !defined(EMBOS_SIM_HOST_POSIX)
  #error "EMBOS_SIM_HOST_WINDOWS and EMBOS_SIM_HOST_POSIX must be defined by the simulation build."
#endif

#if defined(__linux__)
  #define _GNU_SOURCE
#endif

#if (EMBOS_SIM_HOST_WINDOWS != 0)

#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include "RTOS.h"
#include "UDPCOM.h"
#if ((OS_SUPPORT_TRACE_API != 0) && !defined(_WIN64))
#include "SEGGER_SYSVIEW_Win32.h"
#endif

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
//
// If between two embOS system tick ISRs more than TIME_DIFF_MAX milliseconds
// have elapsed, then tDiff will be set to TIME_DIFF_MAX. This might occur when
// one debugs and single steps through the application or if the CPU load is
// very high. This  value can be changed as desired. Please consider that if
// TIME_DIFF_MAX cuts off system ticks, that the embOS time doesn't resemble
// the real time anymore. The limit can be disabled with a value of 0.
//
#ifndef   TIME_DIFF_MAX
  #define TIME_DIFF_MAX  (1)
#endif

/*********************************************************************
*
*       System tick settings
*/
#define OS_TIMER_FREQ  ((OS_U32)_TimerFrequency.QuadPart)
#define OS_TICK_FREQ   (1000u)
#define OS_INT_FREQ    (OS_TICK_FREQ)

/*********************************************************************
*
*       embOSView settings
*/
#ifndef   OS_VIEW_IFSELECT
#if (defined(_WIN64) && (_WIN64 == 1)) || (defined(__x86_64__) && (__x86_64__ == 1))
  #define OS_VIEW_IFSELECT  OS_VIEW_DISABLED  // embOSView communication currently not supported for devices with 64-bit address range
#else
  #define OS_VIEW_IFSELECT  OS_VIEW_IF_ETHERNET
#endif
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static LARGE_INTEGER _TimerFrequency = {0};
static LARGE_INTEGER _CycleStamp     = {0};

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _ISRTickThread()
*/
static void _ISRTickThread(void) {
  int tDiff;
  int t;
  int tLast;

#if ((OS_SUPPORT_TRACE_API != 0) && !defined(_WIN64))
  SEGGER_SYSVIEW_X_SetISRName("Tick ISR");
#endif
  //
  // Switch to higher timer resolution
  //
  timeBeginPeriod(1);
  tLast = timeGetTime();
  while (1) {
    t     = timeGetTime();
    tDiff = t - tLast;
    tLast = t;
#if (TIME_DIFF_MAX != 0)
    if (tDiff > TIME_DIFF_MAX) {
      tDiff = TIME_DIFF_MAX;
    }
#endif
    OS_INT_Enter();
    //
    // Call OS_TICK_Handle() for each passed millisecond.
    //
    while (tDiff-- > 0) {
      OS_TICK_Handle();
    }
    //
    // Save the current cycle counter value. This value is used in _OS_GetHWTimerCycles()
    // in order to return the elapsed cycles since the last recorded system tick.
    //
    QueryPerformanceCounter(&_CycleStamp);
    OS_INT_Leave();
    //
    // SleepEx()'s second parameter *MUST* be TRUE when used with QueueUserAPC. Otherwise 'Nonpaged Pool'
    // (cf. ProcessExplorer) is congested *COMPLETELY* since we are NOT in an alertable state and thus the
    // queue will NEVER be flushed.
    //
    SleepEx(INFINITE, TRUE);
  }
}

/*********************************************************************
*
*       _voidAPC()
*
*  Function description
*    Dummy APC function. Is required because we (ab)use the
*    WIN32 QueueUserAPC() API function to wake up a thread
*/
static void APIENTRY _voidAPC(ULONG_PTR Dummy) {
  OS_USEPARA(Dummy);
}

/*********************************************************************
*
*       _CbSignalTickProc()
*
*  Function description
*    Timer callback function which periodically queues an APC in order
*    to resume the ISR tick thread.
*/
static void CALLBACK _CbSignalTickProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
  static int IsThreadNameSet = 0;

  OS_USEPARA(uID);
  OS_USEPARA(uMsg);
  OS_USEPARA(dw1);
  OS_USEPARA(dw2);
  if (IsThreadNameSet == 0) {
    IsThreadNameSet = 1;
    OS_SIM_SetThreadName(-1, "Tick ISR (Helper Thread)");
  }
  QueueUserAPC(_voidAPC, (void*)dwUser, 0);
}

/*********************************************************************
*
*       _OS_GetHWTimerCycles()
*
*  Function description
*    Returns the current hardware timer count value.
*
*  Return value
*    Current timer count value.
*/
static unsigned int _OS_GetHWTimerCycles(void) {
  unsigned int  Cycles;
  LARGE_INTEGER Counter;

  QueryPerformanceCounter(&Counter);
  Cycles = (unsigned int)(Counter.QuadPart - _CycleStamp.QuadPart);  // Calculate the elapsed cycles since the last system tick.
  return Cycles;
}

/*********************************************************************
*
*       _OS_GetHWTimer_IntPending()
*
*  Function description
*    Returns if the hardware timer interrupt pending flag is set.
*
*  Return value
*    == 0: Interrupt pending flag not set.
*    != 0: Interrupt pending flag set.
*/
static unsigned int _OS_GetHWTimer_IntPending(void) {
  return 0;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_InitHW()
*
*  Function description
*    Initialize the hardware required for embOS to run.
*/
void OS_InitHW(void) {
  HANDLE hISRThread;

  OS_INT_IncDI();
  //
  // Get time stamp counter (TSC) frequency. Usually it's the CPU frequency divided by 1024.
  //
  QueryPerformanceFrequency(&_TimerFrequency);
  //
  // Start tick ISR
  //
  QueryPerformanceCounter(&_CycleStamp);  // Initial value for system tick zero.
  hISRThread = (HANDLE)OS_SIM_CreateISRThreadEx(_ISRTickThread, "Tick ISR");
  timeSetEvent(1, 0, _CbSignalTickProc, (DWORD_PTR)hISRThread, (TIME_PERIODIC | TIME_CALLBACK_FUNCTION));
  //
  // Inform embOS about the timer settings
  //
  {
    OS_SYSTIMER_CONFIG SysTimerConfig = {OS_TIMER_FREQ, OS_INT_FREQ, OS_TIMER_UPCOUNTING, _OS_GetHWTimerCycles, _OS_GetHWTimer_IntPending};
    OS_TIME_ConfigSysTimer(&SysTimerConfig);
  }
  //
  // Initialize communication for embOSView
  //
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_ETHERNET)
  UDP_Process_Init();
#endif
  //
  // Configure and initialize SEGGER SystemView
  //
#if ((OS_SUPPORT_TRACE_API != 0) && !((defined(_WIN64) && (_WIN64 == 1)) || (defined(__x86_64__) && (__x86_64__ == 1))))
  SEGGER_SYSVIEW_Conf();
#endif
  OS_INT_DecRI();
}

/*********************************************************************
*
*       Optional communication with embOSView
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_COM_Send1()
*
*  Function description
*    Sends one character.
*/
void OS_COM_Send1(OS_U8 c) {
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_ETHERNET)
  UDP_Process_Send1(c);
#elif (OS_VIEW_IFSELECT == OS_VIEW_DISABLED)
  OS_USEPARA(c);           // Avoid compiler warning
  OS_COM_ClearTxActive();  // Let embOS know that Tx is not busy
#endif
}

#elif (EMBOS_SIM_HOST_POSIX != 0)

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "RTOS.h"
#include "UDPCOM.h"
#if ((OS_SUPPORT_TRACE_API != 0) && !(OS_64BIT))
#include "SEGGER_SYSVIEW_Posix.h"
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define NSEC_PER_SEC       (1000000000)
#define CYCLES_TO_NSEC(x)  ((x) * ((NSEC_PER_SEC) / (OS_TIMER_FREQ)))
#define NSEC_TO_CYCLES(x)  ((x) / ((NSEC_PER_SEC) / (OS_TIMER_FREQ)))
#define SEC_TO_CYCLES(x)   ((x) * (OS_TIMER_FREQ))

//
// If the host does not schedule the POSIX tick thread for more than
// TIME_DIFF_MAX system ticks, the next tick is re-anchored to the current
// monotonic time. This matches the Windows simulation behavior and prevents
// delayed host scheduling from being converted into a burst of missed ticks.
//
#ifndef   TIME_DIFF_MAX
  #define TIME_DIFF_MAX  (1)
#endif

#ifndef   EMBOS_SIM_TRACE_TICK
  #define EMBOS_SIM_TRACE_TICK         (0)
#endif

#ifndef   EMBOS_SIM_TICK_TRACE_PERIOD
  #define EMBOS_SIM_TICK_TRACE_PERIOD  (1000u)
#endif

#if ((EMBOS_SIM_TRACE_TICK != 0) && (EMBOS_SIM_TICK_TRACE_PERIOD == 0u))
  #error "EMBOS_SIM_TICK_TRACE_PERIOD must be greater than zero."
#endif

/*********************************************************************
*
*       System tick settings
*/
//
// The current implementation of the RTOSInit.c allows to specify the
// timer frequency to be used for the embOS simulation. In order to
// change the timer frequency, only OS_TIMER_FREQ needs to be changed.
// The POSIX sleep APIs support nanosecond resolution, so frequencies up
// to 1 GHz can be used for OS_TIMER_FREQ. Changing OS_TIMER_FREQ also
// affects the cycle resolution. By default, OS_TIMER_FREQ is set to
// 1000000, i.e. 1 MHz, meaning that one cycle equals one microsecond.
//
//
#define OS_TIMER_FREQ       (1000000)  // 1 MHz
#define OS_TICK_FREQ        (1000)     // 1000 ticks per second
#define OS_INT_FREQ         (OS_TICK_FREQ)
#define OS_CYCLES_PER_TICK  (OS_TIMER_FREQ / OS_TICK_FREQ)

#if (OS_TIMER_FREQ > NSEC_PER_SEC)
  #error "Timer frequency values greater than 1 GHz are not supported."
#endif

/*********************************************************************
*
*       embOSView settings
*/
#ifndef   OS_VIEW_IFSELECT
#if (OS_64BIT == 1)
  #define OS_VIEW_IFSELECT  OS_VIEW_DISABLED  // embOSView communication currently not supported for devices with 64-bit address range
#else
  #define OS_VIEW_IFSELECT  OS_VIEW_IF_ETHERNET
#endif
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static struct timespec _LastSystickTimestamp;
static struct timespec _NextSysTickTimestamp;
#if (EMBOS_SIM_TRACE_TICK != 0)
static unsigned int    _TickControllerCnt;
static unsigned int    _TickHandlerCnt;
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _FailPosixError()
*
*  Function description
*    Reports a POSIX runtime error and terminates the process.
*/
static void _FailPosixError(const char* sFunction, int Error) {
  errno = Error;
  perror(sFunction);
  exit(EXIT_FAILURE);
}

/*********************************************************************
*
*       _GetMonotonicTime()
*
*  Function description
*    Reads the monotonic host clock.
*/
static void _GetMonotonicTime(struct timespec* pTimespec) {
  if (clock_gettime(CLOCK_MONOTONIC, pTimespec) != 0) {
    _FailPosixError("clock_gettime", errno);
  }
}

/*********************************************************************
*
*       _TraceTick()
*
*  Function description
*    Prints low-rate POSIX tick diagnostics.
*/
#if (EMBOS_SIM_TRACE_TICK != 0)
static void _TraceTick(const char* sStage, unsigned int TickCnt) {
  if ((TickCnt <= 4u) || ((TickCnt % EMBOS_SIM_TICK_TRACE_PERIOD) == 0u)) {
    fprintf(stderr, "[tick] %s count=%u\n", sStage, TickCnt);
    fflush(stderr);
  }
}
#endif

/*********************************************************************
*
*       _GetCycles()
*
*  Function description
*    If pTimespec is NULL, the current time stamp in cycles is
*    returned. Else, the time stamp pointed to by pTimespec is
*    converted to cycles and returned.
*/
static OS_U64 _GetCycles(const struct timespec* pTimespec) {
  struct timespec Timespec;
  OS_U64          Timestamp;

  if (pTimespec == NULL) {
    _GetMonotonicTime(&Timespec);
    pTimespec = &Timespec;
  }
  Timestamp  =  SEC_TO_CYCLES(pTimespec->tv_sec);
  Timestamp += NSEC_TO_CYCLES(pTimespec->tv_nsec);
  return Timestamp;
}

/*********************************************************************
*
*       _CompareTime()
*
*  Function description
*    Compares two monotonic time stamps.
*
*  Return value
*    < 0: pLeft is earlier than pRight.
*      0: pLeft equals pRight.
*    > 0: pLeft is later than pRight.
*/
static int _CompareTime(const struct timespec* pLeft, const struct timespec* pRight) {
  if (pLeft->tv_sec != pRight->tv_sec) {
    return (pLeft->tv_sec < pRight->tv_sec) ? -1 : 1;
  }
  if (pLeft->tv_nsec != pRight->tv_nsec) {
    return (pLeft->tv_nsec < pRight->tv_nsec) ? -1 : 1;
  }
  return 0;
}

/*********************************************************************
*
*       _AddTime()
*
*  Function description
*    Adds the specified nanoseconds to the timespec structure and
*    ensures that the value of the tv_nsec member doesn't exceed
*    1 billion.
*
*  Parameters
*    pTimespec: Pointer to the timespec structure to adjust.
*    ns:        The nanoseconds to add to the timespec structure.
*/
static void _AddTime(struct timespec* pTimespec, long ns) {
  pTimespec->tv_sec  += ns / NSEC_PER_SEC;
  pTimespec->tv_nsec += ns % NSEC_PER_SEC;

  if (pTimespec->tv_nsec >= NSEC_PER_SEC) {
    pTimespec->tv_nsec -= NSEC_PER_SEC;
    pTimespec->tv_sec++;
  }
}

/*********************************************************************
*
*       _LimitTickCatchUp()
*
*  Function description
*    Caps POSIX host catch-up after delayed tick-thread scheduling.
*/
static void _LimitTickCatchUp(void) {
#if (TIME_DIFF_MAX != 0)
  struct timespec Now;
  struct timespec LatestAcceptedTickTime;

  _GetMonotonicTime(&Now);
  LatestAcceptedTickTime = _NextSysTickTimestamp;
  _AddTime(&LatestAcceptedTickTime, CYCLES_TO_NSEC(OS_CYCLES_PER_TICK) * TIME_DIFF_MAX);

  if (_CompareTime(&Now, &LatestAcceptedTickTime) >= 0) {
    _NextSysTickTimestamp = Now;
  }
#endif
}

/*********************************************************************
*
*       _SleepUntilMonotonicTime()
*
*  Function description
*    Delays the current host thread until the specified monotonic time.
*/
static void _SleepUntilMonotonicTime(const struct timespec* pTimespec) {
#if defined(__linux__)
  int Result;

  do {
    Result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, pTimespec, NULL);
  } while (Result == EINTR);
  if (Result != 0) {
    _FailPosixError("clock_nanosleep", Result);
  }
#else
  struct timespec Now;
  struct timespec Remaining;
  int             Result;

  for (;;) {
    _GetMonotonicTime(&Now);
    if (_CompareTime(&Now, pTimespec) >= 0) {
      break;
    }
    Remaining.tv_sec  = pTimespec->tv_sec - Now.tv_sec;
    Remaining.tv_nsec = pTimespec->tv_nsec - Now.tv_nsec;
    if (Remaining.tv_nsec < 0) {
      Remaining.tv_nsec += NSEC_PER_SEC;
      Remaining.tv_sec--;
    }
    do {
      Result = nanosleep(&Remaining, &Remaining);
    } while ((Result != 0) && (errno == EINTR));
    if (Result != 0) {
      _FailPosixError("nanosleep", errno);
    }
  }
#endif
}

/*********************************************************************
*
*       _SystemTickController()
*
*  Function description
*    This function controls the occurrence of system tick interrupts.
*/
static int _SystemTickController(void) {
  static int InitOnce = 1;

  if (InitOnce != 0) {
    InitOnce = 0;
#if ((OS_SUPPORT_TRACE_API != 0) && !(OS_64BIT))
    SEGGER_SYSVIEW_X_SetISRName("Tick ISR");
#endif
    //
    // Get the initial time stamp.
    //
    _GetMonotonicTime(&_NextSysTickTimestamp);
    _LastSystickTimestamp = _NextSysTickTimestamp;
  }
  //
  // Calculate the absolute time stamp for the next system tick.
  //
  _AddTime(&_NextSysTickTimestamp, CYCLES_TO_NSEC(OS_CYCLES_PER_TICK));
  //
  // Delay until the newly calculated time stamp.
  //
  _SleepUntilMonotonicTime(&_NextSysTickTimestamp);
  _LimitTickCatchUp();
#if (EMBOS_SIM_TRACE_TICK != 0)
  _TickControllerCnt++;
  _TraceTick("controller", _TickControllerCnt);
#endif
  return 1;  // Execute the ISR.
}

/*********************************************************************
*
*       _SystemTickHandler()
*/
static void _SystemTickHandler(void) {
  OS_INT_Enter();
  OS_TICK_Handle();
  //
  // Save the time stamp of the last system tick that was handled.
  // This information is used to calculate the elapsed cycles.
  //
  _LastSystickTimestamp = _NextSysTickTimestamp;
  OS_INT_Leave();
#if (EMBOS_SIM_TRACE_TICK != 0)
  _TickHandlerCnt++;
  _TraceTick("handler", _TickHandlerCnt);
#endif
}

/*********************************************************************
*
*       _OS_GetHWTimerCycles()
*
*  Function description
*    Returns the current hardware timer count value.
*
*  Return value
*    Current timer count value.
*/
static unsigned int _OS_GetHWTimerCycles(void) {
  OS_U64 Cycles;

  //
  // Convert the timespec structure of the last system
  // tick to cycles and use it to calculate the elapsed
  // cycles since the last system tick.
  //
  Cycles = _GetCycles(&_LastSystickTimestamp);
  Cycles = _GetCycles(NULL) - Cycles;
  return (unsigned int)Cycles;
}

/*********************************************************************
*
*       _OS_GetHWTimer_IntPending()
*
*  Function description
*    Returns if the hardware timer interrupt pending flag is set.
*
*  Return value
*    == 0: Interrupt pending flag not set.
*    != 0: Interrupt pending flag set.
*/
static unsigned int _OS_GetHWTimer_IntPending(void) {
  return 0;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_InitHW()
*
*  Function description
*    Initialize the hardware required for embOS to run.
*/
void OS_InitHW(void) {
  OS_INT_IncDI();
  OS_SIM_CreateISRThreadEx(_SystemTickHandler, _SystemTickController, "System Tick ISR");
  {
    OS_SYSTIMER_CONFIG SysTimerConfig = {OS_TIMER_FREQ, OS_INT_FREQ, OS_TIMER_UPCOUNTING, _OS_GetHWTimerCycles, _OS_GetHWTimer_IntPending};
    OS_TIME_ConfigSysTimer(&SysTimerConfig);
  }
  //
  // Initialize communication for embOSView
  //
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_ETHERNET)
  UDP_Process_Init();
#endif
  //
  // Configure and initialize SEGGER SystemView
  //
#if ((OS_SUPPORT_TRACE_API != 0) && !(OS_64BIT))
  SEGGER_SYSVIEW_Conf();
#endif
  OS_INT_DecRI();
}

/*********************************************************************
*
*       Optional communication with embOSView
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_COM_Send1()
*
*  Function description
*    Sends one character.
*/
void OS_COM_Send1(OS_U8 c) {
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_ETHERNET)
  UDP_Process_Send1(c);
#elif (OS_VIEW_IFSELECT == OS_VIEW_DISABLED)
  OS_USE_PARA(c);          // Avoid compiler warning
  OS_COM_ClearTxActive();  // Let embOS know that Tx is not busy
#endif
}

#else
  #error "Unsupported simulation platform."
#endif

/*************************** End of file ****************************/
