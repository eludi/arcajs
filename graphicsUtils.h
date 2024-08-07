#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GFX_FLIP_NONE 0
#define GFX_FLIP_X 1
#define GFX_FLIP_Y 2
#define GFX_FLIP_XY 3

typedef enum {
	GFX_ALIGN_LEFT_TOP = 0,
	GFX_ALIGN_CENTER_TOP = 1,
	GFX_ALIGN_RIGHT_TOP = 2,
	GFX_ALIGN_LEFT_MIDDLE = 4,
	GFX_ALIGN_CENTER_MIDDLE = 5,
	GFX_ALIGN_RIGHT_MIDDLE = 6,
	GFX_ALIGN_LEFT_BOTTOM = 8,
	GFX_ALIGN_CENTER_BOTTOM = 9,
	GFX_ALIGN_RIGHT_BOTTOM = 10,
} GfxAlign;

extern uint32_t hsla2rgba(float h, float s, float l, float a);
/// byte swap 32bit unsigned int
extern uint32_t bswap_uint32( uint32_t val );

/// loads a file from file system
extern size_t loadFile(const char* fname, bool isBinary, void** data);
extern unsigned char* svgRasterize(char* svg, float scale, int* w, int* h, int* d);
extern unsigned char* readImageData(const unsigned char* buf, size_t bufsz, int* w, int* h, int* d);
/// convenience function loading an image file from file system and uploading it to graphics memory in a single call
extern uint32_t gfxImageLoad(const char* fname, uint32_t rMask);
