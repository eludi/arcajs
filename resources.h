#pragma once

#include <stddef.h>
#include <stdint.h>

/// symbolic names for resource types
typedef enum {
	RESOURCE_NONE = 0,
	RESOURCE_IMAGE,
	RESOURCE_AUDIO,
	RESOURCE_FONT,
	RESOURCE_TEXT,
} ResourceTypeId;

/// opens a resource archive for further processing
/** @return handle of corresponding archive or 0 in case of error */
extern size_t ResourceArchiveOpen(const char* url);
/// closes currently opened resources and archive
extern void ResourceArchiveClose();
/// returns pointer to suffix of filename
extern const char* ResourceSuffix(const char* fname);
/// infers resource type from url
extern ResourceTypeId ResourceType(const char* url);
/// returns a duplicate of a resource's or file's basename (without suffix and path)
extern char* ResourceBaseName(const char* fname);
/// returns name/url of resource archive itself
extern const char* ResourceArchiveName();

/// returns handle to an image resource
/** @param scale only relevant for SVG images */
extern size_t ResourceGetImage(const char* name, float scale, int filtering);
/// returns handle to an audio resource
extern size_t ResourceGetAudio(const char* name);
/// returns handle to a font resource
extern size_t ResourceGetFont(const char* name, unsigned fontSize);
/// returns text resource, to be freed by caller
extern char* ResourceGetText(const char* name);
/// returns pointer to binary resource, mainly for plugins, to be freed by caller
extern void* ResourceGetBinary(const char* name, size_t* numBytes);

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
