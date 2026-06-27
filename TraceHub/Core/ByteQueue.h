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
File    : ByteQueue.h
Purpose : Fixed-capacity byte ring queue
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_BYTE_QUEUE_H            // Guard against multiple inclusion
#define TRACEHUB_BYTE_QUEUE_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdbool.h>
#include <stddef.h>

#if defined(__cplusplus)         // Allow usage of this module from C++ files (disable name mangling)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

typedef struct {
    char   *buffer;
    size_t  capacity;
    size_t  read_pos;
    size_t  used;
} ByteQueue_t;

typedef enum {
    BYTE_QUEUE_WRITE_OK,
    BYTE_QUEUE_WRITE_OVERFLOW_WRITTEN,
    BYTE_QUEUE_WRITE_TOO_LARGE
} ByteQueue_WriteResult_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

void                    ByteQueue_Init       (ByteQueue_t *queue, char *buffer, size_t capacity);
bool                    ByteQueue_IsValid    (const ByteQueue_t *queue);
void                    ByteQueue_Clear      (ByteQueue_t *queue);
ByteQueue_WriteResult_t ByteQueue_Write      (ByteQueue_t *queue, const char *data, size_t num_bytes);
size_t                  ByteQueue_Read       (ByteQueue_t *queue, char *buffer, size_t buffer_size);
size_t                  ByteQueue_GetCapacity(const ByteQueue_t *queue);
size_t                  ByteQueue_GetUsed    (const ByteQueue_t *queue);

#if defined(__cplusplus)          // Allow usage of this module from C++ files (disable name mangling)
}                /* Make sure we have C-declarations in C++ programs */
#endif

#endif // end of include guard: TRACEHUB_BYTE_QUEUE_H Avoid multiple inclusion

/*************************** End of file ****************************/
