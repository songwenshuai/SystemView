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
File    : LogSwimLane.c
Purpose : Swimlane log file writer
---------------------------END-OF-HEADER------------------------------
*/

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Log_internal.h"
#include "Utils.h"

/*********************************************************************
*
*       LOG_SwimLaneLogToFile()
*
*  Function description
*    Log a swimlane entry to file with timestamp and source.
*    Format: [timestamp_us] [SOURCE] content
*
*  Parameters
*    file          File handle to write to (or NULL to skip)
*    timestamp_us  Microsecond timestamp
*    source        Source identifier string
*    content       Log content string
*    state         Text sanitizer state
*
*  Return value
*    0   Success or skipped because file is NULL
*   -1   Invalid input or file write failed
*
*  Notes
*    (1) Appends newline if cleaned content doesn't end with one
*    (2) Flushes file after write for real-time logging
*    (3) Caller must create and manage file handle
*/
int LOG_SwimLaneLogToFile(FILE *file,
                          uint64_t timestamp_us,
                          const char *source,
                          const char *content,
                          LOG_TextCleanState_t *state) {
  size_t content_len;
  char   last_clean;
  bool   emitted_clean;

  if (file == NULL) {
    return 0;
  }
  if (source == NULL || content == NULL || state == NULL) {
    return -1;
  }

  //
  // The swimlane prefix is TraceHub metadata; only target content is sanitized.
  //
  errno = 0;
  if (fprintf(file, "[%" PRIu64 "] [%s] ", timestamp_us, source) < 0) {
    return -1;
  }
  content_len = strlen(content);
  if (_Log_WriteCleanTextRange(file,
                               content,
                               content_len,
                               state,
                               &last_clean,
                               &emitted_clean,
                               NULL) != 0) {
    return -1;
  }

  //
  // Preserve one physical line per swimlane entry after text cleanup.
  //
  if (!emitted_clean || last_clean != '\n') {
    errno = 0;
    if (fprintf(file, "\n") < 0) {
      return -1;
    }
  }

  errno = 0;
  if (fflush(file) != 0) {
    return -1;
  }

  return 0;
}
