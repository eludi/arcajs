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
#include <stddef.h>
#include <math.h>

#ifdef __ANDROID__
#include <SDL_rwops.h>
#endif

extern uint32_t gfxImageUpload(const unsigned char* data, int w, int h, int d, uint32_t rMask);

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

typedef struct {
	uint32_t value;
	const char*name;
} NamedColor;

static NamedColor namedColors[] = {
	{ 0xf0f8ffff, "aliceblue" },
	{ 0xfaebd7ff, "antiquewhite" },
	{ 0x00ffffff, "aqua" },
	{ 0x7fffd4ff, "aquamarine" },
	{ 0xf0ffffff, "azure" },
	{ 0xf5f5dcff, "beige" },
	{ 0xffe4c4ff, "bisque" },
	{ 0x000000ff, "black" },
	{ 0xffebcdff, "blanchedalmond" },
	{ 0x0000ffff, "blue" },
	{ 0x8a2be2ff, "blueviolet" },
	{ 0xa52a2aff, "brown" },
	{ 0xdeb887ff, "burlywood" },
	{ 0x5f9ea0ff, "cadetblue" },
	{ 0x7fff00ff, "chartreuse" },
	{ 0xd2691eff, "chocolate" },
	{ 0xff7f50ff, "coral" },
	{ 0x6495edff, "cornflowerblue" },
	{ 0xfff8dcff, "cornsilk" },
	{ 0xdc143cff, "crimson" },
	{ 0x00ffffff, "cyan" },
	{ 0x00008bff, "darkblue" },
	{ 0x008b8bff, "darkcyan" },
	{ 0xb8860bff, "darkgoldenrod" },
	{ 0xa9a9a9ff, "darkgray" },
	{ 0x006400ff, "darkgreen" },
	{ 0xa9a9a9ff, "darkgrey" },
	{ 0xbdb76bff, "darkkhaki" },
	{ 0x8b008bff, "darkmagenta" },
	{ 0x556b2fff, "darkolivegreen" },
	{ 0xff8c00ff, "darkorange" },
	{ 0x9932ccff, "darkorchid" },
	{ 0x8b0000ff, "darkred" },
	{ 0xe9967aff, "darksalmon" },
	{ 0x8fbc8fff, "darkseagreen" },
	{ 0x483d8bff, "darkslateblue" },
	{ 0x2f4f4fff, "darkslategray" },
	{ 0x2f4f4fff, "darkslategrey" },
	{ 0x00ced1ff, "darkturquoise" },
	{ 0x9400d3ff, "darkviolet" },
	{ 0xff1493ff, "deeppink" },
	{ 0x00bfffff, "deepskyblue" },
	{ 0x696969ff, "dimgray" },
	{ 0x696969ff, "dimgrey" },
	{ 0x1e90ffff, "dodgerblue" },
	{ 0xd19275ff, "feldspar" },
	{ 0xb22222ff, "firebrick" },
	{ 0xfffaf0ff, "floralwhite" },
	{ 0x228b22ff, "forestgreen" },
	{ 0xff00ffff, "fuchsia" },
	{ 0xdcdcdcff, "gainsboro" },
	{ 0xf8f8ffff, "ghostwhite" },
	{ 0xffd700ff, "gold" },
	{ 0xdaa520ff, "goldenrod" },
	{ 0x808080ff, "gray" },
	{ 0x008000ff, "green" },
	{ 0xadff2fff, "greenyellow" },
	{ 0x808080ff, "grey" },
	{ 0xf0fff0ff, "honeydew" },
	{ 0xff69b4ff, "hotpink" },
	{ 0xcd5c5cff, "indianred" },
	{ 0x4b0082ff, "indigo" },
	{ 0xfffff0ff, "ivory" },
	{ 0xf0e68cff, "khaki" },
	{ 0xe6e6faff, "lavender" },
	{ 0xfff0f5ff, "lavenderblush" },
	{ 0x7cfc00ff, "lawngreen" },
	{ 0xfffacdff, "lemonchiffon" },
	{ 0xadd8e6ff, "lightblue" },
	{ 0xf08080ff, "lightcoral" },
	{ 0xe0ffffff, "lightcyan" },
	{ 0xfafad2ff, "lightgoldenrodyellow" },
	{ 0xd3d3d3ff, "lightgray" },
	{ 0x90ee90ff, "lightgreen" },
	{ 0xd3d3d3ff, "lightgrey" },
	{ 0xffb6c1ff, "lightpink" },
	{ 0xffa07aff, "lightsalmon" },
	{ 0x20b2aaff, "lightseagreen" },
	{ 0x87cefaff, "lightskyblue" },
	{ 0x8470ffff, "lightslateblue" },
	{ 0x778899ff, "lightslategray" },
	{ 0x778899ff, "lightslategrey" },
	{ 0xb0c4deff, "lightsteelblue" },
	{ 0xffffe0ff, "lightyellow" },
	{ 0x00ff00ff, "lime" },
	{ 0x32cd32ff, "limegreen" },
	{ 0xfaf0e6ff, "linen" },
	{ 0xff00ffff, "magenta" },
	{ 0x800000ff, "maroon" },
	{ 0x66cdaaff, "mediumaquamarine" },
	{ 0x0000cdff, "mediumblue" },
	{ 0xba55d3ff, "mediumorchid" },
	{ 0x9370dbff, "mediumpurple" },
	{ 0x3cb371ff, "mediumseagreen" },
	{ 0x7b68eeff, "mediumslateblue" },
	{ 0x00fa9aff, "mediumspringgreen" },
	{ 0x48d1ccff, "mediumturquoise" },
	{ 0xc71585ff, "mediumvioletred" },
	{ 0x191970ff, "midnightblue" },
	{ 0xf5fffaff, "mintcream" },
	{ 0xffe4e1ff, "mistyrose" },
	{ 0xffe4b5ff, "moccasin" },
	{ 0xffdeadff, "navajowhite" },
	{ 0x000080ff, "navy" },
	{ 0xfdf5e6ff, "oldlace" },
	{ 0x808000ff, "olive" },
	{ 0x6b8e23ff, "olivedrab" },
	{ 0xffa500ff, "orange" },
	{ 0xff4500ff, "orangered" },
	{ 0xda70d6ff, "orchid" },
	{ 0xeee8aaff, "palegoldenrod" },
	{ 0x98fb98ff, "palegreen" },
	{ 0xafeeeeff, "paleturquoise" },
	{ 0xdb7093ff, "palevioletred" },
	{ 0xffefd5ff, "papayawhip" },
	{ 0xffdab9ff, "peachpuff" },
	{ 0xcd853fff, "peru" },
	{ 0xffc0cbff, "pink" },
	{ 0xdda0ddff, "plum" },
	{ 0xb0e0e6ff, "powderblue" },
	{ 0x800080ff, "purple" },
	{ 0xff0000ff, "red" },
	{ 0xbc8f8fff, "rosybrown" },
	{ 0x4169e1ff, "royalblue" },
	{ 0x8b4513ff, "saddlebrown" },
	{ 0xfa8072ff, "salmon" },
	{ 0xf4a460ff, "sandybrown" },
	{ 0x2e8b57ff, "seagreen" },
	{ 0xfff5eeff, "seashell" },
	{ 0xa0522dff, "sienna" },
	{ 0xc0c0c0ff, "silver" },
	{ 0x87ceebff, "skyblue" },
	{ 0x6a5acdff, "slateblue" },
	{ 0x708090ff, "slategray" },
	{ 0x708090ff, "slategrey" },
	{ 0xfffafaff, "snow" },
	{ 0x00ff7fff, "springgreen" },
	{ 0x4682b4ff, "steelblue" },
	{ 0xd2b48cff, "tan" },
	{ 0x008080ff, "teal" },
	{ 0xd8bfd8ff, "thistle" },
	{ 0xff6347ff, "tomato" },
	{ 0x0, "transparent" },
	{ 0x40e0d0ff, "turquoise" },
	{ 0xee82eeff, "violet" },
	{ 0xd02090ff, "violetred" },
	{ 0xf5deb3ff, "wheat" },
	{ 0xffffffff, "white" },
	{ 0xf5f5f5ff, "whitesmoke" },
	{ 0xffff00ff, "yellow" },
	{ 0x9acd32ff, "yellowgreen" },
	{ 0x0, NULL }
};

uint32_t cssColor(const char* str) {
	uint32_t color = 0x000000ff;
	if(!str)
		return color;
	size_t len = strlen(str);
	if(str[0]=='#') {
		++str;
		if(len==7)
			color += strtol(str, NULL, 16) << 8;
		else if(len==4) for(uint32_t shl=24; shl>0; shl-=8, ++str) {
			uint32_t b = *str;
			if(b>='a' && b<='f')
				b -= 'a'-10;
			else if(b>='A' && b<='F')
				b -= 'A'-10;
			else
				b -= '0';
			b += b << 4;
			color += b << shl;
		}
	}
	else if(len>5 && str[0]=='r' && str[1]=='g' && str[2]== 'b') { // rgb(a)
		int hasAlpha = str[3]=='a';
		uint32_t r,g,b;
		float a = 1.0f;
		if((hasAlpha && sscanf(str, "rgba(%u,%u,%u,%f)",&r,&g,&b,&a)==4)
			|| (!hasAlpha && sscanf(str, "rgb(%u,%u,%u)",&r,&g,&b)==3))
			color = (r << 24) + (g << 16) + (b << 8) + (uint32_t)(a*255.0f);
	}
	else if(len>5 && str[0]=='h' && str[1]=='s' && str[2]== 'l') { // hsl(a)
		int hasAlpha = str[3]=='a';
		float h,s,l, a = 1.0f;
		if((hasAlpha && sscanf(str, "hsla(%f,%f%%,%f%%,%f)",&h,&s,&l,&a)==4)
			|| (!hasAlpha && sscanf(str, "hsl(%f,%f%%,%f%%)",&h,&s,&l)==3))
			return hsla2rgba(h, s/100.0f, l/100.0f,a);
	}
	else for(NamedColor* namedColor = namedColors; namedColor->name!=NULL; ++namedColor)
		if(strcmp(str, namedColor->name) == 0) // todo binary search
			return namedColor->value;
	return color;
}

uint32_t bswap_uint32( uint32_t val ) {
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

size_t loadFile(const char* fname, bool isBinary, void** data) {
#ifdef __ANDROID__
	SDL_RWops *io = SDL_RWFromFile(fname, "rb");
	if (!io || io->size(io)<=0)
		return 0;

	size_t sz = io->size(io);
	*data = malloc(sz + (isBinary ? 0 : 1));
	int numRead = SDL_RWread(io, *data, sz, 1);
	SDL_RWclose(io);

	if(numRead!=1) {
		free(*data);
		*data = 0;
		return 0;
	}
#else
	FILE *fp;
	fp = fopen(fname, "rb");
	if (!fp)
		return 0;

	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	rewind(fp);

	*data = malloc(sz);
	size_t szRead = fread(*data, 1, sz, fp);
	fclose(fp);

	if(sz!=szRead) {
		free(*data);
		*data = 0;
		return 0;
	}
#endif
	if(!isBinary)
		((char*)(*data))[sz] = 0;
	return sz;
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

uint32_t gfxImageLoad(const char* fname, uint32_t rMask) {
	void* buf;
	size_t sz = loadFile(fname, true, &buf);
	if(!sz)
		return 0;

	int w,h,d;
	unsigned char *data = stbi_load_from_memory(buf, (int)sz, &w, &h, &d, 0);
	free(buf);
	if(!data)
		return 0;

	uint32_t img = gfxImageUpload(data, w, h, d, rMask);
	free(data);
	return img;
}

