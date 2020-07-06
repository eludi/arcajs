#pragma once

#include "stdint.h"

/// transforms an array containing a list of 2D coordinates, arrSz is number of floats in array
extern void intersectsTransf(float x, float y, float rot, uint32_t arrSz, float* arr);

/// inverse transforms an array containing a list of 2D coordinates, arrSz is number of floats in array
extern void intersectsTransfInv(float x, float y, float rot, uint32_t arrSz, float* arr);

extern int intersectsPointCircle(float x, float y, float cx, float cy, float r);

extern int intersectsPointAlignedRect(float x, float y, float x1, float y1, float x2, float y2);

extern int intersectsPointPolygon(
	float x, float y, uint32_t numCoords, float* polygon);

extern int intersectsCircleCircle(
	float x1, float y1, float r1, float x2, float y2, float r2);

extern int intersectsCircleAlignedRect(
	float cx, float cy, float r, float x0, float y0, float x1, float y1 );

extern int intersectsCirclePolygon(
	float x, float y, float r, uint32_t numCoords, float* poly);

extern int intersectsAlignedRectAlignedRect(
	float x1min, float y1min, float x1max, float y1max,
	float x2min, float y2min, float x2max, float y2max);

extern int intersectsAlignedRectPolygon(
	float x1, float y1, float x2, float y2, uint32_t numCoords, float* poly);

extern int intersectsPolygonPolygon(
	uint32_t numCoords1, float* poly1, uint32_t numCoords2, float* poly2);
