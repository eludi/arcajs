
#include "../external/duk_config.h"
#include "../external/duktape.h"

#if defined __WIN32__ || defined WIN32
# define WINDOWS_LEAN_AND_MEAN
# include <windows.h>
# define _EXPORT __declspec(dllexport)
#else
#  include <unistd.h>
# define _EXPORT
#endif

#include <stdio.h>
#include <stdlib.h>

static duk_ret_t dk_hello(duk_context *ctx) {
	const char* msg = duk_to_string(ctx, 0);
	printf("hello, %s.\n", msg);
	return 0;
}

void _EXPORT dllTest_exports(duk_context *ctx) {
	duk_push_object(ctx);
	duk_push_c_function(ctx, dk_hello, 1);
	duk_put_prop_string(ctx, -2, "hello");
}
