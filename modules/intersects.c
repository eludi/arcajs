#include "intersects.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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

int intersectsPointPolygon(float x, float y, uint32_t numCoords, const float* polygon) {
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
	float x, float y, float r, uint32_t numCoords, const float* polygon)
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

int intersectsCircleTriangles(
	float x1, float y1, float radius,
	uint32_t numTriangles, const float* triangles,
	float x2, float y2, float rot2)
{
	float c[] = { x1, y1 };
	intersectsTransfInv(x2, y2, rot2, 2, c);
	for(uint32_t i=0; i<numTriangles; ++i)
		if(intersectsCirclePolygon(c[0], c[1], radius, 3, &triangles[6*i]))
			return 1;
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
	float x1, float y1, float x2, float y2, uint32_t numCoords, const float* poly)
{
	float arr1[] = { x1,y1, x1,y2, x2,y2, x2,y1 };
	return intersectsPolygonPolygon(4, arr1, numCoords, poly);
}

int intersectsPolygonPolygon(
	uint32_t numCoords1, const float* poly1, uint32_t numCoords2, const float* poly2)
{
	numCoords1*=2;
	numCoords2*=2;

	for (int i = 0; i < 2; i++) {
		// for each polygon, look at each edge of the polygon, and determine
		// if it separates the two shapes
		const float* polygon = i==0 ? poly1 : poly2;
		uint32_t polySz = i==0 ? numCoords1 : numCoords2;

		for (uint32_t i1 = 0; i1 < polySz; i1+=2) {
			// grab 2 vertices to create an edge
			uint32_t i2 = (i1 + 2) % polySz;
			float p1x = polygon[i1], p1y = polygon[i1+1];
			float p2x = polygon[i2], p2y = polygon[i2+1];
			// find the line perpendicular to this edge
			float normalX = p2y - p1y, normalY = p1x - p2x;

			float minA = FLT_MAX, maxA = -FLT_MAX;
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
			float minB = FLT_MAX, maxB = -FLT_MAX;
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

int intersectsPolygonTriangles(
	uint32_t numCoords, const float* poly,
	uint32_t numTriangles, const float* triangles, float x2, float y2, float rot2)
{
	if(x2 || y2 || rot2) {
		for(uint32_t i=0; i<numTriangles; ++i) {
			float tr[] = { triangles[6*i], triangles[6*i+1], triangles[6*i+2],
				triangles[6*i+3], triangles[6*i+4], triangles[6*i+5]};
			intersectsTransf(x2,y2,rot2,6,tr);
			if(intersectsPolygonPolygon(numCoords, poly, 3, tr))
				return 1;
		}
	}
	else for(uint32_t i=0; i<numTriangles; ++i)
		if(intersectsPolygonPolygon(numCoords, poly, 3, &triangles[6*i]))
			return 1;
	return 0;
}

int intersectsTrianglesTriangles(
	uint32_t numTriangles1, const float* triangles1, float x1, float y1, float rot1,
	uint32_t numTriangles2, const float* triangles2, float x2, float y2, float rot2)
{
	for(uint32_t j=0; j<numTriangles1; ++j) {
		float tr1[] = { triangles1[6*j], triangles1[6*j+1], triangles1[6*j+2],
			triangles1[6*j+3], triangles1[6*j+4], triangles1[6*j+5]};
		if(x1 || y1 || rot1)
			intersectsTransf(x1,y1,rot1,6,tr1);
		if(x2 || y2 || rot2)
			intersectsTransfInv(x2,y2,rot2,6,tr1);
		for(uint32_t i=0; i<numTriangles2; ++i) {
			if(intersectsPolygonPolygon(3, tr1, 3, &triangles2[6*i]))
				return 1;
		}
	}
	return 0;
}

//------------------------------------------------------------------

int intersectsSpritesCoarse(Sprite* s1, Sprite* s2) {
	if(s1->radius>=0.0f && s2->radius>=0.0f)
		return intersectsCircleCircle(s1->x, s1->y, s1->radius, s2->x, s2->y, s2->radius);
	if(s1->radius>=0.0f) {
		if(!s2->rot) {
			float x1 = s2->x - s2->cx*s2->w, y1=s2->y - s2->cy*s2->h;
			return intersectsCircleAlignedRect(s1->x, s1->y, s1->radius, x1,y1, x1+s2->w,y1+s2->h);
		}
		float c[] = { s1->x, s1->y };
		float ox = -s2->cx * s2->w, oy = -s2->cy * s2->h;
		float arr[] = {
			ox, oy,
			ox, oy + s2->h,
			ox + s2->w, oy + s2->h,
			ox + s2->w, oy
		};
		intersectsTransfInv(s2->x, s2->y, s2->rot, 2, c);
		return intersectsCirclePolygon(c[0], c[1], s1->radius, 4, arr);
	}
	if(s2->radius>=0.0f) {
		if(!s1->rot) {
			float x1 = s1->x - s1->cx * s1->w, y1=s1->y - s1->cy * s1->h;
			return intersectsCircleAlignedRect(s2->x, s2->y, s2->radius, x1,y1, x1+s1->w,y1+s1->h);
		}
		float c[] = { s2->x, s2->y };
		float ox = -s1->cx * s1->w, oy = -s1->cy * s1->h;
		float arr[] = {
			ox, oy,
			ox, oy + s1->h,
			ox + s1->w, oy + s1->h,
			ox + s1->w, oy
		};
		intersectsTransfInv(s1->x, s1->y, s1->rot, 2, c);
		return intersectsCirclePolygon(c[0], c[1], s2->radius, 4, arr);
	}
	if(!s1->rot && !s2->rot) {
		float x1min = s1->x - s1->cx * s1->w, y1min = s1->y - s1->cy * s1->h;
		float x1max = x1min + s1->w, y1max = y1min + s1->h;
		float x2min = s2->x - s2->cx * s2->w, y2min = s2->y - s2->cy * s2->h;
		float x2max = x2min + s2->w, y2max = y2min + s2->h;
		return intersectsAlignedRectAlignedRect(
			x1min,y1min, x1max,y1max, x2min,y2min, x2max,y2max);
	}

	float ox = -s1->cx * s1->w, oy = -s1->cy * s1->h;
	float arr1[] = {
		ox, oy,
		ox, oy + s1->h,
		ox + s1->w, oy + s1->h,
		ox + s1->w, oy
	};
	intersectsTransf(s1->x, s1->y, s1->rot, 8, arr1);
	ox = -s2->cx * s2->w;
	oy = -s2->cy * s2->h;
	float arr2[] = {
		ox, oy,
		ox, oy + s2->h,
		ox + s2->w, oy + s2->h,
		ox + s2->w, oy
	};
	intersectsTransf(s2->x, s2->y, s2->rot, 8, arr2);
	return intersectsPolygonPolygon(4, arr1, 4, arr2);
}

static void rect2poly(const Sprite* s, float* poly) {
	poly[0] = -s->cx * s->w;
	poly[1] = -s->cy * s->h;
	poly[2]	= poly[0];
	poly[3] = poly[1] + s->h;
	poly[4]	= poly[0] + s->w;
	poly[5] = poly[3];
	poly[6] = poly[4];
 	poly[7] = poly[1];
	intersectsTransf(s->x, s->y, s->rot, 8, poly);
}

int intersectsSpritesPrecise(const Sprite* s1, const Sprite* s2) {
	if(!s1->arrlen && !s2->arrlen)
		return 0;

	if(s1->shape && s2->shape) {
		const float *shape1 = s1->shape, *shape2 = s2->shape; 
		float *arr1transf = NULL, *arr2transf = NULL;
		if(s1->x || s1->y || s1->rot) {
			arr1transf = malloc(s1->arrlen*sizeof(float));
			memcpy(arr1transf, s1->shape, s1->arrlen*sizeof(float));
			intersectsTransf(s1->x, s1->y, s1->rot, s1->arrlen, arr1transf);
			shape1 = arr1transf;
		}
		if(s2->x || s2->y || s2->rot) {
			arr2transf = malloc(s2->arrlen*sizeof(float));
			memcpy(arr2transf, s2->shape, s2->arrlen*sizeof(float));
			intersectsTransf(s2->x, s2->y, s2->rot, s2->arrlen, arr2transf);
			shape2 = arr2transf;
		}
		int intersects = intersectsPolygonPolygon(s1->arrlen/2, shape1, s2->arrlen/2, shape2);
		free(arr1transf);
		free(arr2transf);
		return intersects;
	}

	if(s1->triangles && s2->triangles)
		return intersectsTrianglesTriangles(
			s1->arrlen/6, s1->triangles, s1->x, s1->y, s1->rot,
			s2->arrlen/6, s2->triangles, s2->x, s2->y, s2->rot);

	const Sprite *sShape, *sNoShape;
	if(s1->shape) {
		sShape = s1;
		sNoShape = s2;
	}
	else {
		sShape = s2;
		sNoShape = s1;
	}

	if(sShape->shape) {
		if(sNoShape->triangles) {
			for(uint32_t i=4, end = sShape->arrlen; i<end; i+=2) {
				float tr[] = { sShape->shape[0], sShape->shape[1],
					sShape->shape[i-2], sShape->shape[i-1], sShape->shape[i], sShape->shape[i+1]};
				if(intersectsTrianglesTriangles(
					1, tr, sShape->x, sShape->y, sShape->rot,
					sNoShape->arrlen/6, sNoShape->triangles, sNoShape->x, sNoShape->y, sNoShape->rot))
					return 1;
			}
			return 0;
		}
		if(sNoShape->radius>=0.0f) {
			float c[] = { sNoShape->x, sNoShape->y };
			intersectsTransfInv(sShape->x, sShape->y, sShape->rot, 2, c);
			return intersectsCirclePolygon(c[0], c[1], sNoShape->radius, sShape->arrlen/2, sShape->shape);
		}

		float poly[8];
		rect2poly(sNoShape, poly);

		float *arrTransf = NULL, *shape = sShape->shape;
		if(sShape->x || sShape->y || sShape->rot) {
			arrTransf = malloc(sShape->arrlen*sizeof(float));
			memcpy(arrTransf, shape, sShape->arrlen*sizeof(float));
			intersectsTransf(sShape->x, sShape->y, sShape->rot, sShape->arrlen, arrTransf);
			shape = arrTransf;
		}
		int intersects = intersectsPolygonPolygon(4, poly, sShape->arrlen/2, shape);
		free(arrTransf);
		return intersects;
	}
	
	// triangle list against circle/rect:
	const Sprite *sTr, *sNoTr;
	if(s1->triangles) {
		sTr = s1;
		sNoTr = s2;
	}
	else {
		sTr = s2;
		sNoTr = s1;
	}
	if(sNoTr->radius>=0.0f)
		return intersectsCircleTriangles(sNoTr->x, sNoTr->y, sNoTr->radius,
			sTr->arrlen/6, sTr->triangles, sTr->x, sTr->y, sTr->rot);
	float poly[8];
	rect2poly(sNoTr, poly);
	return intersectsPolygonTriangles(4, poly, sTr->arrlen/6, sTr->triangles, sTr->x, sTr->y, sTr->rot);
}
