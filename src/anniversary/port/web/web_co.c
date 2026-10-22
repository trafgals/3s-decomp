// Web build coroutines.
//
// The desktop port uses libco to emulate the cooperative threads of the PS2.
// libco switches the stack with setjmp/longjmp, which does not survive
// ASYNCIFY - and the browser build needs ASYNCIFY, so that the blocking game
// loop can let the page breathe.
//
// Emscripten has a fiber API that does the same job on top of ASYNCIFY, so
// the browser build uses that instead. This file provides the four libco
// functions the port uses, backed by those fibers.

#include "common.h"

#include "libco.h"
#include "port/port_trace.h"

#include <emscripten/fiber.h>

#include <stdbool.h>
#include <stdlib.h>

// The C stack of a thread and the stack that holds the frames that ASYNCIFY
// unwinds when the thread is suspended.
#define WEB_CO_STACK_SIZE (1024 * 1024)
#define WEB_CO_ASYNCIFY_STACK_SIZE (256 * 1024)

// The main context holds the whole game stack, so it needs more room.
#define WEB_CO_MAIN_ASYNCIFY_STACK_SIZE (1024 * 1024)

typedef struct {
    emscripten_fiber_t fiber;
    void (*entry)(void);
    void* c_stack;
    void* asyncify_stack;
} WebCo;

static WebCo main_co;
static bool main_co_ready = false;
static WebCo* current_co = NULL;

static void web_co_entry(void* arg) {
    WebCo* co = (WebCo*)arg;

    PORT_TRACE("web: a thread starts");
    co->entry();
    PORT_TRACE("web: a thread ends");
}

cothread_t co_active(void) {
    if (!main_co_ready) {
        // The main context is the one that is running right now, so it is
        // captured instead of being created from an entry point.
        main_co.asyncify_stack = malloc(WEB_CO_MAIN_ASYNCIFY_STACK_SIZE);

        if (main_co.asyncify_stack == NULL) {
            fatal_error("web: out of memory while creating the main context");
        }

        emscripten_fiber_init_from_current_context(&main_co.fiber, main_co.asyncify_stack,
                                                   WEB_CO_MAIN_ASYNCIFY_STACK_SIZE);
        main_co_ready = true;
        current_co = &main_co;
    }

    return (cothread_t)current_co;
}

cothread_t co_create(unsigned int size, void (*entrypoint)(void)) {
    WebCo* co = (WebCo*)calloc(1, sizeof(WebCo));

    if (co == NULL) {
        fatal_error("web: out of memory while creating a thread");
    }

    co->entry = entrypoint;
    co->c_stack = malloc(size);
    co->asyncify_stack = malloc(WEB_CO_ASYNCIFY_STACK_SIZE);

    if ((co->c_stack == NULL) || (co->asyncify_stack == NULL)) {
        fatal_error("web: out of memory while creating a thread");
    }

    emscripten_fiber_init(&co->fiber, web_co_entry, co, co->c_stack, size, co->asyncify_stack,
                          WEB_CO_ASYNCIFY_STACK_SIZE);

    return (cothread_t)co;
}

void co_delete(cothread_t handle) {
    WebCo* co = (WebCo*)handle;

    if (co == NULL || co == &main_co) {
        return;
    }

    free(co->c_stack);
    free(co->asyncify_stack);
    free(co);
}

void co_switch(cothread_t handle) {
    WebCo* target = (WebCo*)handle;
    WebCo* previous = current_co;

    if (target == NULL) {
        fatal_error("web: switch to a thread that does not exist");
    }

    current_co = target;
    emscripten_fiber_swap(&previous->fiber, &target->fiber);
}

int co_serializable(void) {
    return 1;
}
