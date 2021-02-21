#include "graphics.h"
#include "graphicsUtils.h"

#include "external/stb_truetype.h"
#include "external/stb_image.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern const unsigned char font12x16[];

static SDL_Renderer* renderer = NULL;
static SDL_Texture* defaultFont;
static float lineWidth = 1.0f;
static float camX=0, camY=0, camSc=1.0f;

static size_t uploadDefaultFont(const unsigned char* font, unsigned char wChar, unsigned char hChar) {
	const unsigned texW = 16*wChar, texH = 16*hChar, bytesPerRow = texW/8;
	uint32_t* data = (uint32_t*)malloc(sizeof(uint32_t)*texW*texH);
	memset(data, 0, sizeof(uint32_t)*texW*texH);

	for(unsigned y=0; y<texH; ++y)
		for(unsigned x=0; x<bytesPerRow; ++x) {
			unsigned char byte = font[y*bytesPerRow + x];
			for(int bit=0; bit<8; ++bit)
				if(byte & (1<<(7-bit)))
					data[y*texW + x*8+bit] = 0xffffffff;
		}
	size_t texId = gfxImageUpload((const unsigned char*)data, texW, texH, 4);
	free(data);
	return texId;
}

void gfxInit(void* render) {
	renderer = (SDL_Renderer*)render;
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	defaultFont = (SDL_Texture*)uploadDefaultFont(font12x16, 12, 16);
}

void* gfxRenderer() {
	return renderer;
}

void gfxClose() {
	if(!renderer)
		return;
	SDL_DestroyTexture(defaultFont);
	renderer = NULL;
}

void gfxTextureFiltering(int level) {
	if(level<=0)
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	else if(level==1)
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
}

void gfxOrigin(float ox, float oy) {
	camX = ox;
	camY = oy;
}

void gfxOriginScreen(float ox, float oy) {
	camX = -ox/camSc;
	camY = -oy/camSc;
}

void gfxScale(float sc) {
	camSc = sc;
}


//--- color --------------------------------------------------------

void gfxColor(uint32_t color) {
	uint8_t r = color >> 24, g = color >> 16, b = color >> 8, a = color;
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void gfxColorRGB(unsigned char r, unsigned char g, unsigned char b) {
	SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
}

void gfxColorRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void gfxColorHSLA(float h, float s, float l, float a) {
	gfxColor(hsla2rgba(h, s, l, a));
}

void gfxLineWidth(float w) {
	lineWidth = w;
}

float gfxGetLineWidth() {
	return lineWidth;
}

void gfxClipDisable() {
	SDL_RenderSetClipRect(renderer, NULL);
}

void gfxClipRect(int x,int y, int w, int h) {
	SDL_Rect pos={x,y,w,h};
	SDL_RenderSetClipRect(renderer, &pos);
}

//--- primitives ---------------------------------------------------

void gfxDrawRectW(float x, float y, float w, float h, float lw) {
	SDL_FRect pos={x,y,w,lw};
	SDL_RenderFillRectF(renderer, &pos);
	pos.y += h-lw;
	SDL_RenderFillRectF(renderer, &pos);
	pos.y = y+lw;
	pos.x = x;
	pos.w = lw;
	pos.h = h-2*lw;
	SDL_RenderFillRectF(renderer, &pos);
	pos.x += w-lw;
	SDL_RenderFillRectF(renderer, &pos);
}

void gfxDrawRect(float x, float y, float w, float h) {
	float lw = lineWidth*camSc;
	if(lw!=1.0f)
		gfxDrawRectW((x-camX)*camSc,(y-camY)*camSc,w*camSc,h*camSc, lw);
	else {
		SDL_FRect pos = {(x-camX)*camSc,(y-camY)*camSc,w*camSc,h*camSc};
		SDL_RenderDrawRectF(renderer, &pos);
	}
}

void gfxFillRect(float x, float y, float w, float h) {
	SDL_FRect pos = {(x-camX)*camSc,(y-camY)*camSc,w*camSc,h*camSc};
	SDL_RenderFillRectF(renderer, &pos);
}

void gfxDrawPoints(unsigned n, const float* coords) {
	if(lineWidth==1.0f && camSc==1.0f)
		SDL_RenderDrawPointsF(renderer, (const SDL_FPoint*)coords, n);
	else {
		const float lw = lineWidth * camSc;
		const float lw2 = lw/2;
		SDL_FRect pos = {0,0,lw,lw};
		for(unsigned i=0; i<n; ++i) {
			pos.x = (coords[i*2]-camX)*camSc - lw2;
			pos.y = (coords[i*2+1]-camY)*camSc - lw2;
			SDL_RenderFillRectF(renderer, &pos);
		}
	}
}

void gfxDrawLineW(float x1, float y1, float x2, float y2, float lw) {
	const float lw2 = lw/2;
	SDL_FRect dest;
	if(x1==x2) {
		dest.x = x1-lw2+1;
		dest.w = lw;
		if(y1==y2) {
			dest.y = y1-lw2;
			dest.h = lw;
		}
		else {
			dest.y = y1<y2 ? y1 : y2;
			dest.h = abs(y2-y1);
		}
		SDL_RenderFillRectF(renderer, &dest);
	}
	else if(y1==y2) {
		dest.x = x1 < x2 ? x1: x2;
		dest.y = y1-lw2+1;
		dest.w = abs(x2-x1);
		dest.h = lw;
		SDL_RenderFillRectF(renderer, &dest);
	}
	else {
	uint8_t r, g, b, a;
		SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
		SDL_SetTextureColorMod(defaultFont, r, g, b);
		SDL_SetTextureAlphaMod(defaultFont, a);
		static SDL_Rect src = { 134,212,1,1 };
		double angle = SDL_atan2(y2 - y1, x2 - x1) * 180.0 / M_PI;
		dest.x = x1; dest.y = y1-lw2;
		dest.w = SDL_sqrt(SDL_pow(x2-x1,2)+SDL_pow(y2-y1,2));
		dest.h = lw;
		SDL_FPoint center = {0,lw2};
		SDL_RenderCopyExF(renderer, defaultFont, &src, &dest, angle, &center, SDL_FLIP_NONE);
	}
}

void gfxDrawLine(float x0, float y0, float x1, float y1) {
	const float lw = lineWidth*camSc;
	if(lw==1.0f)
		SDL_RenderDrawLineF(renderer, (x0-camX)*camSc,(y0-camY)*camSc, (x1-camX)*camSc,(y1-camY)*camSc);
	else
		gfxDrawLineW((x0-camX)*camSc,(y0-camY)*camSc, (x1-camX)*camSc,(y1-camY)*camSc, lw);
}

size_t gfxImageUpload(const unsigned char* data, int w, int h, int d) {
	if(!renderer) {
		fprintf(stderr,"gfxImageUpload ERROR: SDL backend not initialized\n");
		return 0;
	}
	if(!data || !w || !h ||!d)
		return 0;
	if(d==1) {
		fprintf(stderr,"ERROR: grayscale texture currently not supported\n");
		return 0;
	}

	Uint32 rmask = 0x000000ff;
	Uint32 gmask = 0x0000ff00;
	Uint32 bmask = 0x00ff0000;
	Uint32 amask = (d==4) ? 0xff000000 : 0;

	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)data, w, h, d*8, w*d, rmask, gmask, bmask, amask);
	if(!surf) {
		SDL_Log("Creating surface failed: %s", SDL_GetError());
		return 0;
	}
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_FreeSurface(surf);
	if(texture) {
		if(d==4)
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_ClearError();
	}
	return (size_t)texture;
}

size_t gfxImageLoad(const char* fname) {
	int w,h,d;
	unsigned char* data = stbi_load(fname, &w, &h, &d, 0);
	if(!data)
		return 0;

	size_t img = gfxImageUpload(data, w, h, d);
	free(data);
	return img;
}

void gfxImageRelease(size_t img) {
	SDL_DestroyTexture((SDL_Texture*)img);
}

void gfxImageDimensions(size_t img, int* w, int* h) {
	SDL_Texture* texture = (SDL_Texture*)img;
	SDL_QueryTexture(texture, 0, 0, w, h);
}

void gfxDrawImage(size_t img, float x, float y) {
	SDL_Texture* texture = (SDL_Texture*)img;
	uint8_t r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetTextureColorMod(texture, r, g, b);
	SDL_SetTextureAlphaMod(texture, a);

	int w,h;
	SDL_QueryTexture(texture, 0, 0, &w, &h);
	SDL_FRect dest = {(x-camX)*camSc,(y-camY)*camSc,w*camSc,h*camSc};
	SDL_RenderCopyF(renderer, texture, 0, &dest);
}

void gfxDrawImageScaled(size_t img, float x, float y, float w, float h) {
	SDL_Texture* texture = (SDL_Texture*)img;
	uint8_t r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetTextureColorMod(texture, r, g, b);
	SDL_SetTextureAlphaMod(texture, a);

	SDL_FRect dest = {(x-camX)*camSc,(y-camY)*camSc,w*camSc,h*camSc};
	SDL_RenderCopyF(renderer, texture, 0, &dest);
}

void gfxDrawImageEx(size_t img,
	int srcX, int srcY, int srcW, int srcH,
	float destX, float destY, float destW, float destH,
	float cx, float cy, float angle, int flip)
{
	SDL_Texture* texture = (SDL_Texture*)img;
	uint8_t r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetTextureColorMod(texture, r, g, b);
	SDL_SetTextureAlphaMod(texture, a);

	SDL_Rect src = { srcX, srcY, srcW, srcH };
	SDL_FRect dest = {(destX-camX)*camSc,(destY-camY)*camSc,destW*camSc,destH*camSc};
	SDL_FPoint ctr = { cx*camSc, cy*camSc };
	SDL_RenderCopyExF(renderer, texture, &src, &dest, angle*180.0f/M_PI, &ctr, flip);
}


//--- font rendering -----------------------------------------------

static int utf8CharLen( unsigned char utf8Char ) {
    if ( utf8Char < 0x80 ) return 1;
    if ( ( utf8Char & 0x20 ) == 0 ) return 2;
    if ( ( utf8Char & 0x10 ) == 0 ) return 3;
    if ( ( utf8Char & 0x08 ) == 0 ) return 4;
    if ( ( utf8Char & 0x04 ) == 0 ) return 5;
    return 6;
}

static unsigned char utf8ToLatin1( const char *s, size_t *readIndex ) {
    int len = utf8CharLen( (unsigned char)( s[ *readIndex ] ) );
    if ( len == 1 ) {
        unsigned char c = (unsigned char)s[ *readIndex ];
		if(c)
        	(*readIndex)++;
        return c;
    }

    unsigned int v = ( s[ *readIndex ] & ( 0xff >> ( len + 1 ) ) ) << ( ( len - 1 ) * 6 );
    (*readIndex)++;
    for ( len-- ; len > 0 ; len-- )  {
        v |= ( (unsigned char)( s[ *readIndex ] ) - 0x80 ) << ( ( len - 1 ) * 6 );
        (*readIndex)++;
    }
    return ( v > 0xff ) ? 0 : (unsigned char)v;
}


static void gfxDrawBitmapFont(int x, int y, const char* str) {
	const unsigned char wChar = 12, hChar = 16;
	SDL_Texture* texture = defaultFont;

	uint8_t r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetTextureColorMod(texture, r, g, b);
	SDL_SetTextureAlphaMod(texture, a);

	SDL_Rect src  = { 0, 0, wChar, hChar };
	SDL_Rect dest = { (x-camX)*camSc, (y-camY)*camSc, wChar, hChar };
	for(size_t readIndex=0; str[readIndex]; x += wChar) {
		unsigned char c = utf8ToLatin1(str, &readIndex);
		if(c) {
			src.x = (c%16)*wChar;
			src.y = (c/16)*hChar;
			SDL_RenderCopyEx(renderer, texture, &src, &dest, 0, NULL, SDL_FLIP_NONE);
		}
		dest.x += wChar;
	}
}

#define GLYPH_MIN 32
#define GLYPH_COUNT 224

typedef struct {
	SDL_Texture* texture;
	int texW, texH;
	float height, ascent, descent;
	stbtt_bakedchar glyphData[GLYPH_COUNT];
} FontSpec;

size_t gfxFontLoad(const char* fname, float fontHeight) {
	FILE *ff=fopen(fname, "rb");
	if (!ff) {
		fprintf(stderr, "font file not found\n");
		return 0;
	}
	fseek(ff,0,SEEK_END);
	size_t fsize = ftell(ff);
	rewind(ff);
	unsigned char* ttf_buffer = (unsigned char*)malloc(fsize);

	size_t ret = 0;
	if(fread(ttf_buffer, 1,fsize, ff)==fsize)
		ret = gfxFontUpload(ttf_buffer, fsize, fontHeight);
	if(!ret)
		fprintf(stderr, "font file read error\n");
	fclose(ff);
	free(ttf_buffer);
	return ret;
}

size_t gfxFontUpload(void* fontData, unsigned dataSize, float fontHeight) {
	// render glyphs into bitmap buffer:
	const unsigned texW = fontHeight>48.0f? 1024 : 512, texH = fontHeight>64.0f ? 2048 : 1024;
	uint8_t* bitmap = malloc(texW*texH);
	stbtt_bakedchar glyphData[GLYPH_COUNT];
	stbtt_BakeFontBitmap(fontData,0, fontHeight, bitmap, texW, texH, GLYPH_MIN, GLYPH_COUNT, glyphData);

	// transfer data/metrics to texture and font spec:
	uint8_t* pixels = malloc(texW*texH*4);
	memset(pixels, 0, texW*texH*4);

	for(unsigned y=0; y<texH; ++y) for(unsigned x=0; x<texW; ++x) {
		size_t pos = y*texW+x;
		uint8_t px = bitmap[pos];
		if(!px)
			continue;
		pixels[pos*4+0] = pixels[pos*4+1] = pixels[pos*4+2] = 255;
		pixels[pos*4+3] = px;
	}
	size_t texId = gfxImageUpload(pixels, texW, texH, 4);
	free(pixels);
	free(bitmap);

	FontSpec* fnt = (FontSpec*)malloc(sizeof(FontSpec));
	fnt->texture = (SDL_Texture*)texId;
	fnt->texW = texW;
	fnt->texH = texH;
	fnt->height = fontHeight;
	memcpy(fnt->glyphData, glyphData, GLYPH_COUNT*sizeof(stbtt_bakedchar));
	float lineGap;
	stbtt_GetScaledFontVMetrics(fontData,0, fontHeight, &fnt->ascent, &fnt->descent, &lineGap);
	return (size_t)fnt;
}

void gfxFillText(size_t font, float x, float y, const char* text) {
	if(!font) {
		gfxDrawBitmapFont(x, y, text);
		return;
	}
	FontSpec* fnt = (FontSpec*)font;
	uint8_t r, g, b, a;
	SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
	SDL_SetTextureColorMod(fnt->texture, r, g, b);
	SDL_SetTextureAlphaMod(fnt->texture, a);

	y += fnt->ascent;

	for(size_t readIndex=0; text[readIndex]; ) {
		unsigned char c = utf8ToLatin1(text, &readIndex);
		if(c<GLYPH_MIN || c>=GLYPH_MIN+GLYPH_COUNT)
			c = ' '; // render as space

		const stbtt_bakedchar* glyph = &fnt->glyphData[c - GLYPH_MIN];
		int w = glyph->x1 - glyph->x0, h = glyph->y1 - glyph->y0;
		SDL_Rect src  = { .x = glyph->x0, .y = glyph->y0, .w = w, .h = h };
		SDL_Rect dest = { (x + glyph->xoff + 0.5f - camX)*camSc,
			(y + glyph->yoff + 0.5f - camY)*camSc, w*camSc, h*camSc };
		SDL_RenderCopyEx(renderer, fnt->texture, &src, &dest, 0, NULL, SDL_FLIP_NONE);
		x += glyph->xadvance;
	}
}

void gfxFillTextAlign(size_t font, float x, float y, const char* text, GfxAlign align) {
	if(align!=GFX_ALIGN_LEFT_TOP) {
		float w,h;
		gfxMeasureText(font, text, &w, &h, NULL, NULL);
		switch(align) {
		case GFX_ALIGN_LEFT_TOP: break;
		case GFX_ALIGN_CENTER_TOP: x -= w/2; break;
		case GFX_ALIGN_RIGHT_TOP: x -= w; break;
		case GFX_ALIGN_LEFT_MIDDLE: y -= h/2; break;
		case GFX_ALIGN_CENTER_MIDDLE: x -= w/2; y -= h/2; break;
		case GFX_ALIGN_RIGHT_MIDDLE: x -= w; y -= h/2; break;
		case GFX_ALIGN_LEFT_BOTTOM: y -= h; break;
		case GFX_ALIGN_CENTER_BOTTOM: x -= w/2; y -= h; break;
		case GFX_ALIGN_RIGHT_BOTTOM: x -= w; y -= h; break;
		}
	}
	gfxFillText(font, x, y, text);
}

void gfxMeasureText(size_t font, const char* text, float* width, float* height, float* ascent, float* descent) {
	if(!font) {
		if(width)
			*width = strlen(text)*12.0f;
		if(height)
			*height = 16.0f;
		if(ascent)
			*ascent = 12.0f;
		if(descent)
			*descent = -4.0f;
		return;
	}
	FontSpec* fnt = (FontSpec*)font;
	if(width) {
		*width = 0.0f;
		if(text) for(size_t readIndex=0; text[readIndex]; ) {
			unsigned char c = utf8ToLatin1(text, &readIndex);
			if(c<GLYPH_MIN || c>=GLYPH_MIN+GLYPH_COUNT)
				c = ' '; // render as space
			c-=GLYPH_MIN;
			const stbtt_bakedchar* glyph = &fnt->glyphData[c];
			*width += glyph->xadvance;
		}
	}
	if(height)
		*height = fnt->height;
	if(ascent)
		*ascent = fnt->ascent;
	if(descent)
		*descent = fnt->descent;
}

void gfxFontRelease(size_t font) {
	FontSpec* fnt = (FontSpec*)font;
	SDL_DestroyTexture(fnt->texture);
	free(fnt);
}
