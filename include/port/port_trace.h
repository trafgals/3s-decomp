#pragma once

// Progress markers for the port builds.
//
// The browser build blocks the main thread of the page while the game runs,
// so the only way to see how far it got is to report the progress to the dev
// server. The desktop builds print the same markers, which makes it possible
// to compare the two.
//
// The retail PS2 build must stay byte-identical, so the macro is empty there.
// The calls therefore have to sit inside an `#if !defined(TARGET_PS2)` block:
// an empty macro still leaves its argument in the preprocessed source.
//
// The markers are gated by a level, because the browser build sends every
// marker to the server with a *synchronous* request. A game that runs at
// sixty frames per second produces thousands of markers per second, and that
// would slow the game down to a crawl. The level of the browser build is
// taken from the query string of the page ("?trace=1"), and it is zero by
// default. The desktop build defaults to two, and reads the level from the
// PORT_TRACE_LEVEL environment variable.
//
//  0 = no markers at all
//  1 = the milestones of the boot and of the game loop
//  2 = the milestones and the internals of the thread scheduler

#if defined(TARGET_PS2)

#define PORT_TRACE(...)
#define PORT_TRACE_V(...)

#else

// The level of the trace output. Zero means that nothing is reported.
int port_trace_level(void);

#if defined(TARGET_WEB)

#include "port/web/web_trace.h"

#define PORT_TRACE(...)                                                                                                \
    do {                                                                                                               \
        if (port_trace_level() >= 1) {                                                                                 \
            web_tracef(__VA_ARGS__);                                                                                   \
        }                                                                                                              \
    } while (0)

#define PORT_TRACE_V(...)                                                                                              \
    do {                                                                                                               \
        if (port_trace_level() >= 2) {                                                                                 \
            web_tracef(__VA_ARGS__);                                                                                   \
        }                                                                                                              \
    } while (0)

#else

#include <stdio.h>

#define PORT_TRACE(...)                                                                                                \
    do {                                                                                                               \
        if (port_trace_level() >= 1) {                                                                                 \
            printf("[trace] " __VA_ARGS__);                                                                            \
        }                                                                                                              \
    } while (0)

#define PORT_TRACE_V(...)                                                                                              \
    do {                                                                                                               \
        if (port_trace_level() >= 2) {                                                                                 \
            printf("[trace] " __VA_ARGS__);                                                                            \
        }                                                                                                              \
    } while (0)

#endif

#endif
