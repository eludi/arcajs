#include "archive.h"

#include "external/miniz.h"

#include <string.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <dirent.h>

#ifndef _NO_SDL
#include <SDL_rwops.h>
#endif
#ifdef __ANDROID__
#include <android/log.h>
#endif

//--- functions ----------------------------------------------------

static int isDir(const char* path) {
	struct stat buf ;
	if ( stat(path,&buf) == 0 )
		return buf.st_mode & S_IFDIR ? 1 : 0;
	return -1;
}


static char* filePath(const char* path, const char* filename) {
	size_t pathlen = strlen(path);
	char * s = (char*)malloc(pathlen+strlen(filename)+2);
	strcpy(s, path);
	s[pathlen]='/';
	s[pathlen+1]=0;
	strcat(s,filename);
	return s;
}

Archive* ArchiveOpen(const char * url) {
	Archive *ar=0;
	int nFiles;
#ifdef __ANDROID__
	// fixed archive name assets.zip to be placed into the assets folder
	url = "assets.zip";
	__android_log_print(ANDROID_LOG_INFO, "arcajs", "ArchiveOpen %s", url);
	const int isDirectory = 0;
#else
	const int isDirectory = isDir(url);
	if(isDirectory<0) {
		return 0;
	}
#endif
	ar =(Archive*)malloc(sizeof(Archive));
	if(isDirectory) {
		ar->type = 0;
		ar->zipFile = 0;
	}
	else {
		ar->type = 1;

		ar->zipFile = malloc(sizeof(mz_zip_archive));
		memset(ar->zipFile, 0, sizeof(mz_zip_archive));
#ifdef __ANDROID__
		SDL_RWops *io = SDL_RWFromFile(url, "rb");
		if (!io || io->size(io)<=0)
			return 0;
		ar->zipBufSz = io->size(io);
		ar->zipBuf = malloc(ar->zipBufSz);
		const size_t numRead = SDL_RWread(io, ar->zipBuf, 1, ar->zipBufSz);
		SDL_RWclose(io);

		if (numRead != ar->zipBufSz || !mz_zip_reader_init_mem(ar->zipFile, ar->zipBuf, ar->zipBufSz, 0)) {
			free(ar->zipBuf);
			ar->zipBuf = 0;
		    ar->zipBufSz = 0;
#else
		if (!mz_zip_reader_init_file(ar->zipFile, url, 0)) {
#endif
			free(ar->zipFile);
			free(ar);
			return 0;
		}
	}

	ar->path=(char*)malloc(strlen(url)+1);
	strcpy(ar->path,url);
	nFiles = ArchiveContent(ar,&ar->pDir);
	if(nFiles<=0) {
		ar->numFiles = 0;
		ArchiveClose(ar);
		return 0;
	}

	ar->numFiles = nFiles;
	return ar;
}

int ArchiveClose(Archive *ar) {
	if(ar->pDir) {
		unsigned int i;
		for(i=0; i<ar->numFiles; i++)
			free(ar->pDir[i].filename);
		free(ar->pDir);
		ar->pDir=0;
	}
	if(ar->zipFile) {
		mz_zip_reader_end(ar->zipFile);
		free(ar->zipFile);
		ar->zipFile=0;
#ifdef __ANDROID__
		free(ar->zipBuf);
		ar->zipBuf = 0;
		ar->zipBufSz = 0;
#endif
	}
	free(ar->path);
	ar->path=0;
	free(ar);
	return 0;
}

const char* ArchivePath(const Archive* ar) {
	return ar ? ar->path : "";
}

int ArchiveFileExists(Archive *ar, const char* filename) {
	if(ar) for(unsigned int i=0; i<ar->numFiles; i++)
		if(strcmp(ar->pDir[i].filename,filename)==0)
			return 1;
	return 0;
}

static int ArchiveReadDirRecursive(Archive* ar, const char* path, FileInfo** pFileInfo,
	unsigned int* numEntries, unsigned int* capacity)
{
	size_t pathLen = strlen(path);
	int ret = 0;
	DIR *dir;
	struct dirent *ent;
	char* fullPath = pathLen ? filePath(ar->path, path) : ar->path;
	if ((dir = opendir(fullPath)) == NULL) {
		if(pathLen)
			free(fullPath);
		return -1;
	}
	if(*capacity==0) {
		*capacity = 4;
		*pFileInfo = (FileInfo*)malloc(sizeof(FileInfo) * (*capacity));
	}
	while ((ent = readdir(dir)) != NULL && ret>=0) {
		if((strcmp(ent->d_name,".")==0)||(strcmp(ent->d_name,"..")==0)) 
			continue;
		char * fullFilePath = filePath(fullPath, ent->d_name);
		size_t fileSize=0;
		struct stat buf ;
		int isDir = 0;
		if ( stat(fullFilePath,&buf) == 0 ) {
			fileSize = buf.st_size;
			isDir = buf.st_mode & S_IFDIR;
		}
		free(fullFilePath);

		if(isDir) {
			char* dirPath = pathLen ? filePath(path, ent->d_name) : strdup(ent->d_name);
			ret = ArchiveReadDirRecursive(ar, dirPath, pFileInfo, numEntries, capacity);
			free(dirPath);
			continue;
		}
		
		if(++(*numEntries)>*capacity) {
			(*capacity)<<=1;
			*pFileInfo = (FileInfo*)realloc(*pFileInfo, sizeof(FileInfo)*(*capacity));
		}
		FileInfo* zf = *pFileInfo + (*numEntries) - 1;
		zf->size = fileSize;
		zf->filename = pathLen ? filePath(path, ent->d_name) : strdup(ent->d_name);
		//printf("%i\t%s\t%u\n", (*numEntries)-1, zf->filename, (unsigned)zf->size);
	}
	if(pathLen)
		free(fullPath);
	closedir(dir);

	if(ret<0) {
		*numEntries = *capacity = 0;
		free(*pFileInfo);
		*pFileInfo = 0;
		return ret;
	}
	return (int)(*numEntries);
}

int ArchiveContent(Archive* ar, FileInfo** pFileInfo) {
	if(!ar)
		return -1;
	if(ar->type==0) { // directory
		unsigned int numEntries=0, capacity=0;
		return ArchiveReadDirRecursive(ar, "", pFileInfo, &numEntries, &capacity);
	}
	else if(ar->type==1 && ar->zipFile) { // zip archive
		int numEntries = (int)mz_zip_reader_get_num_files(ar->zipFile);
		if(numEntries==0)
			return 0;
		int numFiles = 0;

		*pFileInfo = (FileInfo*)malloc(sizeof(FileInfo)*numEntries);
		FileInfo* zf=pFileInfo[0];

		for (int i = 0; i < numEntries; i++) {
			mz_zip_archive_file_stat file_stat;
			if (!mz_zip_reader_file_stat(ar->zipFile, i, &file_stat)) {
				free(*pFileInfo);
				return -1;
			}

			if(mz_zip_reader_is_file_a_directory(ar->zipFile, i))
				continue;

			char* fname = file_stat.m_filename;
			//printf("Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u\n",
			//	fname, file_stat.m_comment, (unsigned)file_stat.m_uncomp_size, (unsigned)file_stat.m_comp_size);

			zf->size =file_stat.m_uncomp_size;
			size_t fnameLen = strlen(fname);
			zf->filename = (char*)malloc(fnameLen+1);
			memcpy(zf->filename, fname, fnameLen);
			zf->filename[fnameLen] = 0;

			++zf;
			++numFiles;
		}
		return numFiles;
	}
	return -1;
}

size_t ArchiveFileLoad(Archive *ar, const char* filename, void* ptr) {
#ifdef __ANDROID__
	__android_log_print(ANDROID_LOG_INFO, "arcajs", "ArchiveFileLoad %s", filename);
#endif

	unsigned int i;
	for(i=0; i<ar->numFiles; i++)
		if(strcmp(ar->pDir[i].filename,filename)==0)
			break;
	if(i==ar->numFiles)
		return 0;
	
	if(ar->type==0) { // directory
		char * url = filePath(ar->path,filename);
#ifdef __ANDROID__
	__android_log_print(ANDROID_LOG_INFO, "arcajs", "ArchiveFileLoad  url %s", url);
#endif

		size_t ret;
#ifdef _NO_SDL
		FILE *fp = fopen(url, "rb");
		free(url);
		if (!fp)
			return 0;
		ret = fread(ptr, 1, ar->pDir[i].size, fp);
		fclose(fp);
#else
		SDL_RWops *io = SDL_RWFromFile(url, "rb");
		free(url);
		if (!io || io->size(io)<=0)
			return 0;
		ret = SDL_RWread(io, ptr, 1, ar->pDir[i].size);
		SDL_RWclose(io);
#endif
		return ret;
	}

	if(ar->type==1) { // zip archive
		size_t bytesRead;
		void* p = mz_zip_reader_extract_file_to_heap(ar->zipFile, filename, &bytesRead, 0);
		if(!p)
			return 0;
		memcpy(ptr, p, bytesRead);
		mz_free(p);
		return bytesRead;
	}
	return 0;
}

size_t ArchiveFileSize(Archive* ar, const char* filename) {
	unsigned int i;
	for(i=0; i<ar->numFiles; ++i)
		if(strcmp(ar->pDir[i].filename,filename)==0) 
			return ar->pDir[i].size;
	return 0;
}
