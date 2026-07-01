/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : OptionsParse.c
Purpose : TraceHub numeric option parsing helpers
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "Options_internal.h"

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       TraceHubOptions_ParseU64()
*
*  Function description
*    Parse an unsigned 64-bit integer.
*
*  Parameters
*    str    String to parse.
*    value  Parsed value.
*    base   Numeric base.
*
*  Return value
*    0   Success.
*   -1   Invalid input or overflow.
*/
int TraceHubOptions_ParseU64(const char *str, uint64_t *value, int base) {
    char               *end;
    unsigned long long  parsed;

    if ((str == NULL) || (value == NULL) || (str[0] == '\0') ||
        (str[0] == '-') || (str[0] == '+')) {
        return -1;
    }

    errno = 0;
    parsed = strtoull(str, &end, base);
    if ((errno != 0) || (end == str) || (*end != '\0')) {
        return -1;
    }
#if ULLONG_MAX > UINT64_MAX
    if (parsed > UINT64_MAX) {
        return -1;
    }
#endif

    *value = (uint64_t)parsed;
    return 0;
}

/*********************************************************************
*
*       TraceHubOptions_ParseSize()
*
*  Function description
*    Parse an unsigned size value.
*
*  Parameters
*    str    String to parse.
*    value  Parsed value.
*    base   Numeric base.
*
*  Return value
*    0   Success.
*   -1   Invalid input or overflow.
*/
int TraceHubOptions_ParseSize(const char *str, size_t *value, int base) {
    uint64_t parsed;

    if ((value == NULL) || (TraceHubOptions_ParseU64(str, &parsed, base) != 0) || (parsed > SIZE_MAX)) {
        return -1;
    }

    *value = (size_t)parsed;
    return 0;
}

/*********************************************************************
*
*       TraceHubOptions_ParseUnsigned()
*
*  Function description
*    Parse an unsigned integer.
*
*  Parameters
*    str    String to parse.
*    value  Parsed value.
*    base   Numeric base.
*
*  Return value
*    0   Success.
*   -1   Invalid input or overflow.
*/
int TraceHubOptions_ParseUnsigned(const char *str, unsigned *value, int base) {
    uint64_t parsed;

    if ((value == NULL) || (TraceHubOptions_ParseU64(str, &parsed, base) != 0) || (parsed > UINT_MAX)) {
        return -1;
    }

    *value = (unsigned)parsed;
    return 0;
}

/*********************************************************************
*
*       TraceHubOptions_ParsePort()
*
*  Function description
*    Parse a TCP port number.
*
*  Parameters
*    str    String to parse.
*    value  Parsed port.
*
*  Return value
*    0   Success.
*   -1   Invalid input or out of TCP port range.
*/
int TraceHubOptions_ParsePort(const char *str, unsigned *value) {
    unsigned parsed;

    if ((TraceHubOptions_ParseUnsigned(str, &parsed, 10) != 0) || (parsed == 0u) || (parsed > 65535u)) {
        return -1;
    }

    *value = parsed;
    return 0;
}

/*************************** End of file ****************************/
