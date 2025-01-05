#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GFX_IMG_CIRCLE 1
#define GFX_IMG_SQUARE 2

///@{ setup and resource management:
extern void gfxInit(uint16_t vpWidth, uint16_t vpHeight, float resourceScale, void *arg);
extern void gfxClose();
extern void gfxTextureFiltering(int level);

/// uploads an image from a memory buffer and returns a handle (0 in case of an error)
/** rMask parameter allows to specify data layout as RGB(A) =0xff000000 or (A)BGR = 0xff */
extern uint32_t gfxImageUpload(const unsigned char* data, int w, int h, int d, uint32_t rMask);
/// loads an SVG image from string to graphics memory and returns a handle
extern uint32_t gfxSVGUpload(const char* svg, size_t svgSz, float scale);
/// defines an image tile based on an already existing parent image
extern uint32_t gfxImageTile(uint32_t parent, int x, int y, int w, int h);
/// defines image tiles based on an already existing parent image by specifing the number of tiles in x and y dimension and a border width
/** \return image handle of the first (left upper) tile */
extern uint32_t gfxImageTileGrid(uint32_t parent, uint16_t tilesX, uint16_t tilesY, uint16_t border);
/// sets rotation center relative to width/height, 0.0|0.0 means upper left corner 1.0|1.0 lower right corner
extern void gfxImageSetCenter(uint32_t img, float cx, float cy);
/// returns width and height of image in pixels
extern void gfxImageDimensions(uint32_t img, int* w, int* h);
/// releases an image from graphics memory
extern void gfxImageRelease(uint32_t img);
/// uploads a TTF font resource and returns handle
extern uint32_t gfxFontUpload(void* data, size_t dataSize, float fontSize);
/// creates font resource based on a texture containing a fixed 16x16 grid of glyphs
extern uint32_t gfxFontFromImage(uint32_t img, int margin);
/// releases a font from graphics memory
extern void gfxFontRelease(uint32_t font);

extern size_t gfxCanvasCreate(int w, int h, uint32_t color);
extern uint32_t gfxCanvasUpload(size_t canvas);
extern uint32_t gfxVideoCanvasCreate(int w, int h);
extern int gfxVideoCanvasUpdate(uint32_t img,
	const uint8_t* yData, int yPitch, const uint8_t* uData, int uPitch, const uint8_t* vData, int vPitch);
///@}

///@{ render state/context:
extern void gfxBeginFrame(uint32_t clearColor);
extern void gfxEndFrame();
/// resets state to its initial values
extern void gfxStateReset();
/// pushes current state onto a stack
/** 7 stacked states supported */
extern void gfxStateSave();
/// restores previous state from stack
extern void gfxStateRestore();

/// multiplies current transformation with this additional transformation
extern void gfxTransform(float x, float y, float rot, float sc);
/// multiplies current transformation with this additional 3D transformation
extern void gfxTransf3d(float x, float y, float z, float rotX, float rotY, float rotZ, float sc);
/// sets current color
extern void gfxColor(uint32_t color);
/// sets current line width
extern void gfxLineWidth(float w);
/// returns current line width
extern float gfxGetLineWidth();
/// sets viewport / clipping rectangle (in screen coordinates), use negative width/height to disable clipping
extern void gfxClipRect(int x, int y, int w, int h);
/// sets current blend mode
extern void gfxBlend(int mode);
/// returns current blend mode
extern int gfxGetBlend();
///@}

///@{ basic drawing operations:
/// draws a rectangle outline
extern void gfxDrawRect(float x, float y, float w, float h);
/// draws a filled rectangle
extern void gfxFillRect(float x, float y, float w, float h);
/// draws a line
extern void gfxDrawLine(float x0, float y0, float x1, float y1);
/// draws a line strip
extern void gfxDrawLineStrip(uint32_t numCoords, const float* coords);
/// draws a closed line loop
extern void gfxDrawLineLoop(uint32_t numCoords, const float* coords);
// draws an array of points using a point sprite (for example GFX_IMG_CIRCLE) and current line width as size
extern void gfxDrawPoints(uint32_t numCoords, const float* coords, uint32_t img);
/// draws a filled triangle
extern void gfxFillTriangle(float x0, float y0, float x1, float y1, float x2, float y2);
/// draws an image
extern void gfxDrawImage(uint32_t img, float x, float y, float rot, float sc, int flip);
/// draws a stretched image
extern void gfxStretchImage(uint32_t img, float x, float y, float w, float h);
/// renders text
extern void gfxFillText(uint32_t font, float x, float y, const char* text);
/// renders aligned text
extern void gfxFillTextAlign(uint32_t font, float x, float y, const char* str, int align);
/// determines text dimensions
extern void gfxMeasureText(uint32_t font, const char* text, float* width, float* height, float* ascent, float* descent);
///@}

///@{ experimental extensions:
/// draws a tile array
/** all tile images need to be based on the same parent image.
 * \param imgBase is the image handle of the first tile
 * \param colors optional color array, one color per tile. Set NULL to uniformly use the current drawing color.*/
extern void gfxDrawTiles(uint16_t tilesX, uint16_t tilesY, uint32_t stride,
	uint32_t imgBase, const uint32_t* imgOffsets, const uint32_t* colors);

/// symbolic names of array components
typedef enum {
	GFX_COMP_IMG_OFFSET = 1<<0,
	GFX_COMP_ROT = 1<<3,
	GFX_COMP_SCALE = 1<<4,
	GFX_COMP_COLOR_R = 1<<5,
	GFX_COMP_COLOR_G = 1<<6,
	GFX_COMP_COLOR_B = 1<<7,
	GFX_COMP_COLOR_A = 1<<8,
	GFX_COMP_COLOR_RGBA = 15<<5,
} gfxArrayComponents;	
/// draws multiple images based on a data array
/**
 * Useful for rendering many sprites instances or particles.
 * 
 * data array components and corresponding flags:
 * - imgOffset COMP_IMG_OFFSET
 * - x
 * - y
 * - rot COMP_ROT
 * - scale COMP_SCALE
 * - colorR COMP_COLOR_R
 * - colorG COMP_COLOR_G
 * - colorB COMP_COLOR_B
 * - colorA COMP_COLOR_A
 * 
 * Use colorA = 0 to disable an instance. Use app.transformArray() for efficient array updates.
 */
extern void gfxDrawImages(uint32_t imgBase, uint32_t numInstances, uint32_t stride, const gfxArrayComponents comps, const float* arr);
/// draws filled triangles with optional vertex colors and indices
extern void gfxFillTriangles(uint32_t numVertices, const float* coords,
	const uint32_t* colors, uint32_t numIndices, const uint32_t* indices);
/// draws textured triangles with optional vertex colors and indices
extern void gfxTexTriangles(uint32_t img, uint32_t numVertices, const float* coords, const float* uvCoords,
	const uint32_t* colors, uint32_t numIndices, const uint32_t* indices);
///@}
