#pragma once

#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

typedef void (*dukt_debug_session_cb_t)(int);

/// Debug session events passed to the debug session callback
#define DUKT_DEBUG_EVENT_CONTINUE   0
#define DUKT_DEBUG_EVENT_QUIT       1

/**
 * Initialize the Duktape debugger.
 * 
 * @param ctx Duktape context
 * @param port TCP port for debugger connection (0 = disabled)
 * @param breakpointFnName name of the global Duktape function to register (e.g. "breakpoint")
 * @param cb callback invoked when a debug session has ended (can be NULL)
 */
void dukt_debug_init(void* vm, int port, const char *breakpointFnName, dukt_debug_session_cb_t cb);

/** Poll for connections and handle disconnections. Call once per frame. */
void dukt_debug_poll(void);

/** Shutdown debugger, close sockets. */
void dukt_debug_shutdown(void);

#ifdef __cplusplus
}
#endif

// -----------------------------------------------------------------------------
// Implementation (header-only, define DUKT_DEBUG_IMPLEMENTATION before including)
// -----------------------------------------------------------------------------
#ifdef DUKT_DEBUG_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

// Internal state
static int dukt_debug_server = -1;
static int dukt_debug_client = -1;
static dukt_debug_session_cb_t dukt_debug_break_cb = NULL;

// Forward declaration
static duk_ret_t dukt_debug_breakpoint(duk_context *ctx);

void dukt_debug_init(void* vm, int port, const char *breakpointFnName, dukt_debug_session_cb_t cb) {
    duk_context* ctx = (duk_context*)vm;
    if (port > 0) {
        dukt_debug_break_cb = cb;
        dukt_debug_server = socket(AF_INET, SOCK_STREAM, 0);
        if (dukt_debug_server < 0) {
            perror("socket");
            return;
        }
        int opt = 1;
        setsockopt(dukt_debug_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if (bind(dukt_debug_server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(dukt_debug_server);
            dukt_debug_server = -1;
            return;
        }
        listen(dukt_debug_server, 1);
        fcntl(dukt_debug_server, F_SETFL, O_NONBLOCK);

        printf("[dukt_debug] Listening on port %d\n", port);
    }

    // Register breakpoint() in Duktape
    duk_push_c_function(ctx, dukt_debug_breakpoint, DUK_VARARGS);
    duk_put_global_string(ctx, breakpointFnName);
}

void dukt_debug_poll(void) {
    if (dukt_debug_server >= 0 && dukt_debug_client < 0) {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int fd = accept(dukt_debug_server, (struct sockaddr*)&cliaddr, &len);
        if (fd >= 0) {
            fcntl(fd, F_SETFL, O_NONBLOCK);
            dukt_debug_client = fd;
            printf("[dukt_debug] Client connected\n");
        }
    }
    if (dukt_debug_client >= 0) {
        char buf[1];
        ssize_t n = recv(dukt_debug_client, buf, 1, MSG_PEEK);
        if (n == 0) {
            printf("[dukt_debug] Client disconnected\n");
            close(dukt_debug_client);
            dukt_debug_client = -1;
        }
    }
}

void dukt_debug_shutdown(void) {
    if (dukt_debug_client >= 0) { close(dukt_debug_client); dukt_debug_client = -1; }
    if (dukt_debug_server >= 0) { close(dukt_debug_server); dukt_debug_server = -1; }
}

// Minimal REPL loop (blocking until client disconnects)
static int dukt_debug_repl(duk_context *ctx) {
    char line[1024];
    int ret = DUKT_DEBUG_EVENT_CONTINUE;
    while (dukt_debug_client >= 0) {
        dprintf(dukt_debug_client, "\njs> ");

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(dukt_debug_client, &readfds);
        // Wait for input, no timeout → block until debugger sends or disconnects
        const int rv = select(dukt_debug_client + 1, &readfds, NULL, NULL, NULL);
        if (rv <= 0) {
            fprintf(stderr, "Debugger select() error\n");
            break;
        }

        const ssize_t n = recv(dukt_debug_client, line, sizeof(line)-1, 0);
        if (n <= 0) {
            fprintf(stderr, "Debugger disconnected — resuming.\n");
            break;
        }
        if (n >= sizeof(line) - 1) {
            dprintf(dukt_debug_client, "Input too long\n");
            continue;
        }
        line[n] = 0;
        // Remove trailing newline
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0;
        if (line[0] == 0) break; // Empty line: exit REPL
        if (strncmp(line, ".cont", 5) == 0) break;
        if (strncmp(line, ".quit", 5) == 0) {
            ret = DUKT_DEBUG_EVENT_QUIT;
            break;
        }

        if (duk_peval_string(ctx, line) != 0) {
            dprintf(dukt_debug_client, "Error: %s\n", duk_safe_to_string(ctx, -1));
        } else {
            dprintf(dukt_debug_client, "%s\n", duk_safe_to_string(ctx, -1));
        }
        duk_pop(ctx); // Pop result or error
    }
    return ret;
}

static duk_ret_t dukt_debug_breakpoint(duk_context *ctx) {
    if (dukt_debug_client >= 0) {
        dprintf(dukt_debug_client, "\n[dukt_debug] Breakpoint hit!\n");

        // print current code location / stack trace:
        duk_inspect_callstack_entry(ctx, -2);
        duk_get_prop_string(ctx, -1, "lineNumber");
        long lineNumber = (long) duk_to_int(ctx, -1);
        duk_pop(ctx);
        duk_get_prop_string(ctx, -1, "function");
        const char* fname = "(anonymous)";
        if(duk_has_prop_string(ctx, -1, "fileName")) {
            duk_get_prop_string(ctx, -1, "fileName"); // try to grab filename directly
            fname = duk_safe_to_string(ctx, -1);
            duk_pop(ctx);
        }
        dprintf(dukt_debug_client, "  at %s:%ld\n", fname, lineNumber);
        duk_pop_2(ctx);

        // print arguments as json string and collect in array:
        duk_idx_t argc = duk_get_top(ctx);
        duk_idx_t args = duk_push_array(ctx);
        for (duk_idx_t i = 0; i < argc; i++) {
            duk_dup(ctx, i);
            duk_put_prop_index(ctx, args, i);
            duk_json_encode(ctx, i);
            dprintf(dukt_debug_client, "  args[%ld]: %s\n", (long) i, duk_safe_to_string(ctx, i));
        }
        // safe current global args value and replace with new one:
        duk_get_global_literal(ctx, "args");
        duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("args"));
        duk_put_global_literal(ctx, "args");

        const int ret = dukt_debug_repl(ctx);
        if (dukt_debug_break_cb) {
            dukt_debug_break_cb(ret);
        }
        // restore previously saved args value
        duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("args"));
        duk_put_global_literal(ctx, "args");
    }
    return 0;
}

#endif // DUKT_DEBUG_IMPLEMENTATION
