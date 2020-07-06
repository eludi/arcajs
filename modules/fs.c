#include "../external/duk_config.h"
#include "../external/duktape.h"
#include <stdio.h>

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

void fs_exports(duk_context *ctx) {
	duk_push_object(ctx);

	duk_push_c_function(ctx, readFileSync, 2);
	duk_put_prop_literal(ctx, -2, "readFileSync");
	duk_push_c_function(ctx, writeFileSync, 3);
	duk_put_prop_literal(ctx, -2, "writeFileSync");
}
