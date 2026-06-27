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
File    : RTTMemory.h
Purpose : Low-level RTT memory mapping backend abstraction
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_RTTMEMORY_H
#define TRACEHUB_RTTMEMORY_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#ifndef TRACEHUB_MIN
  #define TRACEHUB_MIN(a,b)         (((a) < (b)) ? (a) : (b))
#endif

/*********************************************************************
*
*       Types - Backend abstraction
*
**********************************************************************
*/

/**
 * @brief Backend operations structure for memory access abstraction
 *
 * This structure defines the interface that all memory backends must implement.
 * Different backends (smem, memshm, etc.) provide their own implementations
 * of these operations, allowing the upper layer to be backend-agnostic.
 */
typedef struct RTTMem_Backend {
    const char *name;  /**< Backend name for identification */

    /**
     * @brief Initialize the backend
     * @param path   Device path or shared memory name
     * @param base   Backend base address (0 to auto-detect)
     * @param size   Memory region size (0 to auto-detect)
     * @return 0 on success, -1 on failure
     */
    int (*init)(const char *path, uint64_t base, size_t size);

    /**
     * @brief Cleanup and release resources
     */
    void (*cleanup)(void);

    /**
     * @brief Get the backend base address of the mapped region
     * @return Backend base address, or 0 if not initialized
     */
    uint64_t (*get_base)(void);

    /**
     * @brief Get the size of the mapped region
     * @return Size in bytes, or 0 if not initialized
     */
    size_t (*get_size)(void);

    /**
     * @brief Convert a backend address to a directly accessible local address
     * @param addr  Backend address
     * @param size  Number of bytes that must be accessible from addr
     * @return Local process address, or 0 on error
     */
    uintptr_t (*to_local_address)(uint64_t addr, size_t size);
} RTTMem_Backend_t;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/

int                      RTTMem_InstallBackend       (const RTTMem_Backend_t *backend);
const RTTMem_Backend_t * RTTMem_GetBackend           (void);
const char *             RTTMem_GetBackendName       (void);
void                     RTTMem_SetResetOnInit       (bool enabled);
bool                     RTTMem_GetResetOnInit       (void);
const RTTMem_Backend_t * RTTMem_Backend_SMEM         (void);
const RTTMem_Backend_t * RTTMem_Backend_MEMSHM       (void);
int                      RTTMem_InstallDefaultBackend(void);
int                      RTTMem_Init                 (const char *device_path);
int                      RTTMem_InitEx               (const char *path, uint64_t base, size_t size);
void                     RTTMem_Cleanup              (void);
uint64_t                 RTTMem_GetBackendBase       (void);
size_t                   RTTMem_GetMappedSize        (void);
uintptr_t                RTTMem_ToLocalAddress       (uint64_t address, size_t size);

#if defined(__cplusplus)
}
#endif

#endif /* TRACEHUB_RTTMEMORY_H */

/*************************** End of file ****************************/
