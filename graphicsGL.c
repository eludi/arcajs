#include "graphicsGL.h"

#define GL_GLEXT_PROTOTYPES

#if defined(_WIN32)
#  include <GL/gl3w.h>
#elif !defined(GRAPHICS_API_OPENGL_ES2)
#  include <SDL_opengl.h>
#endif

#define RLGL_STANDALONE
#define RLGL_IMPLEMENTATION
#define RLGL_NO_GLAD
#include "external/rlgl.noglad.h"

#include "external/stb_truetype.h"
#include "external/stb_image.h"

#include <stdlib.h>

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif
#ifndef RAD2DEG
#  define RAD2DEG (180.0f/M_PI)
#endif

extern const unsigned char font12x16[];
static size_t defaultFont;
static int textureFilter=0;

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
	size_t texId = gfxGlImageUpload((const unsigned char*)data, texW, texH, 4);
	free(data);
	return texId;
}


int gfxGlInit(int winSzX, int winSzY) {
#if defined(_WIN32)
	if (gl3wInit()!=0) {
		fprintf(stderr, "failed to initialize OpenGL\n");
		return -1;
	}
	if (!gl3wIsSupported(3, 3)) {
		fprintf(stderr, "OpenGL 3.3 not supported\n");
		return -1;
	}
#endif
	rlglInit(winSzX, winSzY);
	rlViewport(0, 0, winSzX, winSzY);
	rlMatrixMode(RL_PROJECTION);
	rlLoadIdentity();
	rlOrtho(0.0,winSzX,winSzY,0.0,-1,1);
	rlMatrixMode(RL_MODELVIEW);
	rlLoadIdentity();

	defaultFont = uploadDefaultFont(font12x16, 12, 16);
	rlColor4ub(255,255,255,255);
	return 0;
}

void gfxGlClose() {
	gfxGlImageRelease(defaultFont);
	rlglClose();
}

void gfxGlTextureFiltering(int level) {
	textureFilter = level>0 ? 1 : 0;
}

void gfxGlClearRGB(unsigned char r, unsigned char g, unsigned char b) {
	rlClearColor(r, g, b, 255);
	rlClearScreenBuffers();
}

void gfxGlFlush() {
	rlglDraw();
}

void gfxGlColor(uint32_t color) {
	uint8_t r = color >> 24, g = color >> 16, b = color >> 8, a = color;
	rlColor4ub(r, g, b, a);
}

void gfxGlColorRGB(unsigned char r, unsigned char g, unsigned char b) {
	rlColor4ub(r, g, b, 255);
}

void gfxGlColorRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	rlColor4ub(r, g, b, a);
}

void gfxGlColorHSLA(float h, float s, float l, float a) {
	gfxGlColor(hsla2rgba(h, s, l, a));
}

void gfxGlDrawRect(float x,float y, float w, float h) {
	rlBegin(RL_LINES);
	rlVertex2f(x, y);
	rlVertex2f(x, y + h);

	rlVertex2f(x, y + h);
	rlVertex2f(x + w, y + h);

	rlVertex2f(x + w, y + h);
	rlVertex2f(x + w, y);

	rlVertex2f(x + w, y);
	rlVertex2f(x, y);
	rlEnd();
}

void gfxGlFillRect(float x, float y, float w, float h) {
	Texture2D* texture = (Texture2D*)(defaultFont);

	const int srcX = (11*12 + 1), srcY = (13*16 + 1);
	const float srcW = 192.0f, srcH = 256.0f;

	const float tx0 = srcX/srcW;
	const float tx1 = (srcX + 1)/srcW;
	const float ty0 = srcY/srcH;
	const float ty1 =(srcY + 1)/srcH;

	rlEnableTexture(texture->id);
	rlBegin(RL_QUADS);
	rlTexCoord2f(tx0, ty0);
	rlVertex2f(x, y);
	rlTexCoord2f(tx0, ty1);
	rlVertex2f(x, y+h);
	rlTexCoord2f(tx1, ty1);
	rlVertex2f(x+w, y+h);
	rlTexCoord2f(tx1, ty0);
	rlVertex2f(x+w, y);
	rlEnd();
	rlDisableTexture();
}

void gfxGlDrawLine(float x0, float y0, float x1, float y1) {
	rlBegin(RL_LINES);
	rlVertex2f(x0, y0);
	rlVertex2f(x1, y1);
	rlEnd();
}

void gfxGlDrawPoints(unsigned n, const float* coords) {
	for(unsigned i=0; i<n; ++i)
		gfxGlFillRect(coords[i*2], coords[i*2+1], 1,1);
}

//--- image rendering ----------------------------------------------

void gfxGlDrawImage(size_t img, float x, float y) {
	Texture2D* texture = (Texture2D*)(img);
	if (texture->id > 0)
		gfxGlDrawImageScaled(img, x, y, texture->width, texture->height);
}

void gfxGlDrawImageScaled(size_t img, float x, float y, float w, float h) {
	Texture2D* texture = (Texture2D*)(img);
	if (texture->id <= 0)
		return;

	rlEnableTexture(texture->id);
	rlBegin(RL_QUADS);

	rlTexCoord2f(0.0f, 0.0f);
	rlVertex2f(x, y);
	rlTexCoord2f(0.0f, 1.0f);
	rlVertex2f(x, y+h);
	rlTexCoord2f(1.0f, 1.0f);
	rlVertex2f(x+w, y+h);
	rlTexCoord2f(1.0f, 0.0f);
	rlVertex2f(x+w, y);

	rlEnd();
	rlDisableTexture();
}

void gfxGlDrawImageEx(size_t img,
	int srcX, int srcY, int srcW, int srcH,
	float destX, float destY, float destW, float destH,
	float cx, float cy, float angle, int flip)
{
	Texture2D* texture = (Texture2D*)(img);
	if (!texture || texture->id <= 0)
		return;

	float width = (float)texture->width;
	float height = (float)texture->height;
	float tx0,tx1,ty0,ty1;
	if (flip & GFX_FLIP_X) {
		tx0 = (srcX + srcW)/width;
		tx1 = srcX/width;
	}
	else {
		tx0 = srcX/width;
		tx1 = (srcX + srcW)/width;
	}
	if (flip & GFX_FLIP_Y) {
		ty0 = (srcY + srcH)/height;
		ty1 = srcY/height;
	}
	else {
		ty0 = srcY/height;
		ty1 =(srcY + srcH)/height;
	}

	rlEnableTexture(texture->id);
	rlPushMatrix();
	rlTranslatef(destX+cx, destY+cy, 0.0f);
	rlRotatef(angle*RAD2DEG, 0.0f, 0.0f, 1.0f);
	rlTranslatef(-cx, -cy, 0.0f);

	rlBegin(RL_QUADS);
	rlTexCoord2f(tx0, ty0);
	rlVertex2f(0.0f, 0.0f);
	rlTexCoord2f(tx0, ty1);
	rlVertex2f(0.0f, destH);
	rlTexCoord2f(tx1, ty1);
	rlVertex2f(destW, destH);
	rlTexCoord2f(tx1, ty0);
	rlVertex2f(destW, 0.0f);
	rlEnd();

	rlPopMatrix();
	rlDisableTexture();
}

size_t gfxGlImageUpload(const unsigned char* data, int w, int h, int d) {
	if(!data || !w || !h ||!d)
		return 0;
	
	int format;
	switch(d) {
	case 1:
		format = UNCOMPRESSED_GRAYSCALE; break;
	case 2:
		format = UNCOMPRESSED_GRAY_ALPHA; break;
	case 3:
		format = UNCOMPRESSED_R8G8B8; break;
	case 4:
		format = UNCOMPRESSED_R8G8B8A8; break;
	default:
		return 0;
	}
	int mipmaps = 1;

	Texture2D* texture = (Texture2D*)malloc(sizeof(Texture2D));

	texture->id = rlLoadTexture((void*)data, w, h, format, mipmaps);
	texture->width = w;
	texture->height = h;
	texture->mipmaps = mipmaps;
	texture->format = format;
	rlTextureParameters(texture->id, RL_TEXTURE_MAG_FILTER,
		textureFilter ? RL_FILTER_LINEAR : RL_FILTER_NEAREST);
	rlTextureParameters(texture->id, RL_TEXTURE_WRAP_S, RL_WRAP_CLAMP);
	rlTextureParameters(texture->id, RL_TEXTURE_WRAP_T, RL_WRAP_CLAMP);

	return (size_t)texture;
}


size_t gfxGlImageLoad(const char* fname) {
	int w,h,d;
	unsigned char* data = stbi_load(fname, &w, &h, &d, 0);
	if(!data)
		return 0;

	size_t img = gfxGlImageUpload(data, w, h, d);
	free(data);
	return img;
}

void gfxGlImageRelease(size_t img) {
	Texture2D* texture = (Texture2D*)img;
	rlUnloadTexture(texture->id);
	free(texture);
}

void gfxGlImageDimensions(size_t img, int* w, int* h) {
	Texture2D* texture = (Texture2D*)img;
	if(texture) {
		if(w)
			*w = texture->width;
		if(h)
			*h = texture->height;
	}
}

//--- font rendering -----------------------------------------------

static void gfxGlDrawBitmapFont(int x, int y, const char* str) {
	const unsigned char wChar = 12, hChar = 16;
	for(const unsigned char* c=(const unsigned char*)str; *c; ++c, x += wChar) {
		int srcX = ((*c)%16)*wChar;
		int srcY = ((*c)/16)*hChar;
		gfxGlDrawImageEx(defaultFont, srcX,srcY,wChar,hChar, x,y,wChar,hChar, 0,0,0,0);
	}
}

#define GLYPH_MIN 32
#define GLYPH_COUNT 96

typedef struct {
	size_t texture;
	float height, ascent, descent;
	stbtt_bakedchar glyphData[GLYPH_COUNT];
} FontSpec;

size_t gfxGlFontLoad(const char* fname, float fontHeight) {
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
		ret = gfxGlFontUpload(ttf_buffer, fsize, fontHeight);
	if(!ret)
		fprintf(stderr, "font file read error\n");
	fclose(ff);
	free(ttf_buffer);
	return ret;
}

size_t gfxGlFontUpload(void* fontData, unsigned dataSize, float fontHeight) {
	// render glyphs into bitmap buffer:
	const unsigned texW = fontHeight>48.0f? 1024 : 512, texH = fontHeight>64.0f ? 1024 : 512;
	uint8_t* bitmap = malloc(texW*texH);
	stbtt_bakedchar glyphData[GLYPH_COUNT];
	stbtt_BakeFontBitmap(fontData,0, fontHeight, bitmap, texW, texH, GLYPH_MIN, GLYPH_COUNT, glyphData);

	// transfer data/metrics to texture and font spec:
	uint32_t* data = (uint32_t*)malloc(sizeof(uint32_t)*texW*texH);
	memset(data, 0, sizeof(uint32_t)*texW*texH);

	for(unsigned y=0; y<texH; ++y)
		for(unsigned x=0; x<texW; ++x) {
			uint8_t px = bitmap[y*texW+x];
			if(px)
				data[y*texW + x] = 0x00ffffff + (px<<24);
		}
	free(bitmap);

	FontSpec* fnt = (FontSpec*)malloc(sizeof(FontSpec));
	fnt->texture = gfxGlImageUpload((const unsigned char*)data, texW, texH, 4);
	fnt->height = fontHeight;
	memcpy(fnt->glyphData, glyphData, GLYPH_COUNT*sizeof(stbtt_bakedchar));
	float lineGap;
	stbtt_GetScaledFontVMetrics(fontData,0, fontHeight, &fnt->ascent, &fnt->descent, &lineGap);
	free(data);
	return (size_t)fnt;
}

void gfxGlFillText(size_t font, float x, float y, const char* text) {
	if(!font) {
		gfxGlDrawBitmapFont(x, y, text);
		return;
	}
	FontSpec* fnt = (FontSpec*)font;
	y += fnt->ascent;

	for (const char *pc = text; *pc; ++pc) {
		int c = *pc;
		if(c<GLYPH_MIN || c>=GLYPH_MIN+GLYPH_COUNT)
			c = ' '; // render as space

		const stbtt_bakedchar* glyph = &fnt->glyphData[c - GLYPH_MIN];
		int w = glyph->x1 - glyph->x0, h = glyph->y1 - glyph->y0;
		int srcX = glyph->x0, srcY = glyph->y0;
		float destX = x + glyph->xoff + 0.5f, destY = y + glyph->yoff + 0.5f;
		gfxGlDrawImageEx((size_t)fnt->texture, srcX,srcY,w,h, destX,destY,w,h, 0,0,0,0);
		x += glyph->xadvance;
	}
}

void gfxGlFillTextAlign(size_t font, float x, float y, const char* text, GfxAlign align) {
	if(align!=GFX_ALIGN_LEFT_TOP) {
		float w,h;
		gfxGlMeasureText(font, text, &w, &h, NULL, NULL);
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
	gfxGlFillText(font, x, y, text);
}

void gfxGlMeasureText(size_t font, const char* text, float* width, float* height, float* ascent, float* descent) {
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
		for (const char *pc = text; *pc; ++pc) {
			int c = *pc;
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

void gfxGlFontRelease(size_t font) {
	FontSpec* fnt = (FontSpec*)font;
	gfxGlImageRelease((size_t)fnt->texture);
	free(fnt);
}
