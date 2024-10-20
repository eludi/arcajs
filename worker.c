#include "value.h"
#include "jsBindings.h"
#include "log.h"
#include "external/duk_config.h"
#include "external/duktape.h"
#include <SDL_assert.h>

#include <SDL_thread.h>
#include <SDL_mutex.h>
#include <SDL_timer.h>

extern Value* readValue(duk_context *ctx, duk_idx_t objIdx);
extern void pushValue(duk_context* ctx, const Value* value);
extern void bindTimeout(duk_context* ctx);
extern void updateTimeouts(duk_context* ctx, double timestamp);
extern duk_ret_t dk_appInclude(duk_context *ctx);
extern duk_ret_t dk_dummy(duk_context *ctx);

/** @module Worker
 *
 * implements a minimal subset of the Web Workers API
 * 
 * Calls supported by the main context are the Worker constructor and its postMessage and onmessage methods.
 * 
 * The worker itself may communicate with the main context via the postMessage() function and an onmessage callback.
 * In addition, it may import additional javascript sources via the importScripts() function. 
 * Apart from that, only few selected APIs are accessible by arcajs workers: console, setTimeout, clearTimeout.
 * 
 * For further details please refer to the [Web Workers API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API)
 * documentation at MDN.
 */

typedef enum {
	WORKER_STATE_INIT = 0,
	WORKER_STATE_BUSY = 1,
	WORKER_STATE_READY = 2,
	WORKER_STATE_ERROR = -1,
} WorkerState;

typedef struct Worker_s {
	int state, shutdown;
	double timestamp;
	char* fname;
	SDL_Thread *thread;
	duk_context *ctx, *parentCtx;
	SDL_mutex *mutexMsgIn, *mutexMsgOut;
	Value *msgOut, *msgIn;
} Worker;

static duk_ret_t dk_WorkerPostMessageOut(duk_context *ctx) {
	duk_push_global_stash(ctx);
	duk_get_prop_literal(ctx, -1, "worker");
	Worker* worker = (Worker*)duk_get_pointer(ctx, -1);
	duk_pop_2(ctx);

	SDL_LockMutex(worker->mutexMsgOut);
	Value* v = readValue(ctx, 0);
	Value_append(worker->msgOut, v);
	SDL_UnlockMutex(worker->mutexMsgOut);
	return 0;
}

static duk_ret_t dk_WorkerConsoleLog(duk_context *ctx) {
	FILE* f = stdout;
	for(duk_idx_t i=0, argc = duk_get_top(ctx); i<argc; ++i) {
		const char* s;
		if(duk_is_array(ctx, i) || duk_is_object(ctx, i))
			s = duk_json_encode(ctx, i);
		else
			s = duk_to_string(ctx, i);
		fprintf(f, "%s ", s);
	}
	fprintf(f, "\n");
	fflush(f);
	return 0;
}

static int WorkerThread(void* udata) {
	Worker* worker = (Worker*)udata;
	if(jsvmEvalScript((size_t)(worker->ctx), worker->fname) != 0)
		worker->state = WORKER_STATE_ERROR;
	else
		worker->state = WORKER_STATE_READY;

	while(worker->state > WORKER_STATE_ERROR && !worker->shutdown) { // worker main loop
		updateTimeouts(worker->ctx, worker->timestamp);
		switch(worker->state) {
			case WORKER_STATE_READY:
				SDL_Delay(50);
				break;
			case WORKER_STATE_BUSY: {
				SDL_LockMutex(worker->mutexMsgIn);
				Value* msg = Value_popf(worker->msgIn);
				SDL_UnlockMutex(worker->mutexMsgIn);

				if(!msg) {
					worker->state = WORKER_STATE_READY;
					break;
				}

				duk_get_global_literal(worker->ctx, "onmessage");
				if(duk_is_function(worker->ctx, -1)) {
					duk_push_object(worker->ctx);
					pushValue(worker->ctx, msg);
					duk_put_prop_literal(worker->ctx, -2, "data");
					duk_pcall(worker->ctx, 1);
				}
				duk_pop(worker->ctx); // pop undefined value or return code
				Value_delete(msg, false);
				break;
			}
		}
	}
	return worker->state;
}

void WorkerDelete(Worker* worker) {
	if(!worker)
		return;
	if(worker->thread) {
		worker->shutdown = 1;
		SDL_WaitThread(worker->thread, NULL);
		if(worker->mutexMsgIn)
			SDL_DestroyMutex(worker->mutexMsgIn);
		if(worker->mutexMsgOut)
			SDL_DestroyMutex(worker->mutexMsgOut);
	}
	Value_delete(worker->msgIn, true);
	Value_delete(worker->msgOut, true);
	if(worker->ctx)
		duk_destroy_heap(worker->ctx);

	duk_context *ctx = worker->parentCtx;
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("workers"));
	const duk_idx_t arrIdx = duk_get_top_index(ctx);
	const duk_size_t nWorkers = duk_get_length(ctx, arrIdx);
	for(duk_size_t idx=0; idx<nWorkers; ++idx) {
		duk_get_prop_index(ctx, arrIdx, idx);
		duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("worker"));
		Worker* w = (Worker*)duk_get_pointer(ctx, -1);
		duk_pop_2(ctx);
		if(w==worker) {
			duk_push_literal(ctx, "splice");
			duk_push_uint(ctx, idx);
			duk_push_uint(ctx, 1);
			if(duk_pcall_prop(ctx, arrIdx, 2)!=0)
				LogWarn("workers.splice(%u, 1) failed: %s", (uint32_t)idx, duk_safe_to_string(ctx, -1));
			duk_pop(ctx);
			break;
		}
	}
	duk_pop(ctx);

	free(worker->fname);
	free(worker);
}

size_t WorkerCreate(const char* fname) {
	Worker* worker = malloc(sizeof(Worker));
	memset(worker, 0, sizeof(Worker));
	worker->msgOut = Value_new(VALUE_LIST, NULL);
	worker->msgIn = Value_new(VALUE_LIST, NULL);
	worker->fname = strdup(fname);
	worker->mutexMsgIn = SDL_CreateMutex();
	worker->mutexMsgOut = SDL_CreateMutex();

	duk_context *ctx = worker->ctx = duk_create_heap_default();
	if(!ctx || !worker->mutexMsgIn || !worker->mutexMsgOut) {
		WorkerDelete(worker);
		return 0;
	}

	// store pointer to self:
	duk_push_global_stash(ctx);
	duk_push_pointer(ctx, worker);
	duk_put_prop_literal(ctx, -2, "worker");
	duk_pop(ctx);

	// bind postMessage, importScripts, and console to worker context:
	duk_push_c_function(ctx, dk_WorkerPostMessageOut, 1);
	duk_put_global_literal(ctx, "postMessage");
	duk_push_c_function(ctx, dk_appInclude, DUK_VARARGS);
	duk_put_global_literal(ctx, "importScripts");

	duk_push_object(ctx);
	duk_push_c_function(ctx, dk_WorkerConsoleLog, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "log");
	duk_push_c_function(ctx, dk_WorkerConsoleLog, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "error");
	duk_push_c_function(ctx, dk_WorkerConsoleLog, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "warn");
	duk_push_c_function(ctx, dk_dummy, 0);
	duk_put_prop_string(ctx, -2, "group");
	duk_push_c_function(ctx, dk_dummy, 0);
	duk_put_prop_string(ctx, -2, "groupCollapsed");
	duk_push_c_function(ctx, dk_dummy, 0);
	duk_put_prop_string(ctx, -2, "groupEnd");
	duk_put_global_literal(ctx, "console");
	bindTimeout(ctx);

	worker->thread = SDL_CreateThread(WorkerThread, "worker", (void*)worker);
	return (size_t)worker;
}

int WorkerPostMessage(Worker* worker, Value* msg) {
	if(!worker || !msg)
		return -1;
	SDL_LockMutex(worker->mutexMsgIn);
	Value_append(worker->msgIn, msg);
	SDL_UnlockMutex(worker->mutexMsgIn);
	//printf("worker.postMessage("); Value_print(msg, stdout); printf(")\n");
	return 0;
}

Value* WorkerUpdate(Worker* worker, double timestamp) {
	const int state = worker->state;
	//printf("worker state:%i\n", state);

	// read and reset msgOut:
	Value* messages = NULL;
	if(!Value_empty(worker->msgOut)) {
		messages = Value_new(VALUE_LIST, NULL);
		SDL_LockMutex(worker->mutexMsgOut);
		messages->child = worker->msgOut->child;
		worker->msgOut->child  = NULL;
		SDL_UnlockMutex(worker->mutexMsgOut);
	}
	worker->timestamp = timestamp;
	if(state == WORKER_STATE_READY && !Value_empty(worker->msgIn))
		worker->state = WORKER_STATE_BUSY; // initiate inbound message processing
	return messages;
}

//--- worker bindings for main context -----------------------------
static duk_ret_t dk_WorkerDelete(duk_context *ctx) {
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("worker"));
	Worker* worker = (Worker*)duk_get_pointer(ctx, -1);
	WorkerDelete(worker);
	return 0;
}

static duk_ret_t dk_WorkerConstructor(duk_context *ctx) {
	if (!duk_is_constructor_call(ctx)) {
		return DUK_RET_TYPE_ERROR;
	}
	const char* fname = duk_to_string(ctx, 0);
	Worker* worker = (Worker*)WorkerCreate(fname);
	if(!worker)
		return duk_error(ctx, DUK_ERR_ERROR, "Worker creation failed");
	worker->parentCtx = ctx;

	duk_push_this(ctx);
	duk_dup(ctx, 0); // push fname arg to top and store as property
	duk_put_prop_string(ctx, -2, "url");
	duk_push_pointer(ctx, worker);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("worker"));
	duk_push_c_function(ctx, dk_WorkerDelete, 0);
	duk_set_finalizer(ctx, -2);


	// append to workers array:
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("workers"));
	const duk_size_t idx = duk_get_length(ctx, -1);
	duk_push_this(ctx);
	duk_put_prop_index(ctx, -2, idx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t dk_WorkerPostMessageIn(duk_context *ctx) {
	Value* msg = readValue(ctx, 0);
	if(!msg)
		return duk_error(ctx, DUK_ERR_ERROR, "Worker.postMessage() message expected");

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("worker"));
	Worker* worker = (Worker*)duk_get_pointer(ctx, -1);
	if(WorkerPostMessage(worker, msg)!=0)
		return duk_error(ctx, DUK_ERR_ERROR, "Worker.postMessage() failed");
	return 0;
}

void bindWorker(duk_context *ctx) {
	duk_push_c_function(ctx, dk_WorkerConstructor, 1);

	duk_push_object(ctx); // prototype
	duk_push_c_function(ctx, dk_WorkerPostMessageIn, 1);
	duk_put_prop_string(ctx, -2, "postMessage");

 	duk_put_prop_string(ctx, -2, "prototype");
 	duk_put_global_literal(ctx, "Worker");

	duk_push_array(ctx);
	duk_put_global_string(ctx, DUK_HIDDEN_SYMBOL("workers"));
}

void updateWorkers(duk_context *ctx, double timestamp) {
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("workers"));
	const duk_idx_t arrIdx = duk_get_top_index(ctx);
	const duk_size_t nWorkers = duk_get_length(ctx, arrIdx);

	for(duk_size_t idx=0; idx<nWorkers; ++idx) {
		duk_get_prop_index(ctx, arrIdx, idx);
		const duk_idx_t workerIdx = duk_get_top_index(ctx);
		duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("worker"));
		Worker* worker = (Worker*)duk_get_pointer(ctx, -1);
		duk_pop(ctx); // worker pointer
		SDL_assert(worker);
		Value *msg, *messages = WorkerUpdate(worker, timestamp);

		if(messages) {
			duk_get_prop_literal(ctx, workerIdx, "onmessage");
			if(duk_is_function(ctx,-1)) {
				while((msg = Value_popf(messages)) != NULL) {
					duk_dup_top(ctx);
					duk_push_object(ctx);
					pushValue(ctx, msg);
					duk_put_prop_literal(ctx, -2, "data");
					Value_delete(msg, false);
					duk_pcall(ctx, 1);
					duk_pop(ctx); // return value
				}
			}
			else LogWarn("updateWorkers: messages but no onmessage callback");
			duk_pop(ctx); // callback
		}
		Value_delete(messages, false);
		duk_pop(ctx); // worker at idx
	}
	SDL_assert(duk_get_top(ctx));
	duk_pop(ctx);
}