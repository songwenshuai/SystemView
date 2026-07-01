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

Purpose : SystemView protocol compatibility version.
*/

#ifndef SEGGER_SYSVIEW_VERSION_H
#define SEGGER_SYSVIEW_VERSION_H

//
// SystemView compatibility version number.
// Set to minimum required and compatible version of SystemView application.
// Does not reflect the version number of the SystemView Target Source release.
//
#define SEGGER_SYSVIEW_MAJOR          3
#define SEGGER_SYSVIEW_MINOR          32
#define SEGGER_SYSVIEW_REV            0
#define SEGGER_SYSVIEW_VERSION        ((SEGGER_SYSVIEW_MAJOR * 10000) + (SEGGER_SYSVIEW_MINOR * 100) + SEGGER_SYSVIEW_REV)

#endif

/*************************** End of file ****************************/
