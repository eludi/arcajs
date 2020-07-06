#pragma once

#include <stddef.h>
#include <stdint.h>

/// symbolic names for resource types, useful for protecting/validating handles
typedef enum {
	RESOURCE_NONE = 0,
	RESOURCE_IMAGE,
	RESOURCE_AUDIO,
	RESOURCE_FONT,
	RESOURCE_TEXT,
} ResourceType;

/// opens a resource archive for further processing
/** @return handle of corresponding archive or 0 in case of error */
extern size_t ResourceArchiveOpen(const char* url, int uploadToGL);
/// closes currently opened resources and archive
extern void ResourceArchiveClose();
/// returns pointer to suffix of filename
extern const char* ResourceSuffix(const char* fname);
/// returns a duplicate of a resource's or file's basename (without suffix and path)
extern char* ResourceBaseName(const char* fname);
/// return1 if graphics resources are uploaded to OpenGL
extern int ResourceUploadsToGL();

/// returns handle to an image resource
/** @param scale only relevant for SVG images */
extern size_t ResourceGetImage(const char* name, float scale, int filtering);
/// returns handle to an audio resource
extern size_t ResourceGetAudio(const char* name);
/// returns handle to a font resource
extern size_t ResourceGetFont(const char* name, unsigned fontSize);
/// returns text resource, to be freed by caller
extern char* ResourceGetText(const char* name);

/// returns handle to an image resource created from circle parameters
extern size_t ResourceCreateCircleImage(
	float radius, uint32_t fillColor, float strokeWidth, uint32_t strokeColor);
/// returns handle to an image resource created from a path
extern size_t ResourceCreatePathImage(
	int width, int height, const char* path, uint32_t fillColor, float strokeWidth, uint32_t strokeColor);
/// returns handle to an image resource created from an inline SVG string
extern size_t ResourceCreateSVGImage(const char* svg, float scale);
/// returns handle to an RGBA image resource created from a buffer
extern size_t ResourceCreateImage(int width, int height, const unsigned char* data, int filtering);

/// adds an indirection making handles validable
extern uint32_t ResourceProtectHandle(size_t handle, int type);
/// validates a guarded handle, resolves indirection
extern size_t ResourceValidateHandle(uint32_t handle, int type);
