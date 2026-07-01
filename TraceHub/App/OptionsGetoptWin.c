/*********************************************************************
*                             CineLogic                              *
*                      RTT Trace and Debug Bridge                    *
**********************************************************************
----------------------------------------------------------------------
File    : OptionsGetoptWin.c
Purpose : Windows getopt_long compatibility layer
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "Options_internal.h"

#if defined(_WIN32)

#include <string.h>

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/

char *optarg = NULL;
int   optind = 1;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _WinGetOptFindShort()
*
*  Function description
*    Resolve a short option argument contract for the Windows getopt shim.
*/
static int _WinGetOptFindShort(const char *optstring, int opt) {
    const char *p;

    if (optstring == NULL) {
        return 0;
    }
    p = strchr(optstring, opt);
    if (p == NULL) {
        return 0;
    }
    return (p[1] == ':') ? required_argument : no_argument;
}

/*********************************************************************
*
*       Public functions
*
**********************************************************************
*/

/*********************************************************************
*
*       getopt_long()
*
*  Function description
*    Minimal getopt_long-compatible parser used on Windows builds.
*/
int getopt_long(int argc, char * const argv[],
                const char *optstring,
                const struct option *longopts,
                int *longindex) {
    const char *arg;
    const char *name;
    const char *value;
    size_t      name_len;
    int         i;
    int         need_arg;

    optarg = NULL;
    if (optind >= argc) {
        return -1;
    }

    arg = argv[optind];
    if (arg == NULL || arg[0] != '-' || arg[1] == '\0') {
        return -1;
    }
    if (strcmp(arg, "--") == 0) {
        optind++;
        return -1;
    }

    if (arg[1] == '-') {
        name = arg + 2;
        value = strchr(name, '=');
        name_len = (value == NULL) ? strlen(name) : (size_t)(value - name);
        if (name_len == 0u) {
            optind++;
            return '?';
        }
        for (i = 0; longopts != NULL && longopts[i].name != NULL; i++) {
            if (strlen(longopts[i].name) == name_len &&
                strncmp(longopts[i].name, name, name_len) == 0) {
                if (longindex != NULL) {
                    *longindex = i;
                }
                if (longopts[i].has_arg == required_argument) {
                    if (value != NULL) {
                        optarg = (char *)(value + 1);
                    } else {
                        optind++;
                        if (optind >= argc) {
                            return '?';
                        }
                        optarg = argv[optind];
                    }
                } else if (value != NULL) {
                    optind++;
                    return '?';
                }
                optind++;
                if (longopts[i].flag != NULL) {
                    *longopts[i].flag = longopts[i].val;
                    return 0;
                }
                return longopts[i].val;
            }
        }
        optind++;
        return '?';
    }

    need_arg = _WinGetOptFindShort(optstring, (unsigned char)arg[1]);
    if (need_arg == 0 && strchr(optstring, (unsigned char)arg[1]) == NULL) {
        optind++;
        return '?';
    }
    if (need_arg == required_argument) {
        if (arg[2] != '\0') {
            optarg = (char *)(arg + 2);
        } else {
            optind++;
            if (optind >= argc) {
                return '?';
            }
            optarg = argv[optind];
        }
    }
    optind++;
    return (unsigned char)arg[1];
}

#endif

/*************************** End of file ****************************/
