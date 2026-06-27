/*********************************************************************
*                                                                    *
*                Copyright (C) 2023 xrTest Inc.                      *
*                      All rights reserved                           *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : Utils.c
Purpose : Common utility functions for RTT bridge
Author  : songwenshuai <songwenshuai@gmail.com>
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdlib.h>
#include <string.h>

#if defined(__linux__) || defined(__APPLE__)
#include <limits.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#include "Utils.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

//
// Maximum path length including null terminator.
//
#define REAL_PATH_MAX  4096

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       UTILS_LRealPath()
*
*  Function description
*    Get canonical absolute path for a filename.
*    Resolves symbolic links and relative paths.
*
*  Parameters
*    filename  Path to resolve (relative or absolute)
*
*  Return value
*    Pointer to dynamically allocated string with absolute path
*    Caller must free the returned string
*
*  Notes
*    (1) Platform-specific implementation (POSIX vs Windows)
*    (2) Returns copy of original filename if resolution fails
*/
char *UTILS_LRealPath(const char *filename) {
  // Method 1: The system has a compile time upper bound on a filename
  // path.  Use that and realpath() to canonicalize the name.  This is
  // the most common case.  Note that, if there isn't a compile time
  // upper bound, you want to avoid realpath() at all costs.
#if defined(__linux__) || defined(__APPLE__)
  {
    char buf[REAL_PATH_MAX];
    const char *rp = realpath(filename, buf);
    if (rp == NULL)
      rp = filename;
    return strdup(rp);
  }
#elif defined(_WIN32)
  // The MS Windows method.  If we don't have realpath, we assume we
  // don't have symlinks and just canonicalize to a Windows absolute
  // path.  GetFullPath converts ../ and ./ in relative paths to
  // absolute paths, filling in current drive if one is not given
  // or using the current directory of a specified drive (eg, "E:foo").
  // It also converts all forward slashes to back slashes.
  {
    char buf[MAX_PATH];
    char *basename;
    DWORD len = GetFullPathName(filename, MAX_PATH, buf, &basename);
    if (len == 0 || len > MAX_PATH - 1)
      return _strdup(filename);
    else {
      // The file system is case-preserving but case-insensitive,
      // Canonicalize to lowercase, using the codepage associated
      // with the process locale.
      CharLowerBuff(buf, len);
      return _strdup(buf);
    }
  }
#else
  return strdup(filename);
#endif
}

/*************************** End of file ****************************/
