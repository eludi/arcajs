#include "../archive.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char**argv) {
	if(argc<2) {
		fprintf(stderr, "usage: %s archive [file number]\n", argv[0]);
		return 1;
	}

	Archive* ar = ArchiveOpen(argv[1]);
	if(!ar) {
		fprintf(stderr, "could not open file %s\n", argv[1]);
		return 1;
	}
	unsigned int i=0;
	for(; i<ar->numFiles; ++i)
		printf("%i\t%s\t%u\n", i, ar->pDir[i].filename, (unsigned)ar->pDir[i].size);
	if(!ar->numFiles)
		return 0;
	
	if(argc>2) {
		unsigned int i=atoi(argv[2]);
		if(i<ar->numFiles) {
			const char* fname = ar->pDir[i].filename;
			size_t fsize = ArchiveFileSize(ar, fname);
			char * buf =(char*)malloc(fsize+1);
			buf[fsize]=0;
			size_t nBytes = ArchiveFileLoad(ar, fname, buf);
			printf("--- file:%s nBytes:%u ---\n%s\n---\n", fname, (unsigned)nBytes, buf);
			free(buf);
		}
	}
	
	fflush(stdout);
	ArchiveClose(ar);
	return 0;
}
