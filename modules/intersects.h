#pragma once

#include "stdint.h"

/// transforms an array containing a list of 2D coordinates, arrSz is number of floats in array
extern void intersectsTransf(float x, float y, float rot, uint32_t arrSz, float* arr);

/// inverse transforms an array containing a list of 2D coordinates, arrSz is number of floats in array
extern void intersectsTransfInv(float x, float y, float rot, uint32_t arrSz, float* arr);

extern int intersectsPointCircle(float x, float y, float cx, float cy, float r);

extern int intersectsPointAlignedRect(float x, float y, float x1, float y1, float x2, float y2);

extern int intersectsPointPolygon(
	float x, float y, uint32_t numCoords, const float* polygon);

extern int intersectsCircleCircle(
	float x1, float y1, float r1, float x2, float y2, float r2);

extern int intersectsCircleAlignedRect(
	float cx, float cy, float r, float x0, float y0, float x1, float y1 );

extern int intersectsCirclePolygon(
	float x, float y, float r, uint32_t numCoords, const float* poly);

extern int intersectsCircleTriangles(
	float x1, float y1, float r,
	uint32_t numTriangles, const float* triangles, float x2, float y2, float rot2);

extern int intersectsAlignedRectAlignedRect(
	float x1min, float y1min, float x1max, float y1max,
	float x2min, float y2min, float x2max, float y2max);

extern int intersectsAlignedRectPolygon(
	float x1, float y1, float x2, float y2, uint32_t numCoords, const float* poly);

extern int intersectsPolygonPolygon(
	uint32_t numCoords1, const float* poly1, uint32_t numCoords2, const float* poly2);

extern int intersectsPolygonTriangles(
	uint32_t numCoords, const float* poly,
	uint32_t numTriangles, const float* triangles, float x2, float y2, float rot2);

extern int intersectsTrianglesTriangles(
	uint32_t numTriangles1, const float* triangles1, float x1, float y1, float rot1,
	uint32_t numTriangles2, const float* triangles2, float x2, float y2, float rot2);

/// structure for managing a moving or static object with flexible bounding and optional precise geometry
typedef struct {
	/// x position ordinate
	float x;
	/// y position ordinate
	float y;
	/// rotation ordinate
	float rot;
	/// bounding box dimensions for fast coarse-stage collision detection
	float w, h;
	/// center offset for placement and rotations
	float cx, cy;
	/// radius for fast coarse-stage collision detection
	float radius;

	/// precise collision geometry, either convex bounding hull shape or triangle list
	uint32_t arrlen;
	float* shape;
	float* triangles;
} Sprite;

/// tests for intersection of two sprites based on their bounding radius or oriented bounding rectangle
extern int intersectsSpritesCoarse(Sprite* s1, Sprite* s2);
/// tests for intersection of two sprites based on their bounding dimensions, convex hull or triangle list
extern int intersectsSpritesPrecise(const Sprite* s1, const Sprite* s2);
