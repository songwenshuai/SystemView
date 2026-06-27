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
*                                                                    *
*       CineLogic TraceHub * RTT trace and debug bridge              *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* CineLogic strongly recommends to not make any changes              *
* to or modify the source code of this software in order to stay     *
* compatible with the SharedMem and RTT data path.                   *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL CINELOGIC BE LIABLE FOR              *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File    : LogEntry.c
Purpose : Log entry data structure implementation
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

#include "LogEntry.h"

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LogEntry_Create()
*
*  Function description
*    Create a new log entry with the specified parameters.
*    Memory for content is dynamically allocated.
*
*  Parameters
*    timestamp_us  Unified microsecond timestamp
*    source        Log source identifier
*    content       Log content string (will be copied)
*    content_len   Length of content string
*
*  Return value
*    Pointer to newly created LogEntry_t, or NULL on failure
*
*  Note
*    Caller must call LogEntry_Destroy() to free allocated memory.
*/
LogEntry_t* LogEntry_Create(uint64_t timestamp_us, LogSource_t source,
                            const char *content, size_t content_len) {
    return LogEntry_CreateEx(timestamp_us, source, 0u, false, false, content, content_len);
}

/*********************************************************************
*
*       LogEntry_CreateEx()
*
*  Function description
*    Create a new log entry with explicit ordering and fragment metadata.
*/
LogEntry_t* LogEntry_CreateEx(uint64_t timestamp_us, LogSource_t source,
                              uint64_t sequence,
                              bool fragment_continuation,
                              bool fragment_continues,
                              const char *content, size_t content_len) {
    LogEntry_t *entry;
    //
    // Validate parameters
    //
    if (content == NULL || content_len == 0) {
        return NULL;
    }
    if (content_len > LOG_ENTRY_MAX_CONTENT_LEN) {
        return NULL;
    }
    if (source >= LOG_SOURCE_MAX) {
        return NULL;
    }
    //
    // Allocate entry structure
    //
    entry = (LogEntry_t *)malloc(sizeof(LogEntry_t));
    if (entry == NULL) {
        return NULL;
    }
    //
    // Allocate and copy content string
    //
    entry->content = (char *)malloc(content_len + 1);
    if (entry->content == NULL) {
        free(entry);
        return NULL;
    }
    memcpy(entry->content, content, content_len);
    entry->content[content_len] = '\0';
    //
    // Initialize fields
    //
    entry->timestamp_us           = timestamp_us;
    entry->source                 = source;
    entry->sequence               = sequence;
    entry->content_len            = content_len;
    entry->fragment_continuation  = fragment_continuation;
    entry->fragment_continues     = fragment_continues;
    entry->valid                  = true;

    return entry;
}

/*********************************************************************
*
*       LogEntry_Clone()
*
*  Function description
*    Create a deep copy of a log entry.
*
*  Parameters
*    entry  Pointer to LogEntry_t to clone
*
*  Return value
*    Pointer to newly created LogEntry_t copy, or NULL on failure
*
*  Note
*    Caller must call LogEntry_Destroy() to free allocated memory.
*/
LogEntry_t* LogEntry_Clone(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return NULL;
    }
    return LogEntry_CreateEx(entry->timestamp_us, entry->source,
                             entry->sequence,
                             entry->fragment_continuation,
                             entry->fragment_continues,
                             entry->content, entry->content_len);
}

/*********************************************************************
*
*       LogEntry_Destroy()
*
*  Function description
*    Destroy a log entry and free all associated memory.
*
*  Parameters
*    entry  Pointer to LogEntry_t to destroy
*/
void LogEntry_Destroy(LogEntry_t *entry) {
    if (entry == NULL) {
        return;
    }
    //
    // Free content string if allocated
    //
    if (entry->content != NULL) {
        free(entry->content);
        entry->content = NULL;
    }
    //
    // Mark entry as invalid before freeing
    //
    entry->valid = false;
    //
    // Free entry structure
    //
    free(entry);
}

/*********************************************************************
*
*       LogEntry_GetTimestamp()
*
*  Function description
*    Get the timestamp from a log entry.
*
*  Parameters
*    entry  Pointer to LogEntry_t
*
*  Return value
*    Timestamp in microseconds, or 0 if entry is invalid
*/
uint64_t LogEntry_GetTimestamp(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return 0;
    }
    return entry->timestamp_us;
}

/*********************************************************************
*
*       LogEntry_GetSource()
*
*  Function description
*    Get the source identifier from a log entry.
*
*  Parameters
*    entry  Pointer to LogEntry_t
*
*  Return value
*    Log source identifier
*/
LogSource_t LogEntry_GetSource(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return LOG_SOURCE_LINUX;
    }
    return entry->source;
}

/*********************************************************************
*
*       LogEntry_GetSequence()
*
*  Function description
*    Get the source-local sequence number from a log entry.
*/
uint64_t LogEntry_GetSequence(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return 0u;
    }
    return entry->sequence;
}

/*********************************************************************
*
*       LogEntry_GetContent()
*
*  Function description
*    Get the content string from a log entry.
*
*  Parameters
*    entry  Pointer to LogEntry_t
*
*  Return value
*    Pointer to content string, or NULL if entry is invalid
*/
const char* LogEntry_GetContent(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return NULL;
    }
    return entry->content;
}

/*********************************************************************
*
*       LogEntry_GetContentLen()
*
*  Function description
*    Get the content length from a log entry.
*
*  Parameters
*    entry  Pointer to LogEntry_t
*
*  Return value
*    Content length in bytes
*/
size_t LogEntry_GetContentLen(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return 0;
    }
    return entry->content_len;
}

/*********************************************************************
*
*       LogEntry_IsFragmentContinuation()
*
*  Function description
*    Return whether this entry continues a previous line fragment.
*/
bool LogEntry_IsFragmentContinuation(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return false;
    }
    return entry->fragment_continuation;
}

/*********************************************************************
*
*       LogEntry_FragmentContinues()
*
*  Function description
*    Return whether another fragment follows this entry.
*/
bool LogEntry_FragmentContinues(const LogEntry_t *entry) {
    if (entry == NULL || !entry->valid) {
        return false;
    }
    return entry->fragment_continues;
}

/*********************************************************************
*
*       LogEntry_IsValid()
*
*  Function description
*    Check if a log entry is valid.
*
*  Parameters
*    entry  Pointer to LogEntry_t
*
*  Return value
*    true   Entry is valid
*    false  Entry is invalid or NULL
*/
bool LogEntry_IsValid(const LogEntry_t *entry) {
    if (entry == NULL) {
        return false;
    }
    if (!entry->valid) {
        return false;
    }
    if (entry->content == NULL || entry->content_len == 0) {
        return false;
    }
    if (entry->content_len > LOG_ENTRY_MAX_CONTENT_LEN) {
        return false;
    }
    if (entry->source >= LOG_SOURCE_MAX) {
        return false;
    }
    return true;
}

/*********************************************************************
*
*       LogEntry_Compare()
*
*  Function description
*    Compare two log entries by timestamp and source for sorting.
*
*  Parameters
*    a  Pointer to first LogEntry_t
*    b  Pointer to second LogEntry_t
*
*  Return value
*    < 0  a comes before b
*    = 0  a and b have same timestamp and source
*    > 0  a comes after b
*/
int LogEntry_Compare(const LogEntry_t *a, const LogEntry_t *b) {
    uint64_t ts_a;
    uint64_t ts_b;
    bool     valid_a;
    bool     valid_b;
    //
    // Handle invalid entries
    //
    valid_a = LogEntry_IsValid(a);
    valid_b = LogEntry_IsValid(b);

    if (!valid_a && !valid_b) {
        return 0;
    }
    if (!valid_a) {
        return 1;
    }
    if (!valid_b) {
        return -1;
    }
    //
    // Compare timestamps
    //
    ts_a = LogEntry_GetTimestamp(a);
    ts_b = LogEntry_GetTimestamp(b);

    if (ts_a < ts_b) {
        return -1;
    } else if (ts_a > ts_b) {
        return 1;
    }

    if (a->source < b->source) {
        return -1;
    } else if (a->source > b->source) {
        return 1;
    }

    if (a->sequence < b->sequence) {
        return -1;
    } else if (a->sequence > b->sequence) {
        return 1;
    }

    return 0;
}

/*************************** End of file ****************************/
