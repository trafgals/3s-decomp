#include "common.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#define SYMBOL_NAME_MAX 256
#elif !defined(__EMSCRIPTEN__)
// Emscripten has no execinfo.h, so it gets no native stack trace.
#include <execinfo.h>
#else
#include <emscripten/emscripten.h>
#include <emscripten/stack.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(TARGET_WEB)
#include "port/web/web_trace.h"
#endif

#define BACKTRACE_MAX 100

void fatal_error(const s8* fmt, ...) __dead2 {
    va_list args;
    va_start(args, fmt);

#if defined(TARGET_WEB)
    // Report the failure to the dev server as well; the browser tears the
    // page down before the error can be shown on screen.
    {
        va_list copy;
        char message[256];

        va_copy(copy, args);
        vsnprintf(message, sizeof(message), fmt, copy);
        va_end(copy);
        web_trace(message);
    }
#endif

    fprintf(stderr, "Fatal error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
    void* buffer[BACKTRACE_MAX];
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    int nptrs = backtrace(buffer, BACKTRACE_MAX);
    fprintf(stderr, "Stack trace:\n");
    backtrace_symbols_fd(buffer, nptrs, fileno(stderr));
#elif defined(__EMSCRIPTEN__)
    fprintf(stderr, "Stack trace:\n");
#else
    fprintf(stderr, "Stack trace:\n");
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    int nptrs = CaptureStackBackTrace(0, BACKTRACE_MAX, buffer, NULL);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(1, sizeof(SYMBOL_INFO) + SYMBOL_NAME_MAX);
    if (!symbol) {
        fprintf(stderr, "Calloc failed when allocating SYMBOL_INFO, bailing!\n\n");
        SymCleanup(process);
        abort();
    }
    symbol->MaxNameLen = SYMBOL_NAME_MAX;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    for (int i = 0; i < nptrs; i++) {
        SymFromAddr(process, (DWORD64)buffer[i], 0, symbol);
        fprintf(stderr, "%i: %s - 0x%0llX\n", nptrs - i - 1, symbol->Name, symbol->Address);
    }
    free(symbol);
    SymCleanup(process);
#endif
    abort();
}

void not_implemented(const s8* func) __dead2 {
    fatal_error("Function not implemented: %s\n", func);
}

void debug_print(const char* fmt, ...) {
#if defined(DEBUG)
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
#endif
}
