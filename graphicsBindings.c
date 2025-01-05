#include "graphics.h"
#include "graphicsUtils.h"

#include "./external/duk_config.h"
#include "./external/duktape.h"
#include <stdint.h>

extern uint32_t readFloatArray(duk_context *ctx, duk_idx_t idx, float** arr, float** buf);
extern uint32_t readUint8Array(duk_context *ctx, duk_idx_t idx, uint8_t** arr, uint8_t** buf);
extern uint32_t readColor(duk_context *ctx, duk_idx_t idx);
extern float getPropFloatDefault(duk_context *ctx, duk_idx_t idx, const char* key, float defaultValue);
extern uint32_t getPropUint32Default(duk_context *ctx, duk_idx_t idx, const char* key, uint32_t defaultValue);

/** @module graphics
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

uint32_t readUint32Array(duk_context *ctx, duk_idx_t idx, uint32_t** arr, uint32_t** buf) {
	duk_size_t n=0;
	*buf = NULL;
	if(duk_is_buffer_data(ctx, idx))
		*arr = duk_get_buffer_data(ctx, idx, &n);
	else if(duk_is_array(ctx, idx)) {
		n = duk_get_length(ctx, idx);
		*arr = *buf = (uint32_t*)malloc(n*sizeof(uint32_t));
		for(duk_size_t i=0; i<n; ++i) {
			duk_get_prop_index(ctx, idx, i);
			(*buf)[i] = duk_to_uint32(ctx, -1);
			duk_pop(ctx);
		}
	}
	return n/sizeof(uint32_t);
}

/**
 * @function gfx.color
 * sets the current drawing color
 * @param {number} r - RGB red component in range 0..255 or unified uint32 RGBA color
 * @param {number} [g] - RGB green component in range 0..255 or opacity if first argument is a unified uint32 RGBA color 
 * @param {number} [b] - RGB blue component in range 0..255
 * @param {number} [a=255] - opacity between 0 (invisible) and 255 (opaque)
 * @returns {object} - this gfx object
 */
static duk_ret_t dk_gfxColor(duk_context *ctx) {
	if(duk_is_array(ctx, 0))
		gfxColor(readColor(ctx, 0));
	else if(duk_is_undefined(ctx, 1))
		gfxColor(duk_is_string(ctx, 0) ? cssColor(duk_get_string(ctx, 0)) : duk_to_uint(ctx, 0));
	else if(duk_is_undefined(ctx, 2)) { // color, alpha
		uint32_t color = ((duk_is_string(ctx, 0) ? cssColor(duk_get_string(ctx, 0)) : duk_to_uint(ctx, 0)) | 0xff);
		uint32_t alpha =  duk_to_uint32(ctx, 1);
		if(alpha>255)
			alpha = 255;
		gfxColor((color&0xFFffFF00)|alpha);
	}
	else {
		uint32_t r = duk_to_uint32(ctx, 0);
		uint32_t g = duk_to_uint32(ctx, 1);
		uint32_t b = duk_to_uint32(ctx, 2);
		uint32_t a = duk_get_int_default(ctx, 3, 255);
        gfxColor((r<<24)+(g<<16)+(b<<8)+a);
	}
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.lineWidth
 * sets or returns current drawing line width in pixels.
 * @param {number} [w] - line width in pixels
 * @returns {object|number} - this gfx object or line width in pixels if called without arguments 
 */
static duk_ret_t dk_gfxLineWidth(duk_context *ctx) {
	if(duk_is_undefined(ctx, 0))
		duk_push_number(ctx, gfxGetLineWidth());
	else {
		gfxLineWidth(duk_to_number(ctx, 0));
		duk_push_this(ctx);
	}
	return 1;
}

/**
 * @function gfx.blend
 * sets current drawing blend mode.
 * @param {number} [mode] - blend mode, one of the gfx.BLEND_xyz constants
 * @returns {object|number} - this gfx object or current blend mode, if called without parameter
 */
static duk_ret_t dk_gfxBlend(duk_context *ctx) {
	if(duk_is_undefined(ctx, 0)) {
		duk_push_number(ctx, gfxGetBlend());
		return 1;
	}
	gfxBlend(duk_to_number(ctx, 0));
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.clipRect
 * sets viewport/clipping rectangle (in screen coordinates) or turns clipping off if called without parameters or with false as sole parameter
 * @param {number|boolean} [x] - X ordinate or false for turning clipping off
 * @param {number} [y] - Y ordinate
 * @param {number} [w] - width
 * @param {number} [h] - height
 */
static duk_ret_t dk_gfxClipRect(duk_context *ctx) {
	if(duk_is_undefined(ctx, 0) || (duk_is_boolean(ctx,0) && !duk_get_boolean(ctx,0))) {
		gfxClipRect(0,0,-1,-1);
		return 0;
	}
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int w = duk_to_int(ctx, 2);
	int h = duk_to_int(ctx, 3);
	gfxClipRect(x,y,w,h);
	return 0;
}

#define getPropFloatLiteral(ctx, idx, key, value) \
	if(duk_get_prop_literal((ctx), (idx), (key))) {\
		value = duk_to_number((ctx), -1); }\
	duk_pop((ctx));

/**
 * @function gfx.transform
 * sets the current transformation
 * @param {number|object} x - horizontal translation or an object having {x:0,y:0,z:0,rotX:0,rotY:0,rotZ:0,sc:1.0} members of type number
 * @param {number} [y] - vertical translation
 * @param {number} [rot=0] - rotation angle in radians
 * @param {number} [sc=1] - scale factor
 * @returns {object} this gfx object for chained calls
 */
static duk_ret_t dk_gfxTransform(duk_context *ctx) { 
	if(duk_is_object(ctx,0)) {
		float x=0, y=0, z=0, rotX=0, rotY=0, rotZ=0, sc=1.0f;
		getPropFloatLiteral(ctx, 0, "x", x);
		getPropFloatLiteral(ctx, 0, "y", y);
		getPropFloatLiteral(ctx, 0, "z", z);
		getPropFloatLiteral(ctx, 0, "rotX", rotX);
		getPropFloatLiteral(ctx, 0, "rotY", rotY);
		if(duk_has_prop_literal(ctx,0,"rot")) {
			getPropFloatLiteral(ctx, 0, "rot", rotZ);
		}
		else {
			getPropFloatLiteral(ctx, 0, "rotZ", rotZ);
		}
		getPropFloatLiteral(ctx, 0, "sc", sc);
		gfxTransf3d(x, y, z, rotX, rotY, rotZ, sc);
	}
	else {
		float x = duk_to_number(ctx, 0);
		float y = duk_to_number(ctx, 1);
		float rot = duk_get_number_default(ctx, 2, 0.0);
		float sc = duk_get_number_default(ctx, 3, 1.0);
		gfxTransform(x, y, rot, sc);
	}
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.save
 * saves current rendering state, can be nested up to 7 times
 * @returns {object} this gfx object for chained calls
 */
static duk_ret_t dk_gfxStateSave(duk_context *ctx) {
	gfxStateSave();
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.restore
 * restores the last stored rendering state
 * @returns {object} this gfx object for chained calls
 */
static duk_ret_t dk_gfxStateRestore(duk_context *ctx) {
	gfxStateRestore();
	duk_push_this(ctx);
	return 1;
}

/**
 * @function gfx.reset
 * restores the initial rendering state, pops also all stored states
 * @returns {object} this gfx object for chained calls
 */
static duk_ret_t dk_gfxStateReset(duk_context *ctx) {
	gfxStateReset();
	duk_push_this(ctx);
	return 1;
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
 * @function gfx.drawLineLoop
 * draws a closed series of connected lines using the current color and line width.
 * @param {array|Float32Array} arr - array of vertex ordinates
 */
static duk_ret_t dk_gfxDrawLineLoop(duk_context *ctx) {
	float *arr, *buf;
	uint32_t n = readFloatArray(ctx, 0, &arr, &buf);
	gfxDrawLineLoop(n/2, arr);
	free(buf);
	return 0;
}

/**
 * @function gfx.drawPoints
 * draws a series points using an optionally defined image as point sprite and the current line width.
 * @param {array|Float32Array} arr - array of vertex ordinates
 * @param {number} [img=gfx.IMG_CIRCLE] - point sprite image handle
 */
static duk_ret_t dk_gfxDrawPoints(duk_context *ctx) {
	float *arr, *buf;
	uint32_t n = readFloatArray(ctx, 0, &arr, &buf);
	uint32_t img = duk_get_uint_default(ctx, 1, GFX_IMG_CIRCLE);
	gfxDrawPoints(n/2, arr, img);
	free(buf);
	return 0;
}

/**
 * @function gfx.fillTriangle
 * draws a single filled triangle using the current color.
 * @param {number} x1 - X ordinate first point
 * @param {number} y1 - Y ordinate first point
 * @param {number} x2 - X ordinate second point
 * @param {number} y2 - Y ordinate second point
 * @param {number} x3 - X ordinate third point
 * @param {number} y3 - Y ordinate third point
 */
static duk_ret_t dk_gfxFillTriangle(duk_context *ctx) {
	float x0 = duk_to_number(ctx, 0);
	float y0 = duk_to_number(ctx, 1);
	float x1 = duk_to_number(ctx, 2);
	float y1 = duk_to_number(ctx, 3);
	float x2 = duk_to_number(ctx, 4);
	float y2 = duk_to_number(ctx, 5);
	gfxFillTriangle(x0,y0, x1,y1, x2,y2);
	return 0;
}

/**
 * @function gfx.fillTriangles
 * draws filled triangles.
 * @param {array|Float32Array} arr - array of vertex ordinates
 * @param {array|Uint32Array} [colors] - optional array of vertex colors
 */
static duk_ret_t dk_gfxFillTriangles(duk_context *ctx) {
	float *arr, *buf;
	uint32_t n = readFloatArray(ctx, 0, &arr, &buf);
	uint32_t *colors = NULL, *colorBuf = NULL;
	if(!duk_is_undefined(ctx, 1)) {
		uint32_t nColors = readUint32Array(ctx, 1, &colors, &colorBuf);
		if(nColors != n/2)
			return duk_error(ctx, DUK_ERR_ERROR, "triangle colors array size does not fit coordinate size");
	}
	gfxFillTriangles(n/2, arr, colors, 0, NULL);
	free(buf);
	free(colorBuf);
	return 0;
}

/**
 * @function gfx.drawImage
 *
 * gfx.drawImage(img[,x, y, angle, scale, flip])
 *
 * draws an image at a given target position, optionally scaled and flipped
 * @param {number} img - image handle
 * @param {number} [x=0] - destination X position
 * @param {number} [y=0] - destination Y position
 * @param {number} [angle=0] - rotation angle in radians
 * @param {number} [scale=1] - scale factor
 * @param {number} [flip=gfx.FLIP_NONE] - flip image in X (gfx.FLIP_X), Y (gfx.FLIP_Y), or in both (gfx.FLIP_XY) directions
 */
static duk_ret_t dk_gfxDrawImage(duk_context *ctx) {
	uint32_t img = duk_get_uint(ctx, 0);
	if(!img)
		return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "invalid image handle %s", duk_to_string(ctx, 0));

	float x = duk_get_number_default(ctx, 1, 0.0);
	float y = duk_get_number_default(ctx, 2, 0.0);
	float rot = duk_get_number_default(ctx, 3, 0.0);
	float scale = duk_get_number_default(ctx, 4, 1.0);
	int flip = duk_get_int_default(ctx, 5, 0);
	gfxDrawImage(img, x, y, rot, scale, flip);
	return 0;
}

/**
 * @function gfx.stretchImage
 *
 * gfx.stretchImage(img,x1,y1,w,h)
 *
 * draws a stretched image controlled by a corner and width and height
 * @param {number} img - image handle
 * @param {number} x1 - X ordinate upper left corner
 * @param {number} y1 - Y ordinate upper left corner
 * @param {number} w - width
 * @param {number} h - height
 */
static duk_ret_t dk_gfxStretchImage(duk_context *ctx) {
	uint32_t img = duk_get_uint(ctx, 0);
	if(!img)
		return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "invalid image handle %s", duk_to_string(ctx, 0));

	float x1 = duk_to_number(ctx, 1), y1 = duk_to_number(ctx, 2);
	float w = duk_to_number(ctx, 3), h = duk_to_number(ctx, 4);
	gfxStretchImage(img, x1, y1, w, h);
	return 0;
}

/**
 * @function gfx.fillText
 * writes text using a specified font.
 * @param {number} x - X ordinate
 * @param {number} y - Y ordinate
 * @param {string} text - text
 * @param {number} [font=0] - font handle
 * @param {number} [align=gfx.ALIGN_LEFT_TOP] - horizontal and vertical alignment, one of the gfx.ALIGN_xyz constants
 */
static duk_ret_t dk_gfxFillText(duk_context *ctx) {
	float x = duk_to_number(ctx, 0);
	float y = duk_to_number(ctx, 1);
	const char* text = duk_to_string(ctx, 2);
	uint32_t font = duk_is_undefined(ctx,3) ? 0 : duk_to_uint32(ctx,3);
	int align = duk_get_int_default(ctx, 4, 0);
	if(align)
		gfxFillTextAlign(font, x,y, text, align);
	else
		gfxFillText(font, x,y, text);
	return 0;
}

/**
 * @function gfx.drawTiles
 *
 * draws a rectangular grid of multiple tiled images
 * @param {number} imgBase - base image handle
 * @param {number} tilesX - number of tiles in horizontal direction
 * @param {number} tilesY - number of tiles in vertical direction
 * @param {Uint32Array} imgOffsets - array of image handle offsets
 * @param {Uint32Array} [colors] - optional tile color array
 * @param {number} [stride=tilesX] - number of array elements to proceed to next row
 */
static duk_ret_t dk_gfxDrawTiles(duk_context *ctx) {
	uint32_t imgBase = duk_get_uint(ctx, 0);
	uint16_t tilesX = duk_to_uint16(ctx, 1);
	uint16_t tilesY = duk_to_uint16(ctx, 2);
	uint32_t *imgOffsets, *imgOffsetsBuf = NULL;
	uint32_t *colors = NULL, *colorBuf = NULL;
	/*size_t arrSz =*/ readUint32Array(ctx, 3, &imgOffsets, &imgOffsetsBuf);
	if(!duk_is_undefined(ctx, 4))
		readUint32Array(ctx, 4, &colors, &colorBuf);
	uint32_t stride = duk_get_uint_default(ctx, 5, tilesX);
	// TODO validate / adjust tilesX and tilesY to stay within arrSz and adapt imgOffsets and colors pointers respectively
	gfxDrawTiles(tilesX, tilesY, stride, imgBase, imgOffsets, colors);
	free(colorBuf);
	free(imgOffsetsBuf);
	return 0;
}

/**
 * @function gfx.drawImages
 *
 * draws multiple images based on array data
 * @param {number} imgBase - base image handle
 * @param {number} stride - number of array elements to proceed to next image data record
 * @param {number} components - components contained in array, a bitwise combination of gfx.COMP_xyz constants
 * @param {Float32Array} arr - array containing at least the positions of the images to be drawn and optionally further components
 */
static duk_ret_t dk_gfxDrawImages(duk_context *ctx) {
	uint32_t imgBase = duk_get_uint(ctx, 0);
	uint32_t stride = duk_get_uint(ctx, 1);
	int components = duk_get_int_default(ctx, 2, 0);

	float *arr = NULL;
	uint32_t arrLen = 0;
	if(duk_is_buffer_data(ctx, 3)) {
		duk_size_t nBytes;
		arr = duk_get_buffer_data(ctx, 3, &nBytes);
		if(nBytes%sizeof(float) == 0)
			arrLen = nBytes / sizeof(float);
	}
	if(!arrLen)
		return duk_error(ctx, DUK_ERR_ERROR, "app.drawImages() expects Float32Array as 4th argument");
	if(arrLen%stride!=0)
		return duk_error(ctx, DUK_ERR_ERROR, "app.drawImages() array size must be multiple of stride");

	gfxDrawImages(imgBase, arrLen/stride, stride, components, arr);
	return 0;
}

/**
 * @function gfx.drawSprite
 *
 * draws a sprite object based on various object attributes
 * 
 * @param {object} s - sprite object. The following attributes are interpreted:
 * - {number} image - image handle
 * - {number} x,y,[rot=0],[sc=1] - position
 * - {number} [color=0xFFffFFff] - color and opacity
 * - {number} [flip=0] - horizontal (1) and vertical (2) flip flags
 */
static duk_ret_t dk_gfxDrawSprite(duk_context *ctx) {
	if(!duk_is_object(ctx, 0))
		return duk_error(ctx, DUK_ERR_ERROR, "app.drawSprite() expects object as first argument");
	uint32_t img =  getPropUint32Default(ctx,0,"image",0);
	if(!img)
		return 0;
	float x = getPropFloatDefault(ctx,0,"x",0.0f);
	float y = getPropFloatDefault(ctx,0,"y",0.0f);
	float rot = getPropFloatDefault(ctx,0,"rot",0.0f);
	float sc = getPropFloatDefault(ctx,0,"sc",1.0f);
	uint32_t color =  getPropUint32Default(ctx,0,"color",0xFFffFFff);
	uint32_t flip =  getPropUint32Default(ctx,0,"flip",0);

	//printf("img:%u x:%.1f y:%.1f rot:%.1f sc:%.1f color:%#08x flip:%u\n", img,x,y,rot,sc,color,flip);
	gfxColor(color);
	gfxDrawImage(img,x,y,rot,sc,flip);
	return 0;
}

void bindGraphics(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_gfxColor, 4);
	duk_put_prop_string(ctx, -2, "color");
	duk_push_c_function(ctx, dk_gfxLineWidth, 1);
	duk_put_prop_string(ctx, -2, "lineWidth");
	duk_push_c_function(ctx, dk_gfxBlend, 1);
	duk_put_prop_string(ctx, -2, "blend");
	duk_push_c_function(ctx, dk_gfxClipRect, 4);
	duk_put_prop_string(ctx, -2, "clipRect");
	duk_push_c_function(ctx, dk_gfxTransform, 4);
	duk_put_prop_string(ctx, -2, "transform");
	duk_push_c_function(ctx, dk_gfxStateSave, 0);
	duk_put_prop_string(ctx, -2, "save");
	duk_push_c_function(ctx, dk_gfxStateRestore, 0);
	duk_put_prop_string(ctx, -2, "restore");
	duk_push_c_function(ctx, dk_gfxStateReset, 0);
	duk_put_prop_string(ctx, -2, "reset");

	duk_push_c_function(ctx, dk_gfxDrawRect, 4);
	duk_put_prop_string(ctx, -2, "drawRect");
	duk_push_c_function(ctx, dk_gfxFillRect, 4);
	duk_put_prop_string(ctx, -2, "fillRect");
	duk_push_c_function(ctx, dk_gfxFillTriangle, 6);
	duk_put_prop_string(ctx, -2, "fillTriangle");
	duk_push_c_function(ctx, dk_gfxFillTriangles, 2);
	duk_put_prop_string(ctx, -2, "fillTriangles");
	duk_push_c_function(ctx, dk_gfxDrawLine, 4);
	duk_put_prop_string(ctx, -2, "drawLine");
	duk_push_c_function(ctx, dk_gfxDrawLineStrip, 1);
	duk_put_prop_string(ctx, -2, "drawLineStrip");
	duk_push_c_function(ctx, dk_gfxDrawLineLoop, 1);
	duk_put_prop_string(ctx, -2, "drawLineLoop");
	duk_push_c_function(ctx, dk_gfxDrawPoints, 2);
	duk_put_prop_string(ctx, -2, "drawPoints");
	duk_push_c_function(ctx, dk_gfxDrawImage, 6);
	duk_put_prop_string(ctx, -2, "drawImage");
	duk_push_c_function(ctx, dk_gfxStretchImage, 5);
	duk_put_prop_string(ctx, -2, "stretchImage");
	duk_push_c_function(ctx, dk_gfxFillText, 5);
	duk_put_prop_string(ctx, -2, "fillText");

	duk_push_c_function(ctx, dk_gfxDrawTiles, 6);
	duk_put_prop_string(ctx, -2, "drawTiles");
	duk_push_c_function(ctx, dk_gfxDrawImages, 4);
	duk_put_prop_string(ctx, -2, "drawImages");
	duk_push_c_function(ctx, dk_gfxDrawSprite, 1);
	duk_put_prop_string(ctx, -2, "drawSprite");

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
/// @constant {number} gfx.ALIGN_LEFT_TOP
		{ "ALIGN_LEFT_TOP", (double)0 },
/// @constant {number} gfx.ALIGN_CENTER_TOP
		{ "ALIGN_CENTER_TOP", (double)1 },
/// @constant {number} gfx.ALIGN_RIGHT_TOP
		{ "ALIGN_RIGHT_TOP", (double)2 },
/// @constant {number} gfx.ALIGN_LEFT_MIDDLE
		{ "ALIGN_LEFT_MIDDLE", (double)4 },
/// @constant {number} gfx.ALIGN_CENTER_MIDDLE
		{ "ALIGN_CENTER_MIDDLE", (double)5 },
/// @constant {number} gfx.ALIGN_RIGHT_MIDDLE
		{ "ALIGN_RIGHT_MIDDLE", (double)6 },
/// @constant {number} gfx.ALIGN_LEFT_BOTTOM
		{ "ALIGN_LEFT_BOTTOM", (double)8 },
/// @constant {number} gfx.ALIGN_CENTER_BOTTOM
		{ "ALIGN_CENTER_BOTTOM", (double)9 },
/// @constant {number} gfx.ALIGN_RIGHT_BOTTOM
		{ "ALIGN_RIGHT_BOTTOM", (double)10 },

/// @constant {number} gfx.FLIP_NONE
		{ "FLIP_NONE", 0.0 },
/// @constant {number} gfx.FLIP_X
		{ "FLIP_X", 1.0 },
/// @constant {number} gfx.FLIP_Y
		{ "FLIP_Y", 2.0 },
/// @constant {number} gfx.FLIP_XY
		{ "FLIP_XY", 3.0 },

/// @constant {number} gfx.BLEND_NONE
		{ "BLEND_NONE", 0.0 },
/// @constant {number} gfx.BLEND_ALPHA
		{ "BLEND_ALPHA", 1.0 },
/// @constant {number} gfx.BLEND_ADD
		{ "BLEND_ADD", 2.0 },
/// @constant {number} gfx.BLEND_MOD
		{ "BLEND_MOD", 4.0 },
/// @constant {number} gfx.BLEND_MUL
		{ "BLEND_MUL", 8.0 },

/// @constant {number} gfx.IMG_CIRCLE
		{ "IMG_CIRCLE", GFX_IMG_CIRCLE },
/// @constant {number} gfx.IMG_SQUARE
		{ "IMG_SQUARE", GFX_IMG_SQUARE },

/// @constant {number} gfx.COMP_IMG_OFFSET
		{ "COMP_IMG_OFFSET", GFX_COMP_IMG_OFFSET },
/// @constant {number} gfx.COMP_ROT
		{ "COMP_ROT", GFX_COMP_ROT },
/// @constant {number} gfx.COMP_SCALE
		{ "COMP_SCALE", GFX_COMP_SCALE },
/// @constant {number} gfx.COMP_COLOR_R
		{ "COMP_COLOR_R", GFX_COMP_COLOR_R },
/// @constant {number} gfx.COMP_COLOR_G
		{ "COMP_COLOR_G", GFX_COMP_COLOR_G },
/// @constant {number} gfx.COMP_COLOR_B
		{ "COMP_COLOR_B", GFX_COMP_COLOR_B },
/// @constant {number} gfx.COMP_COLOR_A
		{ "COMP_COLOR_A", GFX_COMP_COLOR_A },
/// @constant {number} gfx.COMP_COLOR_RGB
		{ "COMP_COLOR_RGB", GFX_COMP_COLOR_R|GFX_COMP_COLOR_G|GFX_COMP_COLOR_B },
/// @constant {number} gfx.COMP_COLOR_RGBA
		{ "COMP_COLOR_RGBA", GFX_COMP_COLOR_RGBA },
		{ NULL, 0.0 }
	};
	duk_put_number_list(ctx, -1, gfx_consts);
	duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("gfx"));
}
