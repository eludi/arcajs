#include "intersects.h"
#include <float.h>
#include <math.h>

void intersectsTransf(float x, float y, float rot, uint32_t arrSz, float* arr) {
	float cosine = cos(rot), sine = sin(rot);
	for(uint32_t i=0; i+1<arrSz; i+=2) {
		float ax=arr[i], ay=arr[i+1];
		arr[i] = ax*cosine - ay*sine + x, arr[i+1] = ax*sine + ay*cosine + y;
	}
}

void intersectsTransfInv(float x, float y, float rot, uint32_t arrSz, float* arr) {
	float cosine = cos(-rot), sine = sin(-rot);
	for(uint32_t i=0; i+1<arrSz; i+=2) {
		float ax=arr[i]-x, ay=arr[i+1]-y;
		arr[i] = ax*cosine - ay*sine, arr[i+1] = ax*sine + ay*cosine;
	}
}

int intersectsPointCircle(float x, float y, float cx, float cy, float r) {
	float dx = cx-x, dy = cy-y;
	return dx*dx + dy*dy < r*r;
}

int intersectsPointAlignedRect(float x, float y, float x1, float y1, float x2, float y2) {
	return (x > x2 || x1 > x || y > y2 || y1 > y) ? 0 : 1;
}

int intersectsPointPolygon(float x, float y, uint32_t numCoords, float* polygon) {
	uint32_t polySz = 2*numCoords;
	if(polySz < 6)
		return 0; // check if a triangle or higher n-gon

	// keep track of cross product sign changes
	uint32_t pos = 0, neg = 0;

	for (uint32_t i = 0; i < polySz; i+=2) {
		float x1 = polygon[i];
		float y1 = polygon[i+1];

		if (x==x1 && y==y1)
			return 1;

		float x2 = polygon[(i+2)%polySz];
		float y2 = polygon[(i+3)%polySz];

		// compute the cross product
		float d = (x - x1)*(y2 - y1) - (y - y1)*(x2 - x1);

		if (d > 0)
			pos++;
		else if (d < 0)
			neg++;

		// if the sign changes, then point is outside
		if (pos > 0 && neg > 0)
			return 0;
	}
	// if on the same side of all segments then inside
	return 1;
}

int intersectsCircleCircle(
	float x1, float y1, float r1, float x2, float y2, float r2)
{
	float dx = x2-x1, dy=y2-y1, sumR = r1+r2;
	return  dx*dx + dy*dy < sumR*sumR;
}

int intersectsCircleAlignedRect(float cx, float cy, float r, float x0, float y0, float x1, float y1) {
	if(cx<x0-r || cx>=x1+r || cy<y0-r || cy>=y1+r)
		return 0;
	if(cx<x0) {
		if(cy<y0) {
			float dx=x0-cx, dy=y0-cy;
			return dx*dx + dy*dy < r*r;
		}
		if(cy>y1) {
			float dx=x0-cx, dy=cy-y1;
			return dx*dx + dy*dy < r*r;
		}
	}
	else if(cx>x1) {
		if(cy<y0) {
			float dx=cx-x1, dy=y0-cy;
			return dx*dx + dy*dy < r*r;
		}
		if(cy>y1) {
			float dx=cx-x1, dy=cy-y1;
			return dx*dx + dy*dy < r*r;
		}
	}
	return 1;
}

int intersectsCirclePolygon(
	float x, float y, float r, uint32_t numCoords, float* polygon)
{
	if(numCoords < 3)
		return 0; // check if a triangle or higher n-gon
	uint32_t polySz = 2*numCoords;
	// TEST 1: Vertex within circle:
	float rSqr = r*r;
	for(uint32_t i=0; i<polySz; i+=2) {
		float dx = polygon[i]-x, dy=polygon[i+1]-y;
		if(dx*dx + dy*dy < rSqr)
			return 1;
	}
	// TEST 2: Circle center within polygon:
	if(intersectsPointPolygon(x,y, numCoords, polygon))
		return 1;
	// TEST 3: Circle intersects edge:
	for(uint32_t i=0; i<polySz; i+=2) {
		float x1 = polygon[i];
		float y1 = polygon[i+1];
		float x2 = polygon[(i+2)%polySz];
		float y2 = polygon[(i+3)%polySz];

		float v1x = x2-x1, v1y = y2-y1; // already calculated as dx before
		float v2x = x-x1, v2y = y-y1;

		float sc = ( v1x*v2x + v1y*v2y ) / ( v1x*v1x + v1y*v1y );
		if(sc<0.0f || sc>1.0f)
			continue;
		// project circle center onto edge:
		float v3x = v1x*sc, v3y = v1y*sc;
		float v4x = v2x-v3x, v4y = v2y-v3y;
		if(v4x*v4x + v4y*v4y < rSqr)
			return 1;
	}
	return 0;
}

int intersectsAlignedRectAlignedRect(
	float x1min, float y1min, float x1max, float y1max,
	float x2min, float y2min, float x2max, float y2max)
{
	if(x2min > x1max || x1min > x2max)
		return 0;
	if(y2min > y1max || y1min > y2max)
		return 0;
	return 1;
}

 int intersectsAlignedRectPolygon(
	float x1, float y1, float x2, float y2, uint32_t numCoords, float* poly)
{
	float arr1[] = { x1,y1, x1,y2, x2,y2, x2,y1 };
	return intersectsPolygonPolygon(4, arr1, numCoords, poly);
}

int intersectsPolygonPolygon(
	uint32_t numCoords1, float* poly1, uint32_t numCoords2, float* poly2)
{
	numCoords1*=2;
	numCoords2*=2;

	for (int i = 0; i < 2; i++) {

		// for each polygon, look at each edge of the polygon, and determine
		// if it separates the two shapes
		float* polygon = i==0 ? poly1 : poly2;
		uint32_t polySz = i==0 ? numCoords1 : numCoords2;

		for (uint32_t i1 = 0; i1 < polySz; i1+=2) {
			// grab 2 vertices to create an edge
			uint32_t i2 = (i1 + 2) % polySz;
			float p1x = polygon[i1], p1y = polygon[i1+1];
			float p2x = polygon[i2], p2y = polygon[i2+1];
			// find the line perpendicular to this edge
			float normalX = p2y - p1y, normalY = p1x - p2x;

			float minA = FLT_MAX, maxA = FLT_MIN;
			// for each vertex of shape 1, project it onto the line perpendicular
			// to the edge and keep track of the min and max of these values
			for (uint32_t j = 0; j < numCoords1; j+=2) {
				float projected = normalX * poly1[j] + normalY * poly1[j+1];
				if (projected < minA)
					minA = projected;
				if (projected > maxA)
					maxA = projected;
			}

			// for each vertex of shape 2, project it onto the line perpendicular
			// to the edge and keep track of the min and max of these values
			float minB = FLT_MAX, maxB = FLT_MIN;
			for (uint32_t j = 0; j < numCoords2; j+=2) {
				float projected = normalX * poly2[j] + normalY * poly2[j+1];
				if (projected < minB)
					minB = projected;
				if (projected > maxB)
					maxB = projected;
			}

			// if there is no overlap between the projects, this edge separates
			// the two polygons, and we know there is no overlap
			if (maxA < minB || maxB < minA)
				return 0;
		}
	}
	return 1;

}
