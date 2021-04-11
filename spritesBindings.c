#include "sprites.h"
#include "resources.h"

#include "external/duk_config.h"
#include "external/duktape.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

extern uint32_t array2color(duk_context *ctx, duk_idx_t idx);

#define ERROR_MAXLEN 512
static char s_lastError[ERROR_MAXLEN];

static void* getThisInstance(duk_context *ctx) {
	duk_push_this(ctx);
	duk_get_prop_literal(ctx, -1, DUK_HIDDEN_SYMBOL("instance"));
	return duk_get_buffer(ctx, -1, NULL);
}

static duk_bool_t isPrototype(duk_context* ctx, int index, const char* name) {
	duk_get_prototype(ctx, index);
	duk_get_global_string(ctx, name);
	duk_bool_t isProto = duk_equals(ctx, -1,-2);
	duk_pop_2(ctx);
	return isProto;
}

/// @module Sprite

/**
 * @function Sprite.setColor
 * sets the sprite's color possibly blending with its pixel colors
 * @param {number|array|buffer} r - RGB red component in range 0..255 or combined color array/array buffer
 * @param {number} [g] - RGB green component in range 0..255
 * @param {number} [b] - RGB blue component in range 0..255
 * @param {number} [a=255] - opacity between 0 (invisible) and 255 (opaque)
 */
static duk_ret_t dk_SpriteSetColor(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	if(duk_is_array(ctx,0) || duk_is_buffer_data(ctx,0)) {
		uint32_t color = array2color(ctx, 0);
		uint8_t r = color >> 24, g = color >> 16, b = color >> 8, a = color;
		SpriteColorRGBA(sprite, r,g,b,a);
	}
	else if(duk_is_undefined(ctx, 1)) {
		uint32_t v = duk_to_uint(ctx, 0);
		SpriteColorRGBA(sprite, (v>>24)&0xff, (v>>16)&0xff, (v>>8)&0xff ,v&0xff);
	}
	else {
		int r = duk_to_int(ctx, 0);
		int g = duk_to_int(ctx, 1);
		int b = duk_to_int(ctx, 2);
		int a = duk_get_int_default(ctx, 3, 255);
		SpriteColorRGBA(sprite, r,g,b,a);
	}
	return 0;
}

/**
 * @function Sprite.getColor
 * returns the sprite's RGBA color
 * @returns {array} color [r,g,b,a], components in range 0..255
 */
static duk_ret_t dk_SpriteGetColor(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_array(ctx);
	duk_push_int(ctx, sprite->r);
	duk_put_prop_index(ctx, -2, 0);
	duk_push_int(ctx, sprite->g);
	duk_put_prop_index(ctx, -2, 1);
	duk_push_int(ctx, sprite->b);
	duk_put_prop_index(ctx, -2, 2);
	duk_push_int(ctx, sprite->a);
	duk_put_prop_index(ctx, -2, 3);
	return 1;
}

/**
 * @function Sprite.setAlpha
 * sets the sprite's opacity
 * @param {number} opacity - opacity between 0.0 (invisible) and 1.0 (opaque)
 */
static duk_ret_t dk_SpriteSetAlpha(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->a = duk_to_number(ctx, 0)*255.0;
	return 0;
}

/**
 * @function Sprite.getX
 * returns the sprite's X ordinate
 * @returns {number} X ordinate
 */
static duk_ret_t dk_SpriteGetX(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->x);
	return 1;
}

/**
 * @function Sprite.getY
 * returns the sprite's Y ordinate
 * @returns {number} Y ordinate
 */
static duk_ret_t dk_SpriteGetY(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->y);
	return 1;
}

/**
 * @function Sprite.setX
 * sets the sprite's horizontal position
 * @param {number} value - X ordinate
 */
static duk_ret_t dk_SpriteSetX(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->x = duk_to_number(ctx, 0);
	return 0;
}

/**
 * @function Sprite.setY
 * sets the sprite's vertical position
 * @param {number} value - y ordinate
 */
static duk_ret_t dk_SpriteSetY(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->y = duk_to_number(ctx, 0);
	return 0;
}

/**
 * @function Sprite.setPos
 * sets the sprite's position
 * @param {number} x - X ordinate
 * @param {number} y - Y ordinate
 * @param {number} [rot] - rotation in radians
 */
static duk_ret_t dk_SpriteSetPos(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->x = duk_to_number(ctx, 0);
	sprite->y = duk_to_number(ctx, 1);
	if(!duk_is_undefined(ctx, 2))
		sprite->rot = duk_to_number(ctx, 2);
	return 0;
}

/**
 * @function Sprite.setScale
 * sets the sprite's output dimensions relative to its source dimensions. Use
 * negative scales for horizontal/vertical mirroring/flipping.
 * @param {number} scaleX - horizontal scale
 * @param {number} [scaleY=scaleX] - vertical scale
 */
static duk_ret_t dk_SpriteSetScale(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	float scaleX = duk_to_number(ctx, 0), scaleY = scaleX;
	if(scaleX>=0.0f)
		sprite->flip &= ~1;
	else {
		sprite->flip |= 1;
		scaleX *= -1;
	}
	if(!duk_is_undefined(ctx, 1))
		scaleY = duk_to_number(ctx, 1);
	if(scaleY>=0.0f)
		sprite->flip &= ~2;
	else {
		sprite->flip |= 2;
		scaleY *= -1;
	}

	sprite->w = scaleX * sprite->srcW;
	sprite->h = scaleY * sprite->srcH;
	return 0;
}

/**
 * @function Sprite.getScaleX
 * returns the sprite's horizontal scale relative to its source width
 * @returns {number} - horizontal size
 */
static duk_ret_t dk_SpriteGetScaleX(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	float sc = sprite->w / sprite->srcW;
	if(sprite->flip & 1)
		sc *= -1.0f;
	duk_push_number(ctx, sc);
	return 1;
}

/**
 * @function Sprite.getScaleY
 * returns the sprite's vertical scale relative to its source height
 * @returns {number} - vertical size
 */
static duk_ret_t dk_SpriteGetScaleY(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	float sc = sprite->h / sprite->srcH;
	if(sprite->flip & 2)
		sc *= -1.0f;
	duk_push_number(ctx, sc);
	return 1;
}

/**
 * @function Sprite.setDim
 * sets the sprite's absolute output dimensions
 * @param {number} w - width in pixels
 * @param {number} h - height in pixels
 */
static duk_ret_t dk_SpriteSetDim(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->w = duk_to_number(ctx, 0);
	sprite->h = duk_to_number(ctx, 1);
	return 0;
}

/**
 * @function Sprite.getDimX
 * returns the sprite's output width
 * @returns {number} output width
 */
static duk_ret_t dk_SpriteGetDimX(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->w);
	return 1;
}

/**
 * @function Sprite.getDimY
 * returns the sprite's output height
 * @returns {number} output height
 */
static duk_ret_t dk_SpriteGetDimY(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->h);
	return 1;
}

/**
 * @function Sprite.setVel
 * sets the sprite's velocity
 * @param {number} velX - horizontal velocity in pixels per second
 * @param {number} velY - vertical velocity in pixels per second
 * @param {number} [velRot] - rotation velocity in radians per second
 */
static duk_ret_t dk_SpriteSetVel(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->velX = duk_to_number(ctx, 0);
	sprite->velY = duk_to_number(ctx, 1);
	if(!duk_is_undefined(ctx, 2))
		sprite->velRot = duk_to_number(ctx, 2);
	return 0;
}

/**
 * @function Sprite.setVelX
 * sets the sprite's horizontal velocity
 * @param {number} velX - horizontal velocity in pixels per second
 */
static duk_ret_t dk_SpriteSetVelX(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->velX = duk_to_number(ctx, 0);
	return 0;
}

/**
 * @function Sprite.setVelY
 * sets the sprite's vertical velocity
 * @param {number} velY - vertical velocity in pixels per second
 */
static duk_ret_t dk_SpriteSetVelY(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->velY = duk_to_number(ctx, 0);
	return 0;
}

/**
 * @function Sprite.getVelX
 * returns the sprite's horizontal velocity
 * @returns {number} - velocity in pixels per second
 */
static duk_ret_t dk_SpriteGetVelX(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->velX);
	return 1;
}

/**
 * @function Sprite.getVelY
 * returns the sprite's vertical velocity
 * @returns {number} - velocity in pixels per second
 */
static duk_ret_t dk_SpriteGetVelY(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->velY);
	return 1;
}

/**
 * @function Sprite.getVelRot
 * returns the sprite's rotation velocity
 * @returns {number} - rotation velocity in radians per second
 */
static duk_ret_t dk_SpriteGetVelRot(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->velRot);
	return 1;
}

/**
 * @function Sprite.setVelRot
 * sets the sprite's rotation velocity
 * @param {number} rot - rotation velocity in radians per second
 */
static duk_ret_t dk_SpriteSetVelRot(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->velRot = duk_to_number(ctx, 0);
	return 0;
}

/**
 * @function Sprite.setCenter
 * sets the sprite's center coordinates
 * @param {number} cx - center X position, normalized from 0.0..1.0
 * @param {number} cy - center Y position, normalized from 0.0..1.0
 */
static duk_ret_t dk_SpriteSetCenter(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->cx = duk_to_number(ctx, 0);
	sprite->cy = duk_to_number(ctx, 1);
	return 0;
}

/**
 * @function Sprite.getCenterX
 * returns the sprite's center X offset from left border
 * @returns {number} X offset
 */
static duk_ret_t dk_SpriteGetCenterX(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->cx);
	return 1;
}

/**
 * @function Sprite.getCenterY
 * returns the sprite's center Y offset from top border
 * @returns {number} Y offset
 */
static duk_ret_t dk_SpriteGetCenterY(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->cy);
	return 1;
}

/**
 * @function Sprite.getRot
 * returns the sprite's rotation
 * @returns {number} rotation in radians
 */
static duk_ret_t dk_SpriteGetRot(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->rot);
	return 1;
}

/**
 * @function Sprite.setRot
 * sets the sprite's rotation
 * @param {number} rot - rotation in radians
 */
static duk_ret_t dk_SpriteSetRot(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->rot = duk_to_number(ctx, 0);
	return 0;
}

/**
 * @function Sprite.setSource
 * sets the sprite's source dimensions
 * @param {number} x - source x origin in pixels
 * @param {number} y - source y origin in pixels
 * @param {number} w - source width in pixels
 * @param {number} h - source height in pixels
 */
static duk_ret_t dk_SpriteSetSource(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->srcX = duk_to_uint16(ctx, 0);
	sprite->srcY = duk_to_uint16(ctx, 1);
	sprite->srcW = duk_to_uint16(ctx, 2);
	sprite->srcH = duk_to_uint16(ctx, 3);
	return 0;
}

/**
 * @function Sprite.setTile
 * sets the sprite source tile number, given that the sprite's SpriteSet is tiled
 * @param {number} tile - tile number
 */
static duk_ret_t dk_SpriteSetTile(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	SpriteTile(sprite, duk_to_uint16(ctx, 0));
	return 0;
}

/**
 * @function Sprite.getRadius
 * returns the sprite's collision radius
 * @returns {number} - collision radius, -1.0 if disabled
 */
static duk_ret_t dk_SpriteGetRadius(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	duk_push_number(ctx, sprite->radius);
	return 1;
}

/**
 * @function Sprite.setRadius
 * sets the sprite's collision radius
 * @param {number} r - collision radius, use -1.0 to disable
 */
static duk_ret_t dk_SpriteSetRadius(duk_context *ctx) {
	Sprite* sprite = getThisInstance(ctx);
	sprite->radius = duk_to_number(ctx, 0);
	return 0;
}

/**
 * @function Sprite.intersects
 * tests on intersection/collision with another sprite or a point
 * 
 * The test is either based on radius if set (faster), or on possibly rotated bounding rectangle
 * @param {Sprite|number} arg1 - other sprite to be tested on collision or point X ordinate
 * @param {number} [arg2] - point Y ordinate
 * @returns {boolean} - true in case of intersection, otherwise false
 */
static duk_ret_t dk_SpriteIntersects(duk_context *ctx) {
	Sprite* s1 = getThisInstance(ctx);
	duk_int_t arg0type = duk_get_type(ctx, 0);
	if(arg0type == DUK_TYPE_NUMBER) {
		float x = duk_get_number(ctx, 0), y = duk_get_number(ctx, 1);
		duk_push_boolean(ctx, SpriteIntersectsPoint(s1, x, y));
		return 1;
	}
	if(arg0type!=DUK_TYPE_OBJECT || !isPrototype(ctx, 0, DUK_HIDDEN_SYMBOL("Sprite_prototype")))
		return duk_error(ctx, DUK_ERR_SYNTAX_ERROR,
			"Sprite, or number expected as first argument of Sprite.intersects");

	duk_get_prop_literal(ctx, 0, DUK_HIDDEN_SYMBOL("instance"));
	Sprite* s2 = (Sprite*)duk_get_buffer(ctx, -1, NULL);
	int intersects = SpriteIntersectsSprite(s1, s2);
	duk_push_boolean(ctx, intersects);
	return 1;
}

duk_ret_t dk_SpriteFinalizer(duk_context *ctx) {
	duk_get_prop_literal(ctx, 0, DUK_HIDDEN_SYMBOL("instance"));
	Sprite* sprite = (Sprite*)duk_get_buffer(ctx, -1, NULL);
	if(sprite) // NULL if prototype gets finalized itself
		SpriteUnlink(sprite);
	return 0;
}

//--- SpriteSet ----------------------------------------------------

/**
 * @function SpriteSet.createSprite
 * creates a new Sprite instance and appends it to this SpriteSet
 * @param {number} [tile=0|srcX=0] - tile number for tiled source or source x origin
 * @param {number} [srcY=0] - source y origin
 * @param {number} [srcW] - source width, default is parent SpriteSet texture width
 * @param {number} [srcH] - source height, default is parent SpriteSet texture height
 * @returns {object} - created sprite instance
 */
duk_ret_t dk_createSprite(duk_context *ctx) {
	int argc = duk_get_top(ctx);
	SpriteSet* sps = getThisInstance(ctx);

	duk_push_object(ctx);
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("Sprite_prototype"));
	duk_set_prototype(ctx, -2);

	Sprite* sprite = (Sprite*)duk_push_fixed_buffer(ctx, sizeof(Sprite));
	switch(argc) {
	case 4:
		*sprite = SpriteCreateFromClippedImage(
			duk_to_uint16(ctx, 0), duk_to_uint16(ctx, 1), duk_to_uint16(ctx, 2), duk_to_uint16(ctx, 3));
		break;
	case 1: {
		uint16_t tile = duk_to_uint16(ctx, 0);
		if(tile >= sps->tilesX * sps->tilesY)
			return duk_error(ctx, DUK_ERR_RANGE_ERROR, "tile index out of range");
		*sprite = SpriteCreateFromTile(sps, tile);
		break;
	}
	default:
		*sprite = SpriteCreate(sps);
	}
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("instance"));
	SpriteSetAppend(sps, sprite);
	return 1;
}

/**
 * @function SpriteSet.update
 * updates position of contained sprites based on their velocity and passed time 
 * @param {number} deltaT - time since last update in seconds
 */
duk_ret_t dk_SpriteSetUpdate(duk_context *ctx) {
	SpriteSet* sps = getThisInstance(ctx);
	SpriteSetUpdate(sps, duk_to_number(ctx, 0));
	return 0;
}

/**
 * @function SpriteSet.removeSprite
 * removes a sprite from this sprite set 
 * @param {Sprite} sprite - sprite instance to be removed
 */
duk_ret_t dk_removeSprite(duk_context *ctx) {
	//SpriteSet* sps = getThisInstance(ctx);
	if(!duk_is_object(ctx, 0) || !isPrototype(ctx, 0, DUK_HIDDEN_SYMBOL("Sprite_prototype")))
		return duk_error(ctx, DUK_ERR_SYNTAX_ERROR,
			"Sprite expected as first argument of SpriteSet.removeSprite");

	duk_get_prop_literal(ctx, 0, DUK_HIDDEN_SYMBOL("instance"));
	Sprite* sprite = (Sprite*)duk_get_buffer(ctx, -1, NULL);
	SpriteUnlink(sprite);
	return 0;
}

/**
 * @function gfx.drawSprites
 * draws the contained sprites in sequence of their insertion 
 * 
 *  @param {object} spriteset - SpriteSet instance
 */
duk_ret_t dk_SpriteSetDraw(duk_context *ctx) {
	if(!duk_is_object(ctx, 0) || !isPrototype(ctx, 0, DUK_HIDDEN_SYMBOL("SpriteSet_prototype")))
		return duk_error(ctx, DUK_ERR_SYNTAX_ERROR,
			"SpriteSet expected as first argument of drawSprites");

	duk_get_prop_literal(ctx, 0, DUK_HIDDEN_SYMBOL("instance"));
	SpriteSet* sps = (SpriteSet*)duk_get_buffer(ctx, -1, NULL);
	SpriteSetDraw(sps);
	return 0;
}

/**
 * @function gfx.drawTile
 * draws a tile of a tiled sprite set
 *
 * @param {object} spriteset - SpriteSet instance
 * @param {number} tile - tile number
 * @param {number} x - X ordinate
 * @param {number} y - Y ordinate
 * @param {number} [w] - width
 * @param {number} [h] - height
 * @param {number} [align=gfx.ALIGN_LEFT_TOP] - horizontal and vertical alignment, a combination of the gfx.ALIGN_xyz constants
 * @param {number} [angle=0] - rotation angle in radians
 * @param {number} [flip=gfx.FLIP_NONE] - flip tile in X (gfx.FLIP_X), Y (gfx.FLIP_Y), or in both (gfx.FLIP_XY) directions
 */
duk_ret_t dk_SpriteSetDrawTile(duk_context *ctx) {
	if(!duk_is_object(ctx, 0) || !isPrototype(ctx, 0, DUK_HIDDEN_SYMBOL("SpriteSet_prototype")))
		return duk_error(ctx, DUK_ERR_SYNTAX_ERROR,
			"SpriteSet expected as first argument of drawTile");

	duk_get_prop_literal(ctx, 0, DUK_HIDDEN_SYMBOL("instance"));
	SpriteSet* sps = (SpriteSet*)duk_get_buffer(ctx, -1, NULL);
	uint16_t tile = duk_to_uint16(ctx, 1);
	float x = duk_to_number(ctx, 2);
	float y = duk_to_number(ctx, 3);
	float w = duk_get_number_default(ctx, 4, 0.0f);
	float h = duk_get_number_default(ctx, 5, 0.0f);
	int align = duk_get_int_default(ctx, 6, 0);
	float rot = duk_get_number_default(ctx, 7, 0.0f);
	int flip = duk_get_int_default(ctx, 8, 0);
	SpriteSetDrawTile(sps, tile, x,y, w, h, align, rot, flip);
	return 0;
}

/**
 * @function app.createSpriteSet
 * creates a new SpriteSet instance
 * 
 * a SpriteSet is a group of graphics sharing the same texture (atlas) that
 * are drawn and updated together
 * 
 * @param {number} texture - texture image resource handle
 * @param {number} [tilesX=1] - number for tiles in horizontal direction
 * @param {number} [tilesY=1] - number for tiles in vertical direction
 * @param {number} [border=0] - border width (of each tile) in pixels
 * @returns {object} - created SpriteSet instance
 */
duk_ret_t dk_createSpriteSet(duk_context *ctx) {
	int argc = duk_get_top(ctx);

	size_t img = ResourceValidateHandle(duk_get_uint(ctx, 0), RESOURCE_IMAGE);
	if(!img) {
		snprintf(s_lastError, ERROR_MAXLEN, "invalid image %s", duk_to_string(ctx, 0));
		return duk_error(ctx, DUK_ERR_REFERENCE_ERROR, s_lastError);
	}

	duk_push_object(ctx);
	duk_get_global_string(ctx, DUK_HIDDEN_SYMBOL("SpriteSet_prototype"));
	duk_set_prototype(ctx, -2);

	SpriteSet* sps = (SpriteSet*)duk_push_fixed_buffer(ctx, sizeof(SpriteSet));
	if(argc<3)
		*sps = SpriteSetCreate(img);
	else {
		uint16_t tilesX = duk_to_uint16(ctx, 1);
		uint16_t tilesY = duk_to_uint16(ctx, 2);
		uint16_t border = argc>3 ? duk_to_uint16(ctx, 3) : 0;
		*sps = SpriteSetCreateTiled(img, tilesX, tilesY, border);
	}
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("instance"));
	return 1;
}

void sprites_exports(duk_context *ctx) {
	// register Sprite prototype:
	duk_push_object(ctx);
	duk_push_c_function(ctx, dk_SpriteFinalizer, 1);
	duk_set_finalizer(ctx, -2);

	duk_push_c_function(ctx, dk_SpriteSetPos, 3);
	duk_put_prop_literal(ctx, -2, "setPos");
	duk_push_c_function(ctx, dk_SpriteSetX, 1);
	duk_put_prop_literal(ctx, -2, "setX");
	duk_push_c_function(ctx, dk_SpriteSetY, 1);
	duk_put_prop_literal(ctx, -2, "setY");
	duk_push_c_function(ctx, dk_SpriteGetX, 0);
	duk_put_prop_literal(ctx, -2, "getX");
	duk_push_c_function(ctx, dk_SpriteGetY, 0);
	duk_put_prop_literal(ctx, -2, "getY");
	duk_push_c_function(ctx, dk_SpriteGetRot, 0);
	duk_put_prop_literal(ctx, -2, "getRot");
	duk_push_c_function(ctx, dk_SpriteSetRot, 1);
	duk_put_prop_literal(ctx, -2, "setRot");
	duk_push_c_function(ctx, dk_SpriteSetSource, 4);
	duk_put_prop_literal(ctx, -2, "setSource");
	duk_push_c_function(ctx, dk_SpriteSetTile, 1);
	duk_put_prop_literal(ctx, -2, "setTile");

	duk_push_c_function(ctx, dk_SpriteSetScale, 2);
	duk_put_prop_literal(ctx, -2, "setScale");
	duk_push_c_function(ctx, dk_SpriteGetScaleX, 0);
	duk_put_prop_literal(ctx, -2, "getScaleX");
	duk_push_c_function(ctx, dk_SpriteGetScaleY, 0);
	duk_put_prop_literal(ctx, -2, "getScaleY");
	duk_push_c_function(ctx, dk_SpriteSetDim, 2);
	duk_put_prop_literal(ctx, -2, "setDim");
	duk_push_c_function(ctx, dk_SpriteGetDimX, 0);
	duk_put_prop_literal(ctx, -2, "getDimX");
	duk_push_c_function(ctx, dk_SpriteGetDimY, 0);
	duk_put_prop_literal(ctx, -2, "getDimY");

	duk_push_c_function(ctx, dk_SpriteSetVel, 3);
	duk_put_prop_literal(ctx, -2, "setVel");
	duk_push_c_function(ctx, dk_SpriteSetVelX, 1);
	duk_put_prop_literal(ctx, -2, "setVelX");
	duk_push_c_function(ctx, dk_SpriteSetVelY, 1);
	duk_put_prop_literal(ctx, -2, "setVelY");
	duk_push_c_function(ctx, dk_SpriteGetVelX, 0);
	duk_put_prop_literal(ctx, -2, "getVelX");
	duk_push_c_function(ctx, dk_SpriteGetVelY, 0);
	duk_put_prop_literal(ctx, -2, "getVelY");
	duk_push_c_function(ctx, dk_SpriteGetVelRot, 0);
	duk_put_prop_literal(ctx, -2, "getVelRot");
	duk_push_c_function(ctx, dk_SpriteSetVelRot, 1);
	duk_put_prop_literal(ctx, -2, "setVelRot");

	duk_push_c_function(ctx, dk_SpriteSetCenter, 2);
	duk_put_prop_literal(ctx, -2, "setCenter");
	duk_push_c_function(ctx, dk_SpriteGetCenterX, 0);
	duk_put_prop_literal(ctx, -2, "getCenterX");
	duk_push_c_function(ctx, dk_SpriteGetCenterY, 0);
	duk_put_prop_literal(ctx, -2, "getCenterY");

	duk_push_c_function(ctx, dk_SpriteSetColor, 4);
	duk_put_prop_literal(ctx, -2, "setColor");
	duk_push_c_function(ctx, dk_SpriteGetColor, 0);
	duk_put_prop_literal(ctx, -2, "getColor");
	duk_push_c_function(ctx, dk_SpriteSetAlpha, 1);
	duk_put_prop_literal(ctx, -2, "setAlpha");
	duk_push_c_function(ctx, dk_SpriteSetRadius, 1);
	duk_put_prop_literal(ctx, -2, "setRadius");
	duk_push_c_function(ctx, dk_SpriteGetRadius, 0);
	duk_put_prop_literal(ctx, -2, "getRadius");
	duk_push_c_function(ctx, dk_SpriteIntersects, 2);
	duk_put_prop_literal(ctx, -2, "intersects");
	duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("Sprite_prototype"));

	// register SpriteSet prototype:
	duk_push_object(ctx);
	duk_push_c_function(ctx, dk_createSprite, DUK_VARARGS);
	duk_put_prop_literal(ctx, -2, "createSprite");
	duk_push_c_function(ctx, dk_removeSprite, 1);
	duk_put_prop_literal(ctx, -2, "removeSprite");

	duk_push_c_function(ctx, dk_SpriteSetUpdate, 1);
	duk_put_prop_literal(ctx, -2, "update");
	duk_put_global_literal(ctx, DUK_HIDDEN_SYMBOL("SpriteSet_prototype"));

	// append createSpriteSet to app:
	duk_get_global_literal(ctx, "app");
	duk_push_c_function(ctx, dk_createSpriteSet, DUK_VARARGS);
	duk_put_prop_literal(ctx, -2, "createSpriteSet");
	duk_pop(ctx);

	duk_push_c_function(ctx, dk_SpriteSetDraw, 1);
	duk_put_prop_literal(ctx, -2, "drawSprites"); // set method of gfx object, which is on the stack

	duk_push_c_function(ctx, dk_SpriteSetDrawTile, 9);
	duk_put_prop_literal(ctx, -2, "drawTile"); // set method of gfx object, which is on the stack
}
