#include "graphicsUtils.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "external/stb_image.h"

#define NANOSVG_IMPLEMENTATION
#include "external/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "external/nanosvgrast.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static float hue2rgb(float m1, float m2, float h) {
	while(h < 0.0f)
		h += 1.0f;
	while (h > 1.0f)
		h -= 1.0f;

	if(h * 6.0f < 1.0f)
		return m1 + (m2 - m1) * h * 6.0f;
	if(h * 2.0f < 1.0f)
		return m2;
	if(h * 3.0f < 2.0f)
		return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
	return m1;
}

float clampf(float f, float min, float max) {
	return f<min ? min : f>max ? max : f;
}

/// convert from hsla to rgba
/**  the function expects values between 0.0..1.0, hue between 0.0..360.0 */
uint32_t hsla2rgba(float h, float s, float l, float a) {
	uint32_t r,g,b;

	if (s < 5.0e-6)
		r = g = b = l*255;
	else {
		h /= 360.0f;
		while(h < 0.0f)
			h += 1.0f;
		while (h > 1.0f)
			h -= 1.0f;

		float m2 = l <= 0.5f ? l * (s + 1.0f) : l + s - l * s;
		float m1 = l * 2.0f - m2;

		r = clampf(hue2rgb(m1, m2, h + 1.0f / 3.0f), 0.0f,1.0f) * 255.0f;
		g = clampf(hue2rgb(m1, m2, h), 0.0f,1.0f) * 255.0f;
		b = clampf(hue2rgb(m1, m2, h - 1.0f / 3.0f), 0.0f,1.0f) * 255.0f;
	}
	return (r << 24) + (g << 16) + (b << 8) + (uint32_t)(clampf(a,0,1.0f)*255.0f);
}

uint32_t bswap_uint32( uint32_t val ) {
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

unsigned char* svgRasterize(char* svg, float scale, int* w, int* h, int* d) {
	if(!svg || scale <= 0.0f)
		return 0;

	NSVGimage *svgParsed = svg ? nsvgParse(svg, "px", 96.0f) : NULL;
	if (svgParsed == NULL) {
		fprintf(stderr, "Could not parse SVG file.\n");
		return 0;
	}

	unsigned char* img = NULL;
	NSVGrasterizer* rast = nsvgCreateRasterizer();
	if (rast == NULL)
		fprintf(stderr, "Could not initialize SVG rasterizer.\n");
	else {
		*w = svgParsed->width*scale;
		*h = svgParsed->height*scale;
		*d = 4;

		img = malloc(*w * *h * *d);
		if (img == NULL)
			fprintf(stderr, "Could not alloc SVG image buffer.\n");
		else
			nsvgRasterize(rast, svgParsed, 0,0, scale, img, *w, *h, *w*4);
		nsvgDeleteRasterizer(rast);
	}
	nsvgDelete(svgParsed);
	return img;
}

unsigned char* readImageData(const unsigned char* buf, size_t bufsz, int* w, int* h, int* d) {
	unsigned char* data = stbi_load_from_memory(buf, (int)bufsz, w, h, d, 0);
	if(!data)
		w = h = d = 0;
	return data;
}
