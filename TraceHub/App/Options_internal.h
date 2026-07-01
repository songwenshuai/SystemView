/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : Options_internal.h
Purpose : Internal TraceHub option parsing contracts
---------------------------END-OF-HEADER------------------------------
*/

#ifndef TRACEHUB_OPTIONS_INTERNAL_H
#define TRACEHUB_OPTIONS_INTERNAL_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)

#define no_argument       0
#define required_argument 1

struct option {
    const char *name;
    int         has_arg;
    int        *flag;
    int         val;
};

extern char *optarg;
extern int   optind;

int getopt_long(int argc, char * const argv[],
                const char *optstring,
                const struct option *longopts,
                int *longindex);

#else

#include <getopt.h>

#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

enum {
    TRACEHUB_OPT_SHM = 1001,
    TRACEHUB_OPT_SYSTEMVIEW,
    TRACEHUB_OPT_LINUX,
    TRACEHUB_OPT_RTOS,
    TRACEHUB_OPT_LINUX_CHANNEL,
    TRACEHUB_OPT_RTOS_CHANNEL,
    TRACEHUB_OPT_SYSTEMVIEW_CHANNEL,
    TRACEHUB_OPT_SWIMLANE,
    TRACEHUB_OPT_RTT_TIMEOUT_MS,
    TRACEHUB_OPT_MEMSHM_RESET,
    TRACEHUB_OPT_LOG_DIR,
    TRACEHUB_OPT_SWIMLANE_WIDTH
};

/*********************************************************************
*
*       Parse functions
*
**********************************************************************
*/

int TraceHubOptions_ParseU64     (const char *str, uint64_t *value, int base);
int TraceHubOptions_ParseSize    (const char *str, size_t *value, int base);
int TraceHubOptions_ParseUnsigned(const char *str, unsigned *value, int base);
int TraceHubOptions_ParsePort    (const char *str, unsigned *value);

#endif /* TRACEHUB_OPTIONS_INTERNAL_H */

/*************************** End of file ****************************/
