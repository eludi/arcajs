#include "jsBindings.h"
#include "jsCode.h"
#include "window.h"
#include "graphics.h"
#include "graphicsUtils.h"
#include "resources.h"
#include "audio.h"
#include "graphics.h"
#include "console.h"
#include "httpRequest.h"
#include "log.h"

#include "external/duk_config.h"
#include "external/duktape.h"

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <SDL_assert.h>
#include <SDL_thread.h>
#include <SDL_loadso.h>
#include <SDL_filesystem.h>
#include <SDL_misc.h>
#include <SDL_rwops.h>
#ifndef ARCAJS_ARCH
#  define ARCAJS_ARCH "UNKNOWN"
#endif

extern float clampf(float f, float min, float max);
extern uint32_t hsla2rgba(float h, float s, float l, float a);
extern float randf();
extern const char* appVersion;
extern void intersects_exports(duk_context *ctx);
extern void bindGraphics(duk_context *ctx);
extern void bindWorker(duk_context *ctx);
extern void updateWorkers(duk_context* ctx, double timestamp);
extern int debug;

uint32_t rgbaColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	return (r << 24) + (g << 16) + (b << 8) + a;
}

static void dk_defineReadOnlyProperty(duk_context* ctx, const char* key, duk_idx_t idx,
	duk_c_function getter)
{
	duk_push_string(ctx, key);
	duk_push_c_function(ctx, getter, 0);
	duk_def_prop(ctx, idx < 0 ? idx-2 : idx,
		DUK_DEFPROP_HAVE_GETTER |
		DUK_DEFPROP_HAVE_CONFIGURABLE | DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE);
}

const char* dk_join_array(duk_context *ctx, duk_idx_t idx, const char* sep) {
	duk_size_t sz = duk_get_length(ctx, idx);
	if(sep)
		duk_push_string(ctx, sep);
	for(duk_size_t i=0; i<sz; ++i)
		duk_get_prop_index(ctx, idx, i);
	if(sep)
		duk_join(ctx, sz);
	else
		duk_concat(ctx, sz);
	duk_replace(ctx, idx);
	return duk_get_string(ctx, idx);
}

uint32_t readFloatArray(duk_context *ctx, duk_idx_t idx, float** arr, float** buf) {
	uint32_t n=0;
	*buf = NULL;
	if(duk_is_buffer_data(ctx, idx)) {
		duk_size_t nBytes;
		*arr = duk_get_buffer_data(ctx, idx, &nBytes);
		if(nBytes%sizeof(float) != 0)
			return 0;
		n = nBytes / sizeof(float);
	}
	else if(duk_is_array(ctx, idx)) {
		n = duk_get_length(ctx, idx);
		*arr = *buf = (float*)malloc(n*sizeof(float));
		for(uint32_t i=0; i<n; ++i) {
			duk_get_prop_index(ctx, idx, i);
			(*buf)[i] = duk_get_number(ctx, -1);
			duk_pop(ctx);
		}
	}
	return n;
}

uint32_t readUint8Array(duk_context *ctx, duk_idx_t idx, uint8_t** arr, uint8_t** buf) {
	duk_size_t n=0;
	*buf = NULL;
	if(duk_is_buffer_data(ctx, idx))
		*arr = duk_get_buffer_data(ctx, idx, &n);
	else if(duk_is_array(ctx, idx)) {
		n = duk_get_length(ctx, idx);
		*arr = *buf = (uint8_t*)malloc(n);
		for(duk_size_t i=0; i<n; ++i) {
			duk_get_prop_index(ctx, idx, i);
			(*buf)[i] = duk_get_uint(ctx, -1);
			duk_pop(ctx);
		}
	}
	return n;
}

uint32_t readColor(duk_context *ctx, duk_idx_t idx) {
	if(duk_is_buffer_data(ctx, idx)) {
		duk_size_t sz;
		uint8_t* arr = duk_get_buffer_data(ctx, idx, &sz);
		if(sz==3 || sz==4)
			return rgbaColor(arr[0], arr[1], arr[2], sz>3 ? arr[3] : 255);
		return 0xffffffff;
	}
	if(duk_is_array(ctx, idx)) {
		duk_size_t sz = duk_get_length(ctx, idx);
		if(sz!=3 && sz!=4)
			return 0xffffffff;
		uint8_t arr[4]={0,0,0,255};
		for(uint32_t i=0; i<sz; ++i) {
			duk_get_prop_index(ctx, idx, i);
			arr[i] = duk_get_int(ctx, -1);
			duk_pop(ctx);
		}
		return rgbaColor(arr[0], arr[1], arr[2], arr[3]);
	}
	if(duk_is_number(ctx, idx))
		return duk_to_uint(ctx, idx);
	else if(duk_is_string(ctx, idx))
		return cssColor(duk_get_string(ctx, idx));
	return 0xffffffff;
}

Value* readValue(duk_context *ctx, duk_idx_t idx);

static Value* readArray(duk_context *ctx, duk_idx_t objIdx) {
	Value* arr = Value_new(VALUE_LIST, NULL);
	const uint32_t len = duk_get_length(ctx, objIdx);
	for(uint32_t idx=0; idx<len; ++idx) {
		duk_get_prop_index(ctx, objIdx, idx);
		Value_append(arr, readValue(ctx, duk_get_top_index(ctx)));
		duk_pop(ctx);
	}
	return arr;
}

static Value* readObject(duk_context *ctx, duk_idx_t objIdx) {
	SDL_assert_always(duk_get_type(ctx, objIdx) == DUK_TYPE_OBJECT);
	if(duk_is_array(ctx,objIdx))
		return readArray(ctx, objIdx);
	Value* obj = Value_new(VALUE_MAP, NULL);
	duk_enum(ctx, objIdx, DUK_ENUM_OWN_PROPERTIES_ONLY);
	duk_idx_t enumIdx = duk_get_top_index(ctx);
	while (duk_next(ctx, enumIdx, 1 /*get_value*/)) {
		duk_idx_t keyIdx = duk_get_top_index(ctx)-1, valueIdx = keyIdx+1;
		Value_set(obj, duk_to_string(ctx, keyIdx), readValue(ctx, valueIdx));
		//printf("%s -> %s\n", duk_to_string(ctx, keyIdx), duk_to_string(ctx, valueIdx));
		duk_pop_2(ctx);
	}
	//duk_push_context_dump(ctx); printf("%s\n", duk_to_string(ctx, -1)); duk_pop(ctx);
	duk_pop(ctx); // pop enum obj
	return obj;
}

Value* readValue(duk_context *ctx, duk_idx_t idx) {
	if(duk_is_buffer_data(ctx, idx)) { // read as int array
		duk_size_t sz;
		uint8_t* buf = duk_get_buffer_data(ctx, idx, &sz);
		Value* arr = Value_new(VALUE_LIST, NULL);
		if(sz) {
			Value* curr = Value_int(buf[0]);
			Value_append(arr, curr);
			for(size_t i=1; i<sz; ++i) {
				curr->next = Value_int(buf[i]);
				curr = curr->next;
			}
		}
		return arr;
	}
	else switch(duk_get_type(ctx, idx)) {
		case DUK_TYPE_OBJECT: return readObject(ctx, idx);
		case DUK_TYPE_STRING:
			return Value_str(duk_get_string(ctx, idx));
		case DUK_TYPE_NUMBER: {
			double f = duk_get_number(ctx, idx);
			signed long long i = f;
			if(f==(double)i)
				return Value_int(i);
			else
				return Value_float(f);
		}
		case DUK_TYPE_BOOLEAN:
			return Value_bool(duk_to_boolean(ctx, idx));
		case DUK_TYPE_NULL:
		case DUK_TYPE_UNDEFINED:
			return Value_new(VALUE_NONE, NULL);
		default:
			SDL_assert_always(duk_get_type(ctx, idx) == DUK_TYPE_OBJECT); // always false
	}
	return NULL;
}

static duk_ret_t tryJsonDecode(duk_context *ctx, void* udata) {
	duk_json_decode(ctx, -1);
	return 1;
}

void pushValue(duk_context* ctx, const Value* value) {
	if(!value) {
		duk_push_undefined(ctx);
		return;
	}
	switch(value->type) {
	case VALUE_STRING:
	case VALUE_SYMBOL:
		duk_push_string(ctx, value->str); break;
	case VALUE_INT:
		duk_push_int(ctx, value->i); break;
	case VALUE_BOOL:
		duk_push_boolean(ctx, value->i); break;
	case VALUE_FLOAT:
		duk_push_number(ctx, value->f); break;
	case VALUE_LIST: {
		duk_idx_t arr = duk_push_array(ctx);
		duk_uarridx_t idx = 0;
		const Value* item = value->child;
		while(item) {
			pushValue(ctx, item);
			duk_put_prop_index(ctx, arr, idx++);
			item = item->next;
		}
		break;
	}
	case VALUE_MAP: {
		duk_idx_t obj = duk_push_object(ctx);
		for(Value* key = value->child; key!=NULL; key = key->next->next) {
			pushValue(ctx, key->next);
			duk_put_prop_string(ctx, obj, key->str);
		}
		break;
	}
	case VALUE_NONE:
		duk_push_null(ctx); break;
	}
}

static void dk_encodeURI(duk_context *ctx, duk_idx_t objIdx) {
	duk_get_global_literal(ctx, "encodeURIComponent");
	duk_idx_t funcIdx = duk_get_top_index(ctx);
	duk_enum(ctx, objIdx, DUK_ENUM_OWN_PROPERTIES_ONLY);
	duk_idx_t enumIdx = duk_get_top_index(ctx);
	int isFirst = 1;
	while (duk_next(ctx, enumIdx, 1 /*get_value*/)) {
		duk_idx_t keyIdx = duk_get_top_index(ctx)-1, valueIdx = keyIdx+1;
		if(isFirst)
			isFirst = 0;
		else
			duk_push_literal(ctx,"&");

		duk_dup(ctx, funcIdx);
		duk_dup(ctx, keyIdx);
		duk_call(ctx, 1);
		duk_push_literal(ctx,"=");

		duk_dup(ctx, funcIdx);
		duk_dup(ctx, valueIdx);
		duk_call(ctx, 1);
		duk_remove(ctx, valueIdx);
		duk_remove(ctx, keyIdx);
	}
	duk_concat(ctx, duk_get_top_index(ctx)-enumIdx);
	//duk_push_context_dump(ctx); printf("%s\n", duk_to_string(ctx, -1)); duk_pop(ctx);
	duk_replace(ctx, objIdx); // replace object by concat string
	duk_pop_2(ctx); // pop enum obj and funcIdx
}

static float noteStr2freq(const char* s) {
	size_t slen = strlen(s);
	if(slen == 2 || slen == 3) {
		char note = s[0];
		char accidental = (slen==3 && (s[1] == '#' || s[1]=='b')) ? s[1] : ' ';

		if(note!='-' && (s[slen-1] < '0' || s[slen-1]>'9'))
			return -1.0f;
			
		int octave = s[slen-1] - '0';
		return note2freq(note, accidental, octave);
	}
	else return -2.0f;
}

#define ERROR_MAXLEN 512
char s_lastError[ERROR_MAXLEN];

//--- timeouts -----------------------------------------------------

typedef struct {
	int timeoutId;
	double when;
	void* next;
} TimeoutCallback;

typedef struct {
	TimeoutCallback * timeouts;
	int counter;
} TimeoutData;

static duk_ret_t dk_setTimeout(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	if(argc<2 || !duk_is_function(ctx, 0) || !duk_is_number(ctx, 1)) {
		sprintf(s_lastError, "invalid arguments for setTimeout(function, duration[, ...])\n");
		return 0;
	}

	TimeoutCallback* cb = (TimeoutCallback*)malloc(sizeof(TimeoutCallback));	
	cb->when = WindowTimestamp() + duk_to_number(ctx, 1) * 0.001;
	cb->next = NULL;

	duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("timeouts"));
	TimeoutData* td = (TimeoutData*)duk_get_buffer(ctx, -1, NULL);
	duk_pop(ctx);
	cb->timeoutId = ++(td->counter);

	TimeoutCallback *curr = td->timeouts, *prev = NULL;
	if(curr == NULL)
		td->timeouts = cb;
	else {
		while(curr && cb->when>curr->when) {
			prev = curr;
			curr = curr->next;
		}
		cb->next = curr;
		if(prev)
			prev->next = cb;
		else
			td->timeouts = cb;
	}

	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("timeoutCallbacks"));
	duk_push_array(ctx);
	duk_dup(ctx, 0);
	duk_put_prop_index(ctx, -2, 0);

	for(int argn=2; argn<argc; ++argn) { // optional timeout arguments
		duk_dup(ctx, argn);
		duk_put_prop_index(ctx, -2, argn-1);
	}

	duk_put_prop_index(ctx, -2, cb->timeoutId);
	duk_pop(ctx);

	duk_push_int(ctx, cb->timeoutId);
	return 1;
}

static duk_ret_t dk_clearTimeout(duk_context *ctx) {
	const int timeoutId = duk_get_int(ctx,0);
	duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("timeouts"));
	TimeoutData* td = (TimeoutData*)duk_get_buffer(ctx, -1, NULL);
	duk_pop(ctx);
	
	TimeoutCallback *curr = td->timeouts, *prev = NULL;
	while(curr && curr->timeoutId != timeoutId) {
		prev = curr;
		curr = curr->next;
	}
	if(!curr)
		return 0;
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("timeoutCallbacks"));
	duk_del_prop_index(ctx, -1, timeoutId); // delete callback array
	if(prev)
		prev->next = curr->next;
	else
		td->timeouts = curr->next;
	free(curr);
	return 0;
}

void updateTimeouts(duk_context* ctx, double timestamp) {
	duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("timeouts"));
	TimeoutData* td = (TimeoutData*)duk_get_buffer(ctx, -1, NULL);
	duk_pop(ctx);

	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("timeoutCallbacks"));
	for(TimeoutCallback* cb=td->timeouts; cb && cb->when < timestamp; cb=td->timeouts) {
		duk_get_prop_index(ctx, -1, cb->timeoutId); // callback array
		duk_idx_t cbArrIdx = duk_get_top_index(ctx);
		duk_size_t cbArrLen = duk_get_length(ctx, -1);
		for(duk_size_t i=0; i<cbArrLen; ++i)
			duk_get_prop_index(ctx, cbArrIdx, i);
		duk_pcall(ctx, cbArrLen-1);
		duk_pop_2(ctx); // ignore cb return value and pop callback array
		duk_del_prop_index(ctx, -1, cb->timeoutId); // delete callback array

		td->timeouts = cb->next;
		free(cb);
	}
	duk_pop(ctx);
}

void bindTimeout(duk_context *ctx) {
	duk_push_c_function(ctx, dk_setTimeout, DUK_VARARGS);
	duk_put_global_literal(ctx, "setTimeout");
	duk_push_c_function(ctx, dk_clearTimeout, 1);
	duk_put_global_literal(ctx, "clearTimeout");

	duk_push_object(ctx);
	duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("timeoutCallbacks"));
	duk_push_fixed_buffer(ctx, sizeof(TimeoutData));
	duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("timeouts"));
}

//--- console ------------------------------------------------------
/** @module console
 *
 * console input output
 */

static duk_ret_t dk_fprintf(FILE* f, duk_context *ctx) {
	static char msg[256];
	size_t len = 0;
	for(duk_idx_t i=0, argc = duk_get_top(ctx); i<argc; ++i) {
		const char* s;
		if(duk_is_array(ctx, i) || duk_is_object(ctx, i))
			s = duk_json_encode(ctx, i);
		else
			s = duk_to_string(ctx, i);
		fprintf(f, "%s ", s);
		if(len<255) {
			snprintf(msg+len, 255-len, "%s ", s);
			len += strlen(s) + 1;
		}
	}
	fprintf(f, "\n");
	fflush(f);
	if(f==stdout)
		ConsoleLog(msg);
	else
		ConsoleError(msg);
	return 0;
}

static duk_ret_t dk_consoleLog(duk_context *ctx) {
	return dk_fprintf(stdout, ctx);
}

static duk_ret_t dk_consoleErr(duk_context *ctx) {
	return dk_fprintf(stderr, ctx);
}

/**
 * @function console.visible
 *
 * sets or gets console visibility
 *
 * @param {boolean} [isVisible] - new console visibility
 * @returns {boolean|undefined} current console visibility if called without argument
 */
static duk_ret_t dk_consoleVisible(duk_context *ctx) {
	if(!duk_is_undefined(ctx, 0)) {
		if(duk_to_boolean(ctx,0))
			ConsoleShow();
		else
			ConsoleHide();
		return 0;
	}
	duk_push_boolean(ctx, ConsoleVisible());
	return 1;
}

duk_ret_t dk_dummy(duk_context *ctx) {
	(void)ctx;
	return 0;
}

static void bindConsole(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_consoleLog, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "log");
	duk_push_c_function(ctx, dk_consoleErr, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "error");
	duk_push_c_function(ctx, dk_consoleLog, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "warn");
	duk_push_c_function(ctx, dk_consoleVisible, 1);
	duk_put_prop_string(ctx, -2, "visible");
	duk_push_c_function(ctx, dk_dummy, 0);
	duk_put_prop_string(ctx, -2, "group");
	duk_push_c_function(ctx, dk_dummy, 0);
	duk_put_prop_string(ctx, -2, "groupCollapsed");
	duk_push_c_function(ctx, dk_dummy, 0);
	duk_put_prop_string(ctx, -2, "groupEnd");
	duk_put_global_string(ctx, "console");
}

//--- app bindings -------------------------------------------------
/** @module app
 *
 * the single global entry point to the arcajs API. Always available.
 */


static void updateEventHandler(const char* event, duk_context *ctx, duk_idx_t handlerIdx) {
	if(handlerIdx<0)
		handlerIdx = duk_get_top(ctx) + handlerIdx;
	duk_push_global_stash(ctx);
	if(!duk_is_function(ctx, handlerIdx)) {
		duk_del_prop_string(ctx, -1, event);
		if(strcmp(event, "textinput")==0)
			WindowTextInputStop();
	}
	else {
		duk_dup(ctx, handlerIdx); // callback fn
		duk_put_prop_string(ctx, -2, event);
		if(strcmp(event, "textinput")==0)
			WindowTextInputStart();
	}
	duk_pop(ctx);
}

/**
 * @function app.on
 * registers or removes an event listener callback for an application event.
 * The individual application events are described in [EVENTS.md](EVENTS.md).
 * @param {string|object} name - event name or object consisting of name:eventHandler function pairs
 * @param {function|null} callback - function to be executed when the event has happened, set null to remove
 */
static duk_ret_t dk_onEvent(duk_context *ctx) {
	if(!duk_is_object(ctx,0))
		updateEventHandler(duk_to_string(ctx, 0), ctx, 1);
	else {
		if(duk_get_prop_string(ctx, 0, "load"))
			updateEventHandler("load", ctx, duk_get_top_index(ctx));
		duk_pop(ctx);
		if(duk_get_prop_string(ctx, 0, "close"))
			updateEventHandler("close", ctx, duk_get_top_index(ctx));
		duk_pop(ctx);
		duk_set_top(ctx, 1);
		duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("nextHandler"));
	}
	return 0;
}

/**
 * @function app.emit
 * emits an event
 * @param {string} name - event name
 * @param {any} [args] - an arbitrary number of additional arguments to be passed to the event handler
 */
static duk_ret_t dk_appEmit(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	if(argc<1) {
		snprintf(s_lastError, ERROR_MAXLEN,
			"app.emit(evtName[, args...]) expects at at least a single string argument\n");
		return duk_error(ctx, DUK_ERR_ERROR, s_lastError);
	}
	Value* args = NULL, *pred=NULL;
	for(int i=1; i<argc; ++i) {
		Value* arg = NULL;
		switch(duk_get_type(ctx, i)) {
		case DUK_TYPE_STRING:
			arg = Value_str(duk_get_string(ctx, i));
			break;
		case DUK_TYPE_NUMBER: {
			double f = duk_get_number(ctx, i);
			signed long long i = f;
			arg = (f==(double)i) ? Value_int(i) : Value_float(f);
			break;
		}
		case DUK_TYPE_BOOLEAN:
			arg = Value_bool(duk_to_boolean(ctx, i));
			break;
		case DUK_TYPE_NULL:
			arg = Value_new(VALUE_NONE, NULL); break;
		default:
			if(duk_is_array(ctx, i) || duk_is_buffer_data(ctx, i))
				break;
			else if(duk_is_object(ctx, i))
				arg = readValue(ctx, i);
		}

		if(!arg) {
			Value_delete(args, 1);
			snprintf(s_lastError, ERROR_MAXLEN,
				"app.emit(evtName[, args...]) unhandled argument type as argument %i\n", i);
			return duk_error(ctx, DUK_ERR_ERROR, s_lastError);
		}

		if(!pred)
			pred = args = arg;
		else {
			pred->next = arg;
			pred = arg;
		}
	}
	jsvmDispatchEvent((size_t)ctx, duk_to_string(ctx,0), args);
	Value_delete(args, 1);
	return 0;
}

/**
 * @function app.emitAsGamepadEvent
 * re-emits configurable keyboard events as gamepad events
 *
 * This helper function allows games to use a unified input handler by translating
 * configurable keyboard key events to gamepad events.
 *
 * ```javascript
 * app.on('keyboard', function(evt) {
 *     // create a virtual gamepad by interpreting
 *     // WASD as a virtual directional pad and Enter key as primary button:
 *     app.emitAsGamepadEvent(evt, 0, ['a','d', 'w','s'], ['Enter']);
 * });
 * ```
 *
 * @param {object} keyboardEvent - the keyboard event to be translated and re-emitted
 * @param {number} index - the index of the gamepad
 * @param {array} axes - key names of the keys to be interpreted as gamepad axes. Each pair of keys define an axis.
 * @param {array} [buttons] - key names of the keys to be interpreted as gamepad buttons or objects {key:'keyName', location:index}.
 */

float getPropFloatDefault(duk_context *ctx, duk_idx_t idx, const char* key, float defaultValue) {
	float ret = defaultValue;
	if(duk_get_prop_string(ctx, idx, key))
		ret = duk_to_number(ctx, -1);
	duk_pop(ctx);
	return ret;
}

uint32_t getPropUint32Default(duk_context *ctx, duk_idx_t idx, const char* key, uint32_t defaultValue) {
	uint32_t ret = defaultValue;
	if(duk_get_prop_string(ctx, idx, key))
		ret = duk_to_uint32(ctx, -1);
	duk_pop(ctx);
	return ret;
}

uint32_t getPropFloatBuffer(duk_context *ctx, duk_idx_t idx, const char* key, float** arr) {
	if(!duk_is_object(ctx, idx))
		return 0;
	duk_size_t bufSz = 0;
	*arr = (duk_get_prop_string(ctx, idx, key) && duk_is_buffer_data(ctx, -1)) ?
		duk_get_buffer_data(ctx, -1, &bufSz) : NULL;
	duk_pop(ctx);
	return bufSz/sizeof(float);
}

void readImageResourceParams(
	duk_context *ctx, duk_idx_t idx, float* scale, int* filtering, float* cx, float* cy)
{
	if(!duk_is_object(ctx, idx))
		return;
	if(scale) {
		if(duk_get_prop_string(ctx, idx, "scale"))
			*scale = duk_to_number(ctx, -1);
		duk_pop(ctx);
	}
	if(filtering) {
		if(duk_get_prop_string(ctx, idx, "filtering"))
			*filtering = duk_to_int(ctx, -1);
		duk_pop(ctx);
	}
	if(cx)
		*cx = getPropFloatDefault(ctx, idx, "centerX", 0.0f);
	if(cy)
		*cy = getPropFloatDefault(ctx, idx, "centerY", 0.0f);
}

static duk_ret_t dk_getNamedResource(const char* name, duk_context *ctx) {
	const char* suffix = ResourceSuffix(name);
	int filtering = 1;
	float scale = 1.0;
	float cx = 0.0f, cy=0.0f;
	readImageResourceParams(ctx, 1, &scale, &filtering, &cx, &cy);

	size_t handle = ResourceGetImage(name, scale, filtering);
	if(handle)
		gfxImageSetCenter(handle, cx, cy);
	else
		handle = ResourceGetAudio(name);
	if(!handle && SDL_strncasecmp(suffix, "ttf", 3)==0) {
		if(duk_is_object(ctx, 1)) {
			if(duk_get_prop_string(ctx, 1, "size"))
				scale = duk_to_number(ctx, -1);
			duk_pop(ctx);
		}
		handle = ResourceGetFont(name, scale);
	}
	if(handle) {
		duk_push_number(ctx, (double)handle);
		return 1;
	}
	char* text = ResourceGetText(name);
	if(text) {
		if((SDL_strncasecmp(suffix, "html", 4)==0 && strlen(suffix) == 4)
			|| (SDL_strncasecmp(suffix, "xml", 3)==0 && strlen(suffix) == 3)
			|| (SDL_strncasecmp(suffix, "xhtml", 5)==0 && strlen(suffix) == 5))
		{
			Value* v = Value_parseXML(text, NULL);
			pushValue(ctx, v);
			free(v);
		}
		else
			duk_push_string(ctx, text);
		free(text);
		if(SDL_strncasecmp(suffix, "json", 4)==0 && strlen(suffix) == 4)
			duk_json_decode(ctx, -1);
		return 1;
	}
	snprintf(s_lastError, ERROR_MAXLEN, "app.getResource(\"%s\") failed\n", name);
	return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, s_lastError);
}

/**
 * @function app.getResource
 * returns handle to an image/audio/font or text/json resource or array of handles
 * @param {string|array} name - resource file name or list of resource file names
 * @param {object} [params] - optional additional parameters as key-value pairs such as
 *   filtering for images, scale for SVG images, or size for font resources
 * @returns {number|array} resource handle(s)
 */
static duk_ret_t dk_getResource(duk_context *ctx) {
	if(!duk_is_array(ctx,0)) {
		const char* name = duk_to_string(ctx, 0);
		return dk_getNamedResource(name, ctx);
	}
	uint32_t len = duk_get_length(ctx, 0);
	duk_idx_t arr = duk_push_array(ctx);

	for(uint32_t idx=0; idx<len; ++idx) {
		duk_get_prop_index(ctx, 0, idx);
		const char* name = duk_to_string(ctx, -1);
		duk_pop(ctx);
		dk_getNamedResource(name, ctx);
		duk_put_prop_index(ctx, arr, idx);
	}
	return 1;
}

/**
 * @function app.createCircleResource
 * creates a circle image resource
 * @param {number} radius - circle radius
 * @param {array|number} [fillColor=[255,255,255,255]] - fill color (RGBA)
 * @param {number} [strokeWidth=0] - stroke width
 * @param {array|number} [strokeColor=[0,0,0,0]] - stroke color (RGBA)
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createCircleResource(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	float radius = duk_to_number(ctx, 0);
	uint32_t fillColor = argc>1 ? readColor(ctx, 1) : 0xffffffff;
	float strokeWidth = argc>2 ? duk_to_number(ctx, 2) : 0.0f;
	uint32_t strokeColor = argc>3 ? readColor(ctx, 3) : 0xffffffff;
	size_t img = ResourceCreateCircleImage(radius, fillColor, strokeWidth, strokeColor);
	gfxImageSetCenter(img, 0.5f, 0.5f);
	duk_push_uint(ctx, img);
	return 1;
}

/**
 * @function app.createPathResource
 * creates an image resource from an SVG path description
 * @param {number} width - image width
 * @param {number} height - image height
 * @param {string|array} path - path description
 * @param {array|number} [fillColor=[255,255,255,255]] - fill color (RGBA)
 * @param {number} [strokeWidth=0] - stroke width
 * @param {array|number} [strokeColor=[0,0,0,0]] - stroke color (RGBA)
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createPathResource(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	if(argc<3)
		return duk_error(ctx, DUK_ERR_ERROR,
			"createPathResource expects at least width, height, and path parameters");
	int width = duk_to_int(ctx, 0), height = duk_to_int(ctx, 1);
	const char* path = duk_is_array(ctx, 2) ? dk_join_array(ctx, 2, " ") : duk_to_string(ctx, 2);
	uint32_t fillColor = argc>3 ? readColor(ctx, 3) : 0xffffffff;
	float strokeWidth = argc>4 ? duk_to_number(ctx, 4) : 0.0f;
	uint32_t strokeColor = argc>5 ? readColor(ctx, 5) : 0xffffffff;
	size_t img = ResourceCreatePathImage(width, height, path, fillColor, strokeWidth, strokeColor);
	duk_push_uint(ctx, img);
	return 1;
}

/**
 * @function app.createTileResources
 * creates tiled image resources based on an existing image resource
 * @param {number|string} parent - image resource handle or name
 * @param {number} tilesX - number of tiles in horizontal direction
 * @param {number} [tilesY=1] - number of tiles in vertical direction
 * @param {number} [border=0] - border around tiles in pixels
 * @param {object} [params] - optional additional parameters as key-value pairs such as filtering or scale. Only effective if parent is a resource file name.
 * @returns {number} handle of the first created tile image resource
 */
static duk_ret_t dk_createTileResources(duk_context *ctx) {
	uint32_t parent;
	if(duk_is_number(ctx,0))
		parent = duk_to_uint32(ctx, 0);
	else {
		int filtering = 1;
		float scale = 1.0f, cx = 0.0f, cy=0.0f;
		readImageResourceParams(ctx, 4, &scale, &filtering, &cx, &cy);
		parent = ResourceGetImage(duk_to_string(ctx,0), scale, filtering);
		gfxImageSetCenter(parent, cx, cy);
	}
	uint16_t tilesX = duk_to_uint16(ctx, 1);
	uint16_t tilesY = duk_get_uint_default(ctx, 2, 1);
	uint16_t border = duk_get_uint_default(ctx, 3, 0);
	uint32_t img = gfxImageTileGrid(parent, tilesX, tilesY, border);
	duk_push_uint(ctx, img);
	return 1;
}

/**
 * @function app.createTileResource
 * creates image resource based on an existing image resource
 * @param {number} parent - image resource handle
 * @param {number} x - relative new image horizontal origin (0.0..1.0)
 * @param {number} y - relative new image vertical origin (0.0..1.0)
 * @param {number} w - relative new image width (0.0..1.0)
 * @param {number} h - relative new image height (0.0..1.0)
 * @param {object} [params] - additional optional parameters: centerX, centerY
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createTileResource(duk_context *ctx) {
	uint32_t parent = duk_to_uint32(ctx, 0);
	int parentW=0, parentH=0;
	gfxImageDimensions(parent, &parentW, &parentH);
	if(!parentW || !parentH)
		return duk_error(ctx, DUK_ERR_ERROR, "app.createTileResource() invalid parent image handle %s", duk_to_string(ctx, 0));
	float x = duk_to_number(ctx, 1);
	float y = duk_to_number(ctx, 2);
	float w = duk_to_number(ctx, 3);
	float h = duk_to_number(ctx, 4);

	uint32_t img = gfxImageTile(parent, roundf(x*parentW), roundf(y*parentH), roundf(w*parentW), roundf(h*parentH));

	if( duk_is_object(ctx, 5)) {
		float cx = getPropFloatDefault(ctx, 5, "centerX", INFINITY);
		float cy = getPropFloatDefault(ctx, 5, "centerY", INFINITY);
		if(isfinite(cx) && isfinite(cy))
			gfxImageSetCenter(img, cx, cy);
	}
	duk_push_uint(ctx, img);
	return 1;
}

/**
 * @function app.createImageFontResource
 * creates a font based on a texture containing a fixed 16x16 grid of glyphs
 * @param {number|string} img - image resource handle
 * @param {object} [params] - additional optional parameters: border, scale
 * @returns {number} handle of the created font resource
 */

static duk_ret_t dk_createImageFontResource(duk_context *ctx) {
	uint32_t parent;
	float scale = 1.0f, border = 0.0f, cx = 0.0f, cy=0.0f;
	int filtering = 1;
	if( duk_is_object(ctx, 1)) {
		border = getPropFloatDefault(ctx, 1, "border", border);
		readImageResourceParams(ctx, 1, &scale, &filtering, &cx, &cy);
	}
	if(duk_is_number(ctx,0))
		parent = duk_to_uint32(ctx, 0);
	else {
		parent = ResourceGetImage(duk_to_string(ctx,0), scale, filtering);
		gfxImageSetCenter(parent, cx, cy);
		border *= scale;
	}
	if(!parent && scale!=1.0f)
		parent = gfxSVGUpload(0,0, scale);

	duk_push_uint(ctx, parent ? gfxFontFromImage(parent, border) : 0);
	return 1;
}

/**
 * @function app.setImageCenter
 * sets image origin and rotation center relative to upper left (0|0) and lower right (1.0|1.0) corners
 * @param {number} img - image resource handle
 * @param {number} cx - relative horizontal image center
 * @param {number} cy - relative vertical image center
 */
static duk_ret_t dk_appSetImageCenter(duk_context *ctx) {
	uint32_t img = duk_to_uint32(ctx, 0);
	float cx = duk_to_number(ctx, 1);
	float cy = duk_to_number(ctx, 2);
	gfxImageSetCenter(img, cx, cy);
	return 0;
}

/**
 * @function app.createSVGResource
 * creates an image resource from an inline SVG string
 * @param {string} svg - SVG image description
 * @param {object} [params] - image resource params such as scale factor
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createSVGResource(duk_context *ctx) {
	const char* svg = duk_to_string(ctx, 0);
	float scale = 1.0;
	float cx = 0.0f, cy=0.0f;
	readImageResourceParams(ctx, 1, &scale, NULL, &cx, &cy);
	size_t img = ResourceCreateSVGImage(svg, scale);
	gfxImageSetCenter(img, cx, cy);
	duk_push_uint(ctx, img);
	return 1;
}

/**
 * @function app.createImageResource
 * creates an image resource from a buffer or from a callback function
 * @param {number|object} width - image width or an object having width, height, depth, and data properties
 * @param {number} [height] - image height
 * @param {buffer|array|number|string} [data|cb] - RGBA 4-byte per pixel image data or background color if image shall be created via a callback function
 * @param {object|function} [params] - optional additional parameters as key-value pairs such as filtering  or callback function having a graphics context as parameter
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createImageResource(duk_context *ctx) {
	int width, height, depth = 4;
	size_t img = 0;
	int filtering = 1;
	float cx = 0.0f, cy=0.0f;
	if(duk_is_object(ctx,0)) {
		width = getPropUint32Default(ctx,0,"width",0);
		height = getPropUint32Default(ctx,0,"height",0);
		depth = getPropUint32Default(ctx,0,"depth",depth);

		duk_size_t nBytes = 0;
		unsigned char* data = (duk_get_prop_string(ctx, 0, "data") && duk_is_buffer_data(ctx, -1)) ?
			duk_get_buffer_data(ctx, -1, &nBytes) : NULL;
		duk_pop(ctx);
		if(nBytes!=width*height*depth)
			return duk_error(ctx, DUK_ERR_ERROR,
				"createImageResource buffer size does not fit to width and height");

		readImageResourceParams(ctx, 1, NULL, &filtering, &cx, &cy);
		gfxTextureFiltering(filtering);
		img = gfxImageUpload(data, width, height, depth, 0xff);
	}
	else {
		width = duk_to_int(ctx, 0);
		height = duk_to_int(ctx, 1);

		if(duk_is_buffer_data(ctx, 2)) {
			readImageResourceParams(ctx, 3, NULL, &filtering, &cx, &cy);
			duk_size_t nBytes;
			unsigned char* data = duk_get_buffer_data(ctx, 2, &nBytes);
			if(nBytes!=width*height*depth)
				return duk_error(ctx, DUK_ERR_ERROR,
					"createImageResource buffer size does not fit to width and height");
			img = ResourceCreateImage(width, height, data, filtering);
		}
		else if(duk_is_array(ctx, 2)) {
			readImageResourceParams(ctx, 3, NULL, &filtering, &cx, &cy);
			duk_size_t nBytes = duk_get_length(ctx, 2);
			if(nBytes!=width*height*depth)
				return duk_error(ctx, DUK_ERR_ERROR,
					"createImageResource array size does not fit to width and height");
			unsigned char* data = malloc(nBytes);
			for(unsigned i=0; i<nBytes; ++i) {
				duk_get_prop_index(ctx, 2, i);
				int value = duk_get_int_default(ctx, -1, 0);
				if(value<0)
					value = 0;
				else if(value>255)
					value = 255;
				data[i] = value;
				duk_pop(ctx);
			}
			img = ResourceCreateImage(width, height, data, filtering);
			free(data);
		}
		else if(duk_is_function(ctx, 3)) {
			readImageResourceParams(ctx, 4, NULL, &filtering, &cx, &cy);
			const uint32_t color = readColor(ctx, 2);
			size_t canvas = gfxCanvasCreate(width, height, color);
			duk_dup(ctx, 3); // push callback function onto stack
			gfxStateReset();
			duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("gfx")); // push graphics context
			duk_push_int(ctx, width);
			duk_push_int(ctx, height);
			duk_call(ctx, 3);
			img = gfxCanvasUpload(canvas);
		}
	}
	gfxImageSetCenter(img, cx, cy);
	duk_push_uint(ctx, img);
	return 1;
}

/**
 * @function app.releaseResource
 * releases a previously uploaded image, audio, or font resource
 * @param {number} handle - resource handle
 * @param {string} mediaType - mediaType, either 'image', 'audio', or 'font'
 */
static duk_ret_t dk_releaseResource(duk_context *ctx) {
	uint32_t handle = duk_to_uint32(ctx, 0);
	const char* mediaType = duk_to_string(ctx, 1);
	if(strncmp(mediaType, "image", 5)==0)
		gfxImageRelease(handle);
	else if(strncmp(mediaType, "font", 4)==0)
		gfxFontRelease(handle);
	else if(strncmp(mediaType, "audio", 5)==0)
		AudioRelease(handle);
	return 0;
}

/**
 * @function app.setBackground
 * sets window background color
 * @param {number|array|buffer} r - RGB red component in range 0-255 or color array / array buffer
 * @param {number} [g] - RGB green component in range 0-255
 * @param {number} [b] - RGB blue component in range 0-255
 */
static duk_ret_t dk_appSetBackground(duk_context *ctx) {
	if(duk_is_array(ctx, 0))
		WindowClearColor(readColor(ctx, 0));
	else if(duk_is_string(ctx, 0))
		WindowClearColor(cssColor(duk_get_string(ctx, 0)));
	else if(duk_is_undefined(ctx, 1))
		WindowClearColor(duk_to_uint(ctx, 0));
	else {
		uint32_t r = duk_to_uint(ctx, 0);
		uint32_t g = duk_to_uint(ctx, 1);
		uint32_t b = duk_to_uint(ctx, 2);
		WindowClearColor((r<<24) + (g<<16) + (b<<8));
	}
	return 0;
}

/**
 * @function app.setTitle
 * sets window title
 * @param {string} title - new title
 */
static duk_ret_t dk_appSetTitle(duk_context *ctx) {
	WindowTitle(duk_to_string(ctx, 0));
	return 0;
}

/**
 * @function app.resizable
 * sets window resizability
 * @param {bool} isResizable
 */
static duk_ret_t dk_appSetResizable(duk_context *ctx) {
	WindowResizable(duk_to_boolean(ctx, 0));
	return 0;
}

/**
 * @function app.fullscreen
 * toggles window fullscreen or returns fullscreen state
 * @param {bool} [fullscreen]
 * @returns {bool} current fullscreen state, if called without parameter
 */
static duk_ret_t dk_appFullscreen(duk_context *ctx) {
	int isFullscreen = WindowIsFullscreen();
	if(duk_is_undefined(ctx, 0)) {
		duk_push_boolean(ctx, isFullscreen);
		return 1;
	}
	int goFullscreen = duk_to_boolean(ctx, 0);
	if(goFullscreen != isFullscreen)
		WindowToggleFullScreen();
	return 0;
}

/**
 * @function app.minimize
 * minimizes or restores an application window
 * @param {bool} minimized
 */
static duk_ret_t dk_appMinimize(duk_context *ctx) {
	int minimize = duk_to_boolean(ctx, 0);
	if(minimize)
		WindowMinimize();
	else
		WindowRestore();
	return 0;
}

/**
 * @function app.transformArray
 * transforms a Float32Array by applying a function on all groups of members
 * @param {buffer} arr - Float32Array to be transformed
 * @param {number} stride - number of elements of a single logical record
 * @param {any} [param] - zero or more fixed parameters to be passed to the callback function
 * @param {function} callback - function transforming a single logical record at once, signature function(input, output[, param0,...])
 */
static duk_ret_t dk_transformArray(duk_context *ctx) {
	int argc = duk_get_top(ctx);

	float *arr = NULL;
	uint32_t arrLen = 0;
	if(duk_is_buffer_data(ctx, 0)) {
		duk_size_t nBytes;
		arr = duk_get_buffer_data(ctx, 0, &nBytes);
		if(nBytes%sizeof(float) == 0)
			arrLen = nBytes / sizeof(float);
	}
	if(!arrLen)
		return duk_error(ctx, DUK_ERR_ERROR, "app.transformArray() expects Float32Array as first argument");

	uint32_t stride = duk_to_uint32(ctx, 1);
	if(arrLen%stride != 0)
		return duk_error(ctx, DUK_ERR_ERROR, "app.transformArray() first argument array size is not multiple of stride");

	if(!duk_is_function(ctx, argc-1))
		return duk_error(ctx, DUK_ERR_ERROR, "app.transformArray() expects callback function as last argument");

	duk_push_external_buffer(ctx);
	duk_config_buffer(ctx, -1, arr, arrLen*sizeof(float));

	uint32_t recordSz = sizeof(float)*stride;
	float* output = duk_push_fixed_buffer(ctx, recordSz);
	duk_idx_t outBufIndex = duk_get_top_index(ctx), inBufIndex = outBufIndex-1;

	for(size_t i=0, end=arrLen/stride; i<end; ++i) {
		memcpy(output, &arr[i*stride], recordSz);
		duk_dup(ctx, argc-1); // callback
		duk_push_buffer_object(ctx, inBufIndex, i*recordSz, recordSz, DUK_BUFOBJ_FLOAT32ARRAY); // input
		duk_push_buffer_object(ctx, outBufIndex, 0, recordSz, DUK_BUFOBJ_FLOAT32ARRAY); // output
		for(int i=3; i<argc; ++i)
			duk_dup(ctx, i-1); // params
		duk_call(ctx, argc-1);
		duk_pop(ctx); // neglect return value
		memcpy(&arr[i*stride], output, recordSz);
	}
	return 0;
}

/**
 * @function app.setPointer
 * turns mouse pointer visiblity on or off
 * @param {Number} state - visible (1) invisible (0)
 */
static duk_ret_t dk_appSetPointer(duk_context *ctx) {
	WindowShowPointer(duk_to_int(ctx, 0));
	return 0;
}

/**
 * @function app.vibrate
 * vibrates the device, if supported by the platform, likely on mobile browsers only
 * @param {Number} duration - duration in seconds
 */
static duk_ret_t dk_appVibrate(duk_context *ctx) {
	return 0;
}

/**
 * @function app.prompt
 *
 * reads a string from a modal window or popup overlay
 *
 * @param {string|array} message - (multi-line) message to be displayed
 * @param {string} [initialValue] - optional prefilled value
 * @param {object} [options] - display options: font, title, titleFont, color, background, lineBreakAt, icon, button0, button1
 * @returns {string} entered string
 */
static duk_ret_t dk_appPrompt(duk_context *ctx) {
	const char *msg = duk_is_array(ctx, 0) ? dk_join_array(ctx, 0, "\n") : duk_to_string(ctx, 0);
	const char* initialValue = (duk_is_undefined(ctx, 1)||duk_is_null(ctx, 1))
		? NULL : duk_to_string(ctx, 1);
	Value* options = NULL;
	if(duk_is_object(ctx, 2))
		options = readValue(ctx, 2);

	char value[256];
	if(initialValue) {
		strncpy(value, initialValue, 255);
		value[255]=0;
	}
	else value[0] = 0;

	int ret = DialogMessageBox(msg, value, options);

	if(options)
		Value_delete(options, 1);
	if(ret!=0)
		return 0;
	duk_push_string(ctx, value);
	return 1;
}

/**
 * @function app.message
 *
 * displays a modal message window or popup overlay
 *
 * @param {string|array} message - (multi-line) message to be displayed
 * @param {string} [options] - display options: font, title, titleFont, color, background, lineBreakAt, icon, button0, button1
 * @returns {number} index of pressed button
 */
static duk_ret_t dk_appMessage(duk_context *ctx) {
	const char *msg = duk_is_array(ctx, 0) ? dk_join_array(ctx, 0, "\n") : duk_to_string(ctx, 0);
	Value* options = NULL;
	if(duk_is_object(ctx, 1))
		options = readValue(ctx, 1);

	int ret = DialogMessageBox(msg, NULL, options);

	if(options)
		Value_delete(options, 1);
	duk_push_int(ctx, ret);
	return 1;
}

/**
 * @function app.close
 * closes window and application
 */
static duk_ret_t dk_appClose(duk_context *ctx) {
	WindowEmitClose();
	return 0;
}

typedef struct {
	char* url;
	char* data;
	int callbackId;
	int isPost;
} HttpParams;

typedef struct {
	int status;
	char* resp;
	size_t respsz;
	ResourceTypeId mediaType;
} HttpRequest;

#define httpRequestsMax 32
static HttpRequest httpRequests[httpRequestsMax];

static int httpThread(void* udata) {
	HttpParams* params = (HttpParams*)udata;
	char* resp = NULL;
	size_t respsz = 0;
	int status = params->isPost ? httpPost(params->url, params->data, &resp, &respsz)
		: httpGet(params->url, &resp, &respsz);

	if(params->callbackId<0)
		free(resp);
	else {
		HttpRequest* req = &httpRequests[params->callbackId];
		req->resp = resp;
		req->status = status;
		req->respsz = respsz;
		if(resp)
			req->mediaType = ResourceType(params->url);
	}
	free(params->url);
	free(params->data);
	free(params);
	return status;
}

static int initHttpCallback(duk_context *ctx, duk_idx_t cbIndex) {
	int callbackId;
	for(callbackId=0; callbackId<httpRequestsMax; ++callbackId)
		if(!httpRequests[callbackId].status)
			break;

	if(callbackId == httpRequestsMax) {
		fprintf(stderr, "too many parallel http requests\n");
		return -1;
	}
	httpRequests[callbackId].resp = NULL;
	httpRequests[callbackId].respsz = 0;
	httpRequests[callbackId].status = 103;
	httpRequests[callbackId].mediaType = RESOURCE_NONE;
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("httpCallbacks"));
	duk_dup(ctx, cbIndex);
	duk_put_prop_index(ctx, -2, callbackId);
	duk_pop(ctx);
	return callbackId;
}

/**
 * @function app.httpGet
 * initiates a HTTP GET request
 * @param {string} url - requested URL
 * @param {function} [callback] - function to be called when the response is received. The first argument contains the received data, the second argument is the http response status code.
 */
static duk_ret_t dk_httpGet(duk_context *ctx) {
	const char* url = duk_to_string(ctx, 0);
	int callbackId = duk_is_function(ctx, 1) ? initHttpCallback(ctx, 1) : -1;

	HttpParams* params = (HttpParams*)malloc(sizeof(HttpParams));
	params->callbackId = callbackId;
	params->url = strdup(url);
	params->data = NULL;
	params->isPost = 0;
	SDL_Thread *thread = SDL_CreateThread(httpThread, "httpGet", (void*)params);
	SDL_DetachThread(thread);
	return 0;
}

/**
 * @function app.httpPost
 * initiates a HTTP POST request sending data to a URL
 * @param {string} url - target URL
 * @param {string|object} data - data to be sent
 * @param {function} [callback] - function to be called when a response is received. The first argument contains the received data, the second argument is the http response status code.
 */
static duk_ret_t dk_httpPost(duk_context *ctx) {
	const char* url = duk_to_string(ctx, 0);
	if(duk_is_object(ctx, 1))
		dk_encodeURI(ctx, 1);
	const char* data = duk_to_string(ctx, 1);

	int callbackId = duk_is_function(ctx, 2) ? initHttpCallback(ctx, 2) : -1;

	HttpParams* params = (HttpParams*)malloc(sizeof(HttpParams));
	params->callbackId = callbackId;
	params->url = strdup(url);
	params->data = strdup(data);
	params->isPost = 1;
	SDL_Thread *thread = SDL_CreateThread(httpThread, "httpPost", (void*)params);
	SDL_DetachThread(thread);
	return 0;
}

/**
 * @function app.openURL
 * opens a URL in a (new) browser window
 * @param {string} url - target URL
 */
static duk_ret_t dk_appOpenURL(duk_context *ctx) {
	const char* url = duk_to_string(ctx, 0);
	if(SDL_OpenURL(url)!=0) {
		snprintf(s_lastError, ERROR_MAXLEN, "app.openURL error: %s", SDL_GetError());
		SDL_ClearError();
		return duk_error(ctx, DUK_ERR_ERROR, s_lastError);
	}
	return 0;
}

/**
 * @function app.parse
 * parses an XML or (X)HTML, or JSON string as Javascript object
 * @param {string} url - target URL
 */
static duk_ret_t dk_appParse(duk_context *ctx) {
	const char* str = duk_to_string(ctx, 0);
	if(str) {
		const char* ch0 = str;
		while(*ch0 && SDL_isspace(*ch0))
			++ch0;
		Value* v = (SDL_isdigit(*ch0) || *ch0=='[' || *ch0=='"' || *ch0=='{') ?
			Value_parse(str) : Value_parseXML(str, NULL);
		pushValue(ctx, v);
		free(v);
	}
	return 1;
}

/**
 * @function app.include
 * loads JavaScript code from one or more other javascript source files
 * @param {string} filename - file name
 */
duk_ret_t dk_appInclude(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	for(int i=0; i<argc; ++i) {
		const char* fname = duk_to_string(ctx, i);
		if(jsvmEvalScript((size_t)ctx, fname)!=0)
			return duk_error(ctx, DUK_ERR_ERROR, s_lastError);
	}
	return 0;
}

/**
 * @function app.require
 * loads a module written in C or JavaScript (registered via app.exports)
 * @param {string} name - module file name without suffix
 * @returns {object|function} loaded module
 */
static duk_ret_t dk_appRequire(duk_context *ctx) {
	const char* dllName = duk_to_string(ctx, 0);
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("modules"));
	if(duk_get_prop_string(ctx, -1, dllName))
		return 1;
	duk_pop(ctx);

	if(jsvmRequire((size_t)ctx, dllName)!=0)
		return duk_error(ctx, DUK_ERR_ERROR, s_lastError);
	duk_dup_top(ctx);
	duk_put_prop_string(ctx, -3, dllName);
	return 1;
}

/**
 * @function app.exports
 * exports a JavaScript module that can be accessed via app.require
 * @param {string} id - module identifier to be used by require
 * @param {object|function} module - exported module
 */
static duk_ret_t dk_appExports(duk_context *ctx) {
	const char* id = duk_to_string(ctx, 0);
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("modules"));
	duk_dup(ctx, 1);
	duk_put_prop_string(ctx, -2, id);
	return 0;
}

/**
 * @function app.queryFont
 * measures text dimensions using a specified font.
 * @param {number} font - font resource handle, use 0 for built-in default 12x16 font
 * @param {string} [text] - optional text for width calculation
 * @returns {object} an object having the properties width, height, fontBoundingBoxAscent, fontBoundingBoxDescent
 */
static duk_ret_t dk_gfxQueryFont(duk_context *ctx) {
	size_t font = duk_to_number(ctx, 0);

	int noText = duk_is_undefined(ctx, 1);
	const char* text = noText ? "m" : duk_to_string(ctx, 1);
	float width, height, ascent, descent;
	gfxMeasureText(font, text, &width, &height, &ascent, &descent);
	duk_push_object(ctx);
	if(!noText) {
		duk_push_number(ctx, width);
		duk_put_prop_string(ctx, -2, "width");
	}
	duk_push_number(ctx, height);
	duk_put_prop_string(ctx, -2, "height");
	duk_push_number(ctx, ascent);
	duk_put_prop_string(ctx, -2, "fontBoundingBoxAscent");
	duk_push_number(ctx, descent);
	duk_put_prop_string(ctx, -2, "fontBoundingBoxDescent");
	return 1;
}

/**
 * @function app.queryImage
 * measures image dimensions.
 * @param {number} img - image resource handle
 * @returns {object} an object having the properties width and height
 */
static duk_ret_t dk_gfxQueryImage(duk_context *ctx) {
	int width=0, height=0;
	uint32_t img = duk_get_uint(ctx, 0);
	gfxImageDimensions(img, &width, &height);
	if(!width || !height) {
		snprintf(s_lastError, ERROR_MAXLEN, "invalid image handle %s", duk_to_string(ctx, 0));
		return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, s_lastError);
	}
	duk_push_object(ctx);
	duk_push_number(ctx, width);
	duk_put_prop_string(ctx, -2, "width");
	duk_push_number(ctx, height);
	duk_put_prop_string(ctx, -2, "height");
	return 1;
}

/**
 * @function app.hsl
 * converts a HSL color defined by hue, saturation, lightness, and optionally opacity to a single RGB color number.
 * @param {number} h - hue, value range 0.0..360.0
 * @param {number} s - saturation, value range 0.0..1.0
 * @param {number} l - lightness, value range 0.0..1.0
 * @param {number} [a=1.0] - opacity, value between 0.0 (invisible) and 1.0 (opaque)
 * @returns {number} - RGBA color value
 */
static duk_ret_t dk_appHSL(duk_context *ctx) {
	float h = duk_to_number(ctx, 0);
	float s = duk_to_number(ctx, 1);
	float l = duk_to_number(ctx, 2);
	float a = duk_get_number_default(ctx, 3, 1.0);
	duk_push_uint(ctx, hsla2rgba(h,s,l,a));
	return 1;
}

/**
 * @function app.cssColor
 * converts any CSS color string to a single RGB(A) color number.
 * @param {number|string} color - color string or number
 * @returns {number} - RGBA color value
 */
static duk_ret_t dk_appCssColor(duk_context *ctx) {
	const uint32_t color = duk_is_number(ctx, 0) ? duk_get_uint_default(ctx, 0, 0) : cssColor(duk_to_string(ctx, 0));
	duk_push_uint(ctx, color);
	return 1;
}

/**
 * @function app.createColorArray
 * creates an Uint32Array of colors having an appropriate native format for color arrays
 * @param {number}[, {number}...] colors - color values as numbers in format #RRGGBBAA (e.g., #00FF00FF for opaque green)
 * @returns {Uint32Array} - color values
 */
static duk_ret_t dk_appColorArray(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	const size_t bufSz =sizeof(uint32_t)*argc;
	uint32_t* buf = duk_push_fixed_buffer(ctx, bufSz);
	for(int i=0; i<argc; ++i)
		buf[i] = bswap_uint32(duk_to_uint32(ctx, i));
	duk_push_buffer_object(ctx, -1, 0, bufSz, DUK_BUFOBJ_UINT32ARRAY);
	return 1;
}

/**
 * @function app.arrayColor
 * returns a single color number having the appropriate (reverse) byte order for color arrays. May be used for writing or reading individual values of color Uint32Arrays
 * @param {number} color - color value as number in format #RRGGBBAA (e.g., #00FF00FF for opaque green)
 * @returns {number} - color value in appropriate byte order for color arrays
 */
static duk_ret_t dk_appArrayColor(duk_context *ctx) {
	duk_push_uint(ctx, bswap_uint32(duk_to_uint32(ctx, 0)));
	return 1;
}

static void joinStackTop(duk_context *ctx, duk_idx_t count) {
	for(duk_idx_t i=0; i<count; ++i)
		if(duk_is_array(ctx, -1-i) || duk_is_object(ctx, -1-i))
			duk_json_encode(ctx, -1-i);

	duk_push_string(ctx, " ");
	duk_insert(ctx, -count);
	duk_join(ctx, count);
}

/**
 * @function app.log
 * writes an info message to application log
 * @param {any} value - one or more values to write
 */
static duk_ret_t dk_appLogInfo(duk_context *ctx) {
	joinStackTop(ctx, duk_get_top(ctx));
	LogInfo("%s", duk_get_string(ctx, -1));
	return 0;
}

/**
 * @function app.warn
 * writes a warning message to application log
 * @param {any} value - one or more values to write
 */
static duk_ret_t dk_appLogWarn(duk_context *ctx) {
	joinStackTop(ctx, duk_get_top(ctx));
	LogWarn("%s", duk_get_string(ctx, -1));
	return 0;
}

/**
 * @function app.error
 * writes an error message to application log
 * @param {any} value - one or more values to write
 */
static duk_ret_t dk_appLogError(duk_context *ctx) {
	joinStackTop(ctx, duk_get_top(ctx));
	LogError("%s", duk_get_string(ctx, -1));
	return 0;
}

/// @property {array} app.args - script-relevant command line arguments (or URL parameters), to be passed after a -- as separator as key value pairs, keys start with a -- or -

/// @property {string} app.version - arcajs version
static duk_ret_t dk_appVersion(duk_context *ctx) {
	duk_push_string(ctx, appVersion);
	return 1;
}

/// @property {string} app.platform - arcajs platform, either 'browser' or 'standalone'
static duk_ret_t dk_appPlatform(duk_context *ctx) {
	duk_push_literal(ctx, "standalone");
	return 1;
}

/// @property {string} app.arch - operating system name and architecture, for example Linux_x86_64
static duk_ret_t dk_appArch(duk_context *ctx) {
	duk_push_literal(ctx, ARCAJS_ARCH);
	return 1;
}

/// @property {int} app.numControllers - number of currently connected game controllers
static duk_ret_t dk_appNumControllers(duk_context *ctx) {
	duk_push_int(ctx, WindowNumControllers());
	return 1;
}

/// @property {number} app.width - window width in logical pixels
static duk_ret_t dk_getWindowWidth(duk_context * ctx) {
	duk_push_int(ctx, WindowWidth());
	return 1;
}

/// @property {number} app.height - window height in logical pixels
static duk_ret_t dk_getWindowHeight(duk_context * ctx) {
	duk_push_int(ctx, WindowHeight());
	return 1;
}

/// @property {number} app.pixelRatio - ratio physical to logical pixels
static duk_ret_t dk_getWindowPixelRatio(duk_context * ctx) {
	duk_push_number(ctx, WindowPixelRatio());
	return 1;
}

static void bindApp(duk_context *ctx, const Value* args) {
	duk_push_object(ctx);

	pushValue(ctx, args);
	duk_put_prop_literal(ctx, -2, "args");

	duk_push_c_function(ctx, dk_onEvent, 2);
	duk_put_prop_string(ctx, -2, "on");
	duk_push_c_function(ctx, dk_appEmit, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "emit");
	duk_push_c_function(ctx, dk_getResource, 2);
	duk_put_prop_string(ctx, -2, "getResource");
	duk_push_c_function(ctx, dk_createCircleResource, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "createCircleResource");
	duk_push_c_function(ctx, dk_createPathResource, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "createPathResource");
	duk_push_c_function(ctx, dk_createTileResources, 5);
	duk_put_prop_literal(ctx, -2, "createTileResources");
	duk_push_c_function(ctx, dk_createTileResource, 6);
	duk_put_prop_literal(ctx, -2, "createTileResource");
	duk_push_c_function(ctx, dk_createSVGResource, 2);
	duk_put_prop_string(ctx, -2, "createSVGResource");
	duk_push_c_function(ctx, dk_createImageResource, 5);
	duk_put_prop_string(ctx, -2, "createImageResource");
	duk_push_c_function(ctx, dk_createImageFontResource, 2);
	duk_put_prop_string(ctx, -2, "createImageFontResource");
	duk_push_c_function(ctx, dk_releaseResource, 2);
	duk_put_prop_string(ctx, -2, "releaseResource");
	duk_push_c_function(ctx, dk_appSetImageCenter, 3);
	duk_put_prop_string(ctx, -2, "setImageCenter");
	duk_push_c_function(ctx, dk_gfxQueryImage, 1);
	duk_put_prop_string(ctx, -2, "queryImage");
	duk_push_c_function(ctx, dk_gfxQueryFont, 2);
	duk_put_prop_string(ctx, -2, "queryFont");
	duk_push_c_function(ctx, dk_appHSL, 4);
	duk_put_prop_string(ctx, -2, "hsl");
	duk_push_c_function(ctx, dk_appCssColor, 1);
	duk_put_prop_string(ctx, -2, "cssColor");
	duk_push_c_function(ctx, dk_appColorArray, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "createColorArray");
	duk_push_c_function(ctx, dk_appArrayColor, 1);
	duk_put_prop_string(ctx, -2, "arrayColor");

	duk_push_c_function(ctx, dk_appSetBackground, 3);
	duk_put_prop_string(ctx, -2, "setBackground");
	duk_push_c_function(ctx, dk_appSetTitle, 1);
	duk_put_prop_string(ctx, -2, "setTitle");
	duk_push_c_function(ctx, dk_appSetResizable, 1);
	duk_put_prop_string(ctx, -2, "resizable");
	duk_push_c_function(ctx, dk_appFullscreen, 1);
	duk_put_prop_string(ctx, -2, "fullscreen");
	duk_push_c_function(ctx, dk_appMinimize, 1);
	duk_put_prop_string(ctx, -2, "minimize");
	duk_push_c_function(ctx, dk_transformArray, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "transformArray");
	duk_push_c_function(ctx, dk_appSetPointer, 1);
	duk_put_prop_string(ctx, -2, "setPointer");
	duk_push_c_function(ctx, dk_appPrompt, 3);
	duk_put_prop_string(ctx, -2, "prompt");
	duk_push_c_function(ctx, dk_appMessage, 2);
	duk_put_prop_string(ctx, -2, "message");
	duk_push_c_function(ctx, dk_appClose, 0);
	duk_put_prop_string(ctx, -2, "close");
	duk_push_c_function(ctx, dk_httpGet, 2);
	duk_put_prop_string(ctx, -2, "httpGet");
	duk_push_c_function(ctx, dk_httpPost, 3);
	duk_put_prop_string(ctx, -2, "httpPost");
	duk_push_c_function(ctx, dk_appOpenURL, 1);
	duk_put_prop_string(ctx, -2, "openURL");
	duk_push_c_function(ctx, dk_appParse, 1);
	duk_put_prop_string(ctx, -2, "parse");
	duk_push_c_function(ctx, dk_appInclude, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "include");
	duk_push_c_function(ctx, dk_appRequire, 1);
	duk_put_prop_string(ctx, -2, "require");
	duk_push_c_function(ctx, dk_appExports, 2);
	duk_put_prop_string(ctx, -2, "exports");
	duk_push_c_function(ctx, dk_appVibrate, 1);
	duk_put_prop_string(ctx, -2, "vibrate");
	duk_push_c_function(ctx, dk_appLogInfo, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "log");
	duk_push_c_function(ctx, dk_appLogWarn, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "warn");
	duk_push_c_function(ctx, dk_appLogError, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "error");

	duk_push_literal(ctx, "arcajs_builtin_js_func_emitAsGamepadEvent");
	duk_compile_string_filename(ctx, DUK_COMPILE_FUNCTION, emitAsGamepadEvent_js);
	duk_put_prop_literal(ctx, -2, "emitAsGamepadEvent");

	dk_defineReadOnlyProperty(ctx,"width", -1, dk_getWindowWidth);
	dk_defineReadOnlyProperty(ctx,"height", -1, dk_getWindowHeight);
	dk_defineReadOnlyProperty(ctx,"pixelRatio", -1, dk_getWindowPixelRatio);
	dk_defineReadOnlyProperty(ctx,"numControllers", -1, dk_appNumControllers);
	dk_defineReadOnlyProperty(ctx,"version", -1, dk_appVersion);
	dk_defineReadOnlyProperty(ctx,"platform", -1, dk_appPlatform);
	dk_defineReadOnlyProperty(ctx,"arch", -1, dk_appArch);

	duk_put_global_string(ctx, "app");

	duk_push_object(ctx);
	duk_put_global_string(ctx, DUK_HIDDEN_SYMBOL("httpCallbacks"));

	memset(httpRequests, 0, sizeof(HttpRequest)*httpRequestsMax);
}

//--- audio bindings -----------------------------------------------

/** @module audio
 *
 * a collection of basic sound synthesis and replay functions
 *
 * ```javascript
 * var audio = app.require('audio');
 * ```
 */

/**
 * @function audio.volume
 * sets or returns master volume or volume of a currently playing track
 * @param {number} [track] - track ID
 * @param {number} [v] - new volume, a number between 0.0 and 1.0
 * @returns {number} the current master volume if called without arguments
 */
static duk_ret_t dk_audioVolume(duk_context *ctx) {
	if(duk_is_undefined(ctx, 0)) {
		duk_push_number(ctx, AudioGetVolume());
		return 1;
	}
	if(duk_is_undefined(ctx, 1)) {
		float volume = duk_to_number(ctx, 0);
		AudioSetVolume(volume);
	}
	AudioAdjustVolume(duk_to_number(ctx, 0), duk_to_number(ctx, 1));
	return 0;
}

/**
 * @function audio.playing
 * checks if a track or any track is currently playing
 * @param {number} [track] - track ID
 * @returns {boolean} true if the given track (or any track) is playing, otherwise false
 */
static duk_ret_t dk_audioPlaying(duk_context *ctx) {
	int isPlaying = 0;
	if(AudioIsRunning()) {
		if(duk_is_undefined(ctx, 0))
				for(unsigned i=0, end = AudioTracks(); i<end && !isPlaying; ++i)
					isPlaying = AudioPlaying(i);
		else
			isPlaying = AudioPlaying(duk_to_number(ctx, 0));
	}
	duk_push_boolean(ctx, isPlaying);
	return 1;
}

/**
 * @function audio.stop
 * immediately stops an individual track or all tracks
 * @param {number} [track] - track ID
 */
static duk_ret_t dk_audioStop(duk_context *ctx) {
	int isPlaying = 0;
	if(duk_is_undefined(ctx, 0))
		for(unsigned i=0, end = AudioTracks(); i<end && !isPlaying; ++i)
			AudioStop(i);
	else
		AudioStop(duk_to_number(ctx, 0));
	return 0;
}

/**
 * @function audio.suspend
 * (temporarily) suspends all audio output
 */
static duk_ret_t dk_audioSuspend(duk_context *ctx) {
	AudioSuspend();
	return 0;
}

/**
 * @function audio.resume
 * resumes previously suspended audio output
 */
static duk_ret_t dk_audioResume(duk_context *ctx) {
	AudioResume();
	return 0;
}

/**
 * @function audio.fadeOut
 * linearly fades out a currently playing track
 * @param {number} track - track ID
 * @param {number} deltaT - time from now in seconds until silence
 */
static duk_ret_t dk_audioFadeOut(duk_context *ctx) {
	AudioFadeOut(duk_to_number(ctx, 0), duk_to_number(ctx, 1));
	return 0;
}

/**
 * @function audio.replay
 * immediately plays a buffered PCM sample
 * @param {number|array} sample - sample handle or array of alternative samples (randomly chosen)
 * @param {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
 * @param {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
 * @param {number} [detune=0.0] - sample pitch shift in half tones. For example, -12.0 means half replay speed/ one octave less
 * @returns {number} track number playing this sound or UINT_MAX if no track is available
 */
static duk_ret_t dk_audioReplay(duk_context *ctx) {
	size_t sample;
	if(!duk_is_array(ctx,0))
		sample = duk_to_number(ctx, 0);
	else {
		uint32_t len = duk_get_length(ctx, 0);
		uint32_t idx = randf() * len;
		duk_get_prop_index(ctx, 0, idx);
		sample = duk_to_number(ctx, -1);
		//printf("random sample (%u/%u): %u\n", idx, len, (unsigned)sample);
		duk_pop(ctx);
	}
	if(!sample)
		return 0;
	float volume = duk_get_number_default(ctx, 1, 1.0);
	float balance = duk_get_number_default(ctx, 2, 0.0);
	float detune = duk_get_number_default(ctx, 3, 0.0);
	duk_push_number(ctx, AudioReplay(sample, volume, balance, detune));
	return 1;
}

/**
 * @function audio.loop
 * immediately and repeatedly plays a buffered PCM sample
 * @param {number|array} sample - sample handle or array of alternative samples (randomly chosen)
 * @param {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
 * @param {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
 * @param {number} [detune=0.0] - sample pitch shift in half tones. For example, -12.0 means half replay speed/ one octave less
 * @returns {number} track number playing this sound or UINT_MAX if no track is available
 */
static duk_ret_t dk_audioLoop(duk_context *ctx) {
	size_t sample;
	if(!duk_is_array(ctx,0))
		sample = duk_to_number(ctx, 0);
	else {
		uint32_t len = duk_get_length(ctx, 0);
		uint32_t idx = randf() * len;
		duk_get_prop_index(ctx, 0, idx);
		sample = duk_to_number(ctx, -1);
		//printf("random sample (%u/%u): %u\n", idx, len, (unsigned)sample);
		duk_pop(ctx);
	}
	if(!sample)
		return 0;
	float volume = duk_get_number_default(ctx, 1, 1.0);
	float balance = duk_get_number_default(ctx, 2, 0.0);
	float detune = duk_get_number_default(ctx, 3, 0.0);
	duk_push_number(ctx, AudioLoop(sample, volume, balance, detune));
	return 1;
}

static SoundWave readWaveForm(duk_context *ctx, duk_idx_t idx) {
	const char* w = duk_to_string(ctx, idx);
	if(strncmp(w, "sine", 2)==0)
		return WAVE_SINE;
	else if(strncmp(w, "triangle", 2)==0)
		return WAVE_TRIANGLE;
	else if(strncmp(w, "square", 2)==0)
		return WAVE_SQUARE;
	else if(strncmp(w, "sawtooth", 2)==0)
		return WAVE_SAWTOOTH;
	else if(strncmp(w, "noise", 2)==0)
		return WAVE_NOISE;
	else if(strncmp(w, "bin", 2)==0)
		return WAVE_BINNOISE;
	return WAVE_NONE;
}

/**
 * @function audio.sound
 * immediately plays an oscillator-generated sound
 * @param {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
 * @param {number|string} freq - frequency in Hz or note in form of F#2, A4
 * @param {number} duration - duration in seconds
 * @param {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
 * @param {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
 * @returns {number} track number playing this sound or UINT_MAX if no track is available
 */
static duk_ret_t dk_audioSound(duk_context *ctx) {
	SoundWave waveForm = readWaveForm(ctx, 0);
	if(!waveForm)
		return 0;
	float freq = duk_is_number(ctx, 1) ? duk_get_number(ctx, 1) : noteStr2freq(duk_to_string(ctx, 1));
	float duration = duk_to_number(ctx, 2);
	float volume = duk_get_number_default(ctx, 3, 1.0);
	float balance = duk_get_number_default(ctx, 4, 0.0);
	duk_push_number(ctx, AudioSound(waveForm, freq, duration, volume, balance));
	return 1;
}

/**
 * @function audio.createSound
 * creates a complex oscillator-generated sound
 * @param {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
 * @param {number|string} - one or more control points consisting of frequency/time interval/volume/shape 
 * @returns {number} a handle identifying this sound for later replay
 */
static duk_ret_t dk_audioCreateSound(duk_context *ctx) {
	SoundWave waveForm = readWaveForm(ctx, 0);
	if(!waveForm)
		return 0;
	duk_idx_t argc = duk_get_top(ctx);
	float* params = malloc(sizeof(float)*(--argc));
	for(duk_idx_t i=1; i<=argc; ++i) {
		float param;
		if(duk_is_number(ctx, i))
			param = duk_get_number(ctx, i);
		else {
			param = noteStr2freq(duk_to_string(ctx, i));
			if(param<0.0f)
				return duk_error(ctx, DUK_ERR_ERROR,  "invalid note as argument %u\n", i);
		}
		params[i-1] = param;
	}
	duk_push_number(ctx, AudioCreateSound(waveForm, argc/4, params));
	return 1;
}

/**
 * @function audio.createSoundBuffer
 * creates a complex oscillator-generated sound buffer
 * @param {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
 * @param {number|string} - one or more control points consisting of frequency/time interval/volume/shape 
 * @returns {Float32Array} a PCM sound buffer
 */
static duk_ret_t dk_audioCreateSoundBuffer(duk_context *ctx) {
	SoundWave waveForm = readWaveForm(ctx, 0);
	if(!waveForm)
		return 0;
	duk_idx_t argc = duk_get_top(ctx);
	float* params = malloc(sizeof(float)*(--argc));
	for(duk_idx_t i=1; i<=argc; ++i) {
		float param;
		if(duk_is_number(ctx, i))
			param = duk_get_number(ctx, i);
		else {
			param = noteStr2freq(duk_to_string(ctx, i));
			if(param<0.0f)
				return duk_error(ctx, DUK_ERR_ERROR,  "invalid note as argument %u\n", i);
		}
		params[i-1] = param;
	}

	uint32_t numSamples;
	float* buf = AudioCreateSoundBuffer(waveForm, argc/4, params, &numSamples);
	if(!buf || !numSamples)
		return duk_error(ctx, DUK_ERR_ERROR,  "failed to create sound");
	uint32_t bufSz = numSamples*sizeof(float);

	memcpy(duk_push_fixed_buffer(ctx, bufSz), buf, bufSz);
	free(buf);
	duk_push_buffer_object(ctx, -1, 0, bufSz, DUK_BUFOBJ_FLOAT32ARRAY);
	return 1;
}

/**
 * @function audio.melody
 * immediately plays an FM-generated melody based on a compact string notation
 * @param {string} melody - melody notated as a series of wave form descriptions and notes
 * @param {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
 * @param {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
 * @returns {number} track number playing this sound or UINT_MAX if no track is available
 */
static duk_ret_t dk_audioMelody(duk_context *ctx) {
	const char* melody = duk_to_string(ctx, 0);
	if(!melody)
		return 0;
	float volume = duk_get_number_default(ctx, 1, 1.0);
	float balance = duk_get_number_default(ctx, 2, 0.0);
	duk_push_number(ctx, AudioMelody(melody, volume, balance));
	return 1;
}

/**
 * @function audio.uploadPCM
 * uploads PCM data from an array of floating point numbers and returns a handle for later playback
 * @param {array|Float32Array|object} data - array of PCM sample values in range -1.0..1.0, or an object having data, channels, and offset attributes
 * @param {number} [channels=1] - number of channels, 1=mono, 2=stereo
 * @returns {number} sample handle to be used in audio
 */
static duk_ret_t dk_audioUploadPCM(duk_context *ctx) {
	float *waveData=0, *buf=0;
	uint32_t sampleLen=0, offset=0;
	uint8_t numChannels = 1;
	if(duk_is_array(ctx, 0) || duk_is_buffer_data(ctx, 0)) {
		sampleLen = readFloatArray(ctx, 0, &waveData, &buf);
		numChannels = duk_get_uint_default(ctx, 1, numChannels);
	}
	else if(duk_is_object(ctx, 0)) {
		sampleLen = getPropUint32Default(ctx, 0, "samples",0);
		numChannels = getPropUint32Default(ctx, 0, "channels", numChannels);
		offset = getPropUint32Default(ctx, 0, "offset", offset);

		duk_size_t nBytes = 0;
		waveData = (duk_get_prop_string(ctx, 0, "data") && duk_is_buffer_data(ctx, -1)) ?
			duk_get_buffer_data(ctx, -1, &nBytes) : NULL;
		duk_pop(ctx);
		if(nBytes != sampleLen*numChannels*sizeof(float))
			return duk_error(ctx, DUK_ERR_ERROR,
				"audio.uploadPCM buffer size does not fit to samples and channels count");
	}
	if(waveData && !buf) {
		buf = (float*)malloc(sampleLen*sizeof(float));
		memcpy(buf, waveData, sampleLen*sizeof(float));
	}
	size_t sample = AudioUploadPCM(buf, sampleLen/numChannels, numChannels, offset);
	duk_push_number(ctx, sample);
	return 1;
}

/**
 * @function audio.note2freq
 * translates a musical note (e.g., A4 , Bb5 C#3) to the corresponding frequency
 * @param {string|number} note - musical note pitch as string or as numeric frequency
 * @returns {number} - corresponding frequency
 */
static duk_ret_t dk_audioNote2freq(duk_context *ctx) {
	if(duk_is_number(ctx, 0))
		duk_dup(ctx, 0);
	else {
		float freq = noteStr2freq(duk_to_string(ctx, 0));
		if(freq<0.0f)
			return duk_error(ctx, DUK_ERR_ERROR,  "invalid note: %s\n", duk_to_string(ctx, 0));
		duk_push_number(ctx, freq);
	}
	return 1;
}

/**
 * @function audio.sampleBuffer
 * provides access to a sample's buffer
 * @param {number} sample - sample handle
 * @returns {Float32Array} - float32 buffer object containing the PCM samples
 */
static duk_ret_t dk_audioSampleBuffer(duk_context *ctx) {
	uint32_t sampleLen, handle = duk_to_uint32(ctx, 0);
	float *waveData = AudioSampleBuffer(handle, &sampleLen);
	if(!sampleLen || !waveData)
		return duk_error(ctx, DUK_ERR_ERROR,  "invalid sample handle %u\n", handle);
	duk_push_external_buffer(ctx);
	size_t bufSz = sampleLen*sizeof(float);
	duk_config_buffer(ctx, -1, waveData, bufSz);
	duk_push_buffer_object(ctx, -1, 0, bufSz, DUK_BUFOBJ_FLOAT32ARRAY);
	return 1;
}

/**
 * @function audio.clampBuffer
 * clamps a sample buffer' value range to given minimum and maximum values
 * @param {Float32Array} buffer - sample buffer to be truncated
 * @param {number} [minValue=-1.0] - minimum value
 * @param {number} [maxValue=+1.0] - maximum value
 */
static duk_ret_t dk_audioClampBuffer(duk_context *ctx) {
	float *waveData, *buf;
	uint32_t bufSz = readFloatArray(ctx, 0, &waveData, &buf);
	if(buf) {
		free(buf);
		return duk_error(ctx, DUK_ERR_ERROR,  "Float32Array buffer expected as first argument\n");
	}
	float minValue = duk_get_number_default(ctx, 1, -1.0);
	float maxValue = duk_get_number_default(ctx, 2, +1.0);
	AudioClampBuffer(bufSz, waveData, minValue, maxValue);
	return 0;
}

/**
 * @function audio.mixToBuffer
 * mixes a mono PCM sample to an existing stereo buffer. Both need to have the same sample
 * rate as the current audio device.
 * @param {Float32Array} target - target stereo buffer
 * @param {number|Float32Array} source - source mono sample or buffer
 * @param {number} [startTime=0.0] - start time offset
 * @param {number} [volume=1.0] - maximum value
 * @param {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
 */
static duk_ret_t dk_audioMixToBuffer(duk_context *ctx) {
	float *stereoBuffer, *buf=NULL, *sampleBuffer;
	uint32_t stereoBufSz = readFloatArray(ctx, 0, &stereoBuffer, &buf), sampleBufSz;
	if(buf) {
		free(buf);
		return duk_error(ctx, DUK_ERR_ERROR,  "Float32Array buffer expected as first argument\n");
	}

	if(duk_is_number(ctx, 1))
		sampleBuffer = AudioSampleBuffer(duk_to_uint32(ctx, 1), &sampleBufSz);
	else
		sampleBufSz = readFloatArray(ctx, 1, &sampleBuffer, &buf);
	if(buf || !sampleBuffer) {
		free(buf);
		return duk_error(ctx, DUK_ERR_ERROR,  "Float32Array buffer or sample handle expected as second argument\n");
	}

	double startTime = duk_get_number_default(ctx, 2, 0.0);
	float volume = duk_get_number_default(ctx, 3, 1.0);
	float balance = duk_get_number_default(ctx, 4, 0.0);
	AudioMixToBuffer(stereoBufSz/2, stereoBuffer, sampleBufSz, sampleBuffer, startTime, volume, balance);
	return 0;
}

/// @property {number} audio.sampleRate - audio device sample rate in Hz
static duk_ret_t dk_audioSampleRate(duk_context * ctx) {
	duk_push_uint(ctx, AudioSampleRate());
	return 1;
}

/// @property {number} audio.tracks - number of parallel audio tracks
static duk_ret_t dk_audioNumTracks(duk_context * ctx) {
	duk_push_uint(ctx, AudioTracks());
	return 1;
}

static void audio_exports(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_audioVolume, 2);
	duk_put_prop_string(ctx, -2, "volume");
	duk_push_c_function(ctx, dk_audioPlaying, 1);
	duk_put_prop_string(ctx, -2, "playing");
	duk_push_c_function(ctx, dk_audioStop, 1);
	duk_put_prop_string(ctx, -2, "stop");
	duk_push_c_function(ctx, dk_audioSuspend, 0);
	duk_put_prop_string(ctx, -2, "suspend");
	duk_push_c_function(ctx, dk_audioResume, 0);
	duk_put_prop_string(ctx, -2, "resume");
	duk_push_c_function(ctx, dk_audioFadeOut, 2);
	duk_put_prop_string(ctx, -2, "fadeOut");
	duk_push_c_function(ctx, dk_audioReplay, 4);
	duk_put_prop_string(ctx, -2, "replay");
	duk_push_c_function(ctx, dk_audioLoop, 4);
	duk_put_prop_string(ctx, -2, "loop");
	duk_push_c_function(ctx, dk_audioSound, 5);
	duk_put_prop_string(ctx, -2, "sound");
	duk_push_c_function(ctx, dk_audioCreateSound, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "createSound");
	duk_push_c_function(ctx, dk_audioCreateSoundBuffer, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "createSoundBuffer");
	duk_push_c_function(ctx, dk_audioMelody, 3);
	duk_put_prop_string(ctx, -2, "melody");
	duk_push_c_function(ctx, dk_audioUploadPCM, 2);
	duk_put_prop_string(ctx, -2, "uploadPCM");
	duk_push_c_function(ctx, dk_audioNote2freq, 1);
	duk_put_prop_string(ctx, -2, "note2freq");
	duk_push_c_function(ctx, dk_audioSampleBuffer, 1);
	duk_put_prop_string(ctx, -2, "sampleBuffer");
	duk_push_c_function(ctx, dk_audioClampBuffer, 3);
	duk_put_prop_string(ctx, -2, "clampBuffer");
	duk_push_c_function(ctx, dk_audioMixToBuffer, 5);
	duk_put_prop_string(ctx, -2, "mixToBuffer");
	dk_defineReadOnlyProperty(ctx,"sampleRate", -1, dk_audioSampleRate);
	dk_defineReadOnlyProperty(ctx,"tracks", -1, dk_audioNumTracks);
}

//--- LocalStorage -------------------------------------------------

enum {
	STORAGE_ERROR = -2,
	STORAGE_UNINITIALIZED = -1,
	STORAGE_UNCHANGED = 0,
	STORAGE_CHANGED = 1,
};

void LocalStorageLoad(duk_context *ctx, const char* fname) {
	SDL_RWops *io = SDL_RWFromFile(fname, "rb");
	if (!io || io->size(io)<=0) {
		fprintf(stderr, "localStorage file \"%s\" not found\n", fname);
		return;
	}
	const size_t fsize = io->size(io);

	char* buffer = (char*)malloc(fsize+1);
	buffer[fsize] = 0;
	if(SDL_RWread(io, buffer, 1, fsize) != fsize)
		fprintf(stderr, "localStorage file \"%s\" read error\n", fname);
	else {
		duk_get_global_string(ctx, "localStorage");
		duk_push_string(ctx, buffer);
		if (duk_safe_call(ctx, tryJsonDecode, NULL, 1 /*nargs*/, 1 /*nrets*/) != DUK_EXEC_SUCCESS)
			fprintf(stderr, "localStorage load file \"%s\" JSON decode error\n", fname);
		else
			duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("data"));
		duk_pop(ctx);
	}
	SDL_RWclose(io);
	free(buffer);
}

void LocalStoragePersistChanges(duk_context *ctx) {
	duk_get_global_string(ctx, "localStorage");
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("status"));
	int status = duk_get_int_default(ctx, -1, 0);
	duk_pop(ctx);
	if(status<STORAGE_CHANGED) { // not changed
		duk_pop(ctx);
		return;
	}

	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("fname"));
	const char* fname = duk_get_string(ctx, -1);
	SDL_RWops *io = SDL_RWFromFile(fname, "wb");
	duk_pop(ctx);

	if (!io) {
		duk_pop(ctx);
		fprintf(stderr, "localStorage file not writable\n");
		return;
	}
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("data"));
	const char* json = duk_json_encode(ctx, -1);

	size_t len = strlen(json);
	if(SDL_RWwrite(io, json, 1, len) != len)
		fprintf(stderr, "localStorage file write error\n");
	SDL_RWclose(io);

	duk_pop(ctx);
	duk_push_int(ctx, STORAGE_UNCHANGED);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("status"));
	duk_pop(ctx);
}

void LocalStoragePush(duk_context* ctx) {
	duk_get_global_string(ctx, "localStorage");
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("status"));
	int status = duk_get_int_default(ctx, -1, 0);
	duk_pop(ctx);
	if(status == STORAGE_UNINITIALIZED) {
		duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("fname"));
		const char * fname = duk_get_string(ctx, -1);
		LocalStorageLoad(ctx, fname);
		duk_pop(ctx);
		duk_push_int(ctx, STORAGE_UNCHANGED);
		duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("status"));
	}
}

static duk_ret_t dk_storageSetItem(duk_context *ctx) {
	LocalStoragePush(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("data"));
	duk_dup(ctx, 0);
	duk_dup(ctx, 1);
	duk_put_prop(ctx, -3);
	duk_pop(ctx);

	duk_push_int(ctx, STORAGE_CHANGED);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("status"));
	return 0;
}

static duk_ret_t dk_storageGetItem(duk_context *ctx) {
	LocalStoragePush(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("data"));
	duk_dup(ctx, 0);
	duk_get_prop(ctx, -2);
	if(duk_is_undefined(ctx, -1))
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t dk_storageRemoveItem(duk_context *ctx) {
	LocalStoragePush(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("data"));
	duk_dup(ctx, 0);
	duk_del_prop(ctx, -2);
	duk_pop(ctx);

	duk_push_int(ctx, STORAGE_CHANGED);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("status"));
	return 0;
}

static duk_ret_t dk_storageClear(duk_context *ctx) {
	LocalStoragePush(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("data"));

	duk_push_int(ctx, STORAGE_CHANGED);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("status"));
	return 0;
}

void bindLocalStorage(duk_context *ctx, const char* storageFileName) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_storageSetItem, 2);
	duk_put_prop_string(ctx, -2, "setItem");
	duk_push_c_function(ctx, dk_storageGetItem, 1);
	duk_put_prop_string(ctx, -2, "getItem");
	duk_push_c_function(ctx, dk_storageRemoveItem, 1);
	duk_put_prop_string(ctx, -2, "removeItem");
	duk_push_c_function(ctx, dk_storageClear, 1);
	duk_put_prop_string(ctx, -2, "clear");

	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("data"));
	duk_push_int(ctx, STORAGE_UNINITIALIZED);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("status"));
	duk_push_string(ctx, storageFileName);
	duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("fname"));

	duk_put_global_string(ctx, "localStorage");
}


//--- module handling ----------------------------------------------

typedef struct {
	void* handle;
	void (*unloadFunc)();
} ModuleHandle;

static ModuleHandle* modules = NULL;
static unsigned numModules = 0u;
static unsigned numModulesMax = 0u;

static void* moduleLoad(const char* dllName) {
	size_t len = strlen(dllName), dllNameWithSuffixLen = len+5;
	char* dllNameWithSuffix = malloc(dllNameWithSuffixLen);
	strncpy(dllNameWithSuffix, dllName, len);
#if defined __WIN32__ || defined WIN32
	const char* suffix = ".dll";
	const char* sep = "\\";
#else
	const char* suffix = ".so";
	const char* sep = "/";
#endif
	strcpy(dllNameWithSuffix+len, suffix);
	void* libHandle = SDL_LoadObject(dllNameWithSuffix);
	if(debug)
		printf("(1) looking for module at %s... ", dllNameWithSuffix);

	if(!libHandle) {
		char* basePath = SDL_GetBasePath();
		size_t basePathLen = strlen(basePath);

		char* fullPath = (char*)malloc(basePathLen + dllNameWithSuffixLen);
		strcpy(fullPath, basePath);
		strcat(fullPath, dllNameWithSuffix);
		if(debug)
			printf("(2) looking for module at %s... ", fullPath);
		libHandle = SDL_LoadObject(fullPath);
		SDL_free(basePath);
		free(fullPath);
	}
	if(!libHandle) {
		const char* resourcePath = ResourceArchiveName();
		size_t resourcePathLen = strlen(resourcePath);
		if(resourcePathLen && !strlen(ResourceSuffix(resourcePath))) {
			char* fullPath = (char*)malloc(resourcePathLen + dllNameWithSuffixLen + sizeof(ARCAJS_ARCH) + 2);
			strcpy(fullPath, resourcePath);
			char lastChar = resourcePath[resourcePathLen-1];
			if(lastChar!=sep[0])
				strcat(fullPath, sep);
			strcat(fullPath, dllName);
			strcat(fullPath, ".");
			strcat(fullPath, ARCAJS_ARCH);
			strcat(fullPath, suffix);
			if(debug)
				printf("(3) looking for module at %s... ", fullPath);
			libHandle = SDL_LoadObject(fullPath);
			free(fullPath);
		}
	}

	free(dllNameWithSuffix);
	if(!libHandle)
		return 0;
	SDL_ClearError();

	const char unloadFuncNameSuffix[] = "_unload";
	char* unloadFuncName = malloc(len+sizeof(unloadFuncNameSuffix)+1);
	strncpy(unloadFuncName, dllName, len);
	strcpy(unloadFuncName+len, unloadFuncNameSuffix);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	void (*unloadFunc)() = (void (*)())SDL_LoadFunction(libHandle, unloadFuncName);
#pragma GCC diagnostic pop
	if(!unloadFunc)
		SDL_ClearError();
	free(unloadFuncName);

	if(numModulesMax==0) {
		numModulesMax=4;
		modules = (ModuleHandle*)malloc(numModulesMax*sizeof(ModuleHandle));
	}
	else if(numModules == numModulesMax) {
		numModulesMax *= 2;
		modules = (ModuleHandle*)realloc(modules, numModulesMax*sizeof(ModuleHandle));
	}
	modules[numModules].unloadFunc = unloadFunc;
	modules[numModules++].handle = libHandle;
	return libHandle;
}

static void modulesUnload() {
	for(unsigned i=numModules; i-->0; ) {
		if(modules[i].unloadFunc)
			(modules[i].unloadFunc)();
		SDL_UnloadObject(modules[i].handle);
	}
	numModules = 0;
}

//--- public interface ---------------------------------------------

size_t jsvmInit(const char* storageFileName, const Value* args) {
	s_lastError[0] = 0;
	duk_context *ctx = duk_create_heap_default();
	if(!ctx)
		return 0;

	bindApp(ctx, args);
	bindGraphics(ctx);
	bindConsole(ctx);
	bindLocalStorage(ctx, storageFileName);
	bindWorker(ctx);
	bindTimeout(ctx);

	// built-in modules:
	duk_push_object(ctx);
	intersects_exports(ctx);
	duk_put_prop_literal(ctx, -2, "intersects");
	audio_exports(ctx);
	duk_put_prop_literal(ctx, -2, "audio");
	duk_put_global_string(ctx, DUK_HIDDEN_SYMBOL("modules"));

	return (size_t)ctx;
}

int jsvmEval(size_t vm, const char* src, const char* fname) {
	if(!src || !vm)
		return -1;
	duk_context *ctx = (duk_context*)vm;
	duk_push_string(ctx, fname);
	if (duk_pcompile_string_filename(ctx, 0, src) != 0) {
		duk_get_prop_string(ctx, -1, "lineNumber");
		snprintf(s_lastError, ERROR_MAXLEN,
			"%s:%i: compile time error: %s\n", fname, duk_to_int(ctx, -1), duk_safe_to_string(ctx, -2));
		duk_pop_2(ctx);
		return -1;
	}
	int ret = duk_pcall(ctx, 0);
	if (ret != 0) {
		duk_get_prop_string(ctx, -1, "lineNumber");
		snprintf(s_lastError, ERROR_MAXLEN,
			"%s:%i: runtime error: %s\n", fname, duk_to_int(ctx, -1), duk_safe_to_string(ctx, -2));
		duk_pop_2(ctx);
	}
	return ret;
}

int jsvmEvalScript(size_t vm, const char* fname) {
	char* script = ResourceGetText(fname);
	if(!script) {
		snprintf(s_lastError, ERROR_MAXLEN, "Could not find \"%s\" in \"%s\", exiting.\n", fname, ResourceArchiveName());
		return -1;
	}
	int ret = jsvmEval(vm, script, fname);
	free(script);
	return ret;
}

static void callEventHandler(
	duk_context *ctx, const char* event, duk_bool_t isMethod, duk_idx_t nargs)
{
	if((isMethod ? duk_pcall_method(ctx, nargs) : duk_pcall(ctx, nargs))!=0) {
		duk_get_prop_string(ctx, -1, "lineNumber");
		duk_get_prop_string(ctx, -2, "fileName");

		snprintf(s_lastError, ERROR_MAXLEN, "%s:%i: runtime error during '%s' event: %s\n",
			duk_safe_to_string(ctx, -1), duk_to_int(ctx, -2), event, duk_safe_to_string(ctx, -3));
		duk_pop_2(ctx);
	}
}

void jsvmDispatchEvent(size_t vm, const char* event, const Value* data) {
	duk_context *ctx = (duk_context*)vm;

	duk_push_global_stash(ctx);
	if(!duk_get_prop_string(ctx, -1, event) || !duk_is_function(ctx, -1)) { // no function listening
		duk_pop_n(ctx, 2);
		return;
	}

	duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("currHandler"));
	duk_bool_t isMethod = duk_is_object(ctx, -1);
	if(!isMethod)
		duk_pop(ctx);

	duk_idx_t nargs = 0;
	if(strcmp(event, "resize")==0) {
		duk_push_int(ctx, Value_get(data, "width")->i);
		duk_push_int(ctx, Value_get(data, "height")->i);
		nargs = 2;
	}
	else for(const Value* arg=data; arg!=NULL; arg = arg->next, ++nargs)
		pushValue(ctx, arg);
	callEventHandler(ctx, event, isMethod, nargs);
	duk_pop_2(ctx); // pop result and global stash
}

void jsvmDispatchDrawEvent(size_t vm) {
	static size_t frameCounter = 0;
	duk_context *ctx = (duk_context*)vm;

	duk_push_global_stash(ctx);
	if(duk_get_prop_literal(ctx, -1, "draw") && duk_is_function(ctx, -1)) { // function listening
		duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("currHandler"));
		duk_bool_t isMethod = duk_is_object(ctx, -1);
		if(!isMethod)
			duk_pop(ctx);
		duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("gfx"));
		callEventHandler(ctx, "draw", isMethod, 1);
	}
	duk_pop_2(ctx); // pop result or "draw" and global stash

	if(++frameCounter%30==0)
		LocalStoragePersistChanges(ctx);
}

static void dispatchAxisEvent(size_t id, uint8_t axis, float value, void* vm) {
	Value* event = Value_new(VALUE_MAP, NULL);
	Value_set(event, "evt", Value_str("gamepad"));
	Value_set(event, "index", Value_int(id));
	Value_set(event, "type", Value_str("axis"));
	Value_set(event, "axis", Value_int(axis));
	Value_set(event, "value", Value_float(value));
	jsvmDispatchEvent((size_t)vm, "gamepad", event);
	Value_delete(event, 1);
}

static void dispatchButtonEvent(size_t id, uint8_t button, float value, void* vm) {
	Value* event = Value_new(VALUE_MAP, NULL);
	Value_set(event, "evt", Value_str("gamepad"));
	Value_set(event, "index", Value_int(id));
	Value_set(event, "type", Value_str("button"));
	Value_set(event, "button", Value_int(button));
	Value_set(event, "value", Value_float(value));
	jsvmDispatchEvent((size_t)vm, "gamepad", event);
	Value_delete(event, 1);
}

void jsvmDispatchGamepadEvents(size_t vm) {
	duk_context *ctx = (duk_context*)vm;
	duk_push_global_stash(ctx);
	duk_bool_t hasListener = duk_has_prop_literal(ctx, -1, "gamepad");
	duk_pop(ctx);
	if(hasListener)
		WindowControllerEvents(0.1, ctx, dispatchAxisEvent, dispatchButtonEvent);
}

void jsvmUpdateEventListeners(size_t vm) {
	duk_context *ctx = (duk_context*)vm;
	duk_get_global_literal(ctx, DUK_HIDDEN_SYMBOL("nextHandler"));
	duk_idx_t handlerIdx = duk_get_top_index(ctx);
	if(duk_is_object(ctx, handlerIdx)) {
		jsvmDispatchEvent(vm, "leave", NULL);
		duk_dup(ctx, handlerIdx);
		duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("currHandler"));
		// update listeners:
		static const char* events[] = {
			"update", "draw", "resize", "keyboard", "pointer", "gamepad", "enter", "leave",
			"visibilitychange", "custom", "textinput", "wheel", NULL
		};
		for(const char**evt = events; *evt; ++evt) {
			duk_get_prop_string(ctx, handlerIdx, *evt);
			updateEventHandler(*evt, ctx, -1);
			duk_pop(ctx);
		}
		jsvmDispatchEvent(vm, "enter", NULL);
		// cleanup next listeners:
		duk_push_global_object(ctx);
		duk_del_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("nextHandler"));
		duk_pop(ctx);
	}
	duk_pop(ctx);
}

void jsvmAsyncCalls(size_t vm, double timestamp) {
	duk_context *ctx = (duk_context*)vm;
	for(int i=0; i<httpRequestsMax; ++i) {
		if(httpRequests[i].status!=0 && httpRequests[i].status!=103) {
			HttpRequest* req = &httpRequests[i];
			duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("httpCallbacks"));
			duk_get_prop_index(ctx, -1, i);
			unsigned char* data = 0;
			if(req->resp && req->mediaType == RESOURCE_IMAGE) {
				int w,h,d;
				data = readImageData((const unsigned char *)(req->resp), req->respsz, &w, &h, &d);
				if(data) {
					const size_t bufSz = w * h * d;
					duk_push_object(ctx);
					memcpy(duk_push_fixed_buffer(ctx, bufSz), data, bufSz);
					duk_push_buffer_object(ctx, -1, 0, bufSz, DUK_BUFOBJ_UINT8ARRAY);
					duk_put_prop_string(ctx, -3, "data");
					duk_pop(ctx); // fixed buffer no longer needed on the stack, bound by buffer object
					duk_push_number(ctx, w);
					duk_put_prop_string(ctx, -2, "width");
					duk_push_number(ctx, h);
					duk_put_prop_string(ctx, -2, "height");
					duk_push_number(ctx, d);
					duk_put_prop_string(ctx, -2, "depth");
					duk_push_literal(ctx, "image");
					duk_put_prop_string(ctx, -2, "mediaType");
				}
			}
			else if(req->resp && req->mediaType == RESOURCE_AUDIO) {
				uint32_t samples = 0, offset = 0;
				uint8_t channels=0;
				data = (unsigned char*)AudioRead(req->resp, req->respsz, &samples, &channels, &offset);
				if(data) {
					const size_t bufSz = samples * channels * sizeof(float);
					duk_push_object(ctx);
					memcpy(duk_push_fixed_buffer(ctx, bufSz), data, bufSz);
					duk_push_buffer_object(ctx, -1, 0, bufSz, DUK_BUFOBJ_FLOAT32ARRAY);
					duk_put_prop_string(ctx, -3, "data");
					duk_pop(ctx); // fixed buffer no longer needed on the stack, bound by buffer object
					duk_push_number(ctx, samples);
					duk_put_prop_string(ctx, -2, "samples");
					duk_push_number(ctx, channels);
					duk_put_prop_string(ctx, -2, "channels");
					duk_push_number(ctx, offset);
					duk_put_prop_string(ctx, -2, "offset");
					duk_push_literal(ctx, "audio");
					duk_put_prop_string(ctx, -2, "mediaType");
				}
			}
			else if(req->resp && req->mediaType == RESOURCE_FONT) {
				snprintf(s_lastError, ERROR_MAXLEN, "loading of fonts via http not implemented");
			}

			if(data)
				free(data);
			else
				duk_push_string(ctx, req->resp);
			duk_push_int(ctx, req->status);
			duk_pcall(ctx, 2);
			duk_pop(ctx); // ignore cb return value
			duk_del_prop_index(ctx, -1, i);
			duk_pop(ctx);
			free(req->resp);
			req->resp = NULL;
			req->respsz = 0;
			req->status = 0;
			req->mediaType = RESOURCE_NONE;
		}
	}
	updateTimeouts(ctx, timestamp);
	updateWorkers(ctx, timestamp);
}

const char* jsvmLastError(size_t vm) {
	return s_lastError[0] ? s_lastError : NULL;
}

void jsvmClose(size_t vm) {
	duk_context *ctx = (duk_context*)vm;
	LocalStoragePersistChanges(ctx);
	duk_destroy_heap(ctx);
	modulesUnload();
}

int jsvmRequire(size_t vm, const char* dllName) {
	void* libHandle = moduleLoad(dllName);
	if(!libHandle) {
		snprintf(s_lastError, ERROR_MAXLEN, "could not load shared library %s", dllName);
		return -1;
	}

	size_t len = strlen(dllName);
	const char exportsFuncNameSuffix[] = "_exports";
	char* exportsFuncName = malloc(len+sizeof(exportsFuncNameSuffix)+1);
	strncpy(exportsFuncName, dllName, len);
	strcpy(exportsFuncName+len, exportsFuncNameSuffix);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	void (*bindFunc)(duk_context *ctx)
		= (void (*)(duk_context *))SDL_LoadFunction(libHandle, exportsFuncName);
#pragma GCC diagnostic pop
	if(!bindFunc) {
		snprintf(s_lastError, ERROR_MAXLEN,
			"shared library %s does not contain %s(ctx) function", dllName, exportsFuncName);
		free(exportsFuncName);
		return -2;
	}
	free(exportsFuncName);
	duk_context *ctx = (duk_context*)vm;
	bindFunc(ctx);
	return 0;
}

//--- JSON ---------------------------------------------------------

size_t jsonDecode(const char* str) {
	duk_context *ctx = duk_create_heap_default();
	duk_push_string(ctx, str);
	if (duk_safe_call(ctx, tryJsonDecode, NULL, 1 /*nargs*/, 1 /*nrets*/) == DUK_EXEC_SUCCESS)
		return (size_t)ctx;
	jsonClose((size_t)ctx);
	return 0;
}

char* jsonGetString(size_t json, const char* key) {
	duk_context *ctx = (duk_context*)json;
	if(!ctx)
		return NULL;
	char *str = duk_get_prop_string(ctx, -1, key) ? strdup(duk_to_string(ctx, -1)) : NULL;
	duk_pop(ctx);
	return str;
}

char** jsonGetStringArray(size_t json, const char* key) {
	duk_context *ctx = (duk_context*)json;
	if(!ctx)
		return NULL;

	char** vs=NULL;
	if(duk_get_prop_string(ctx, -1, key)) {
		if(duk_is_array(ctx, -1)) {
			duk_idx_t arrIdx = duk_get_top_index(ctx);
			size_t len = duk_get_length(ctx, arrIdx);
			vs = (char**)malloc(sizeof(char*)*(len+1));
			vs[len] = NULL;

			for(size_t i=0; i<len; ++i) {
				duk_get_prop_index(ctx, arrIdx, i);
				vs[i] = strdup(duk_to_string(ctx, -1));
				duk_pop(ctx);
			}
		}
		else if(duk_is_object(ctx, -1)) {
			// first count properties:
			size_t len=0, idx=0;
			duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
			duk_idx_t enumIdx = duk_get_top_index(ctx);
			while (duk_next(ctx, enumIdx, 1 /*get_value*/)) {
				len += 2;
				duk_pop_2(ctx);
			}
			duk_pop(ctx); // pop enum obj

			// now read key value pairs:
			vs = (char**)malloc(sizeof(char*)*(len+1));
			vs[len] = NULL;
			duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
			enumIdx = duk_get_top_index(ctx);
			while (duk_next(ctx, enumIdx, 1 /*get_value*/)) {
				vs[idx++] = strdup(duk_to_string(ctx, -2));
				vs[idx++] = strdup(duk_to_string(ctx, -1));
				duk_pop_2(ctx);
			}
			duk_pop(ctx); // pop enum obj
		}
	}
	duk_pop(ctx);
	return vs;
}

double jsonGetNumber(size_t json, const char* key, double defaultValue) {
	duk_context *ctx = (duk_context*)json;
	if(!ctx)
		return defaultValue;
	double f = (duk_get_prop_string(ctx, -1, key) && duk_is_number(ctx, -1)) ?
		duk_to_number(ctx, -1) : defaultValue;
	duk_pop(ctx);
	return f;
}

void jsonClose(size_t json) {
	duk_context *ctx = (duk_context*)json;
	if(ctx)
		duk_destroy_heap(ctx);
}
