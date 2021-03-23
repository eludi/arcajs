#include "jsBindings.h"
#include "window.h"
#include "graphics.h"
#include "resources.h"
#include "audio.h"
#include "console.h"
#include "httpRequest.h"

#include "external/duk_config.h"
#include "external/duktape.h"

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <SDL_thread.h>
#include <SDL_loadso.h>

extern float clampf(float f, float min, float max);
extern float randf();
extern const char* appVersion;
extern void sprites_exports(duk_context *ctx);
extern void intersects_exports(duk_context *ctx);

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

uint32_t array2color(duk_context *ctx, duk_idx_t idx) {
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
	return 0xffffffff;
}

Value* readValue(duk_context *ctx, duk_idx_t objIdx) {
	Value* obj = Value_new(VALUE_MAP, NULL);
	duk_enum(ctx, objIdx, DUK_ENUM_OWN_PROPERTIES_ONLY);
	duk_idx_t enumIdx = duk_get_top_index(ctx);
	while (duk_next(ctx, enumIdx, 1 /*get_value*/)) {
		duk_idx_t keyIdx = duk_get_top_index(ctx)-1, valueIdx = keyIdx+1;

		switch(duk_get_type(ctx, valueIdx)) {
		case DUK_TYPE_STRING:
			Value_set(obj, duk_to_string(ctx, keyIdx), Value_str(duk_to_string(ctx, valueIdx)));
			break;
		case DUK_TYPE_NUMBER: {
			double f = duk_get_number(ctx, valueIdx);
			signed long long i = f;
			if(f==(double)i)
				Value_set(obj, duk_to_string(ctx, keyIdx), Value_int(i));
			else
				Value_set(obj, duk_to_string(ctx, keyIdx), Value_float(f));
			break;
		}
		case DUK_TYPE_BOOLEAN:
			Value_set(obj, duk_to_string(ctx, keyIdx), Value_bool(duk_to_boolean(ctx, valueIdx)));
			break;
		case DUK_TYPE_NULL:
			Value_set(obj, duk_to_string(ctx, keyIdx), Value_new(VALUE_NONE, NULL));
			break;
		default:
			if(duk_is_array(ctx, valueIdx) || duk_is_buffer_data(ctx, valueIdx))
				Value_set(obj, duk_to_string(ctx, keyIdx), Value_int(array2color(ctx, valueIdx)));
		}
		//printf("%s -> %s\n", duk_to_string(ctx, keyIdx), duk_to_string(ctx, valueIdx));
		duk_pop_2(ctx);
	}
	//duk_push_context_dump(ctx); printf("%s\n", duk_to_string(ctx, -1)); duk_pop(ctx);
	duk_pop(ctx); // pop enum obj
	return obj;
}

static duk_ret_t tryJsonDecode(duk_context *ctx, void* udata) {
	duk_json_decode(ctx, -1);
	return 1;
}

static void dk_push_value(duk_context* ctx, const Value* value) {
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
			dk_push_value(ctx, item);
			duk_put_prop_index(ctx, arr, idx++);
			item = item->next;
		}
		break;
	}
	case VALUE_MAP: {
		duk_idx_t obj = duk_push_object(ctx);
		for(Value* key = value->child; key!=NULL; key = key->next->next) {
			dk_push_value(ctx, key->next);
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

static duk_ret_t dk_dummy(duk_context *ctx) {
	return 0;
}

#define ERROR_MAXLEN 512
static char s_lastError[ERROR_MAXLEN];

//--- timeouts -----------------------------------------------------

static int timeoutCounter = 0;

typedef struct {
	int timeoutId;
	double when;
	void* next;
} TimeoutCallback;

static TimeoutCallback* timeoutCallbacks = NULL;

static duk_ret_t dk_setTimeout(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	if(argc<2 || !duk_is_function(ctx, 0) || !duk_is_number(ctx, 1)) {
		sprintf(s_lastError, "invalid arguments for setTimeout(function, duration[, ...])\n");
		return 0;
	}

	TimeoutCallback* cb = (TimeoutCallback*)malloc(sizeof(TimeoutCallback));
	cb->timeoutId = ++timeoutCounter;
	cb->when = WindowTimestamp() + duk_to_number(ctx, 1)*0.001;
	cb->next = NULL;

	TimeoutCallback *curr = timeoutCallbacks, *prev = NULL;
	if(curr == NULL)
		timeoutCallbacks = cb;
	else {
		while(curr && cb->when>curr->when) {
			prev = curr;
			curr = curr->next;
		}
		cb->next = curr;
		if(prev)
			prev->next = cb;
		else
			timeoutCallbacks = cb;
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

static void bindConsole(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_consoleLog, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "log");
	duk_push_c_function(ctx, dk_consoleErr, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "error");
	duk_push_c_function(ctx, dk_consoleErr, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "warn");
	duk_push_c_function(ctx, dk_consoleVisible, 1);
	duk_put_prop_string(ctx, -2, "visible");
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

static void readImageResourceParams(
	duk_context *ctx, duk_idx_t idx, float* scale, int* filtering)
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
}

static duk_ret_t dk_getNamedResource(const char* name, duk_context *ctx) {
	int filtering = 1;
	float scale = 1.0;
	readImageResourceParams(ctx, 1, &scale, &filtering);

	size_t handle = ResourceProtectHandle(ResourceGetImage(name, scale, filtering), RESOURCE_IMAGE);
	if(!handle)
		handle = ResourceGetAudio(name);
	if(!handle) {
		if(duk_is_object(ctx, 1)) {
			if(duk_get_prop_string(ctx, 1, "size"))
				scale = duk_to_number(ctx, -1);
			duk_pop(ctx);
		}
		handle = ResourceProtectHandle(ResourceGetFont(name, scale), RESOURCE_FONT);
	}
	if(handle) {
		duk_push_number(ctx, (double)handle);
		return 1;
	}
	char* text = ResourceGetText(name);
	if(text) {
		duk_push_string(ctx, text);
		free(text);
		return 1;
	}
	snprintf(s_lastError, ERROR_MAXLEN, "app.getResource(\"%s\") failed\n", name);
	return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, s_lastError);
}

/**
 * @function app.getResource
 * returns handle to an image/audio/font or text resource or array of handles
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
 * @param {array} [fillColor=[255,255,255,255]] - fill color (RGBA)
 * @param {number} [strokeWidth=0] - stroke width
 * @param {array} [strokeColor=[0,0,0,0]] - stroke color (RGBA)
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createCircleResource(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	float radius = duk_to_number(ctx, 0);
	uint32_t fillColor = argc>1 ? array2color(ctx, 1) : 0xffffffff;
	float strokeWidth = argc>2 ? duk_to_number(ctx, 2) : 0.0f;
	uint32_t strokeColor = argc>3 ? array2color(ctx, 3) : 0xffffffff;
	size_t img = ResourceCreateCircleImage(radius, fillColor, strokeWidth, strokeColor);
	duk_push_uint(ctx, ResourceProtectHandle(img, RESOURCE_IMAGE));
	return 1;
}

/**
 * @function app.createPathResource
 * creates an image resource from an SVG path description
 * @param {number} width - image width
 * @param {number} height - image height
 * @param {string|array} path - path description
 * @param {array} [fillColor=[255,255,255,255]] - fill color (RGBA)
 * @param {number} [strokeWidth=0] - stroke width
 * @param {array} [strokeColor=[0,0,0,0]] - stroke color (RGBA)
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createPathResource(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	if(argc<3)
		return duk_error(ctx, DUK_ERR_ERROR,
			"createPathResource expects at least width, height, and path string parameters");
	int width = duk_to_int(ctx, 0), height = duk_to_int(ctx, 1);
	const char* path = duk_to_string(ctx, 2);
	uint32_t fillColor = argc>3 ? array2color(ctx, 3) : 0xffffffff;
	float strokeWidth = argc>4 ? duk_to_number(ctx, 4) : 0.0f;
	uint32_t strokeColor = argc>5 ? array2color(ctx, 5) : 0xffffffff;
	size_t img = ResourceCreatePathImage(width, height, path, fillColor, strokeWidth, strokeColor);
	duk_push_uint(ctx, ResourceProtectHandle(img, RESOURCE_IMAGE));
	return 1;
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
	readImageResourceParams(ctx, 1, &scale, NULL);
	size_t img = ResourceCreateSVGImage(svg, scale);
	duk_push_uint(ctx, ResourceProtectHandle(img, RESOURCE_IMAGE));
	return 1;
}

/**
 * @function app.createImageResource
 * creates an RGBA image resource from an buffer
 * @param {number} width - image width
 * @param {number} height - image height
 * @param {buffer|array} data - RGBA 4-byte per pixel image data
 * @param {object} [params] - optional additional parameters as key-value pairs such as filtering
 * @returns {number} handle of the created image resource
 */
static duk_ret_t dk_createImageResource(duk_context *ctx) {
	int width = duk_to_int(ctx, 0), height = duk_to_int(ctx, 1);
	size_t img = 0;
	int filtering = 1;
	readImageResourceParams(ctx, 3, NULL, &filtering);

	if(duk_is_buffer_data(ctx, 2)) {
		duk_size_t nBytes;
		unsigned char* data = duk_get_buffer_data(ctx, 2, &nBytes);
		if(nBytes!=width*height*4)
			return duk_error(ctx, DUK_ERR_ERROR,
				"createImageResource buffer size does not fit to width and height");
		img = ResourceCreateImage(width, height, data, filtering);
	}
	else if(duk_is_array(ctx, 2)) {
 		duk_size_t nBytes = duk_get_length(ctx, 2);
		if(nBytes!=width*height*4)
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
	duk_push_uint(ctx, ResourceProtectHandle(img, RESOURCE_IMAGE));
	return 1;
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
		WindowClearColor(array2color(ctx, 0));
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
 * reads a string from a modal window
 *
 * @param {string|array} message - (multi-line) message to be displayed
 * @param {string} [initialValue] - optional prefilled value
 * @param {string} [options] - display options
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
 * displays a modal message window
 *
 * @param {string|array} message - (multi-line) message to be displayed
 * @param {string} [options] - display options
 */
static duk_ret_t dk_appMessage(duk_context *ctx) {
	const char *msg = duk_is_array(ctx, 0) ? dk_join_array(ctx, 0, "\n") : duk_to_string(ctx, 0);
	Value* options = NULL;
	if(duk_is_object(ctx, 1))
		options = readValue(ctx, 1);

	DialogMessageBox(msg, NULL, options);

	if(options)
		Value_delete(options, 1);
	return 0;
}

/**
 * @function app.close
 * closes window and application
 */
static duk_ret_t dk_appClose(duk_context *ctx) {
	WindowClose();
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
} HttpRequest;

#define httpRequestsMax 32
static HttpRequest httpRequests[httpRequestsMax];

static int httpThread(void* udata) {
	HttpParams* params = (HttpParams*)udata;
	char* resp = NULL;
	int status = params->isPost ? httpPost(params->url, params->data, &resp)
		: httpGet(params->url, &resp);

	if(params->callbackId<0)
		free(resp);
	else {
		HttpRequest* req = &httpRequests[params->callbackId];
		req->resp = resp;
		req->status = status;
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
	httpRequests[callbackId].status = 103;
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
 * @param {function} [callback] - function to be called when the response is received
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
 * @param {function} [callback] - function to be called when a response is received
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
 * @param {number} font - font resource handle, use 0 for built-in default 10x16 pixel font
 * @param {string} [text] - optional text for width calculation
 * @returns {object} an object having the properties width, height, fontBoundingBoxAscent, fontBoundingBoxDescent
 */
static duk_ret_t dk_gfxQueryFont(duk_context *ctx) {
	size_t font = duk_to_number(ctx, 0);
	if(font>0)
		font = ResourceValidateHandle(font, RESOURCE_FONT);

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
	size_t img = ResourceValidateHandle(duk_get_uint(ctx, 0), RESOURCE_IMAGE);
	if(!img) {
		snprintf(s_lastError, ERROR_MAXLEN, "invalid image handle %s", duk_to_string(ctx, 0));
		return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, s_lastError);
	}

	int width, height;
	gfxImageDimensions(img, &width, &height);

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

static void bindApp(duk_context *ctx) {
	duk_push_object(ctx);

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
	duk_push_c_function(ctx, dk_createSVGResource, 2);
	duk_put_prop_string(ctx, -2, "createSVGResource");
	duk_push_c_function(ctx, dk_createImageResource, 4);
	duk_put_prop_string(ctx, -2, "createImageResource");
	duk_push_c_function(ctx, dk_gfxQueryImage, 1);
	duk_put_prop_string(ctx, -2, "queryImage");
	duk_push_c_function(ctx, dk_gfxQueryFont, 2);
	duk_put_prop_string(ctx, -2, "queryFont");
	duk_push_c_function(ctx, dk_appHSL, 4);
	duk_put_prop_string(ctx, -2, "hsl");

	duk_push_c_function(ctx, dk_appSetBackground, 3);
	duk_put_prop_string(ctx, -2, "setBackground");
	duk_push_c_function(ctx, dk_appSetTitle, 1);
	duk_put_prop_string(ctx, -2, "setTitle");
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
	duk_push_c_function(ctx, dk_appRequire, 1);
	duk_put_prop_string(ctx, -2, "require");
	duk_push_c_function(ctx, dk_appExports, 2);
	duk_put_prop_string(ctx, -2, "exports");
	duk_push_c_function(ctx, dk_appVibrate, 1);
	duk_put_prop_string(ctx, -2, "vibrate");
	dk_defineReadOnlyProperty(ctx,"width", -1, dk_getWindowWidth);
	dk_defineReadOnlyProperty(ctx,"height", -1, dk_getWindowHeight);
	dk_defineReadOnlyProperty(ctx,"pixelRatio", -1, dk_getWindowPixelRatio);
	dk_defineReadOnlyProperty(ctx,"version", -1, dk_appVersion);
	dk_defineReadOnlyProperty(ctx,"platform", -1, dk_appPlatform);

	duk_put_global_string(ctx, "app");

	duk_push_object(ctx);
	duk_put_global_string(ctx, DUK_HIDDEN_SYMBOL("httpCallbacks"));

	memset(httpRequests, 0, sizeof(HttpRequest)*httpRequestsMax);
}

//--- SDL graphics primitives --------------------------------------
/** @module gfx
 *
 * drawing functions, only available within the  draw event callback function
 *
 * ```javascript
 * app.on('draw', function(gfx) {
 *     gfx.color(255, 0, 0);
 *     gfx.fillRect(50, 50, 200, 100);
 *     //...
 * });
 * ```
 */

/**
 * @function gfx.color
 * sets the current drawing color
 * @param {number|array|buffer} r - RGB red component in range 0..255 or color array/array buffer
 * @param {number} [g] - RGB green component in range 0..255
 * @param {number} [b] - RGB blue component in range 0..255
 * @param {number} [a=255] - opacity between 0 (invisible) and 255 (opaque)
 * @returns {object} - this gfx object
 */
static duk_ret_t dk_gfxColor(duk_context *ctx) {
	if(duk_is_array(ctx, 0))
		gfxColor(array2color(ctx, 0));
	else if(duk_is_undefined(ctx, 1))
		gfxColor(duk_to_uint(ctx, 0));
	else {
		int r = duk_to_int(ctx, 0);
		int g = duk_to_int(ctx, 1);
		int b = duk_to_int(ctx, 2);
		int a = duk_get_int_default(ctx, 3, 255);
		gfxColorRGBA(r,g,b,a);
	}
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.colorf
 * sets the current drawing color using normalized floating point values
 * @param {number} r - RGB red component in range 0.0..1.0
 * @param {number} g - RGB green component in range 0.0..1.0
 * @param {number} b - RGB blue component in range 0.0..1.0
 * @param {number} [a=255] - opacity between 0.0 (invisible) and 1.0 (opaque)
 * @returns {object} - this gfx object
 */
static duk_ret_t dk_gfxColorf(duk_context *ctx) {
	float r = duk_to_number(ctx, 0);
	float g = duk_to_number(ctx, 1);
	float b = duk_to_number(ctx, 2);
	float a = duk_get_number_default(ctx, 3, 1.0);
	gfxColorRGBA(r*255,g*255,b*255,a*255);
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.lineWidth
 * sets current drawing line width in pixels.
 * @param {number} [w] - line width in pixels
 * @returns {object|number} - this gfx object or current line width, if called without width parameter
 */
static duk_ret_t dk_gfxLineWidth(duk_context *ctx) {
	if(duk_is_undefined(ctx, 0)) {
		duk_push_number(ctx, gfxGetLineWidth());
		return 1;
	}
	gfxLineWidth(duk_to_number(ctx, 0));
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.origin
 * sets drawing origin
 * @param {number} ox - horizontal origin
 * @param {number} oy - vertical origin
 * @param {boolean} [isScreen=true] - flag switching between screen and model space
 * @returns {object} - this gfx object
 */
static duk_ret_t dk_gfxOrigin(duk_context *ctx) {
	float scx = duk_to_number(ctx, 0);
	float scy = duk_to_number(ctx, 1);
	int isScreen = duk_get_boolean_default(ctx, 2, 1);
	if(isScreen)
		gfxOriginScreen(scx,scy);
	else
		gfxOrigin(scx,scy);
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.scale
 * sets drawing scale
 * @param {number} sc - scale
 * @returns {object} - this gfx object
 */
static duk_ret_t dk_gfxScale(duk_context *ctx) {
	gfxScale(duk_to_number(ctx, 0));
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.clipRect
 * sets viewport/clipping rectangle (in screen coordinates) or turns clipping off if called without parameters
 * @param {number} [x] - X ordinate
 * @param {number} [y] - Y ordinate
 * @param {number} [w] - width
 * @param {number} [h] - height
 */
static duk_ret_t dk_gfxClipRect(duk_context *ctx) {
	if(duk_is_undefined(ctx, 0)) {
		gfxClipDisable();
		return 0;
	}
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	gfxClipRect(x,y,w,h);
	return 0;
}

/**
 * @function gfx.drawRect
 * draws a rectangular boundary line identified by a left upper coordinate, width, and height.
 * @param {number} x - X ordinate
 * @param {number} y - Y ordinate
 * @param {number} w - width
 * @param {number} h - height
 */
static duk_ret_t dk_gfxDrawRect(duk_context *ctx) {
	float x = duk_to_number(ctx, 0);
	float y = duk_to_number(ctx, 1);
	float w = duk_to_number(ctx, 2);
	float h = duk_to_number(ctx, 3);
	gfxDrawRect(x,y,w,h);
	return 0;
}

/**
 * @function gfx.fillRect
 * fills a rectangular screen area identified by a left upper coordinate, width, and height.
 * @param {number} x - X ordinate
 * @param {number} y - Y ordinate
 * @param {number} w - width
 * @param {number} h - height
 */
static duk_ret_t dk_gfxFillRect(duk_context *ctx) {
	float x = duk_to_number(ctx, 0);
	float y = duk_to_number(ctx, 1);
	float w = duk_to_number(ctx, 2);
	float h = duk_to_number(ctx, 3);
	gfxFillRect(x,y,w,h);
	return 0;
}

/**
 * @function gfx.drawLine
 * draws a line between two coordinates.
 * @param {number} x1 - X ordinate first point
 * @param {number} y1 - Y ordinate first point
 * @param {number} x2 - X ordinate second point
 * @param {number} y2 - Y ordinate second point
 */
static duk_ret_t dk_gfxDrawLine(duk_context *ctx) {
	float x0 = duk_to_number(ctx, 0);
	float y0 = duk_to_number(ctx, 1);
	float x1 = duk_to_number(ctx, 2);
	float y1 = duk_to_number(ctx, 3);
	gfxDrawLine(x0,y0,x1,y1);
	return 0;
}

/**
 * @function gfx.drawLineStrip
 * draws a series of connected lines using the current color and line width.
 * @param {array|Float32Array} arr - array of vertex ordinates
 */
static duk_ret_t dk_gfxDrawLineStrip(duk_context *ctx) {
	float *arr, *buf;
	uint32_t n = readFloatArray(ctx, 0, &arr, &buf);
	gfxDrawLineStrip(n/2, arr);
	free(buf);
	return 0;
}

/**
 * @function gfx.drawPoints
 * draws an array of individual points using the current color and line width.
 * @param {array|Float32Array} arr - array of vertex ordinates
 */
static duk_ret_t dk_gfxDrawPoints(duk_context *ctx) {
	float *arr, *buf;
	uint32_t n = readFloatArray(ctx, 0, &arr, &buf);
	gfxDrawPoints(n/2, arr);
	free(buf);
	return 0;
}

/**
 * @function gfx.drawImage
 *
 * gfx.drawImage(dstX, dstY[, dstW, dstH])(dstX, dstY[, dstW, dstH])
 * gfx.drawImage(srcX,srcY, srcW, srcH, dstX, dstY, dstW, dstH[, cX, cY, angle, flip])
 *
 * draws an image or part of an image at a given target position, optionally scaled
 * @param {number} img - image handle
 * @param {number} dstX - destination X position
 * @param {number} dstY - destination Y position
 * @param {number} [destW=srcW] - destination width
 * @param {number} [dstH=srcH] - destination height
 * @param {number} [srcX=0] - source origin X in pixels
 * @param {number} [srcY=0] - source origin Y in pixels
 * @param {number} [srcW=imgW] - source width in pixels
 * @param {number} [srcH=imgH] - source height in pixels
 * @param {number} [cX=0] - rotation center X offset in pixels
 * @param {number} [cY=0] - rotation center Y offset in pixels
 * @param {number} [angle=0] - rotation angle in radians
 * @param {number} [flip=gfx.FLIP_NONE] - flip image in X (gfx.FLIP_X), Y (gfx.FLIP_Y), or in both (gfx.FLIP_XY) directions
 */
static duk_ret_t dk_gfxDrawImage(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	if(argc<3)
		return 0;

	size_t img = ResourceValidateHandle(duk_get_uint(ctx, 0), RESOURCE_IMAGE);
	if(!img) {
		snprintf(s_lastError, ERROR_MAXLEN, "invalid image handle %s", duk_to_string(ctx, 0));
		return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, s_lastError);
	}

	if(argc<5) {
		float x = duk_to_number(ctx, 1);
		float y = duk_to_number(ctx, 2);
		gfxDrawImage(img, x, y);
	}
	else if(argc<6) {
		float x = duk_to_number(ctx, 1);
		float y = duk_to_number(ctx, 2);
		float w = duk_to_number(ctx, 3);
		float h = duk_to_number(ctx, 4);
		gfxDrawImageScaled(img, x, y, w, h);
	}
	else if(argc>8) {
		int srcX = duk_to_int(ctx, 1);
		int srcY = duk_to_int(ctx, 2);
		int srcW = duk_to_int(ctx, 3);
		int srcH = duk_to_int(ctx, 4);
		float destX = duk_to_number(ctx, 5);
		float destY = duk_to_number(ctx, 6);
		float destW = duk_to_number(ctx, 7);
		float destH = duk_to_number(ctx, 8);
		float cx = argc>9 ? duk_to_number(ctx, 9) : 0.0f;
		float cy = argc>10 ? duk_to_number(ctx, 10) : 0.0f;
		float angle = argc>11 ? duk_to_number(ctx, 11) : 0.0f;
		int flip = argc>12 ? duk_to_int(ctx, 12) : 0;
		gfxDrawImageEx(img, srcX, srcY, srcW, srcH, destX, destY, destW, destH, cx, cy, angle, flip);
	}
	return 0;
}

/**
 * @function gfx.fillText
 * writes text using a specified font.
 * @param {number} font - font resource handle, use 0 for built-in default 12x16 pixel font
 * @param {number} x - X ordinate
 * @param {number} y - Y ordinate
 * @param {string} text - text
 * @param {number} [align=gfx.ALIGN_LEFT|gfx.ALIGN_TOP] - horizontal and vertical alignment, a combination of the gfx.ALIGN_xyz constants
 */
static duk_ret_t dk_gfxFillText(duk_context *ctx) {
	size_t font = duk_get_number_default(ctx, 0, 0);
	if(font>0)
		font = ResourceValidateHandle(font, RESOURCE_FONT);
	float x = duk_to_number(ctx, 1);
	float y = duk_to_number(ctx, 2);
	int align = duk_get_int_default(ctx, 4, 0);
	if(!duk_is_buffer_data(ctx, 3))
		gfxFillTextAlign(font, x, y, duk_to_string(ctx, 3), align);
	else {
		duk_size_t len;
		void* ptr = duk_get_buffer_data(ctx, 3, &len);
		char* s = (char*)malloc(len+1);
		memcpy(s, ptr, len);
		s[len] = 0;
		gfxFillTextAlign(font, x, y, s, align);
		free(s);
	}
	return 0;
}

static void bindGraphicsCommon(duk_context* ctx) {
	const duk_number_list_entry gfx_consts[] = {
/// @constant {number} gfx.ALIGN_LEFT
		{ "ALIGN_LEFT", (double)0 },
/// @constant {number} gfx.ALIGN_CENTER
		{ "ALIGN_CENTER", (double)1 },
/// @constant {number} gfx.ALIGN_RIGHT
		{ "ALIGN_RIGHT", (double)2 },
/// @constant {number} gfx.ALIGN_TOP
		{ "ALIGN_TOP", (double)0 },
/// @constant {number} gfx.ALIGN_MIDDLE
		{ "ALIGN_MIDDLE", (double)4 },
/// @constant {number} gfx.ALIGN_BOTTOM
		{ "ALIGN_BOTTOM", (double)8 },
/// @constant {number} gfx.FLIP_NONE
		{ "FLIP_NONE", 0.0 },
/// @constant {number} gfx.FLIP_X
		{ "FLIP_X", 1.0 },
/// @constant {number} gfx.FLIP_Y
		{ "FLIP_Y", 2.0 },
/// @constant {number} gfx.FLIP_XY
		{ "FLIP_XY", 3.0 },
		{ NULL, 0.0 }
	};
	duk_put_number_list(ctx, -1, gfx_consts);

	sprites_exports(ctx);
}

static void bindGraphics(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_gfxOrigin, 3);
	duk_put_prop_string(ctx, -2, "origin");
	duk_push_c_function(ctx, dk_gfxScale, 1);
	duk_put_prop_string(ctx, -2, "scale");
	duk_push_c_function(ctx, dk_dummy, 0);
	duk_put_prop_string(ctx, -2, "flush");
	duk_push_c_function(ctx, dk_gfxColor, 4);
	duk_put_prop_string(ctx, -2, "color");
	duk_push_c_function(ctx, dk_gfxColorf, 4);
	duk_put_prop_string(ctx, -2, "colorf");
	duk_push_c_function(ctx, dk_gfxLineWidth, 1);
	duk_put_prop_string(ctx, -2, "lineWidth");
	duk_push_c_function(ctx, dk_gfxClipRect, 4);
	duk_put_prop_string(ctx, -2, "clipRect");
	duk_push_c_function(ctx, dk_gfxDrawRect, 4);
	duk_put_prop_string(ctx, -2, "drawRect");
	duk_push_c_function(ctx, dk_gfxFillRect, 4);
	duk_put_prop_string(ctx, -2, "fillRect");
	duk_push_c_function(ctx, dk_gfxDrawLine, 4);
	duk_put_prop_string(ctx, -2, "drawLine");
	duk_push_c_function(ctx, dk_gfxDrawLineStrip, 1);
	duk_put_prop_string(ctx, -2, "drawLineStrip");
	duk_push_c_function(ctx, dk_gfxDrawPoints, 1);
	duk_put_prop_string(ctx, -2, "drawPoints");
	duk_push_c_function(ctx, dk_gfxDrawImage, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "drawImage");
	duk_push_c_function(ctx, dk_gfxFillText, 5);
	duk_put_prop_string(ctx, -2, "fillText");

	bindGraphicsCommon(ctx);
	duk_put_global_string(ctx, DUK_HIDDEN_SYMBOL("gfx"));
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
 * sets or returns master volume, a number between 0.0 and 1.0
 * @param {number} [v] - sets master volume
 * @returns {number} the current master volume if called without parameter
 */
static duk_ret_t dk_audioVolume(duk_context *ctx) {
	if(duk_is_undefined(ctx, 0) || duk_is_null(ctx, 0)) {
		duk_push_number(ctx, AudioGetVolume());
		return 1;
	}
	float volume = duk_to_number(ctx, 0);
	AudioSetVolume(volume);
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
	if(duk_is_undefined(ctx, 0))
		for(unsigned i=0, end = AudioTracks(); i<end && !isPlaying; ++i)
			isPlaying = AudioPlaying(i);
	else
		isPlaying = AudioPlaying(duk_to_number(ctx, 0));
	duk_push_boolean(ctx, isPlaying);
	return 1;
}

/**
 * @function audio.stop
 * immediate stops an individual track or all tracks
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
 * @function audio.sound
 * immediately plays an FM-generated sound
 * @param {string} wave - wave form, either 'sin'(e), 'tri'(angle), 'squ'(are), 'saw'(tooth), or 'noi'(se)
 * @param {number} freq - frequency in Hz
 * @param {number} duration - duration in seconds
 * @param {number} [vol=1.0] - volume/maximum amplitude, value range 0.0..1.0
 * @param {number} [balance=0.0] - stereo balance, value range -1.0 (left)..+1.0 (right)
 * @returns {number} track number playing this sound or UINT_MAX if no track is available
 */
static duk_ret_t dk_audioSound(duk_context *ctx) {
	SoundWave waveForm = WAVE_NONE;
	const char* w = duk_to_string(ctx, 0);
	if(strncmp(w, "sine", 3)==0)
		waveForm = WAVE_SINE;
	else if(strncmp(w, "triangle", 3)==0)
		waveForm = WAVE_TRIANGLE;
	else if(strncmp(w, "square", 3)==0)
		waveForm = WAVE_SQUARE;
	else if(strncmp(w, "sawtooth", 3)==0)
		waveForm = WAVE_SAWTOOTH;
	else if(strncmp(w, "noise", 3)==0)
		waveForm = WAVE_NOISE;
	if(!waveForm)
		return 0;

	int freq = duk_to_int(ctx, 1);
	float duration = duk_to_number(ctx, 2);
	float volume = duk_get_number_default(ctx, 3, 1.0);
	float balance = duk_get_number_default(ctx, 4, 0.0);
	duk_push_number(ctx, AudioSound(waveForm, freq, duration, volume, balance));
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
 * @function audio.sample
 * creates an audio sample from an array of floating point numbers
 * @param {array|Float32Array} data - array of PCM sample values in range -1.0..1.0
 * @returns {number} sample handle to be used in audio.replay
 */
static duk_ret_t dk_audioSample(duk_context *ctx) {
	float *waveData, *buf;
	uint32_t sampleLen = readFloatArray(ctx, 0, &waveData, &buf);
	if(!buf) {
		buf = (float*)malloc(sampleLen*sizeof(float));
		memcpy(buf, waveData, sampleLen*sizeof(float));
	}
	size_t sample = AudioSample(buf, sampleLen, 0);
	duk_push_number(ctx, sample);
	return 1;
}

/// @property {number} audio.sampleRate - audio device sample rate in Hz
static duk_ret_t dk_audioSampleRate(duk_context * ctx) {
	duk_push_uint(ctx,AudioSampleRate());
	return 1;
}

static void audio_exports(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_audioVolume, 1);
	duk_put_prop_string(ctx, -2, "volume");
	duk_push_c_function(ctx, dk_audioPlaying, 1);
	duk_put_prop_string(ctx, -2, "playing");
	duk_push_c_function(ctx, dk_audioStop, 1);
	duk_put_prop_string(ctx, -2, "stop");
	duk_push_c_function(ctx, dk_audioReplay, 4);
	duk_put_prop_string(ctx, -2, "replay");
	duk_push_c_function(ctx, dk_audioSound, 5);
	duk_put_prop_string(ctx, -2, "sound");
	duk_push_c_function(ctx, dk_audioMelody, 3);
	duk_put_prop_string(ctx, -2, "melody");
	duk_push_c_function(ctx, dk_audioSample, 2);
	duk_put_prop_string(ctx, -2, "sample");
	dk_defineReadOnlyProperty(ctx,"sampleRate", -1, dk_audioSampleRate);
}

//--- LocalStorage -------------------------------------------------

enum {
	STORAGE_ERROR = -2,
	STORAGE_UNINITIALIZED = -1,
	STORAGE_UNCHANGED = 0,
	STORAGE_CHANGED = 1,
};

void LocalStorageLoad(duk_context *ctx, const char* fname) {
	FILE *f=fopen(fname, "rb");
	if (!f) {
		fprintf(stderr, "localStorage file \"%s\" not found\n", fname);
		return;
	}
	fseek(f,0,SEEK_END);
	size_t fsize = ftell(f);
	rewind(f);

	char* buffer = (char*)malloc(fsize+1);
	buffer[fsize] = 0;
	if(fread(buffer, 1,fsize, f)!=fsize)
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
	fclose(f);
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
	FILE *f=fopen(fname, "wb");
	duk_pop(ctx);

	if (!f) {
		duk_pop(ctx);
		fprintf(stderr, "localStorage file not writable\n");
		return;
	}
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("data"));
	const char* json = duk_json_encode(ctx, -1);

	size_t len = strlen(json);
	if(fwrite(json, 1, len, f)!=len)
		fprintf(stderr, "localStorage file write error\n");
	fclose(f);

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
	size_t len = strlen(dllName);
	char* dllNameWithSuffix = malloc(len+5);
	strncpy(dllNameWithSuffix, dllName, len);
#if defined __WIN32__ || defined WIN32
	const char* suffix = ".dll";
#else
	const char* suffix = ".so";
#endif
	strcpy(dllNameWithSuffix+len, suffix);

	void* libHandle = SDL_LoadObject(dllNameWithSuffix);
	free(dllNameWithSuffix);
	if(!libHandle)
		return 0;

	const char unloadFuncNameSuffix[] = "_unload";
	char* unloadFuncName = malloc(len+sizeof(unloadFuncNameSuffix)+1);
	strncpy(unloadFuncName, dllName, len);
	strcpy(unloadFuncName+len, unloadFuncNameSuffix);
	void (*unloadFunc)() = (void (*)())SDL_LoadFunction(libHandle, unloadFuncName);
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

size_t jsvmInit(const char* storageFileName) {
	s_lastError[0] = 0;
	duk_context *ctx = duk_create_heap_default();
	if(!ctx)
		return 0;

	bindApp(ctx);
	bindGraphics(ctx);
	bindConsole(ctx);
	bindLocalStorage(ctx, storageFileName);

	duk_push_c_function(ctx, dk_setTimeout, DUK_VARARGS);
	duk_put_global_string(ctx, "setTimeout");
	duk_push_object(ctx);
	duk_put_global_string(ctx, DUK_HIDDEN_SYMBOL("timeoutCallbacks"));

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
	for(const Value* arg=data; arg!=NULL; arg = arg->next, ++nargs)
		dk_push_value(ctx, arg);
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
	Value_set(event, "type", Value_str("axis"));
	Value_set(event, "index", Value_int(id));
	Value_set(event, "axis", Value_int(axis));
	Value_set(event, "value", Value_float(value));
	jsvmDispatchEvent((size_t)vm, "gamepad", event);
	Value_delete(event, 1);
}

static void dispatchButtonEvent(size_t id, uint8_t button, uint8_t value, void* vm) {
	Value* event = Value_new(VALUE_MAP, NULL);
	Value_set(event, "evt", Value_str("gamepad"));
	Value_set(event, "type", Value_str("button"));
	Value_set(event, "index", Value_int(id));
	Value_set(event, "button", Value_int(button));
	Value_set(event, "value", Value_bool(value));
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
			"update", "draw", "resize", "keyboard", "pointer", "gamepad", "enter", "leave", "custom", NULL
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
			duk_push_string(ctx, req->resp);
			duk_push_int(ctx, req->status);
			duk_pcall(ctx, 2);
			duk_pop(ctx); // ignore cb return value
			duk_del_prop_index(ctx, -1, i);
			duk_pop(ctx);
			free(req->resp);
			req->resp = NULL;
			req->status = 0;
		}
	}
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("timeoutCallbacks"));
	for(TimeoutCallback* cb=timeoutCallbacks; cb && cb->when < timestamp; cb=timeoutCallbacks) {
		duk_get_prop_index(ctx, -1, cb->timeoutId); // callback array
		duk_idx_t cbArrIdx = duk_get_top_index(ctx);
		duk_size_t cbArrLen = duk_get_length(ctx, -1);
		for(duk_size_t i=0; i<cbArrLen; ++i)
			duk_get_prop_index(ctx, cbArrIdx, i);
		duk_pcall(ctx, cbArrLen-1);
		duk_pop_2(ctx); // ignore cb return value and pop callback array
		duk_del_prop_index(ctx, -1, cb->timeoutId); // delete callback array

		timeoutCallbacks = cb->next;
		free(cb);
	}
	duk_pop(ctx);
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
	void (*bindFunc)(duk_context *ctx)
		= (void (*)(duk_context *))SDL_LoadFunction(libHandle, exportsFuncName);
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
	if(duk_get_prop_string(ctx, -1, key) && duk_is_array(ctx, -1)) {
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
