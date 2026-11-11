#ifndef WEB_TRACE_H
#define WEB_TRACE_H

// Report a trace point to the dev server. Only used by the web build.
void web_trace(const char* text);

// Same, but with a printf style format string.
void web_tracef(const char* format, ...);

// The level of the trace output, taken from the query string of the page.
int web_trace_level(void);

// Send a note to the dev server without waiting for the answer. The notes
// are meant for the diagnosis of a stall, so they do not block the page.
void web_note(const char* text);

#endif // WEB_TRACE_H
