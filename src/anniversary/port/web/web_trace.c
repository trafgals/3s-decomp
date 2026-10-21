// Web build tracing.
//
// The browser build can block the main thread (the game loop is a blocking
// loop, and some of the work it does is synchronous), which makes it hard to
// tell how far it got from the JavaScript side. These trace points report
// progress to the dev server, which logs them like any other request.
//
// The report is sent with a synchronous request on purpose: the beacons of
// the browser are dropped when the page switches the stack of a fiber, and a
// trace that arrives half of the time is worse than no trace at all.

#include "common.h"
#include "port/web/web_trace.h"

#include <emscripten/emscripten.h>

#include <stdarg.h>
#include <stdio.h>

EM_JS(void, web_trace_send, (const char* text), {
    var xhr = new XMLHttpRequest();

    try {
        xhr.open('POST', '/__trace?t=' + encodeURIComponent(UTF8ToString(text)), false);
        xhr.send(null);
    } catch (e) {
        // The dev server is optional, so a failure to report is not fatal.
    }
});

// The level of the trace output, taken from the query string of the page
// ("?trace=1" turns the milestones on, "?trace=2" adds the internals of the
// thread scheduler). It is zero by default, because a marker of the browser
// build costs a synchronous request and the game loop produces thousands of
// them per second.
EM_JS(int, web_trace_level_query, (), {
    var value = parseInt(new URLSearchParams(location.search).get('trace') || '0', 10);
    return isNaN(value) ? 0 : value;
});

int web_trace_level(void) {
    static int level = -1;

    if (level < 0) {
        level = web_trace_level_query();
    }

    return level;
}

void web_trace(const char* text) {
    web_trace_send(text);
}

// Send a note to the dev server without waiting for the answer.
//
// The progress markers of the game travel with a synchronous request, which
// blocks the page. A note is meant for the diagnosis of a stall, so it goes
// with a beacon: a beacon does not wait for the answer and is sent even when
// the stack of the game is stuck.
EM_JS(void, web_note_send, (const char* text), {
    try {
        navigator.sendBeacon('/__trace?t=' + encodeURIComponent(UTF8ToString(text)));
    } catch (e) {
        // The dev server is optional, so a failure to report is not fatal.
    }
});

void web_note(const char* text) {
    web_note_send(text);
}

void web_tracef(const char* format, ...) {
    char text[512];
    va_list args;

    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    web_trace_send(text);
}
