// The level of the trace output of the port builds.
//
// The desktop build reads the level from the PORT_TRACE_LEVEL environment
// variable and defaults to two. The browser build takes it from the query
// string of the page and defaults to zero, because every marker of the
// browser build is sent to the dev server with a synchronous request (see
// port/web/web_trace.c).

#include "common.h"
#include "port/port_trace.h"

#if defined(TARGET_WEB)

extern int web_trace_level(void);

int port_trace_level(void) {
    return web_trace_level();
}

#else

#include <stdlib.h>

int port_trace_level(void) {
    static int level = -1;

    if (level < 0) {
        const char* value = getenv("PORT_TRACE_LEVEL");
        level = (value == NULL) ? 2 : atoi(value);
    }

    return level;
}

#endif
