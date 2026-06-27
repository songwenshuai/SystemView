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
File    : ByteQueue.c
Purpose : Fixed-capacity byte ring queue
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <string.h>

#include "ByteQueue.h"

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       ByteQueue_Init()
*/
void ByteQueue_Init(ByteQueue_t *queue, char *buffer, size_t capacity) {
    if (queue == NULL) {
        return;
    }

    queue->buffer   = buffer;
    queue->capacity = capacity;
    queue->read_pos = 0u;
    queue->used     = 0u;
}

/*********************************************************************
*
*       ByteQueue_IsValid()
*/
bool ByteQueue_IsValid(const ByteQueue_t *queue) {
    return (queue != NULL) && (queue->buffer != NULL) && (queue->capacity > 0u);
}

/*********************************************************************
*
*       ByteQueue_Clear()
*/
void ByteQueue_Clear(ByteQueue_t *queue) {
    if (queue == NULL) {
        return;
    }

    queue->read_pos = 0u;
    queue->used     = 0u;
}

/*********************************************************************
*
*       ByteQueue_Write()
*/
ByteQueue_WriteResult_t ByteQueue_Write(ByteQueue_t *queue, const char *data, size_t num_bytes) {
    size_t                  write_pos;
    size_t                  first_chunk;
    ByteQueue_WriteResult_t result;

    if (num_bytes == 0u) {
        return BYTE_QUEUE_WRITE_OK;
    }
    if (data == NULL) {
        return BYTE_QUEUE_WRITE_TOO_LARGE;
    }
    if (!ByteQueue_IsValid(queue) || (num_bytes > queue->capacity)) {
        return BYTE_QUEUE_WRITE_TOO_LARGE;
    }

    result = BYTE_QUEUE_WRITE_OK;
    if (num_bytes > (queue->capacity - queue->used)) {
        ByteQueue_Clear(queue);
        result = BYTE_QUEUE_WRITE_OVERFLOW_WRITTEN;
    }

    write_pos = (queue->read_pos + queue->used) % queue->capacity;
    first_chunk = queue->capacity - write_pos;
    if (first_chunk > num_bytes) {
        first_chunk = num_bytes;
    }

    memcpy(&queue->buffer[write_pos], data, first_chunk);
    if (first_chunk < num_bytes) {
        memcpy(&queue->buffer[0], data + first_chunk, num_bytes - first_chunk);
    }
    queue->used += num_bytes;

    return result;
}

/*********************************************************************
*
*       ByteQueue_Read()
*/
size_t ByteQueue_Read(ByteQueue_t *queue, char *buffer, size_t buffer_size) {
    size_t num_bytes;
    size_t first_chunk;

    if (!ByteQueue_IsValid(queue) || (buffer == NULL) || (buffer_size == 0u)) {
        return 0u;
    }

    num_bytes = queue->used;
    if (num_bytes > buffer_size) {
        num_bytes = buffer_size;
    }
    if (num_bytes == 0u) {
        return 0u;
    }

    first_chunk = queue->capacity - queue->read_pos;
    if (first_chunk > num_bytes) {
        first_chunk = num_bytes;
    }

    memcpy(buffer, &queue->buffer[queue->read_pos], first_chunk);
    if (first_chunk < num_bytes) {
        memcpy(buffer + first_chunk, &queue->buffer[0], num_bytes - first_chunk);
    }

    queue->read_pos = (queue->read_pos + num_bytes) % queue->capacity;
    queue->used -= num_bytes;
    if (queue->used == 0u) {
        queue->read_pos = 0u;
    }

    return num_bytes;
}

/*********************************************************************
*
*       ByteQueue_GetCapacity()
*/
size_t ByteQueue_GetCapacity(const ByteQueue_t *queue) {
    if (queue == NULL) {
        return 0u;
    }
    return queue->capacity;
}

/*********************************************************************
*
*       ByteQueue_GetUsed()
*/
size_t ByteQueue_GetUsed(const ByteQueue_t *queue) {
    if (queue == NULL) {
        return 0u;
    }
    return queue->used;
}

/*************************** End of file ****************************/
