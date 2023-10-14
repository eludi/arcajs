#include "../external/duk_config.h"
#include "../external/duktape.h"
#include <stdio.h>
#include <stdlib.h>

# include <sys/stat.h>
# include <dirent.h>

static duk_ret_t readFileSync(duk_context *ctx) {
	const char* fname = duk_to_string(ctx, 0);
	duk_bool_t isBufferRequested = duk_is_undefined(ctx, 1);

	FILE *f=fopen(fname, "rb");
	if (!f)
		return duk_error(ctx, DUK_ERR_TYPE_ERROR, "readFileSync file \"%s\" not found\n", fname);

	fseek(f,0,SEEK_END);
	size_t sz = ftell(f);
	rewind(f);
	int isOk = 1;

	if(!isBufferRequested) {
		char* buffer = (char*)malloc(sz+1);
		buffer[sz] = 0;
		if(fread(buffer, 1, sz, f)==sz)
			duk_push_lstring(ctx, buffer, sz);
		else
			isOk = 0;
		free(buffer);
	}
	else {
		void* buffer = duk_push_fixed_buffer(ctx, sz);
		if(fread(buffer, 1, sz, f)==sz)
			duk_push_buffer_object(ctx, -1, sz, sz, DUK_BUFOBJ_ARRAYBUFFER);
		else
			isOk = 0;
	}
	fclose(f);
	if(!isOk)
		return duk_error(ctx, DUK_ERR_TYPE_ERROR, "readFileSync file \"%s\" read error\n", fname);
	return 1;
}

static duk_ret_t writeFileSync(duk_context *ctx) {
	const char* fname = duk_to_string(ctx, 0);

	FILE *f=fopen(fname, "wb");
	if (!f)
		return duk_error(ctx, DUK_ERR_TYPE_ERROR, "writeFileSync failed to open file \"%s\"\n", fname);

	if(duk_is_string(ctx, 1)) {
		duk_size_t sz;
		const char* s = duk_get_lstring(ctx, 1, &sz);
		if(s && sz)
			if(fwrite(s, 1, sz, f) != sz)
				fprintf(stderr, "writeFileSync failed to write to file \"%s\"\n", fname);
	}
	else if(duk_is_buffer_data(ctx, 1)) {
		duk_size_t sz;
		void* data = duk_get_buffer_data(ctx, 1, &sz);
		if(sz)
			if(fwrite(data,1,sz, f) != sz)
				fprintf(stderr, "writeFileSync failed to write to file \"%s\"\n", fname);
	}
	fclose(f);
	return 0;
}

static duk_ret_t systemCall(duk_context *ctx) {
	const char* cmd = duk_to_string(ctx, 0);
	int ret = system(cmd);
	duk_push_int(ctx, ret);
	return 1;
}

static duk_ret_t renameFile(duk_context *ctx) {
	const char* oldName = duk_to_string(ctx, 0);
	const char* newName = duk_to_string(ctx, 1);
	int ret = rename(oldName, newName);
	duk_push_boolean(ctx, ret == 0);
	return 1;
}

static duk_ret_t removeFile(duk_context *ctx) {
	const char* fname = duk_to_string(ctx, 0);
	int ret = remove(fname);
	duk_push_boolean(ctx, ret == 0);
	return 1;
}

static duk_ret_t execSync(duk_context *ctx) {
	const char* cmd = duk_to_string(ctx, 0);
	FILE *fp = popen(cmd, "r");
	if (fp == NULL)
		return duk_error(ctx, DUK_ERR_TYPE_ERROR, "Failed to execute command %s\n", cmd);

	char line[2048];
	duk_idx_t arr = duk_push_array(ctx);
	duk_uarridx_t idx = 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		size_t len = strlen(line);
		if(len && line[len-1]=='\n')
			line[--len]=0;
		duk_push_lstring(ctx, line, len);
		duk_put_prop_index(ctx, arr, idx++);
	}

	pclose(fp);
	return 1;
}

static duk_ret_t readDirSync(duk_context *ctx) {
	const char* path = duk_to_string(ctx, 0);
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(path)) == NULL)
		return  duk_error(ctx, DUK_ERR_TYPE_ERROR, "Failed to open directory %s\n", path);

	duk_idx_t arr = duk_push_array(ctx);
	duk_uarridx_t idx = 0;

	while ((ent = readdir(dir)) != NULL) {
		if((strcmp(ent->d_name,".")==0)||(strcmp(ent->d_name,"..")==0)) 
			continue;
	
		size_t fileSize=0;
		struct stat buf;
		duk_bool_t isDir = 0;
		if ( stat(ent->d_name, &buf) == 0 ) {
			fileSize = buf.st_size;
			isDir = buf.st_mode & S_IFDIR;
		}

		duk_idx_t obj = duk_push_object(ctx);
		duk_push_string(ctx, ent->d_name);
		duk_put_prop_literal(ctx, obj, "name");
		duk_push_number(ctx, fileSize);
		duk_put_prop_literal(ctx, obj, "size");
		duk_push_boolean(ctx, isDir);
		duk_put_prop_literal(ctx, obj, "isDir");

		duk_put_prop_index(ctx, arr, idx++);
	}
	closedir(dir);
	return 1;
}

static duk_ret_t osPlatform(duk_context *ctx) {
#if defined __WIN32__ || defined WIN32
	duk_push_literal(ctx, "win32");
#elif defined __APPLE__
	duk_push_literal(ctx, "macos");
#elif defined __ANDROID__
	duk_push_literal(ctx, "android");
#elif defined __linux__
	duk_push_literal(ctx, "linux");
#else
	duk_push_literal(ctx, "unknown");
#endif
	return 1;
}


void os_exports(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, readFileSync, 2);
	duk_put_prop_literal(ctx, -2, "readFileSync");
	duk_push_c_function(ctx, writeFileSync, 3);
	duk_put_prop_literal(ctx, -2, "writeFileSync");
	duk_push_c_function(ctx, systemCall, 1);
	duk_put_prop_literal(ctx, -2, "system");
	duk_push_c_function(ctx, execSync, 1);
	duk_put_prop_literal(ctx, -2, "execSync");
	duk_push_c_function(ctx, readDirSync, 1);
	duk_put_prop_literal(ctx, -2, "readDirSync");
	duk_push_c_function(ctx, renameFile, 2);
	duk_put_prop_literal(ctx, -2, "rename");
	duk_push_c_function(ctx, removeFile, 1);
	duk_put_prop_literal(ctx, -2, "remove");

	duk_push_literal(ctx, "platform");
	duk_push_c_function(ctx, osPlatform, 0);
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_GETTER |
		DUK_DEFPROP_HAVE_CONFIGURABLE | DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE);
}
