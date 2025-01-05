#include "graphics.h"
#include "graphicsUtils.h"
#include "font12x16.h"
#include "external/stb_truetype.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define countof(arr) sizeof(arr) / sizeof(arr[0])

static SDL_Renderer* renderer = NULL;
static uint32_t defaultFont;
static float mat[] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };

typedef struct {
	float transf[4];
	float lineWidth;
	SDL_Color clr;
	int blendMode;
} GfxState;

static uint8_t dtransf = 0, dtransfMax = 7;
static GfxState gs[8];

typedef struct {
	SDL_Texture *tex;
	SDL_Rect src;
	bool ownsTexture;
	float cx, cy, sc;
} ImgResource;

static ImgResource* images=NULL;
static uint32_t numImages=0, numImagesMax=0;
static uint32_t numFonts=0, numFontsMax=0;

void gfxInit(uint16_t vpWidth, uint16_t vpHeight, float resScale, void *arg) {
	(void)vpWidth;
	(void)vpHeight;

	gfxStateReset();
	renderer = (SDL_Renderer*)arg;
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	defaultFont = gfxSVGUpload(font12x16, sizeof(font12x16), resScale);
	images[defaultFont].sc = 1.0f/resScale;
	gfxImageTile(defaultFont, 160*resScale,0,32*resScale,32*resScale); // circle texture
	gfxImageSetCenter(defaultFont+1, 0.5f, 0.5f);
	images[defaultFont+1].sc = 1.0f/32.0f/resScale;
	gfxImageTile(defaultFont, 126*resScale,40*resScale,1*resScale,1*resScale); // square texture
	gfxImageSetCenter(defaultFont+2, 0.5f, 0.5f); 
	images[defaultFont+2].sc = 1.0f/resScale;
}

void gfxClose() {
	if(!renderer)
		return;
	while(numFonts)
		gfxFontRelease(numFonts); 
	while(numImages)
		gfxImageRelease(numImages-1); 
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

void gfxBeginFrame(uint32_t clearColor) {
	SDL_SetRenderDrawColor(renderer, clearColor >> 24, clearColor >> 16, clearColor >> 8, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
	gfxStateReset();
	//if(resScale!=1.0f) gfxTransform(0,0,0,resScale);
}

void gfxEndFrame() {
	SDL_RenderPresent(renderer);
}

uint32_t gfxSVGUpload(const char* svg, size_t svgLen, float scale) {
	if(!svg) {
		svg = font12x16;
		svgLen = sizeof(font12x16);
		scale /= images[defaultFont].sc;
	}
	char* svgCopy = malloc(svgLen+1);
	memcpy(svgCopy, svg, svgLen);
	svgCopy[svgLen]=0;
	int w, h, d;
	unsigned char* data = svgRasterize(svgCopy, scale, &w, &h, &d);
	free(svgCopy);

	if(data == NULL) {
		fprintf(stderr, "Could not rasterize SVG image.\n");
		return 0;
	}
	uint32_t img = gfxImageUpload(data, w, h, d, 0xff);
	free(data);
	return img;
}

uint32_t storeTexture(SDL_Texture* texture, int w, int h, bool ownsTexture) {
	if(numImagesMax==0) {
		numImagesMax=4;
		images = (ImgResource*)malloc(numImagesMax*sizeof(ImgResource));
	}
	else if(numImages == numImagesMax) {
		numImagesMax *= 2;
		images = (ImgResource*)realloc(images, numImagesMax*sizeof(ImgResource));
	}
	images[numImages].tex = texture;
	images[numImages].src = (SDL_Rect){0, 0, w, h};
	images[numImages].ownsTexture = ownsTexture;
	images[numImages].cx = images[numImages].cy = 0.0f;
	images[numImages].sc = 1.0f;
	return ++numImages -1;
}

uint32_t gfxImageUpload(const unsigned char* data, int w, int h, int d, uint32_t rMask) {
	if(!renderer) {
		fprintf(stderr,"gfxImageUpload ERROR: SDL backend not initialized\n");
		return 0;
	}
	if(!data || !w || !h ||!d)
		return 0;

	uint32_t gMask, bMask, aMask;
	if(d==1) {
		rMask = gMask = bMask = 0xff; aMask = 0;
	}
	else if(d==2) {
		gMask = bMask = rMask; aMask = (rMask==0xff) ? 0xff00 : 0xff;
	}
	else if(rMask==0xff) {
		gMask = 0x0000ff00; bMask = 0x00ff0000; aMask = (d==4) ? 0xff000000 : 0;
	}
	else {
		gMask = 0x00ff0000; bMask = 0x0000ff00; aMask = (d==4) ? 0xff : 0;
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)data, w, h, d*8, w*d, rMask, gMask, bMask, aMask);
	if(!surf) {
		SDL_Log("Creating surface failed: %s", SDL_GetError());
		return 0;
	}
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surf);
	SDL_FreeSurface(surf);
	if(texture) {
		if(d==2 || d==4)
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_ClearError();
		return storeTexture(texture, w, h, true);
	}
	return 0;
}

void gfxImageSetCenter(uint32_t img, float cx, float cy) {
	if(img < numImages) {
		images[img].cx = cx * images[img].src.w;
		images[img].cy = cy * images[img].src.h;
		//printf("setCenter:%.2f %.2f -> %.2f %.2f\n", cx, cy, images[img].cx, images[img].cy);
	}
}

void gfxImageRelease(uint32_t img) {
	if(img < numImages) {
		//printf("%u %i %i %lu\n", img, images[img].src.w, images[img].src.h, (size_t)images[img].tex);
		if(images[img].ownsTexture && images[img].tex) {
			SDL_DestroyTexture(images[img].tex);
			images[img].ownsTexture = false;
		}
		images[img].tex = NULL;
		images[img].src.x = images[img].src.y = images[img].src.w = images[img].src.h = 0;
	}
	while(numImages>0 && !images[numImages-1].tex)
		--numImages;
}

size_t gfxCanvasCreate(int w, int h, uint32_t color) {
	SDL_Texture * texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, w, h);
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color >> 24, color >> 16, color >> 8, color & 0xff);
	SDL_RenderClear(renderer);
	return (size_t)texture;
}

uint32_t gfxCanvasUpload(size_t canvas) {
	SDL_SetRenderTarget(renderer, NULL);
	SDL_Texture* texture = (SDL_Texture *)canvas;
	int w,h;
	SDL_QueryTexture(texture, NULL, NULL, &w, &h);
	return storeTexture(texture, w,h, SDL_TRUE);
}

uint32_t gfxVideoCanvasCreate(int w, int h) {
	SDL_Texture * texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, w, h);
	return storeTexture(texture, w,h, SDL_TRUE);
}

int gfxVideoCanvasUpdate(uint32_t img,
	const uint8_t* yData, int yPitch, const uint8_t* uData, int uPitch, const uint8_t* vData, int vPitch) {
	if(img >= numImages)
		return -1;
	SDL_Texture * texture = images[img].tex;
	if(SDL_UpdateYUVTexture(texture, NULL, yData, yPitch, uData, uPitch, vData, vPitch) !=0) {
		printf( "Unable to update texture! %s\n", SDL_GetError() );
		return -2;
	}
	return 0;
}

uint32_t gfxImageTile(uint32_t parent, int x, int y, int w, int h) {
	if(parent >= numImages)
		return 0;
	if(numImages == numImagesMax) {
		numImagesMax *= 2;
		images = (ImgResource*)realloc(images, numImagesMax*sizeof(ImgResource));
	}

	images[numImages].tex = images[parent].tex;
	images[numImages].src = (SDL_Rect){x, y, w, h};
	images[numImages].ownsTexture = false;
	const float parentW = images[parent].src.w, parentH = images[parent].src.h;
	images[numImages].cx = images[parent].cx * w/parentW;
	images[numImages].cy = images[parent].cy * h/parentH;
	images[numImages].sc = images[parent].sc;
	return ++numImages - 1;
}

uint32_t gfxImageTileGrid(uint32_t parent, uint16_t tilesX, uint16_t tilesY, uint16_t border) {
	if(parent >= numImages)
		return 0;
	float w = images[parent].src.w/(float)tilesX;
	float h = images[parent].src.h/(float)tilesY;
	uint32_t ret = numImages;

	for(uint16_t j=0; j<tilesY; ++j) for(uint16_t i=0; i<tilesX; ++i) {
		gfxImageTile(parent, i*w+border, j*h+border, w-2*border, h-2*border);
	}
	return ret;
}

void gfxImageDimensions(uint32_t img, int* w, int* h) {
	if(!img || img >= numImages)
		return;
	if(w)
		*w = images[img].src.w;
	if(h)
		*h = images[img].src.h;
}

//--- font handling ------------------------------------------------

#define GLYPH_MIN 32
#define GLYPH_COUNT 224

typedef struct {
	uint32_t texId;
	int texW, texH, margin;
	float height, ascent, descent;
	stbtt_bakedchar glyphData[GLYPH_COUNT];
} FontResource;

static FontResource* fonts=NULL;

static FontResource* pushNewFont() {
	if(numFontsMax==0) {
		numFontsMax=4;
		fonts = (FontResource*)malloc(numFontsMax*sizeof(FontResource));
	}
	else if(numFonts == numFontsMax) {
		numFontsMax *= 2;
		fonts = (FontResource*)realloc(fonts, numFontsMax*sizeof(FontResource));
	}
	return &fonts[numFonts];
}

uint32_t gfxFontUpload(void* fontData, size_t dataSize, float fontHeight) {
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
	uint32_t texId = gfxImageUpload(pixels, texW, texH, 4, 0xff);
	free(pixels);
	free(bitmap);

	FontResource* fnt = pushNewFont();
	fnt->texId = texId;
	fnt->texW = texW;
	fnt->texH = texH;
	fnt->margin = -1; // only relevant for fixed width image fonts
	fnt->height = fontHeight;
	memcpy(fnt->glyphData, glyphData, GLYPH_COUNT*sizeof(stbtt_bakedchar));
	float lineGap;
	stbtt_GetScaledFontVMetrics(fontData,0, fontHeight, &fnt->ascent, &fnt->descent, &lineGap);

	return ++numFonts;
}

uint32_t gfxFontFromImage(uint32_t img, int margin) {
	FontResource* fnt = pushNewFont();
	fnt->texId = img;
	fnt->texW = fnt->texH = 0;
	fnt->margin = margin; // only relevant for fixed width image fonts
	fnt->height = fnt->ascent = fnt->descent = 0;
	memset(fnt->glyphData, 0, sizeof(stbtt_bakedchar)*GLYPH_COUNT);
	return ++numFonts;
}

void gfxFontRelease(uint32_t font) {
	if(font<1 || font>numFonts)
		return;
	FontResource* fnt = &fonts[font-1];
	//printf("fontRelease %u tex:%u %i\n", font, fnt->texId, fnt->margin);
	if(!fnt->texId)
		return;
	if(fnt->margin<0) // means uploaded font, not font from image
		gfxImageRelease(fnt->texId);
	fnt->texId = 0;

	while(numFonts>0 && !fonts[numFonts-1].texId)
		--numFonts;
}

//--- state --------------------------------------------------------

static void setMat(const float* transf) {
	mat[0] = cos(transf[2])*transf[3]; mat[1] = -sin(transf[2])*transf[3];
	mat[2] = transf[0]; mat[3] = -mat[1]; mat[4] = mat[0]; mat[5] = transf[1];
}

void gfxStateReset() {
	dtransf = 0;
	gs[0].transf[0] = gs[0].transf[1] = gs[0].transf[2] = 0.0f;
	gs[0].transf[3] = 1.0f;
	gs[0].lineWidth = 1.0f;
	gs[0].clr.r = gs[0].clr.g = gs[0].clr.b = gs[0].clr.a = 255;
	setMat(gs[0].transf);
	gs[0].blendMode = SDL_BLENDMODE_BLEND;
}

void gfxStateSave() {
	if(dtransf >= dtransfMax)
		return;
	const uint8_t d = dtransf;
	memcpy(&gs[++dtransf], &gs[d], sizeof(GfxState));
}

void gfxStateRestore() {
	if(dtransf < 1)
		return;
	if(gs[dtransf].blendMode != gs[dtransf-1].blendMode)
		SDL_SetRenderDrawBlendMode(renderer, gs[dtransf-1].blendMode);
	setMat(gs[--dtransf].transf);
}

void gfxColor(uint32_t color) {
	SDL_Color* clr = &gs[dtransf].clr;
	clr->r = color >> 24, clr->g = color >> 16, clr->b = color >> 8, clr->a = color & 0xff;
	SDL_SetRenderDrawColor(renderer, clr->r, clr->g, clr->b, clr->a);
}

void gfxLineWidth(float w) {
	gs[dtransf].lineWidth = w;
}
float gfxGetLineWidth() {
	return gs[dtransf].lineWidth;
}

void gfxBlend(int mode) {
	SDL_SetRenderDrawBlendMode(renderer, mode);
	gs[dtransf].blendMode = mode;
}
int gfxGetBlend() {
	return gs[dtransf].blendMode;
}

void gfxClipRect(int x, int y, int w, int h) {
	SDL_Rect pos={x,y,w,h};
	SDL_RenderSetClipRect(renderer, (w<0||h<0) ? NULL : &pos);
}

void gfxTransform(float x, float y, float rot, float sc) {
	float* transf = &gs[dtransf].transf[0];
	transf[0] += mat[0]*x + mat[1]*y;
	transf[1] += mat[3]*x + mat[4]*y; 
	transf[2] += rot;
	transf[3] *= sc;
	setMat(transf);
}

void gfxTransf3d(float x, float y, float z, float rotX, float rotY, float rotZ, float sc) {
	(void)z;
	(void)rotX;
	(void)rotY;
	gfxTransform(x,y,rotZ,sc);
}

//--- low level ops ------------------------------------------------

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

/// generic affine 2d array transformation using a row-wise ordered float[6] transformation matrix
/** arrIn and arrOut must have the same size but may point to the same memory */
static void arrayTransf2d(const float transf[], uint32_t arrSz, const float* arrIn, float* arrOut) {
	for(uint32_t i=0; i+1<arrSz; i+=2) {
		float x=arrIn[i], y=arrIn[i+1];
		arrOut[i] = x*transf[0] + y*transf[1] + transf[2];
		arrOut[i+1] = x*transf[3] + y*transf[4] + transf[5];
	}
}

// coords must already be transformed and lw already scaled
static void gfxDrawLineW(float x1, float y1, float x2, float y2, float lw) {
	//printf("gfxDrawLineW(%.1f,%.1f, %.1f,%.1f, %.1f)\n", x1,y1, x2,y2, lw);
	const float dx = x2-x1, dy=y2-y1, d=sqrt(dx*dx+dy*dy);
	const float lw2 = lw/2.0f, nx = lw2*-dy/d, ny=lw2*dx/d;
	float c[] = {x1-nx, y1-ny, x2-nx, y2-ny, x2+nx, y2+ny, x1+nx, y1+ny};
	static const uint8_t indices[] = { 0,1,2, 2,3,0 };
	const SDL_Color* clr = &gs[dtransf].clr;
	SDL_RenderGeometryRaw(renderer, NULL, c, 2*sizeof(float), clr, 0, NULL, 0, 4, indices, 6, 1);
}

void gfxDrawImageEx(SDL_Texture* texture,
	int srcX, int srcY, int srcW, int srcH,
	float destX, float destY, float destW, float destH,
	float cx, float cy, float angle, int flip)
{
	const SDL_Color* clr = &gs[dtransf].clr;
	SDL_SetTextureColorMod(texture, clr->r, clr->g, clr->b);
	SDL_SetTextureAlphaMod(texture, clr->a);
	SDL_SetTextureBlendMode(texture, gs[dtransf].blendMode);

	SDL_Rect src = { srcX, srcY, srcW, srcH };
	const float sc = gs[dtransf].transf[3];
	SDL_FPoint ctr = { cx*sc*destW/srcW, cy*sc*destH/srcH };
	SDL_FRect dest = {
		destX*mat[0] + destY*mat[1] + mat[2] - ctr.x,
		destX*mat[3] + destY*mat[4] + mat[5] - ctr.y,
		destW*sc, destH*sc};
	SDL_RenderCopyExF(renderer, texture, &src, &dest, (angle + gs[dtransf].transf[2])*180.0f/M_PI, &ctr, flip);
}

//--- basic drawing operations -------------------------------------

void gfxDrawRect(float x, float y, float w, float h) {
	float lw = gs[dtransf].lineWidth, lw2 = lw/2.0f;
	float c[] = {x,y+lw2, x+w,y+lw2, x+w-lw2,y+lw, x+w-lw2,y+h-lw, x+w,y+h-lw2, x,y+h-lw2, x+lw2,y+h-lw, x+lw2,y+lw };
	arrayTransf2d(mat, countof(c), c, c);
	lw *= gs[dtransf].transf[3];
	gfxDrawLineW(c[0],c[1], c[2],c[3], lw);
	gfxDrawLineW(c[4],c[5], c[6],c[7], lw);
	gfxDrawLineW(c[8],c[9], c[10],c[11], lw);
	gfxDrawLineW(c[12],c[13], c[14],c[15], lw);
}

void gfxFillRect(float x, float y, float w, float h) {
	float c[] = {x,y, x+w,y, x+w,y+h, x,y+h };
	static const uint8_t indices[] = { 0,1,2, 2,3,0 };
	arrayTransf2d(mat, countof(c), c, c);
	const SDL_Color* clr = &gs[dtransf].clr;
	SDL_RenderGeometryRaw(renderer, NULL, c, 2*sizeof(float), clr, 0, NULL, 0, 4, indices, 6, 1);
}

void gfxFillTriangle(float x0, float y0, float x1, float y1, float x2, float y2) {
	float c[] = {x0,y0, x1,y1, x2,y2 };
	arrayTransf2d(mat, countof(c), c, c);
	const SDL_Color* clr = &gs[dtransf].clr;
	SDL_RenderGeometryRaw(renderer, NULL, c, 2*sizeof(float), clr, 0, NULL, 0, 3, NULL, 0, 0);
}

void gfxDrawLine(float x0, float y0, float x1, float y1) {
	float c[] = {x0,y0, x1,y1 };
	arrayTransf2d(mat, countof(c), c, c);
	gfxDrawLineW(c[0],c[1], c[2],c[3], gs[dtransf].lineWidth * gs[dtransf].transf[3]);
}

void gfxDrawLineStrip(uint32_t numCoords, const float* coords) {
	for(uint32_t i=2, end=numCoords*2; i<end; i+=2)
		gfxDrawLine(coords[i-2], coords[i-1], coords[i], coords[i+1]);
}

void gfxDrawLineLoop(uint32_t numCoords, const float* coords) {
	gfxDrawLineStrip(numCoords, coords);
	if(numCoords>2)
		gfxDrawLine(coords[numCoords*2-2], coords[numCoords*2-1], coords[0], coords[1]);
}

void gfxDrawPoints(uint32_t numCoords, const float* coords, uint32_t img) {
	float lineWidth = gs[dtransf].lineWidth;
	const float* transf = gs[dtransf].transf;
	if(lineWidth==1.0f && transf[0]==0.0f && transf[1]==0.0f && transf[2] == 0.0f && transf[3]==1.0f) {
		SDL_RenderDrawPointsF(renderer, (const SDL_FPoint*)coords, numCoords);
		return;
	}
	if(!img || img >= numImages)
		img = GFX_IMG_CIRCLE;
	ImgResource* res = &images[img];
	for(uint32_t i=0; i<numCoords; ++i) {
		gfxDrawImageEx(res->tex, res->src.x,res->src.y,res->src.w,res->src.h,
			coords[i*2],coords[i*2+1], lineWidth,lineWidth, res->cx,res->cy, 0,0);
	}
}

void gfxDrawImage(uint32_t img, float x, float y, float rot, float sc, int flip) {
	if(!img || img >= numImages)
		return;
	ImgResource* res = &images[img];
	gfxDrawImageEx(res->tex, res->src.x,res->src.y,res->src.w,res->src.h,
		x,y,res->src.w*res->sc*sc,res->src.h*res->sc*sc, res->cx,res->cy, rot, flip);
}

void gfxStretchImage(uint32_t img, float x, float y, float w, float h) {
	if(!img || img >= numImages)
		return;
	ImgResource* res = &images[img];
	gfxDrawImageEx(res->tex, res->src.x,res->src.y,res->src.w,res->src.h, x,y,w,h, 0,0,0,0);
}

/// write text using a texture containing a fixed 16x16 grid of glyphs
static void gfxFillTextFixedFont(uint32_t img, float x, float y, int margin, const char* str) {
	if(img>=numImages)
		img = 0;
	const unsigned char wCell = images[img].src.w/16, wChar = wCell - margin*2;
	const unsigned char hCell = images[img].src.h/16, hChar = hCell - margin*2;
	SDL_Texture* texture = images[img].tex;

	const SDL_Color* clr = &gs[dtransf].clr;
	SDL_SetTextureColorMod(texture, clr->r, clr->g, clr->b);
	SDL_SetTextureAlphaMod(texture, clr->a);
	SDL_SetTextureBlendMode(texture, gs[dtransf].blendMode);

	SDL_Rect src  = { 0, 0, wChar, hChar };
	float rot = gs[dtransf].transf[2]*180.0f/M_PI, sc = gs[dtransf].transf[3] * images[img].sc;
	SDL_FRect dest = {x*mat[0] + y*mat[1] + mat[2], x*mat[3] + y*mat[4] + mat[5], wChar*sc, hChar*sc};
	static const SDL_FPoint ctr = { 0, 0 };
	const float dx = wChar*mat[0] * images[img].sc, dy = wChar*mat[3] * images[img].sc;
	for(size_t readIndex=0; str[readIndex]; x += wChar) {
		unsigned char c = utf8ToLatin1(str, &readIndex);
		if(c) {
			src.x = (c%16)*wCell + margin;
			src.y = (c/16)*hCell + margin;
			SDL_RenderCopyExF(renderer, texture, &src, &dest, rot, &ctr, SDL_FLIP_NONE);
		}
		dest.x += dx;
		dest.y += dy;
	}
}

static void gfxFillTextProportionalFont(uint32_t font, float x, float y, const char* str) {
	const FontResource* fnt = &fonts[font-1];
	if(fnt->margin >= 0) {
		gfxFillTextFixedFont(fnt->texId, x,y, fnt->margin, str);
		return;
	}
	SDL_Texture* texture = images[fnt->texId].tex;
	const SDL_Color* clr = &gs[dtransf].clr;
	SDL_SetTextureColorMod(texture, clr->r, clr->g, clr->b);
	SDL_SetTextureAlphaMod(texture, clr->a);
	SDL_SetTextureBlendMode(texture, gs[dtransf].blendMode);

	y += fnt->ascent;
	float destX = x*mat[0] + y*mat[1] + mat[2], destY = x*mat[3] + y*mat[4] + mat[5];
	static const SDL_FPoint ctr = { 0, 0 };
	float rot = gs[dtransf].transf[2]*180.0f/M_PI, sc = gs[dtransf].transf[3];

	for(size_t readIndex=0; str[readIndex]; ) {
		unsigned char c = utf8ToLatin1(str, &readIndex);
		if(c<GLYPH_MIN)
			c = ' '; // render as space

		const stbtt_bakedchar* glyph = &fnt->glyphData[c - GLYPH_MIN];
		int wChar = glyph->x1 - glyph->x0, hChar = glyph->y1 - glyph->y0;
		SDL_Rect src  = { .x = glyph->x0, .y = glyph->y0, .w = wChar, .h = hChar };
		float xoff = (glyph->xoff + 0.5f)*mat[0] + (glyph->yoff + 0.5f)*mat[1];
		float yoff = (glyph->xoff + 0.5f)*mat[3] + (glyph->yoff + 0.5f)*mat[4];
		SDL_FRect dest = { destX + xoff, destY + yoff, wChar*sc, hChar*sc };
		SDL_RenderCopyExF(renderer, texture, &src, &dest, rot, &ctr, SDL_FLIP_NONE);
		destX += glyph->xadvance * mat[0];
		destY += glyph->xadvance * mat[3];
	}
}

void gfxFillText(uint32_t font, float x, float y, const char* str) {
	if(!font || font>numFonts)
		gfxFillTextFixedFont(0, x,y, 0, str);
	else
		gfxFillTextProportionalFont(font, x,y, str);
}

void gfxFillTextAlign(uint32_t font, float x, float y, const char* str, int align) {
	float width, height;
	gfxMeasureText(font, str, &width, &height, NULL, NULL);
	if(align & GFX_ALIGN_RIGHT_TOP)
		x-=width;
	else if(align & GFX_ALIGN_CENTER_TOP)
		x-=width/2;
	if(align & GFX_ALIGN_LEFT_BOTTOM)
		y-=height;
	else if(align & GFX_ALIGN_LEFT_MIDDLE)
		y-=height/2;
	gfxFillText(font, x,y, str);
}

void gfxMeasureText(uint32_t font, const char* text, float* width, float* height, float* ascent, float* descent) {
	if(!font || font>numFonts || fonts[font-1].margin >= 0) {
		uint32_t img = (!font || font>numFonts) ? 0 : fonts[font-1].texId;
		if(img>=numImages)
			img = 0;
		const int margin = img ? fonts[font-1].margin : 0;
		const unsigned char wChar = images[img].src.w/16 - margin*2, hChar = images[img].src.h/16 - margin*2;
		if(width) {
			*width = 0;
			for(size_t readIndex=0; text[readIndex]; utf8ToLatin1(text, &readIndex))
				*width += wChar * images[img].sc;
		}
		if(height)
			*height = hChar * images[img].sc;
		if(ascent)
			*ascent = wChar * images[img].sc;
		if(descent)
			*descent = wChar-hChar * images[img].sc;
		return;
	}

	if(!fonts[font-1].texId)
		return;
	const FontResource* fnt = &fonts[font-1];
	if(width) {
		*width = 0.0f;
		if(text) for(size_t readIndex=0; text[readIndex]; ) {
			unsigned char c = utf8ToLatin1(text, &readIndex);
			if(c<GLYPH_MIN)
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

//--- experimental extensions --------------------------------------

void gfxDrawTiles(uint16_t tilesX, uint16_t tilesY, uint32_t stride,
	uint32_t imgBase, const uint32_t* imgOffsets, const uint32_t* colors)
{
	const float w = images[imgBase].src.w, h=images[imgBase].src.h;
	for(uint16_t j=0; j<tilesY; ++j) for(uint16_t i=0; i<tilesX; ++i) {
		size_t index = j*stride+i;
		if(colors)
			gfxColor(colors[index]);
		uint32_t img = imgOffsets ? imgBase+imgOffsets[index] : imgBase;
		gfxDrawImage(img, i*w, j*h,0,1.0,0);
	}
}

void gfxDrawImages(uint32_t imgBase, uint32_t numInstances, uint32_t stride,
	const gfxArrayComponents comps, const float* arr)
{
	int hasColors = (comps&GFX_COMP_COLOR_RGBA) != 0;
	SDL_Color* clr = &gs[dtransf].clr;
	for(uint32_t i=0; i<numInstances; ++i) {
		const float* data = &arr[i*stride];
		uint32_t img = imgBase, j=0;
		if(comps & GFX_COMP_IMG_OFFSET)
			img += data[j++];
		if(!img || img >= numImages)
			continue;

		float x = data[j++], y = data[j++];
		float rot = (comps & GFX_COMP_ROT) ? data[j++] : 0.0f;
		float sc = (comps & GFX_COMP_SCALE) ? data[j++] : 1.0f;
		if(hasColors) {
			clr->r = (comps & GFX_COMP_COLOR_R) ? data[j++] : clr->r;
			clr->g = (comps & GFX_COMP_COLOR_G) ? data[j++] : clr->g;
			clr->b = (comps & GFX_COMP_COLOR_B) ? data[j++] : clr->b;
			clr->a = (comps & GFX_COMP_COLOR_A) ? data[j++] : clr->a;
			if(!clr->a)
				continue;
			SDL_SetRenderDrawColor(renderer, clr->r, clr->g, clr->b, clr->a);
		}
		//printf("x:%.1f y:%.1f rot:%.1f sc:%.2f r:%u g:%u b:%u a:%u\n", x,y,rot,sc, clr->r, clr->g, clr->b, clr->a);
		ImgResource* res = &images[img];
		gfxDrawImageEx(res->tex, res->src.x,res->src.y,res->src.w,res->src.h,
			x,y,res->src.w*res->sc*sc,res->src.h*res->sc*sc, res->cx,res->cy,rot,0);
	}
}

void gfxFillTriangles(uint32_t numVertices, const float* coords,
	const uint32_t* colors, uint32_t numIndices, const uint32_t* indices)
{
	const SDL_Color* clr = &gs[dtransf].clr;
	static float coordsTrans[1000*6*2];
	const uint32_t numTrianglesMax = 2000;
	for(uint32_t offset=0; offset<numVertices; offset+=numTrianglesMax*3*2) {
		uint32_t numCoords = numVertices - offset;
		if(numCoords>numTrianglesMax*3*2)
			numCoords = numTrianglesMax*3*2;
		arrayTransf2d(mat, numCoords*2, &coords[offset*2], coordsTrans);
		SDL_RenderGeometryRaw(renderer, NULL, coordsTrans, 2*sizeof(float),
			(colors ? (SDL_Color*)(colors+offset) : clr), colors ? sizeof(uint32_t) : 0, NULL, 0, numCoords,
			indices, numIndices, indices? 4 : 0);
	}
}

void gfxTexTriangles(uint32_t img, uint32_t numVertices, const float* coords, const float* uvCoords,
	const uint32_t* colors, uint32_t numIndices, const uint32_t* indices)
{
	SDL_Texture* tex = (!img || img >= numImages) ? NULL : images[img].tex;
	const SDL_Color* clr = &gs[dtransf].clr;

	static float coordsTrans[1000*6*2];
	const uint32_t numTrianglesMax = 2000;
	for(uint32_t offset=0; offset<numVertices; offset+=numTrianglesMax*3*2) {
		uint32_t numCoords = numVertices - offset;
		if(numCoords>numTrianglesMax*3*2)
			numCoords = numTrianglesMax*3*2;
		arrayTransf2d(mat, numCoords*2, &coords[offset*2], coordsTrans);
		SDL_RenderGeometryRaw(renderer, tex, coordsTrans, 2*sizeof(float),
			(colors ? (SDL_Color*)(colors+offset) : clr), colors ? sizeof(uint32_t) : 0, uvCoords, 2*sizeof(float), numCoords,
			indices, numIndices, indices? 4 : 0);
	}
}
