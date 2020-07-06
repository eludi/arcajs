#include "archive.h"

#if defined(__GNUC__)
	// Ensure we get the 64-bit variants of the CRT's file I/O calls
	#ifndef _FILE_OFFSET_BITS
		#define _FILE_OFFSET_BITS 64
	#endif
	#ifndef _LARGEFILE64_SOURCE
		#define _LARGEFILE64_SOURCE 1
	#endif
#endif
#include "external/miniz.h"

#include <string.h>
#include <stdlib.h>

# include <sys/stat.h>
#if defined __WIN32__ || defined WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <direct.h>
#endif

#ifndef _MSC_VER
# include <dirent.h>
#endif

//--- functions ----------------------------------------------------

static int isDir(const char* path) {
#ifdef _MSC_VER
	struct _stat buf;
	if ( _stat(path,&buf) == 0 )
		return (buf.st_mode & _S_IFDIR) ? 1 : 0;
#else
	struct stat buf ;
	if ( stat(path,&buf) == 0 )
		return buf.st_mode & S_IFDIR ? 1 : 0;
#endif
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
	int isDirectory = isDir(url);
	if(isDirectory<0)
		return 0;
	ar =(Archive*)malloc(sizeof(Archive));
	if(isDirectory) {
		ar->type = 0;
		ar->zipFile = 0;
	}
	else {
		ar->type = 1;

		ar->zipFile = malloc(sizeof(mz_zip_archive));
		memset(ar->zipFile, 0, sizeof(mz_zip_archive));
		if (!mz_zip_reader_init_file(ar->zipFile, url, 0)) {
			free(ar->zipFile);
			free(ar);
			return 0;
		}
	}

	ar->path=(char*)malloc(strlen(url)+1);
	strcpy(ar->path,url);
	nFiles = ArchiveContent(ar,&ar->pDir);
	if(nFiles<=0) {
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
	}
	free(ar->path);
	ar->path=0;
	free(ar);
	return 0;
}

int ArchiveFileExists(Archive *ar, const char* filename) {
	if(ar) for(unsigned int i=0; i<ar->numFiles; i++)
		if(strcmp(ar->pDir[i].filename,filename)==0)
			return 1;
	return 0;
}

int ArchiveContent(Archive* ar, FileInfo** pFileInfo) {
	if(!ar)
		return -1;
	if(ar->type==0) { // directory
#ifdef _MSC_VER
		WIN32_FIND_DATA findFileData;
		HANDLE hFind;
		unsigned int numEntries = 0;
		unsigned int capacity = 4; // lets start with space for 4 entries
		char *tmp = (char*)malloc(strlen(ar->path)+5);
		strcpy(tmp,ar->path);
		strcat(tmp,"\\*.*");
		hFind = FindFirstFile(tmp, &findFileData);
		free(tmp);
		if (hFind == INVALID_HANDLE_VALUE) return -1;
		*pFileInfo = (FileInfo*)malloc(sizeof(FileInfo)*capacity);
		do {
			FileInfo* zf;
			if(findFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
				continue;
			if(++numEntries>capacity) {
				capacity<<=1;
				*pFileInfo = (FileInfo*)realloc(*pFileInfo, sizeof(FileInfo)*capacity);
			}
			zf=*pFileInfo+numEntries-1;
			zf->size = findFileData.nFileSizeLow; // more than 4GB are stll unlikely...
			zf->filename = (char*)malloc(strlen(findFileData.cFileName)+1);
			strcpy(zf->filename,findFileData.cFileName);
		} while(FindNextFile(hFind, &findFileData));		
		FindClose(hFind);
#else
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(ar->path)) == NULL)
			return -1;
		unsigned int numEntries = 0;
		unsigned int capacity = 4; // lets start with space for 4 entries
		*pFileInfo = (FileInfo*)malloc(sizeof(FileInfo)*capacity);
		while ((ent = readdir(dir)) != NULL) {
			if((strcmp(ent->d_name,".")==0)||(strcmp(ent->d_name,"..")==0)) 
				continue;
			char * completeFilePath = filePath(ar->path,ent->d_name);
			size_t fileSize=0;
			struct stat buf ;
			int isDir = 0;
			if ( stat(completeFilePath,&buf) == 0 ) {
				fileSize = buf.st_size;
				isDir = buf.st_mode & S_IFDIR;
			}
			free(completeFilePath);
			if(isDir)
				continue;
			
			numEntries++;
			if(numEntries>capacity) {
				capacity<<=1;
				*pFileInfo = (FileInfo*)realloc(*pFileInfo, sizeof(FileInfo)*capacity);
			}
			FileInfo* zf=*pFileInfo+numEntries-1;
			zf->size = fileSize;
			zf->filename = (char*)malloc(strlen(ent->d_name)+1);
			strcpy(zf->filename, ent->d_name);
			//printf("%i\t%s\t%li\n", numEntries-1, zf->filename, zf->size);
		}
		closedir(dir);
#endif
		return numEntries;
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
	unsigned int i;
	for(i=0; i<ar->numFiles; i++)
		if(strcmp(ar->pDir[i].filename,filename)==0)
			break;
	if(i==ar->numFiles)
		return 0;
	
	if(ar->type==0) { // directory
		char * url = filePath(ar->path,filename);
		FILE *fp;
		size_t ret;
#ifdef _MSC_VER
		fopen_s(&fp, url, "rb");
#else
		fp = fopen(url, "rb");
#endif
		free(url);
		if (!fp)
			return 0;
		ret = fread(ptr, 1, ar->pDir[i].size, fp);
		fclose(fp);
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
