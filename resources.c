#include "resources.h"
#include "archive.h"
#include "graphics.h"
#include "graphicsGL.h"
#include "graphicsUtils.h"

#include "audio.h"

#include "external/stb_image.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	char* name;
	size_t handle;
} Resource;

typedef struct {
	char* name;
	size_t handle;
	unsigned size;
} FontResource;

typedef struct {
	Archive* ar;
	FontResource* fonts;
	unsigned numFonts, numFontsMax;
	Resource *images, *samples;
	unsigned numImages, numImagesMax, numSamples, numSamplesMax;
	int uploadToGL;
} ResArchive;
static ResArchive* ra = NULL;

//--- functions ----------------------------------------------------

const char* ResourceSuffix(const char* fname) {
	size_t len = strlen(fname);
	if(len>2) for(size_t pos = len-2; pos>0; --pos)
		switch(fname[pos]) {
		case '.':
		 	return fname+pos+1;
		case '/':
		case '\\':
			break;
		}
	return "";
}

char* ResourceBaseName(const char* fname) {
	size_t len = strlen(fname);
	if(len<2)
		return strdup(fname);
	size_t pos = len-1, start=0, end=len;
	while(pos>0 && start==0) switch(fname[pos]) {
		case '/':
		case '\\':
			if(pos==len-1) {
				end = pos--;
				continue;
			}
			start = pos+1;
			break;
		case '.':
			if(end==len)
				end = pos;
		default:
			--pos;
		}
	char* s = (char*)malloc(end-start+1);
	s[end-start]=0;
	return strncpy(s, fname+start, end-start);
}

int ResourceUploadsToGL() {
	return ra && ra->uploadToGL;
}

int isImageFile(const char* fname) {
	const char* suffix = ResourceSuffix(fname);
	if(!suffix)
		return 0;
	if(strcasecmp(suffix, "png")==0 || strcasecmp(suffix, "jpg")==0)
		return 1;
	if(strcasecmp(suffix, "svg")==0)
		return 2;
	return 0;
}
int isAudioFile(const char* fname) {
	const char* suffix = ResourceSuffix(fname);
	return suffix && (strcasecmp(suffix, "mp3")==0);
}
int isFontFile(const char* fname) {
	const char* suffix = ResourceSuffix(fname);
	return suffix && (strcasecmp(suffix, "ttf")==0);
}

//------------------------------------------------------------------
static void* ArchiveLoadBinary(Archive* ar, const char* fname, size_t* size) {
	*size = ArchiveFileSize(ar, fname);
	if(!*size)
		return 0;

	void* buf = malloc(*size);
	size_t numBytesRead = ArchiveFileLoad(ar, fname, buf);
	if(numBytesRead != *size) {
		free(buf);
		return 0;
	}
	return buf;
}

static size_t ArchiveLoadImage(Archive* ar, const char* fname, int uploadToGL) {
	size_t fsize;
	void* buf = ArchiveLoadBinary(ar, fname, &fsize);
	if(!buf)
		return 0;

	int w,h,d;
	unsigned char *data = stbi_load_from_memory(buf, (int)fsize, &w, &h, &d, 0);
	free(buf);
	if(!data)
		return 0;

	size_t img = uploadToGL ? gfxGlImageUpload(data, w, h, d) : gfxImageUpload(data, w, h, d);
	free(data);
	return img;
}

static size_t ArchiveLoadSVG(char* svg, float scale, int uploadToGL) {
	int w, h, d;
	unsigned char* data = svgRasterize(svg, scale, &w, &h, &d);
	free(svg);
	if(data == NULL) {
		fprintf(stderr, "Could not rasterize SVG image.\n");
		return 0;
	}
	size_t img = uploadToGL ? gfxGlImageUpload(data, w, h, d) : gfxImageUpload(data, w, h, d);
	free(data);
	return img;
}

static size_t ArchiveLoadAudio(Archive* ar, const char* fname) {
	size_t fsize;
	void* buf = ArchiveLoadBinary(ar, fname, &fsize);
	if(!buf)
		return 0;

	size_t sample = AudioUpload(buf, fsize);
	free(buf);
	return sample;
}

//------------------------------------------------------------------

/// opens a resource archive for further processing
size_t ResourceArchiveOpen(const char* url, int uploadToGL) {
	if(ra)
		return 0;
	Archive* ar = ArchiveOpen(url);
	if(!ar)
		return 0;
	ra = (ResArchive*)malloc(sizeof(ResArchive));
	ra->ar = ar;
	ra->fonts = NULL;
	ra->numFonts = ra->numFontsMax = 0;
	ra->images = ra->samples = NULL;
	ra->numImages = ra->numImagesMax = ra->numSamples = ra->numSamplesMax = 0;
	ra->uploadToGL = uploadToGL;
	return (size_t)ar;
}
/// closes currently opened resources and archive
void ResourceArchiveClose() {
	if(!ra)
		return;
	ArchiveClose(ra->ar);

	for(unsigned i=0; i<ra->numFonts; ++i)
		free(ra->fonts[i].name);
	free(ra->fonts);

	for(unsigned i=0; i<ra->numImages; ++i)
		free(ra->images[i].name);
	free(ra->images);

	for(unsigned i=0; i<ra->numSamples; ++i)
		free(ra->samples[i].name);
	free(ra->samples);

	free(ra);
	ra = NULL;
}

size_t ResourceGetImage(const char* name, float scale, int filtering) {
	int isImage = isImageFile(name);
	if(!ra || !isImage)
		return 0;

	for(unsigned i=0; i<ra->numImages; ++i) // lookup by name
		if(strcmp(ra->images[i].name, name)==0)
			return ra->images[i].handle;

	if(ra->uploadToGL)
		gfxGlTextureFiltering(filtering);
	else
		gfxTextureFiltering(filtering);

	size_t handle = (isImage==1) ?
		ArchiveLoadImage(ra->ar, name, ra->uploadToGL) :
		ArchiveLoadSVG(ResourceGetText(name), scale, ra->uploadToGL);
	if(!handle)
		return 0;

	// buffer:
	if(ra->numImages == ra->numImagesMax) {
		ra->numImagesMax = ra->numImagesMax ? ra->numImagesMax*2 : 1;
		ra->images = (Resource*)realloc(ra->images, ra->numImagesMax*sizeof(Resource));
	}
	Resource* res = &ra->images[ra->numImages++];
	res->handle = handle;
	res->name = strdup(name);
	return handle;
}

size_t ResourceGetAudio(const char* name) {
	if(!ra || !isAudioFile(name))
		return 0;

	for(unsigned i=0; i<ra->numSamples; ++i) // lookup by name
		if(strcmp(ra->samples[i].name, name)==0)
			return ra->samples[i].handle;

	size_t handle = ArchiveLoadAudio(ra->ar, name);
	if(!handle)
		return 0;

	// buffer:
	if(ra->numSamples == ra->numSamplesMax) {
		ra->numSamplesMax = ra->numSamplesMax ? ra->numSamplesMax*2 : 1;
		ra->samples = (Resource*)realloc(ra->samples, ra->numSamplesMax*sizeof(Resource));
	}
	Resource* res = &ra->samples[ra->numSamples++];
	res->handle = handle;
	res->name = strdup(name);
	return handle;
}

size_t ResourceGetFont(const char* name, unsigned fontSize) {
	if(!ra)
		return 0;
	for(unsigned i=0; i<ra->numFonts; ++i)
		if(strcmp(ra->fonts[i].name, name)==0 && fontSize==ra->fonts[i].size)
			return ra->fonts[i].handle;

	size_t fsize;
	void* buf = ArchiveLoadBinary(ra->ar, name, &fsize);
	if(!buf)
		return 0;

	size_t handle = ra->uploadToGL ? gfxGlFontUpload(buf, fsize, fontSize) : gfxFontUpload(buf, fsize, fontSize);
	free(buf);
	if(handle) {
		if(ra->numFonts == ra->numFontsMax) {
			ra->numFontsMax = ra->numFontsMax ? ra->numFontsMax*2 : 1;
			ra->fonts = (FontResource*)realloc(ra->fonts, ra->numFontsMax*sizeof(FontResource));
		}
		FontResource* font = &ra->fonts[ra->numFonts++];
		font->handle = handle;
		font->name = strdup(name);
		font->size = fontSize;
	}
	return handle;
}

char* ResourceGetText(const char* name) {
	if(!ra)
		return 0;

	size_t fsize = ArchiveFileSize(ra->ar, name);
	if(!fsize)
		return 0;

	char* buf =(char*)malloc(fsize+1);
	buf[fsize] = 0;
	size_t numBytesRead = ArchiveFileLoad(ra->ar, name, buf);
	if(numBytesRead != fsize) {
		free(buf);
		return 0;
	}
	return buf;
}

size_t ResourceCreateCircleImage(
	float radius, uint32_t fillColor, float strokeWidth, uint32_t strokeColor)
{
	if(!ra || radius<0.0f)
		return 0;
	if(strokeWidth<0.0f || !(strokeColor&0xff))
		strokeWidth=0.0f;

	const size_t bufLen = 256;
	char* svg = (char*)malloc(bufLen+1);
	float cx = radius+strokeWidth*0.5f;
	unsigned sz = 2*ceilf(cx);
	snprintf(svg, bufLen,"<svg width=\"%u\" height=\"%u\"><circle cx=\"%f\" cy=\"%f\" r=\"%f\" "
		"style=\"fill:#%06x;fill-opacity:%f;stroke:#%06x;stroke-opacity:%f;stroke-width:%f\"/></svg>",
		sz, sz, cx, cx, radius, fillColor >> 8, (fillColor&0xff)/255.0f,
		strokeColor >> 8, (strokeColor&0xff)/255.0f, strokeWidth);
	return ArchiveLoadSVG(svg, 1.0f, ra->uploadToGL);
}

size_t ResourceCreatePathImage(
	int width, int height, const char* path, uint32_t fillColor, float strokeWidth, uint32_t strokeColor)
{
	if(!ra || width<=0 || height<=0 || !path)
		return 0;
	size_t bufLen = strlen(path)+200;
	char* svg = (char*)malloc(bufLen+1);
	snprintf(svg, bufLen,"<svg width=\"%i\" height=\"%i\"><path d=\"%s\" "
		"style=\"fill:#%06x;fill-opacity:%f;stroke:#%06x;stroke-opacity:%f;stroke-width:%f\"/></svg>",
		width, height, path, fillColor >> 8, (fillColor&0xff)/255.0f,
		strokeColor >> 8, (strokeColor&0xff)/255.0f, strokeWidth);
	return ArchiveLoadSVG(svg, 1.0f, ra->uploadToGL);
}


size_t ResourceCreateSVGImage(const char* svg, float scale) {
	return ra ? ArchiveLoadSVG(strdup(svg), scale, ra->uploadToGL) : 0;
}

size_t ResourceCreateImage(int width, int height, const unsigned char* data, int filtering) {
	if(!ra)
		return 0;

	if(ra->uploadToGL)
		gfxGlTextureFiltering(filtering);
	else
		gfxTextureFiltering(filtering);

	return ra->uploadToGL ? gfxGlImageUpload(data, width, height, 4)
		: gfxImageUpload(data, width, height, 4);
}

//------------------------------------------------------------------
typedef struct {
	size_t handle;
	int type;
} ProtectedHandle;

static ProtectedHandle* handles=NULL;
static unsigned numHandles=0;
static unsigned numHandlesMax=0;

uint32_t ResourceProtectHandle(size_t handle, int type) {
	if(!handle || numHandles==UINT32_MAX)
		return 0;
	if(numHandlesMax==0) {
		numHandlesMax=4;
		handles = (ProtectedHandle*)malloc(numHandlesMax*sizeof(ProtectedHandle));
	}
	else if(numHandles == numHandlesMax) {
		numHandlesMax *= 2;
		handles = (ProtectedHandle*)realloc(handles, numHandlesMax*sizeof(ProtectedHandle));
	}
	handles[numHandles].handle = handle;
	handles[numHandles].type = type;
	return (uint32_t)++numHandles;
}

size_t ResourceValidateHandle(uint32_t handle, int type) {
	if(!handle || handle>numHandles)
		return 0;
	if(handles[--handle].type != type)
		return 0;
	return handles[handle].handle;
}
