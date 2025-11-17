#include "intersects.h"

#include "../external/duk_config.h"
#include "../external/duktape.h"
#include <stdint.h>

extern uint32_t readFloatArray(duk_context *ctx, duk_idx_t idx, float** arr, float** buf);
extern float getPropFloatDefault(duk_context *ctx, duk_idx_t idx, const char* key, float defaultValue);
extern uint32_t getPropFloatBuffer(duk_context *ctx, duk_idx_t idx, const char* key, float** arr);

/** @module intersects
 * 
 * a collection of intersection/collision test functions
 * 
 * ```javascript
 * var intersects = app.require('intersects');
 * ```
 */

/** @function intersects.pointPoint
 * Test if two points have the same position
 * @param {number} x1 - test point1 X ordinate
 * @param {number} y1 - test point1 Y ordinate
 * @param {number} x2 - test point2 X ordinate
 * @param {number} y2 - test point2 Y ordinate
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_pointPoint(duk_context *ctx) {
	float x1 = duk_to_number(ctx,0);
	float y1 = duk_to_number(ctx,1);
	float x2 = duk_to_number(ctx,2);
	float y2 = duk_to_number(ctx,3);
	duk_push_boolean(ctx, x1==x2 && y1==y2);
	return 1;
}

/** @function intersects.pointCircle
 * Test if a points lies within a circle
 * @param {number} x - test point X ordinate
 * @param {number} y - test point Y ordinate
 * @param {number} cx - circle center X ordinate
 * @param {number} cy - circle center Y ordinate
 * @param {number} r - circle radius
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_pointCircle(duk_context *ctx) {
	float x = duk_to_number(ctx,0);
	float y = duk_to_number(ctx,1);
	float cx = duk_to_number(ctx,2);
	float cy = duk_to_number(ctx,3);
	float r = duk_to_number(ctx,4);
	duk_push_boolean(ctx, intersectsPointCircle(x,y,cx,cy,r));
	return 1;
}

/** @function intersects.pointAlignedRect
 * Test if a points lies within an axis-aligned rectangle
 * @param {number} x - test point X ordinate
 * @param {number} y - test point Y ordinate
 * @param {number} x1 - rectangle minimum X ordinate
 * @param {number} y1 - rectangle minimum Y ordinate
 * @param {number} x2 - rectangle maximum X ordinate
 * @param {number} y2 - rectangle maximum Y ordinate
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_pointAlignedRect(duk_context *ctx) {
	float x = duk_to_number(ctx,0);
	float y = duk_to_number(ctx,1);
	float x1 = duk_to_number(ctx,2);
	float y1 = duk_to_number(ctx,3);
	float x2 = duk_to_number(ctx,4);
	float y2 = duk_to_number(ctx,5);
	duk_push_boolean(ctx, intersectsPointAlignedRect(x,y,x1,y1,x2,y2));
	return 1;
}

/** @function intersects.pointPolygon
 * Test if point(x,y) is within a convex polygon
 * 
 * Based on https://stackoverflow.com/a/34689268 .
 * 
 * @param {number} x - test point X ordinate
 * @param {number} y - test point Y ordinate
 * @param {array|ArrayBuffer} polygon - polygon ordinates
 * @returns {boolean} true if point is within polygon
 */
static duk_ret_t dk_pointPolygon(duk_context *ctx) {
	float x = duk_to_number(ctx,0);
	float y = duk_to_number(ctx,1);

	float *arr, *buf;
	uint32_t n=readFloatArray(ctx, 2, &arr, &buf);
	if(!n || n%2)
		return duk_error(ctx, DUK_ERR_ERROR,
			"polygon data expected as number array or Float32Array of even size");

	duk_push_boolean(ctx, intersectsPointPolygon(x,y, n/2,arr));
	free(buf);
	return 1;
}

/** @function intersects.circleCircle
 * Test if two circles intersect
 * @param {number} x1 - circle 1 center X ordinate
 * @param {number} y1 - circle 1 center Y ordinate
 * @param {number} r1 - circle 1 radius
 * @param {number} x2 - circle 2 center X ordinate
 * @param {number} y2 - circle 2 center Y ordinate
 * @param {number} r2 - circle 2 radius
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_circleCircle(duk_context *ctx) {
	float x1 = duk_to_number(ctx,0);
	float y1 = duk_to_number(ctx,1);
	float r1 = duk_to_number(ctx,2);
	float x2 = duk_to_number(ctx,3);
	float y2 = duk_to_number(ctx,4);
	float r2 = duk_to_number(ctx,5);
	duk_push_boolean(ctx, intersectsCircleCircle(x1,y1,r1, x2,y2,r2));
	return 1;
}

/** @function intersects.circleAlignedRect
 * Test if a circle and an axis-aligned rectangle intersect
 * @param {number} x - circle center X ordinate
 * @param {number} y - circle center Y ordinate
 * @param {number} r - circle radius
 * @param {number} x1 - rectangle minimum X ordinate
 * @param {number} y1 - rectangle minimum Y ordinate
 * @param {number} x2 - rectangle maximum X ordinate
 * @param {number} y2 - rectangle maximum Y ordinate
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_circleAlignedRect(duk_context *ctx) {
	float x = duk_to_number(ctx,0);
	float y = duk_to_number(ctx,1);
	float r = duk_to_number(ctx,2);
	float x1 = duk_to_number(ctx,3);
	float y1 = duk_to_number(ctx,4);
	float x2 = duk_to_number(ctx,5);
	float y2 = duk_to_number(ctx,6);
	duk_push_boolean(ctx, intersectsCircleAlignedRect(x,y,r, x1,y1,x2,y2));
	return 1;
}

/** @function intersects.circlePolygon
 * Test if a circle and a convex polygon intersect
 * @param {number} x - circle center X ordinate
 * @param {number} y - circle center Y ordinate
 * @param {number} r - circle radius
 * @param {array|ArrayBuffer} polygon - polygon ordinates
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_circlePolygon(duk_context *ctx) {
	float x = duk_to_number(ctx,0);
	float y = duk_to_number(ctx,1);
	float r = duk_to_number(ctx,2);
	
	float *arr, *buf;
	uint32_t n=readFloatArray(ctx, 3, &arr, &buf);
	if(!n || n%2)
		return duk_error(ctx, DUK_ERR_ERROR,
			"polygon data expected as number array or Float32Array of even size");

	duk_push_boolean(ctx, intersectsCirclePolygon(x,y,r, n/2,arr));
	free(buf);
	return 1;
}

/** @function intersects.circleTriangles
 * Test if a circle and a list of optionally transformed triangles intersect
 * @param {number} cx - circle center X ordinate
 * @param {number} cy - circle center Y ordinate
 * @param {number} cr - circle radius
 * @param {array|ArrayBuffer} triangles - triangle ordinates
 * @param {number} [tx=0] - triangles x translation
 * @param {number} [ty=0] - triangles y translation
 * @param {number} [trot=0] - triangles rotation
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_circleTriangles(duk_context *ctx) {
	float cx = duk_to_number(ctx,0);
	float cy = duk_to_number(ctx,1);
	float cr = duk_to_number(ctx,2);
	
	float *arr, *buf;
	uint32_t n=readFloatArray(ctx, 3, &arr, &buf);
	if(!n || n%6)
		return duk_error(ctx, DUK_ERR_ERROR,
			"triangle data expected as number array or Float32Array of a size divisible by 6");

	float tx = duk_get_number_default(ctx, 4, 0.0);
	float ty = duk_get_number_default(ctx, 5, 0.0);
	float trot = duk_get_number_default(ctx, 6, 0.0);
	duk_push_boolean(ctx, intersectsCircleTriangles(cx,cy,cr, n/6,arr, tx,ty,trot));
	free(buf);
	return 1;
}

/** @function intersects.alignedRectAlignedRect
 * Test if two axis-aligned rectangles intersect
 * @param {number} x1 - rectangle 1 minimum X ordinate
 * @param {number} y1 - rectangle 1 minimum Y ordinate
 * @param {number} x2 - rectangle 1 maximum X ordinate
 * @param {number} y2 - rectangle 1 maximum Y ordinate
 * @param {number} x3 - rectangle 2 minimum X ordinate
 * @param {number} y3 - rectangle 2 minimum Y ordinate
 * @param {number} x4 - rectangle 2 maximum X ordinate
 * @param {number} y4 - rectangle 2 maximum Y ordinate
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_AlignedRectAlignedRect(duk_context *ctx) {
	float x1 = duk_to_number(ctx,0);
	float y1 = duk_to_number(ctx,1);
	float x2 = duk_to_number(ctx,2);
	float y2 = duk_to_number(ctx,3);
	float x3 = duk_to_number(ctx,4);
	float y3 = duk_to_number(ctx,5);
	float x4 = duk_to_number(ctx,6);
	float y4 = duk_to_number(ctx,7);
	duk_push_boolean(ctx, intersectsAlignedRectAlignedRect(x1,y1,x2,y2,x3,y3,x4,y4));
	return 1;
}

/** @function intersects.alignedRectPolygon
 * Test if an axis-aligned rectangle and a convex polygon intersect
 * @param {number} x1 - rectangle minimum X ordinate
 * @param {number} y1 - rectangle minimum Y ordinate
 * @param {number} x2 - rectangle maximum X ordinate
 * @param {number} y2 - rectangle maximum Y ordinate
 * @param {array|ArrayBuffer} polygon - polygon ordinates
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_AlignedRectPolygon(duk_context *ctx) {
	float x1 = duk_to_number(ctx,0);
	float y1 = duk_to_number(ctx,1);
	float x2 = duk_to_number(ctx,2);
	float y2 = duk_to_number(ctx,3);
	float *arr, *buf;
	uint32_t n=readFloatArray(ctx, 4, &arr, &buf);
	if(!n || n%2)
		return duk_error(ctx, DUK_ERR_ERROR,
			"polygon data expected as number array or Float32Array of even size");

	duk_push_boolean(ctx, intersectsAlignedRectPolygon(x1,y1,x2,y2, n/2,arr));
	free(buf);
	return 1;
}

/** @function intersects.polygonPolygon
 * Test if two convex polygons intersect
 * @param {array|ArrayBuffer} polygon1 - polygon 1 ordinates
 * @param {array|ArrayBuffer} polygon2 - polygon 2 ordinates
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_PolygonPolygon(duk_context *ctx) {
	float *arr1, *buf1, *arr2, *buf2;
	uint32_t n1=readFloatArray(ctx, 0, &arr1, &buf1);
	uint32_t n2=readFloatArray(ctx, 1, &arr2, &buf2);
	if(n1%2 || n2%2)
		return duk_error(ctx, DUK_ERR_ERROR,
			"polygon data expected as number array or Float32Array of even size");

	duk_push_boolean(ctx, intersectsPolygonPolygon(n1/2, arr1, n2/2, arr2));
	free(buf1);
	free(buf2);
	return 1;
}

/** @function intersects.polygonTriangles
 * Test if a convex polygon and a triangle list intersect, both optionally transformed
 * @param {array|ArrayBuffer} polygon - polygon ordinates
 * @param {number} x1 - polygon x translation
 * @param {number} y1 - polygon y translation
 * @param {number} rot1 - polygon rotation
 * @param {array|ArrayBuffer} triangles - triangle ordinates
 * @param {number} [x2=0] - triangles x translation
 * @param {number} [y2=0] - triangles y translation
 * @param {number} [rot2=0] - triangles rotation
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_PolygonTriangles(duk_context *ctx) {
	float *arr1, *buf1, *arr2, *buf2;
	uint32_t n1=readFloatArray(ctx, 0, &arr1, &buf1);
	float x1 = duk_to_number(ctx,1);
	float y1 = duk_to_number(ctx,2);
	float rot1 = duk_get_number_default(ctx,3, 0.0);
	uint32_t n2=readFloatArray(ctx, 4, &arr2, &buf2);
	if(n1%2 || n2%6)
		return duk_error(ctx, DUK_ERR_ERROR,
			"polygon/triangle data expected as number array or Float32Array of appropriate size");
	float x2 = duk_get_number_default(ctx,5, 0.0);
	float y2 = duk_get_number_default(ctx,6, 0.0);
	float rot2 = duk_get_number_default(ctx,7, 0.0);

	if(x1||y1||rot1) {
		if(!buf1) {
			buf1 = malloc(n1*sizeof(float));
			memcpy(buf1, arr1, n1*sizeof(float));
		}
		intersectsTransf(x1, y1, rot1, n1, buf1);
	}
	int intersects = 0;
	for(uint32_t i=0; i<n2 && !intersects; i+=6) {
		float tr[] = {
			arr2[i], arr2[i+1], arr2[i+2], arr2[i+3], arr2[i+4], arr2[i+5] };
		intersectsTransf(x2, y2, rot2, 6, tr);
		intersects = intersectsPolygonPolygon(n1/2, buf1 ? buf1 : arr1, 3, tr);
	}
	duk_push_boolean(ctx, intersects);
	free(buf1);
	free(buf2);
	return 1;
}

/** @function intersects.trianglesTriangles
 * Test if two triangle lists intersect, both optionally transformed
 * @param {array|ArrayBuffer} tr1 - first triangle ordinates
 * @param {number} x1 - first triangles x translation
 * @param {number} y1 - first triangles y translation
 * @param {number} rot1 - first triangles rotation
 * @param {array|ArrayBuffer} second tr2 - second triangle ordinates
 * @param {number} [x2=0] - second triangles x translation
 * @param {number} [y2=0] - second triangles y translation
 * @param {number} [rot2=0] - triangles rotation
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_TrianglesTriangles(duk_context *ctx) {
	float *arr1, *buf1, *arr2, *buf2;
	uint32_t n1=readFloatArray(ctx, 0, &arr1, &buf1);
	float x1 = duk_to_number(ctx,1);
	float y1 = duk_to_number(ctx,2);
	float rot1 = duk_get_number_default(ctx,3, 0.0);
	uint32_t n2=readFloatArray(ctx, 4, &arr2, &buf2);
	if(n1%6 || n2%6)
		return duk_error(ctx, DUK_ERR_ERROR,
			"triangle data expected as number array or Float32Array of appropriate size");
	float x2 = duk_get_number_default(ctx,5, 0.0);
	float y2 = duk_get_number_default(ctx,6, 0.0);
	float rot2 = duk_get_number_default(ctx,7, 0.0);

	if(x1||y1||rot1) {
		if(!buf1) {
			buf1 = malloc(n1*sizeof(float));
			memcpy(buf1, arr1, n1*sizeof(float));
		}
		intersectsTransf(x1, y1, rot1, n1, buf1);
	}
	int intersects = 0;
	for(uint32_t j=0; j<n1 && !intersects; j+=6) {
		const float* tr1 = buf1 ? &buf1[j] : &arr1[j];
		for(uint32_t i=0; i<n2 && !intersects; i+=6) {
			float tr2[] = { arr2[i], arr2[i+1], arr2[i+2], arr2[i+3], arr2[i+4], arr2[i+5] };
			intersectsTransf(x2, y2, rot2, 6, tr2);
			intersects = intersectsPolygonPolygon(3, tr1, 3, tr2);
		}
	}
	duk_push_boolean(ctx, intersects);
	free(buf1);
	free(buf2);
	return 1;
}

/** @function intersects.sprites
 * Test if two sprite objects intersect
 * 
 * The test is based on the following object attributes:
 * - position (x,y,rot) currently NO scale
 * - bounding radius (radius) or rectangle (w,h) with optional center (cx,cy)
 * - optionally convex hull (shape) or triangle list (triangles)
 * 
 * @param {object} s1 - first sprite
 * @param {object} s2 - second sprite
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_sprites(duk_context *ctx) {
	Sprite s1, s2;
	if(!duk_is_object(ctx, 0) || !duk_is_object(ctx, 1))
		return duk_error(ctx, DUK_ERR_ERROR, "intersects.sprites() expects 2 objects as arguments");
	s1.x = getPropFloatDefault(ctx,0,"x",0.0f);
	s1.y = getPropFloatDefault(ctx,0,"y",0.0f);
	s1.rot = getPropFloatDefault(ctx,0,"rot",0.0f);
	s1.radius = getPropFloatDefault(ctx,0,"radius",-1.0f);
	if(s1.radius<0.0f) {
		s1.w = getPropFloatDefault(ctx,0,"w",-1.0f);
		s1.h = getPropFloatDefault(ctx,0,"h",-1.0f);
		if(s1.w<0.0f || s1.h<0.0f) {
			//duk_push_false(ctx); return 1;
			return duk_error(ctx, DUK_ERR_ERROR, "intersect.sprites() expects either radius or w/h properties of first object");
		}
		s1.cx = getPropFloatDefault(ctx,0,"cx",0.0f);
		s1.cy = getPropFloatDefault(ctx,0,"cy",0.0f);
	}

	s2.x = getPropFloatDefault(ctx,1,"x",0.0f);
	s2.y = getPropFloatDefault(ctx,1,"y",0.0f);
	s2.rot = getPropFloatDefault(ctx,1,"rot",0.0f);
	s2.radius = getPropFloatDefault(ctx,1,"radius",-1.0f);
	if(s2.radius<0.0f) {
		s2.w = getPropFloatDefault(ctx,1,"w",-1.0f);
		s2.h = getPropFloatDefault(ctx,1,"h",-1.0f);
		if(s2.w<0.0f || s2.h<0.0f)
			return duk_error(ctx, DUK_ERR_ERROR,
				"intersect.sprites() expects either radius or w/h properties of second object");
		s2.cx = getPropFloatDefault(ctx,1,"cx",0.0f);
		s2.cy = getPropFloatDefault(ctx,1,"cy",0.0f);
	}

	int intersects = intersectsSpritesCoarse(&s1, &s2);
	if(intersects) { // consider convex hull and triangles
		s1.shape = s1.triangles = s2.shape = s2.triangles = NULL;
		s1.arrlen = getPropFloatBuffer(ctx,0, "shape", &s1.shape);
		if(!s1.arrlen)
			s1.arrlen = getPropFloatBuffer(ctx,0, "triangles", &s1.triangles);

		s2.arrlen = getPropFloatBuffer(ctx,1, "shape", &s2.shape);
		if(!s2.arrlen)
			s2.arrlen = getPropFloatBuffer(ctx,1, "triangles", &s2.triangles);

		if(s1.arrlen || s2.arrlen)
			intersects = intersectsSpritesPrecise(&s1, &s2);
	}
	duk_push_boolean(ctx, intersects);
	return 1;
}

/** @function intersects.arrays
 * Test if two geometries intersect
 *
 * Depending on the number of ordinates, the arrays are interpreted either as point (2), circle (3),
 * axis-aligned rectangle (4), or polygon (6+).
 *
 * @param {array|ArrayBuffer} array1 - first test geometry ordinates
 * @param {array|ArrayBuffer} array2 - second test geometry ordinates
 * @returns {boolean} true if the two objects intersect
 */
static duk_ret_t dk_arrays(duk_context *ctx) {
	float *arr1, *buf1, *arr2, *buf2;
	uint32_t n1=readFloatArray(ctx, 0, &arr1, &buf1)/2;
	uint32_t n2=readFloatArray(ctx, 1, &arr2, &buf2)/2;
	if(n1<2 || n2<2)
		return duk_error(ctx, DUK_ERR_ERROR,
			"data expected as number array or Float32Array of even size or size 3 for circles");
	if(n2<n1) {
		float* tmp = arr1;
		arr1 = arr2;
		arr2 = tmp;
	}

	switch(n1) {
	case 2:
		switch(n2) {
		case 2:
			duk_push_boolean(ctx, arr1[0]==arr2[0] && arr1[1]==arr2[1]);
			break;
		case 3:
			duk_push_boolean(ctx, intersectsPointCircle(arr1[0],arr1[1], arr2[0],arr2[1],arr2[2]));
			break;
		case 4:
			duk_push_boolean(ctx, intersectsPointAlignedRect(arr1[0],arr1[1], arr2[0],arr2[1],arr2[2],arr2[3]));
			break;
		default:
			duk_push_boolean(ctx, intersectsPointPolygon(arr1[0],arr1[1], n2, arr2));
		}
		break;
	case 3:
		switch(n2) {
		case 3:
			duk_push_boolean(ctx, intersectsCircleCircle(arr1[0],arr1[1],arr1[2], arr2[0],arr2[1],arr2[2]));
			break;
		case 4:
			duk_push_boolean(ctx, intersectsCircleAlignedRect(arr1[0],arr1[1],arr1[2], arr2[0],arr2[1],arr2[2],arr2[3]));
			break;
		default:
			duk_push_boolean(ctx, intersectsCirclePolygon(arr1[0],arr1[1],arr1[2], n2, arr2));
		}
		break;
	case 4:
		switch(n2) {
		case 4:
			duk_push_boolean(ctx, intersectsAlignedRectAlignedRect(arr1[0],arr1[1],arr1[2],arr1[3], arr2[0],arr2[1],arr2[2],arr2[3]));
			break;
		default:
			duk_push_boolean(ctx, intersectsAlignedRectPolygon(arr1[0],arr1[1],arr1[2],arr1[3], n2, arr2));
		}
		break;
	default:
		duk_push_boolean(ctx, intersectsPolygonPolygon(n1,arr1, n2,arr2));
	}

	free(buf1);
	free(buf2);
	return 1;
}

void intersects_exports(duk_context *ctx) {
	// register intersects functions:
	duk_push_object(ctx);

	duk_push_c_function(ctx, dk_pointPoint, 4);
	duk_put_prop_literal(ctx, -2, "pointPoint");
	duk_push_c_function(ctx, dk_pointCircle, 5);
	duk_put_prop_literal(ctx, -2, "pointCircle");
	duk_push_c_function(ctx, dk_pointAlignedRect, 6);
	duk_put_prop_literal(ctx, -2, "pointAlignedRect");
	duk_push_c_function(ctx, dk_pointPolygon, 3);
	duk_put_prop_literal(ctx, -2, "pointPolygon");
	duk_push_c_function(ctx, dk_circleCircle, 6);
	duk_put_prop_literal(ctx, -2, "circleCircle");
	duk_push_c_function(ctx, dk_circleAlignedRect, 7);
	duk_put_prop_literal(ctx, -2, "circleAlignedRect");
	duk_push_c_function(ctx, dk_circlePolygon, 4);
	duk_put_prop_literal(ctx, -2, "circlePolygon");
	duk_push_c_function(ctx, dk_circleTriangles, 7);
	duk_put_prop_literal(ctx, -2, "circleTriangles");
	duk_push_c_function(ctx, dk_AlignedRectAlignedRect, 8);
	duk_put_prop_literal(ctx, -2, "alignedRectAlignedRect");
	duk_push_c_function(ctx, dk_AlignedRectPolygon, 5);
	duk_put_prop_literal(ctx, -2, "alignedRectPolygon");
	duk_push_c_function(ctx, dk_PolygonPolygon, 2);
	duk_put_prop_literal(ctx, -2, "polygonPolygon");
	duk_push_c_function(ctx, dk_PolygonTriangles, 8);
	duk_put_prop_literal(ctx, -2, "polygonTriangles");
	duk_push_c_function(ctx, dk_TrianglesTriangles, 8);
	duk_put_prop_literal(ctx, -2, "trianglesTriangles");
	duk_push_c_function(ctx, dk_sprites, 2);
	duk_put_prop_literal(ctx, -2, "sprites");
	duk_push_c_function(ctx, dk_arrays, 2);
	duk_put_prop_literal(ctx, -2, "arrays");
}
