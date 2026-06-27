/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : Utils.h
Purpose : Common utility functions for RTT bridge
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_UTILS_H            // Guard against multiple inclusion
#define TRACEHUB_UTILS_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

char *UTILS_LRealPath(const char *filename);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_UTILS_H Avoid multiple inclusion

/*************************** End of file ****************************/
