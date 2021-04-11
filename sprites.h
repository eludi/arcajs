#pragma once

#include <stddef.h>
#include <stdint.h>

/// structure for managing a single sprite
typedef struct {
	/// x position ordinate
	float x;
	/// y position ordinate
	float y;
	/// rotation ordinate
	float rot;
	/// dimensions
	float w, h;
	/// center offset for placement and rotations
	float cx, cy;
	/// (blend) color
	uint8_t r, g, b, a;

	/// source offset in pixels
	uint16_t srcX, srcY;
	/// source dimensions in pixels
	uint16_t srcW, srcH;
	/// flip/mirror
	int flip;
	/// index within parent
	uint32_t index;
	/// parent entity
	void* parent;
	/// current velocity
	float velX, velY, velRot;
	/// radius for fast coarse-stage collision detection
	float radius;
} Sprite;

/// container for a group of sprites sharing the same texture (atlas)
typedef struct {
	/// texture (atlas) handle
	size_t img;
	uint16_t imgW, imgH;
	uint16_t tilesX, tilesY;
	uint16_t border, unused;
	uint32_t numItems;
	uint32_t numItemsMax;
	Sprite** items;
} SpriteSet;

extern Sprite SpriteCreate(SpriteSet* sps);
extern Sprite SpriteCreateFromClippedImage(uint16_t srcX, uint16_t srcY, uint16_t srcW, uint16_t srcH);
extern Sprite SpriteCreateFromTile(SpriteSet* sps, uint16_t tile);
extern void SpriteUnlink(Sprite* sprite);
extern void SpriteColorRGBA(Sprite* sprite, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
/// sets sprite source to tile n if spriteset contains a regular grid of tiles
extern void SpriteTile(Sprite* sprite, uint16_t tile);
/// tests for intersection of two sprites based on their bounding radius or oriented bounding rectangle
extern int SpriteIntersectsSprite(Sprite* s1, Sprite* s2);
/// tests for intersection of sprite with a point x|y
extern int SpriteIntersectsPoint(Sprite* sprite, float x, float y);

extern SpriteSet SpriteSetCreate(size_t img);
extern SpriteSet SpriteSetCreateTiled(size_t img, uint16_t tilesX, uint16_t tilesY, uint16_t border);
extern void SpriteSetAppend(SpriteSet* sps, Sprite* sprite);
extern void SpriteSetUpdate(SpriteSet* sps, double deltaT);
extern void SpriteSetDraw(SpriteSet* sps);
extern void SpriteSetDrawTile(SpriteSet* sps, uint16_t tile, float x, float y, float w, float h, int align, float rot, int flip);
