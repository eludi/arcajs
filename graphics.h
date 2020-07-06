#pragma once

#include "graphicsUtils.h"
#include <stdint.h>
#include <stddef.h>

extern void gfxInit(void* renderer);
extern void* gfxRenderer();
extern void gfxClose();
extern void gfxTextureFiltering(int level);

/// sets current color
extern void gfxColor(uint32_t color);
/// sets current color to an opaque RGB color value
extern void gfxColorRGB(unsigned char r, unsigned char g, unsigned char b);
/// sets current color to an RGBA color value with alpha transparency
extern void gfxColorRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
/// sets current color to a HSLA color value with alpha transparency
/** the function expects hue values between 0.0..360.0, other values between 0.0..1.0 */
extern void gfxColorHSLA(float h, float s, float l, float a);

/// draws a rectangle outline
extern void gfxDrawRect(float x,float y, float w, float h);
/// draws a filled rectangle
extern void gfxFillRect(float x,float y, float w, float h);
/// draws a line
extern void gfxDrawLine(float x0, float y0, float x1, float y1);
/// draws points
extern void gfxDrawPoints(unsigned numCoords, const float* coords);

///@{ image management and rendering:
/// draws an image
extern void gfxDrawImage(size_t img, float x, float y);
/// draws an image scaled
extern void gfxDrawImageScaled(size_t img, float x, float y, float w, float h);
/// draws an image, optionally clipped, scaled, rotated, and mirrored
extern void gfxDrawImageEx(size_t img,
	int srcX, int srcY, int srcW, int srcH,
	float destX, float destY, float destW, float destH,
	float cx, float cy, float angle, int flip);
/// uploads an image from a file to graphics memory and returns a handle (0 in case of an error)
extern size_t gfxImageLoad(const char* fname);
/// uploads an image from a memory buffer and returns a handle (0 in case of an error)
extern size_t gfxImageUpload(const unsigned char* data, int w, int h, int d);
/// releases an image form graphics memory
extern void gfxImageRelease(size_t img);
/// returns width and height of image in pixels
extern void gfxImageDimensions(size_t img, int* w, int* h);
///@}

///@{ text rendering:
extern size_t gfxFontLoad(const char* fname, float fontSize);
extern size_t gfxFontUpload(void* data, unsigned dataSize, float fontSize);
extern void gfxFontRelease(size_t font);
extern void gfxFillText(size_t font, float x, float y, const char* text);
extern void gfxFillTextAlign(size_t font, float x, float y, const char* text, GfxAlign align);
extern void gfxMeasureText(size_t font, const char* text, float* width, float* height, float* ascent, float* descent);
///@}
