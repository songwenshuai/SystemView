/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
*                                                                    *
*                    (c) 2023 - 2026 CineLogic                       *
*                                                                    *
*                  Support: wenshuaisong@gmail.com                   *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : TraceHubDefaults.h
Purpose : Compile-time defaults for the TraceHub application
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_DEFAULTS_H
#define TRACEHUB_DEFAULTS_H

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#ifndef TRACEHUB_DEFAULT_RTT_ADDR
  #define TRACEHUB_DEFAULT_RTT_ADDR 0u
#endif

#ifndef TRACEHUB_DEFAULT_RTT_SIZE
  #define TRACEHUB_DEFAULT_RTT_SIZE 0u
#endif

#ifndef TRACEHUB_DEFAULT_MEMORY_PATH
  #if defined(RTTMEM_USE_MEMSHM)
    #define TRACEHUB_DEFAULT_MEMORY_PATH "/rtt_sim"
  #else
    #define TRACEHUB_DEFAULT_MEMORY_PATH "/dev/shared_mem0"
  #endif
#endif

#endif /* TRACEHUB_DEFAULTS_H */

/*************************** End of file ****************************/
