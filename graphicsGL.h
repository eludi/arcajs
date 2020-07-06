#pragma once

#include "graphicsUtils.h"

#include <stddef.h>
#include <stdint.h>

extern int gfxGlInit(int winSzX, int winSzY);
extern void gfxGlFlush();
extern void gfxGlClose();
extern void gfxGlTextureFiltering(int level);

/// sets current color
extern void gfxGlColor(uint32_t color);
/// sets current color to a HSLA color value with alpha transparency
/** the function expects hue values between 0.0..360.0, other values between 0.0..1.0 */
extern void gfxGlColorHSLA(float h, float s, float l, float a);
/// sets current color to an opaque RGB color value
extern void gfxGlColorRGB(unsigned char r, unsigned char g, unsigned char b);
/// sets current color to an RGBA color value with alpha transparency
extern void gfxGlColorRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
/// sets current background/clear color to an RGB color value
extern void gfxGlClearRGB(unsigned char r, unsigned char g, unsigned char b);

/// draws a rectangle outline
extern void gfxGlDrawRect(float x,float y, float w, float h);
/// draws a filled rectangle
extern void gfxGlFillRect(float x,float y, float w, float h);
/// draws a line
extern void gfxGlDrawLine(float x0, float y0, float x1, float y1);
/// draws points
extern void gfxGlDrawPoints(unsigned numCoords, const float* coords);

///@{ image management and rendering:
/// draws an image
extern void gfxGlDrawImage(size_t img, float x, float y);
/// draws an image scaled
extern void gfxGlDrawImageScaled(size_t img, float x, float y, float w, float h);
/// draws an image detail, optionally scaled rotated, and mirrored
extern void gfxGlDrawImageEx(size_t img,
	int srcX, int srcY, int srcW, int srcH,
	float destX, float destY, float destW, float destH,
	float cx, float cy, float angle, int flip);
/// uploads an image from a file to graphics memory and returns a handle (0 in case of an error)
extern size_t gfxGlImageLoad(const char* fname);
/// uploads an image from a memory buffer and returns a handle (0 in case of an error)
extern size_t gfxGlImageUpload(const unsigned char* data, int w, int h, int d);
/// releases an image form graphics memory
extern void gfxGlImageRelease(size_t img);
/// returns width and height of image in pixels
extern void gfxGlImageDimensions(size_t img, int* w, int* h);
///@}

///@{ text rendering:
extern size_t gfxGlFontLoad(const char* fname, float fontSize);
extern size_t gfxGlFontUpload(void* data, unsigned dataSize, float fontSize);
extern void gfxGlFontRelease(size_t font);
extern void gfxGlFillText(size_t font, float x, float y, const char* text);
extern void gfxGlFillTextAlign(size_t font, float x, float y, const char* text, GfxAlign align);
extern void gfxGlMeasureText(size_t font, const char* text, float* width, float* height, float* ascent, float* descent);
///@}
