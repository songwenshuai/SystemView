/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : CLI.h
Purpose : Command line banner and usage output
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_CLI_H
#define TRACEHUB_CLI_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

void CLI_PrintBanner(FILE *stream);
void CLI_PrintUsage(void);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_CLI_H */

/*************************** End of file ****************************/
