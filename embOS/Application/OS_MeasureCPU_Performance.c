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
File    : OS_MeasureCPU_Performance.c
Purpose : embOS sample program that checks the performance of the
          entire system, outputting one result value per second. Higher
          values indicate better performance.
          The program computes the first 1000 prime numbers in a loop.
          The output value is the number of times this could be performed
          within one second.
Reference values -  RAM configuration:

Core           Device          Frequency   IDE/Compiler           Executed From      Loops/sec   Loops/sec/Mhz     Additional information
=========================================================================================================================================
ARM926EJ-S                      201.0MHz   IAR EWARM V5.20                               11834         58.9        Thumb Mode, fully cached
ARM926EJ-S                      201.0MHz   IAR EWARM V5.20                               12497         62.2        ARM   Mode, fully cached
ARM926EJ-S                      180.0MHz   IAR EWARM V5.30.2                             16829         93.5        ARM   Mode, fully cached
ARM7-TDMI                        47.9MHz   IAR EWARM V5.20                                2656         55.4        Thumb Mode
ARM7-TDMI                        47.9MHz   IAR EWARM V5.20                                2646         55.2        ARM   Mode
Cortex-M7      SAMV71Q21        300.0MHz   ES V4.22               Flash                  35546        118.5
Cortex-M7      iMXRT1064        600.0MHz   ES V4.22               Flash                  73141        121.9
Cortex-M33     LPC55S69         100.0MHz   ES V4.22               Flash                   5970         59.7
Cortex-M0      STM32F072         48.0MHz   ES V4.22               Flash                   1632         34.0
Cortex-M3      STM32F207        120.0MHz   ES V4.22               Flash                   6901         57.5
Cortex-M4      STM32F429        168.0MHz   ES V4.22               Flash                   8874         52.8
Cortex-M7      STM32F756        200.0MHz   ES V4.22               Flash                  24505        122.5
Cortex-M4      STM32L4R9        120.0MHz   ES V4.22               Flash                   6690         55.8
Cortex-M7      STM32H743        400.0MHz   ES V4.22               Flash                  47472        118.7
Cortex-A9      R7S72100         399.9MHz   ES V4.22               SRAM                   37983         95.0
Cortex-A9      R7S72100         399.9MHz   ES V4.22               SPI Flash              33448         83.6
Cortex-A5      ATSAMA5D36       498.0MHz   ES V4.22               SDRAM                  33721         67.7
ARM7TDMI       AT91SAM7S256      47.9MHz   ES V4.22               Flash                   1504         31.4
ARM926EJ-S     AT91SAM9263      200.0MHz   ES V4.22               SRAM                   12064         60.3
RX72T          R5F572TK         200.0MHz   IAR EWRX V4.12A        Flash                  21899        109.5
*/

#include "RTOS.h"
#include <stdio.h>
#include <stdlib.h>

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int StackHP[128];  // Task stack
static OS_TASK         TCBHP;         // Task control block
static char            aIsPrime[1000];
static unsigned int    NumPrimes;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _CalcPrimes()
*/
static void _CalcPrimes(unsigned int NumItems) {
  unsigned int i;
  unsigned int j;

  //
  // Mark all as potential prime numbers
  //
  memset(aIsPrime, 1, NumItems);
  //
  // 2 deserves a special treatment
  //
  for (i = 4; i < NumItems; i += 2) {
    aIsPrime[i] = 0;     // Cross it out: not a prime
  }
  //
  // Cross out multiples of every prime starting at 3. Crossing out starts at i^2.
  //
  for (i = 3; i * i < NumItems; i++) {
    if (aIsPrime[i]) {
      j = i * i;    // The square of this prime is the first we need to cross out
      do {
        aIsPrime[j] = 0;     // Cross it out: not a prime
        j += 2 * i;          // Skip even multiples (only 3*, 5*, 7* etc)
      } while (j < NumItems);
    }
  }
  //
  // Count prime numbers
  //
  NumPrimes = 0;
  for (i = 2; i < NumItems; i++) {
    if (aIsPrime[i]) {
      NumPrimes++;
    }
  }
}

/*********************************************************************
*
*       _PrintDec()
*/
static void _PrintDec(unsigned int v) {
  unsigned int Digit;
  unsigned int r;

  Digit = 10;
  while (Digit < v) {
    Digit *= 10;
  }
  do {
    Digit /= 10;
    r = v / Digit;
    v -= r * Digit;
    putchar(r + '0');
  } while (v | (Digit > 1));
}

/*********************************************************************
*
*       _PrintResult()
*/
static void _PrintResult(unsigned int Cnt) {
  if (NumPrimes != 168) {
    puts("Error");
  } else {
    puts("Loops/sec:");
    _PrintDec(Cnt);
  }
  puts("\n");
}

/*********************************************************************
*
*       HPTask()
*/
static void HPTask(void) {
  unsigned int Cnt;
  OS_U64       tEnd;
  OS_U64       CyclesPerSecond;

  CyclesPerSecond = OS_TIME_Convertms2Cycles(1000u);
  while(1) {
    Cnt = 0;
    OS_TASK_Delay(1);  // Sync to tick
    tEnd = OS_TIME_Get_Cycles() + CyclesPerSecond;
    while (tEnd >= OS_TIME_Get_Cycles()) {
      _CalcPrimes(sizeof(aIsPrime));
      Cnt++;
    }
    _PrintResult(Cnt);
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware
  OS_TASK_CREATE(&TCBHP, "HP Task", 100, HPTask, StackHP);
  OS_Start();   // Start embOS
  return 0;
}

/*************************** End of file ****************************/
