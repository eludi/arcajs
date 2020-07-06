#pragma once

// simple JSON struct / variant type in C
// (c) 2019-05-27 by gerald.franz@eludi.net

#include <stdio.h>

//--- struct Value --------------------------------------------------

enum Type {
	VALUE_NONE ='_',
	VALUE_BOOL = '?',
	VALUE_STRING = '$',
	VALUE_SYMBOL = '@',
	VALUE_INT = '#',
	VALUE_FLOAT = '%',
	VALUE_LIST = '[',
	VALUE_MAP = '{',

	CTRL_ERR = '~',
	CTRL_LISTEND = ']',
	CTRL_MAPEND = '}',
};


typedef struct Value {
	union {
		/// stores size
		signed long long m_size;
		struct {
			/// unused padding
			unsigned char padding[6];
			/// stores type
			unsigned char type;
			/// stores capacity
			unsigned char capacity;
		};
	};

	union {
		signed long long i;
		double f;
		char* str;
		struct Value* child;
	};
	/// pointer to next Value
	struct Value* next;
} Value;

extern Value* Value_new(char type, const char* value);
extern Value* Value_str(const char* value);
extern Value* Value_int(signed long long value);
extern Value* Value_bool(int value);
extern Value* Value_float(double value);
extern Value* Value_parse(const char* text);
extern void Value_delete(Value* v, int deleteChain);

/// append to a list
extern void Value_append(Value* parent, Value* child);
/// set key-value of a map
extern void Value_set(Value* parent, const char* key, Value* value);
/// get value of a map identified by its key
extern Value* Value_get(Value* parent, const char* key);
/// returns key n of a map or NULL, if beyond range
extern const char* Value_key(const Value* parent, unsigned n);
/// get list element
extern Value* Value_at(Value* parent, unsigned n);
/// gets value identified by its path, e.g. "/species/1/color/0"
extern Value* Value_atPath(Value* parent, const char* path);
/// output to a file stream
extern void Value_print(Value* v, FILE* stream);
