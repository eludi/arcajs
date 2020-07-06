#include "sprites.h"
#ifdef _SPRITE_GL
#include "graphicsGL.h"
#else
#  include "graphics.h"
#endif
#include "modules/intersects.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

//------------------------------------------------------------------

Sprite SpriteCreate(SpriteSet* sps) {
	return SpriteCreateFromClippedImage(0,0,sps->imgW,sps->imgH);
}

Sprite SpriteCreateFromTile(SpriteSet* sps, uint16_t tile) {
	uint16_t srcW = sps->imgW/sps->tilesX, srcH=sps->imgH/sps->tilesY;
	uint16_t srcX = (tile%sps->tilesX)*srcW, srcY=(tile/sps->tilesX)*srcH;
	return SpriteCreateFromClippedImage(
		srcX+sps->border, srcY+sps->border, srcW-2*sps->border, srcH-2*sps->border);
}

void SpriteDraw(Sprite* s, size_t img) {
	gfxColorRGBA(s->r, s->g, s->b, s->a);
	float cx = s->cx*s->w, cy = s->cy*s->h;
	gfxDrawImageEx(img, s->srcX, s->srcY, s->srcW, s->srcH,
		s->x-cx, s->y-cy, s->w, s->h, cx, cy, s->rot, s->flip);
}

#ifndef _SPRITE_GL
Sprite SpriteCreateFromClippedImage(uint16_t srcX, uint16_t srcY, uint16_t srcW, uint16_t srcH) {
	Sprite sprite = { 0.0f,0.0f,0.0f, srcW,srcH, 0.5f,0.5f,
		255,255,255,255, srcX,srcY,srcW,srcH, 0, 0, NULL, 0.0f,0.0f,0.0f, -1.0f};
	return sprite;
}

void SpriteUnlink(Sprite* sprite) {
	if(!sprite->parent)
		return;
	SpriteSet* sps = (SpriteSet*)sprite->parent;
	sps->items[sprite->index]=NULL;
	while(sps->numItems && !sps->items[sps->numItems-1])
		--sps->numItems;
	sprite->parent = NULL;
}

void SpriteColorRGBA(Sprite* s, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	s->r = r;
	s->g = g;
	s->b = b;
	s->a = a;
}

void SpriteTile(Sprite* s, uint16_t tile) {
	if(!s->parent)
		return;
	SpriteSet* sps = (SpriteSet*)s->parent;
	if(tile >= sps->tilesX*sps->tilesY)
		return;

	uint16_t srcW = sps->imgW/sps->tilesX, srcH=sps->imgH/sps->tilesY;
	s->srcX = (tile%sps->tilesX)*srcW + sps->border;
	s->srcY = (tile/sps->tilesX)*srcH + sps->border;
	s->srcW = srcW-2*sps->border;
	s->srcH = srcH-2*sps->border;
}

int SpriteIntersectsSprite(Sprite* s1, Sprite* s2) {
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

int SpriteIntersectsPoint(Sprite* s, float x, float y) {
	if(s->radius>=0.0f)
		return intersectsPointCircle(x, y, s->x, s->y, s->radius);
	if(!s->rot) {
		float x1 = s->x - s->cx * s->w, y1 = s->y - s->cy * s->h;
		float x2 = x1 + s->w, y2 = y1 + s->h;
		return intersectsPointAlignedRect(x, y, x1,y1, x2,y2);
	}
	float pt[] = { x, y };
	float ox = -s->cx * s->w, oy = -s->cy * s->h;
	float arr[] = {
		ox, oy,
		ox, oy + s->h,
		ox + s->w, oy + s->h,
		ox + s->w, oy
	};
	intersectsTransfInv(s->x, s->y, s->rot, 2, pt);
	return intersectsPointPolygon(pt[0], pt[1], 4, arr);
}

//--- SpriteSet ----------------------------------------------------

void SpriteSetAppend(SpriteSet* sps, Sprite* sprite) {
	if(sps->numItems == sps->numItemsMax) {
		sps->numItemsMax *= 2;
		sps->items = (Sprite**)realloc(sps->items, sps->numItemsMax*sizeof(Sprite*));
	}
	sps->items[sps->numItems] = sprite;
	sprite->parent = sps;
	sprite->index = sps->numItems++;
}

void SpriteSetUpdate(SpriteSet* sps, double deltaT) {
	for(unsigned i=0; i<sps->numItems; ++i) {
		Sprite* sprite = sps->items[i];
		if(sprite) {
			sprite->x += sprite->velX * deltaT;
			sprite->y += sprite->velY * deltaT;
			sprite->rot += sprite->velRot * deltaT;
		}
	}
}
#endif

SpriteSet SpriteSetCreate(size_t img) {
		return SpriteSetCreateTiled(img, 1, 1, 0);
}

SpriteSet SpriteSetCreateTiled(
	size_t img, uint16_t tilesX, uint16_t tilesY, uint16_t border)
{
	int w, h;
	gfxImageDimensions(img, &w, &h);
	SpriteSet sps = { img, w, h, tilesX, tilesY, border,0, 0, 4, malloc(4*sizeof(Sprite*))};
	return sps;
}

void SpriteSetDraw(SpriteSet* sps) {
	for(unsigned i=0; i<sps->numItems; ++i) {
		Sprite* sprite = sps->items[i];
		if(sprite)
			SpriteDraw(sprite, sps->img);
	}
}
