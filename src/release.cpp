/* <<< Release Notice for library >>> */

#include "proj.h"
#include "proj_internal.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

char const pj_version[] = STR(PROJ_VERSION_MAJOR) "." STR(
    PROJ_VERSION_MINOR) "." STR(PROJ_VERSION_PATCH);

const char *pj_get_version() { return pj_version; }

char const pj_release[] = "Rel. " STR(PROJ_VERSION_MAJOR) "." STR(
    PROJ_VERSION_MINOR) "." STR(PROJ_VERSION_PATCH) ", "
                                                    "September 1st, 2023";

const char *pj_get_release() { return pj_release; }
