#pragma once
#include <stdio.h>

/// helper struct storing information on zip archive member files
typedef struct fileInfo_s {
	/// name of the archived file
	char* filename;
	/// uncompressed size of the file in archive
	size_t size;
} FileInfo;

typedef struct Archive_s {
	enum {
		TypeDir = 0,
		TypeZip = 1,
	} ArchiveType;
	/// stores file handle of archive
	void* zipFile;
#if defined(__ANDROID__)
	/// on Android, archive file ops need to be performed via SDL_RWops, therefore archive needs tob e read into a memory buffer first
	void* zipBuf;
	/// size of memory buffer
	size_t zipBufSz;
#endif
	/// pointer to directory structure
	FileInfo* pDir;
	/// number of files in current archive/directory
	unsigned int numFiles;
	/// stores type, currently 0 is normal directory, 1 zip archive
	int type;
	/// stores path to archive / directory
	char * path;
} Archive;

// function collection for reading from directories / zip archives

/// opens a resource archive for further reading.
Archive* ArchiveOpen(const char* url);
/// closes currently opened resource archive
int ArchiveClose(Archive* ar);
/// returns path and name of archive
const char* ArchivePath(const Archive* ar);
/// reads information about files in the archive
/** \return number of fileInfos returned, or otherwise an error code */
int ArchiveContent(Archive* ar, FileInfo** pFileInfo);

/// tests whether file file exists and returns 1 in case of existence.
int ArchiveFileExists(Archive* ar, const char* filename);
/// loads a file from current Archive in one step
/** \param ptr must point to a memory area large enough to hold the file
 \return number of bytes actually read */
size_t ArchiveFileLoad(Archive* ar, const char* filename, void* ptr);
/// returns the uncompressed file size in bytes.
/** This function is transparent for files in zip archives. */
size_t ArchiveFileSize(Archive* ar, const char* filename);
