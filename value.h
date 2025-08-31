#ifndef _VALUE_H
#define _VALUE_H
// simple JSON struct / variant type in C
// (c) 2019-05-27 by gerald.franz@eludi.net

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

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
	VALUE_ERR = '!',
	VALUE_BUF = '&',

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
		uint8_t* buf;
		struct Value* child;
	};
	/// pointer to next Value
	struct Value* next;
} Value;

extern Value* Value_new(char type, const char* value);
extern Value* Value_str(const char* value);
extern Value* Value_int(signed long long value);
extern Value* Value_bool(bool value);
extern Value* Value_float(double value);
extern Value* Value_err(const char* msg);
/// creates a byte buffer of a given size. Either initializes it by copying the provided buffer or by setting it to zeros
extern Value* Value_buf(size_t size, const uint8_t* buf);
extern Value* Value_parse(const char* text);
extern Value* Value_parseXML(const char* xmlstr, const char** customCodes);

extern void Value_delete(Value* v, bool deleteChain);

/// append to a list
extern void Value_append(Value* parent, Value* child);
/// returns and removes first element of a list, or a NULL pointer if empty
extern Value* Value_popf(Value* parent);
/// returns true if a list value is empty
extern bool Value_empty(const Value* v);
/// set a key-value pair of a map
extern void Value_set(Value* parent, const char* key, Value* value);
/// set a string key-value pair of a map
extern void Value_sets(Value* parent, const char* key, const char* value);
/// get value of a map identified by its key
extern Value* Value_get(const Value* parent, const char* key);
/// gets integer value of a map identified by its key
extern signed long long Value_geti(const Value* parent, const char* key, signed long long defaultValue);
/// gets string value of a map identified by its key
extern const char* Value_gets(const Value* parent, const char* key, const char* defaultValue);
/// gets float value of a map identified by its key
extern double Value_getf(const Value* parent, const char* key, double defaultValue);
/// returns key n of a map or NULL, if beyond range
extern const char* Value_key(const Value* parent, unsigned n);
/// get list element
extern Value* Value_at(const Value* parent, unsigned n);
/// gets value identified by its path, e.g. "/species/1/color/0"
extern Value* Value_atPath(const Value* parent, const char* path);
/// output to a file stream
extern void Value_print(const Value* v, FILE* stream);

//--- Stack --------------------------------------------------------

/// a generic stack structure of non-owning void pointers, useful for writing Value parsers
typedef struct Stack {
	void** data;
	size_t sz;
	size_t capacity;
} Stack;

extern void Stack_init(Stack* stack, size_t initial_capacity);
extern void* Stack_top(Stack* stack);
extern void* Stack_at(Stack* stack, size_t index);
extern void Stack_push(Stack* stack, void* data);
extern void* Stack_pop(Stack * stack);
extern void Stack_delete(Stack* stack);
#endif // _VALUE_H

#ifdef VALUE_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

//--- struct Value --------------------------------------------------

Value* Value_new(char type, const char* value) {
	if(type==CTRL_ERR || type==CTRL_LISTEND || type==CTRL_MAPEND)
		return 0;
	Value* v = (Value*)malloc(sizeof(Value));
	v->type = type;
	v->next = NULL;
	switch(type) {
	case VALUE_INT: {
		char * end;
		v->i = strtol(value, &end, 10);
		break;
	}
	case VALUE_FLOAT: {
		char * end;
		v->f = strtod(value, &end);
		break;
	}
	case VALUE_ERR:
	case VALUE_SYMBOL:
	case VALUE_STRING: {
		size_t sz = strlen(value);
		v->str = (char *)malloc(sz+1);
		memcpy(v->str, value, sz);
		v->str[sz]=0;
		break;
	}
	case VALUE_BOOL:
		v->i = (strncmp(value, "true", 4)==0);
		break;
	default:
		v->i = 0;
	};
	return v;
}

Value* Value_str(const char* value) { return Value_new(VALUE_STRING, value); }
Value* Value_err(const char* msg) { return Value_new(VALUE_ERR, msg); }
Value* Value_int(signed long long value) {
	Value* v = (Value*)malloc(sizeof(Value));
	v->type = VALUE_INT;
	v->next = NULL;
	v->i = value;
	return v;
}
Value* Value_bool(bool value) {
	Value* v = (Value*)malloc(sizeof(Value));
	v->type = VALUE_BOOL;
	v->next = NULL;
	v->i = value ? 1 : 0;
	return v;
}
Value* Value_float(double value) {
	Value* v = (Value*)malloc(sizeof(Value));
	v->type = VALUE_FLOAT;
	v->next = NULL;
	v->f = value;
	return v;
}
Value* Value_buf(size_t size, const uint8_t* buf) {
	Value* v = (Value*)malloc(sizeof(Value));
	v->m_size = size;
	if(size) {
		v->buf = (uint8_t*)malloc(size);
		if(buf)
			memcpy(v->buf, buf, size);
		else
			memset(v->buf, 0, size);
	} else {
		v->buf = NULL;
	}
	v->type = VALUE_BUF;
	v->next = NULL;
	return v;
}

void Value_delete(Value* v, bool deleteChain) {
	if(!v)
		return;
	if(deleteChain)
		Value_delete(v->next, 1);
	switch(v->type) {
	case VALUE_LIST:
	case VALUE_MAP:
		Value_delete(v->child, 1);
		break;
	case VALUE_SYMBOL:
	case VALUE_STRING:
		free(v->str);
		break;
	case VALUE_BUF:
		free(v->buf);
		break;
	}
	free(v);
}

void Value_append(Value* parent, Value* child) {
	if(parent->type != VALUE_LIST && parent->type != VALUE_MAP) // MAP temporary
		return;
	if(!parent->child) {
		parent->child=child;
		return;
	}
	Value* sibling = parent->child;
	while(sibling->next)
		sibling = sibling->next;
	sibling->next = child;
}

Value* Value_popf(Value* parent) {
	if(parent->type != VALUE_LIST || !parent->child)
		return 0;
	Value* child = parent->child;
	parent->child = child->next;
	child->next = 0;
	return child;
}

bool Value_empty(const Value* v) {
	return v && v->type==VALUE_LIST && !v->child;
}

void Value_set(Value* parent, const char* key, Value* value) {
	if(!parent || parent->type != VALUE_MAP)
		return;
	if(!parent->child) {
		parent->child=Value_new(VALUE_STRING, key);
		parent->child->next = value;
		return;
	}
	Value* sibling = parent->child;
	while(1) {
		if(strcmp(key, sibling->str)==0) {
			Value_delete(sibling->next, 0);
			sibling->next = value;
			return;
		}
		if(sibling->next->next)
			sibling = sibling->next->next;
		else {
			sibling = sibling->next;
			break;
		}
	}
	sibling->next = Value_new(VALUE_STRING, key);
	sibling->next->next = value;
}

void Value_sets(Value* parent, const char* key, const char* value) {
	Value_set(parent, key, Value_new(VALUE_STRING, value));
}

Value* Value_get(const Value* parent, const char* key) {
	if(!parent || parent->type != VALUE_MAP)
		return 0;
	Value *pKey = parent->child;
	while(pKey) {
		Value* pValue = pKey->next;
		if(strcmp(key, pKey->str)==0)
			return pValue;
		pKey = pValue->next;
	}
	return 0;
}

static Value* Value_getn(const Value* parent, const char* key, size_t n) {
	if(!parent || parent->type != VALUE_MAP)
		return 0;
	Value *pKey = parent->child;
	while(pKey) {
		Value* pValue = pKey->next;
		if(strncmp(key, pKey->str, n)==0)
			return pValue;
		pKey = pValue->next;
	}
	return 0;
}

signed long long Value_geti(const Value* parent, const char* key, signed long long defaultValue) {
	Value* v = Value_get(parent, key);
	if(!v)
		return defaultValue;
	switch(v->type) {
		case VALUE_BOOL:
		case VALUE_INT: return v->i;
		case VALUE_FLOAT: return v->f;
		case VALUE_STRING: return atoll(v->str);
		default: return defaultValue;
	}
}

double Value_getf(const Value* parent, const char* key, double defaultValue) {
	Value* v = Value_get(parent, key);
	if(!v)
		return defaultValue;
	switch(v->type) {
		case VALUE_BOOL:
		case VALUE_INT: return v->i;
		case VALUE_FLOAT: return v->f;
		case VALUE_STRING: return atof(v->str);
		default: return defaultValue;
	}
}

const char* Value_gets(const Value* parent, const char* key, const char* defaultValue) {
	Value* v = Value_get(parent, key);
	if(v && v->type==VALUE_STRING)
		return v->str;
	return defaultValue;
}

Value* Value_at(const Value* parent, unsigned n) {
	if(!parent || parent->type != VALUE_LIST)
		return 0;
	Value* child = parent->child;
	for(unsigned i=0; i<n && child; ++i)
		child = child->next;
	return child;
}

const char* Value_key(const Value* parent, unsigned n) {
	if(!parent || parent->type != VALUE_MAP)
		return 0;
	unsigned i=0;
	for(const Value *key = parent->child; key!=NULL; key=key->next->next, ++i)
		if(i==n)
			return key->str;
	return NULL;
}

Value* Value_atPath(const Value* parent, const char* path) {
	if(!parent || !path)
		return 0;
	size_t pos = 0, sz = strlen(path);

	if(pos<sz && path[pos]=='/')
		++pos;
	size_t nextSep = pos;
	while(nextSep<sz && path[nextSep]!='/')
		++nextSep;
	if(nextSep==pos)
		return 0;

	if(path[pos]>='0' && path[pos]<='9') {
		unsigned index = strtoul(&path[pos], 0, 10);
		parent = Value_at(parent, index);
	}
	else
		parent = Value_getn(parent, &path[pos], nextSep-pos);
	pos = nextSep+1;
	if(pos>=sz)
		return (Value*)parent;
	return Value_atPath(parent, &path[pos]);
}


//--- tokenizer ----------------------------------------------------

typedef struct Tokenizer {
	/// stores string to be tokenized
	const char* s;
	/// stores size of string to be tokenized
	size_t s_size;
	/// stores current read position
	size_t i;
	/// pointer to current token
	char* m_token;
	/// size of current token
	size_t m_token_size;
	/// capacity of current token
	size_t m_token_capacity;
	/// stores current token type
	char type;
} Tokenizer;

static Tokenizer* Tokenizer_new(const char* str, size_t str_size) {
	Tokenizer *tok = (Tokenizer*)malloc(sizeof(Tokenizer));
	memset(tok, 0, sizeof(Tokenizer));
	tok->s_size = str_size;
	tok->s = str;
	tok->type = ';';
	return tok;
}

static void Tokenizer_delete(Tokenizer* tok) {
	free(tok->m_token);
	free(tok);
}

static void Tokenizer_append(Tokenizer* tok, char ch) {
	if(tok->m_token_size+1>=tok->m_token_capacity) {
		tok->m_token_capacity = (tok->m_token_capacity==0) ? 16 : tok->m_token_capacity*2;
		tok->m_token = (char*)realloc(tok->m_token, tok->m_token_capacity);
	}
	tok->m_token[tok->m_token_size]=ch;
	tok->m_token[++tok->m_token_size]=0;
}

static const char* Tokenizer_next(Tokenizer* tok) {
	if(tok->m_token) {
		free(tok->m_token);
		tok->m_token = 0;
		tok->m_token_size=tok->m_token_capacity = 0;
	}

	int quotMode=0;
	int tokenComplete = 0;
	while(tok->i < tok->s_size) {
		int advance=1;
		switch(tok->s[tok->i]) {
		case '"':
		case '\'':
			if(!quotMode) {
				if(tok->m_token_size) {
					tokenComplete=1;
					advance=0;
				}
				quotMode=tok->s[tok->i];
			}
			else if(quotMode==tok->s[tok->i])
				quotMode=0;
			Tokenizer_append(tok, tok->s[tok->i]);
			break;
		case VALUE_LIST:
		case VALUE_MAP:
		case CTRL_LISTEND:
		case CTRL_MAPEND:
			if(quotMode)
				Tokenizer_append(tok, tok->s[tok->i]);
			else {
				if(tok->m_token_size)
					advance=0;
				else Tokenizer_append(tok, tok->s[tok->i]);
				tokenComplete=1;
			}
			break;
		case '\r':
			break; // ignore
		case ' ':
		case '\t':
		case '\n':
		case ':':
		case ',':
			if(quotMode)
				Tokenizer_append(tok, tok->s[tok->i]);
			else if(tok->m_token_size)
				tokenComplete=1;
			break;
		default:
			Tokenizer_append(tok, tok->s[tok->i]);
			tokenComplete=0;
		}
		tok->i+=advance;

		if((tok->i>=tok->s_size)||(tokenComplete&&tok->m_token_size)) {
			tokenComplete=0;
			if(tok->m_token_size)
				break;
		}
	}
	// determine token type:
	if(!tok->m_token_size)
		tok->type=0;
	else switch(tok->m_token[0]) {
	case '[':
	case ']':
	case '{':
	case '}':
		tok->type=tok->m_token[0];
		break;
	case '.':
	case '+':
	case '-':
		if(tok->m_token_size==1) {
			tok->type = VALUE_SYMBOL;
			break;
		}
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		// if  strtod endptr<m_token_size ERROR invalid number
		tok->type = (strcspn(tok->m_token, ".e")<tok->m_token_size) ? VALUE_FLOAT : VALUE_INT;
		break;
	case '\'':
	case '"':
		tok->type=VALUE_STRING;
		tok->m_token[tok->m_token_size-1]=0;

		return tok->m_token+1;
	default:
		tok->type=VALUE_SYMBOL;
	}

	if(tok->type == VALUE_SYMBOL) {
		if((tok->m_token_size==4 && strncmp(tok->m_token, "true", tok->m_token_size)==0)
			|| (tok->m_token_size==5 && strncmp(tok->m_token, "false", tok->m_token_size)==0))
			tok->type = VALUE_BOOL;
	}
	return tok->m_token;
}

//--- Stack --------------------------------------------------------

void Stack_init(Stack * stack, size_t capacity) {
	stack->sz=0;
	stack->capacity = capacity;
	stack->data = (void**)malloc(capacity*sizeof(void*));
}
void* Stack_top(Stack* stack) {
	return stack->sz ? stack->data[stack->sz-1] : 0;
}
void* Stack_at(Stack* stack, size_t index) {
	return index<stack->sz ? stack->data[index] : 0;
}
void Stack_push(Stack * stack, void * data) {
	if(stack->sz==stack->capacity) {
		stack->capacity*=2;
		stack->data = (void**)realloc(stack->data, stack->capacity*sizeof(void*));
	}
	stack->data[stack->sz] = data;
	++stack->sz;
}
void* Stack_pop(Stack * stack) {
	return !stack->sz ? 0 : stack->data[--stack->sz];
}
void Stack_delete(Stack* stack) {
	free(stack->data);
	stack->data = 0;
	stack->capacity = 0;
}

//--- parser -------------------------------------------------------

Value* Value_parse(const char * src) {
	const int nestingLimit = 256;
	char nesting[nestingLimit];
	int nestingLevel=0;
	nesting[nestingLevel]=0;

	Value* root = 0;
	Stack stack;
	Stack_init(&stack, 16);

	Tokenizer* tok = Tokenizer_new(src, strlen(src));
	const char* token = 0;
	while((token=Tokenizer_next(tok))!=0) {
		char type=tok->type;
		switch(type) {
		case VALUE_LIST:
		case VALUE_MAP: {
			nesting[nestingLevel]=type;
			nesting[++nestingLevel]=0;

			Value* v = Value_new(type, token);
			if(root)
				Value_append(Stack_top(&stack), v);
			else
				root = v;
			Stack_push(&stack, v);
			break;
		}
		case CTRL_MAPEND: 
		case CTRL_LISTEND: {
			if(nestingLevel && nesting[nestingLevel-1] == type-2) {
				nesting[--nestingLevel]=0;
				Stack_pop(&stack);
			}
			// else ERROR invalid nesting!
			break;
		}
		case CTRL_ERR:
			Tokenizer_delete(tok);
			Value_delete(root, 1);
			Stack_delete(&stack);
			return 0;
		default:
			Value_append(Stack_top(&stack), Value_new(type, token));
		}
		//fprintf(stdout, "%c <%s>\t%s\n", type, token, nesting);
	}
	//fflush(stdout);
	Tokenizer_delete(tok);
	Stack_delete(&stack);
	return root;
}

static void Value_print2(FILE* stream, const Value* v, int indent, int indentSelf, const char* tail) {
	for(int i=0; i<indentSelf; ++i)
		fprintf(stream, "\t");
	switch(v->type) {
		case VALUE_SYMBOL:
			fprintf(stream, "%s%s", v->str, tail);
			break;
		case VALUE_STRING:
			fprintf(stream, "\"%s\"%s", v->str, tail);
			break;
		case VALUE_ERR:
			fprintf(stream, "ERROR %s%s", v->str, tail);
			break;
		case VALUE_INT:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#ifdef __MINGW32__
			fprintf(stream, "%I64d%s", v->i, tail);
#else
			fprintf(stream, "%lld%s", v->i, tail);
#endif
#pragma GCC diagnostic pop
			break;
		case VALUE_BOOL:
			fprintf(stream, v->i ? "true%s" : "false%s", tail);
			break;
		case VALUE_FLOAT:
			fprintf(stream, "%lf%s", v->f, tail);
			break;
		case VALUE_NONE:
			fprintf(stream, "null%s", tail);
			break;
		case VALUE_LIST:
			fprintf(stream, "[\n");
			for(Value* c=v->child; c!=NULL; c=c->next)
				Value_print2(stream, c, indent+1, indent+1, c->next ? ",\n" :"\n");
			for(int i=0; i<indent; ++i)
				fprintf(stream, "\t");
			fprintf(stream, "]%s", tail);
			break;
		case VALUE_MAP:
			fprintf(stream, "{\n");
			for(Value* c=v->child; c!=NULL; c=c->next) {
				Value_print2(stream, c, 0, indent+1, ": ");
				c = c->next;
				if(c)
					Value_print2(stream, c, indent+1, 0, c->next ? ",\n" :"\n");
			}
			for(int i=0; i<indent; ++i)
				fprintf(stream, "\t");
			fprintf(stream, "}%s", tail);
			break;
		default:
			fprintf(stream, "%c%s", v->type, tail);
	}
}

void Value_print(const Value* v, FILE* stream) {
	if(v) {
		Value_print2(stream, v, 0, 0, v->next ? ",\n" :"\n");
		if(v->next)
			Value_print(v->next, stream);
	}
}


//--- XML tokenizer ------------------------------------------------

static const char ESC=27;
static const char OPN=28;
static const char CLS=29;

typedef struct {
	/// stores string to be tokenized
	const char* s;
	/// stores size of string to be tokenized
	size_t s_size;
	/// stores current read position
	size_t i;
	/// stores current read context
	int tagMode;
	/// stores next token, if already determined
	const char* m_next;
	/// size of next token
	size_t m_next_size;
	/// pointer to current token
	char* m_token;
	/// size of current token
	size_t m_token_size;
	/// capacity of current token
	size_t m_token_capacity;
} XmlTokenizer;

static XmlTokenizer* XmlTokenizer_new(const char* str, size_t str_size) {
	XmlTokenizer *tok = (XmlTokenizer*)malloc(sizeof(XmlTokenizer));
	memset(tok, 0, sizeof(XmlTokenizer));
	tok->s_size = str_size;
	tok->s = str;
	return tok;
}

static void XmlTokenizer_delete(XmlTokenizer* tok) {
	free(tok->m_token);
	free(tok);
}

//static void XmlTokenizer_print(XmlTokenizer* tok) { printf("  @%u %s\n", tok->i, !tok->m_token ? "(null)" : (tok->m_token[0]==ESC)?"(esc)" : (tok->m_token[0]==OPN)?"(open)": (tok->m_token[0]==CLS)?"(close)" : tok->m_token); fflush(stdout); }

static const char* XmlTokenizer_set(XmlTokenizer* tok, const char* s, size_t size) {
	if(!size||!s) return 0;
	free(tok->m_token);
	tok->m_token = (char*)malloc(size+1);
	strncpy(tok->m_token,s, size);
	tok->m_token[size] = 0;
	tok->m_token_size = tok->m_token_capacity = size;
	//XmlTokenizer_print(tok);
	return tok->m_token;
}

static void XmlTokenizer_append(XmlTokenizer* tok, char ch) {
	if(tok->m_token_size+1>=tok->m_token_capacity) {
		tok->m_token_capacity = (tok->m_token_capacity==0) ? 16 : tok->m_token_capacity*2;
		tok->m_token = (char*)realloc(tok->m_token, tok->m_token_capacity);
	}
	tok->m_token[tok->m_token_size]=ch;
	tok->m_token[++tok->m_token_size]=0;
}

static size_t find(const char* s, const char* pattern, size_t start) {
	const char* found =strstr(s+start, pattern);
	return found ? found-s : strlen(s);
}


static const char* XmlTokenizer_next(XmlTokenizer* tok) {
	const char* ESC_str = "\033";
	const char* OPEN_str = "\034";
	const char* CLOSE_str = "\035";

	
	if(tok->m_token) {
		free(tok->m_token);
		tok->m_token = 0;
		tok->m_token_size=tok->m_token_capacity = 0;
	}
	
	int quotMode=0;
	int tokenComplete = 0;
	while(tok->m_next_size || (tok->i < tok->s_size)) {

		if(tok->m_next_size) {
			XmlTokenizer_set(tok, tok->m_next, tok->m_next_size);
			tok->m_next=0;
			tok->m_next_size=0;
			return tok->m_token;
		}

		switch(tok->s[tok->i]) {
		    case '"':
		    case '\'':
			if(tok->tagMode) {
				if(!quotMode) quotMode=tok->s[tok->i];
				else if(quotMode==tok->s[tok->i]) quotMode=0;
			}
			XmlTokenizer_append(tok, tok->s[tok->i]);
			break;
		    case '<':
			if(!quotMode&&(tok->i+4<tok->s_size)&&(strncmp(tok->s+tok->i,"<!--",4)==0)) // strip comments
			    tok->i=find(tok->s, "-->", tok->i+4)+2;
			else if(!quotMode&&(tok->i+9<tok->s_size)&&(strncmp(tok->s+tok->i,"<![CDATA[",9)==0)) { // interpet CDATA
			    size_t b=tok->i+9;
			    tok->i=find(tok->s, "]]>",b)+3;
			    if(!tok->m_token_size) return XmlTokenizer_set(tok, tok->s+b, tok->i-b-3);
			    tokenComplete = 1;
			    tok->m_next = tok->s+b;
			    tok->m_next_size = tok->i-b-3;
			    --tok->i;
			}
			else if(!quotMode&&(tok->i+1<tok->s_size)&&((tok->s[tok->i+1]=='?')||(tok->s[tok->i+1]=='!'))) // strip meta information
			    tok->i=find(tok->s, ">", tok->i+2);
			else if(!quotMode&&!tok->tagMode) {
				if((tok->i+1<tok->s_size)&&(tok->s[tok->i+1]=='/')) {
					tok->m_next=ESC_str;
					tok->m_next_size = 1;
					tok->i=find(tok->s, ">", tok->i+2);
				}
				else {
					tok->m_next = OPEN_str;
					tok->m_next_size = 1;
					tok->tagMode=1; 
				}
				tokenComplete = 1;
			}
			else XmlTokenizer_append(tok, tok->s[tok->i]);
			break;
		    case '/': 
			if(tok->tagMode&&!quotMode) {
				tokenComplete = 1;
				if((tok->i+1 < tok->s_size) && (tok->s[tok->i+1]=='>')) {
					tok->tagMode=0;
					tok->m_next=ESC_str;
					tok->m_next_size = 1;
					++tok->i;
				}
				else XmlTokenizer_append(tok, tok->s[tok->i]);
			}                    
			else XmlTokenizer_append(tok, tok->s[tok->i]);
			break;
		    case '>': 
			if(!quotMode&&tok->tagMode) {
				tok->tagMode=0;
				tokenComplete = 1;
				tok->m_next = CLOSE_str;
				tok->m_next_size = 1;
			}                    
			else XmlTokenizer_append(tok, tok->s[tok->i]);
			break;
		    case ' ':
		    case '\r':
		    case '\n':
		    case '\t':
			if(tok->tagMode&&!quotMode) {
			    if(tok->m_token_size) tokenComplete=1;
			}
			else if(tok->m_token_size) XmlTokenizer_append(tok, tok->s[tok->i]);
			break;
		    default: XmlTokenizer_append(tok, tok->s[tok->i]);
		}
		++tok->i;
		if((tok->i>=tok->s_size)||(tokenComplete&&tok->m_token_size)) {
			tokenComplete=0;
			while(tok->m_token_size&&isspace(tok->m_token[tok->m_token_size-1])) // trim whitespace
				tok->m_token[--tok->m_token_size]=0;
			if(tok->m_token_size) break;
		}
	}
	//XmlTokenizer_print(tok);
	return tok->m_token;
}

//--- XML parser internal variables and functions ------------------

/// stores code table for special characters
static const char *sv_code[]={
	"&", "&amp;",
	"<", "&lt;",
	">", "&gt;",
	"\"", "&quot;",
	"'", "&apos;", NULL
};

static char* Xml_decode(const char* s, size_t s_size, const char** customCodes) {
	char* buf = malloc(s_size+1);
	memset(buf, 0, s_size+1);
	for(size_t pos=0, out=0; pos<s_size; ++pos) {
		if(s[pos]!='&') {
			buf[out++] = s[pos];
			continue;
		}
		if(++pos==s_size)
			break;
		if(s[pos]=='#' && ++pos<s_size) { // numerically encoded character
			char ch = 0;
			size_t start=pos;
			for(size_t end=pos+3; pos<end && pos<s_size; ++pos)
				if(!isdigit(s[pos]))
					break;
				else
					ch = ch*10 + (s[pos]-'0');
			if(pos>start && s[pos] == ';')
				buf[out++] = ch;
		}
		else {
			if(customCodes) while(*customCodes) {
				const char* decoded = *customCodes;
				const char* encoded = *(++customCodes)+1;
				const size_t encoded_size = strlen(encoded);
				if(encoded && strncmp(&s[pos], encoded, encoded_size)==0) {
					buf[out++] = *decoded;
					pos += encoded_size;
					break;
				}
				++customCodes;
			}
			for(const char** codes=sv_code; *codes; ++codes) { // sv_code lookup
				const char* decoded = *codes;
				const char* encoded = (*(++codes))+1;
				const size_t encoded_size = strlen(encoded);
				if(strncmp(&s[pos], encoded, encoded_size)==0) {
					buf[out++] = *decoded;
					pos += encoded_size;
					break;
				}
			}
		}
	}
	return buf;
}

static Value* childNodes(Value* currSt, Stack* ancestors) {
	if(!currSt || currSt->type == VALUE_LIST)
		return currSt;
	if(currSt->type == VALUE_MAP) {
		Value* childNodes = Value_get(currSt, "childNodes");
		if(!childNodes) {
			childNodes = Value_new(VALUE_LIST, NULL);
			Value_set(currSt, "childNodes", childNodes);
			Stack_push(ancestors, childNodes);
		}
		return childNodes;
	}
	return NULL;
}

//--- public methods -----------------------------------------------

Value* Value_parseXML(const char* str, const char** customCodes) {
	const size_t str_size=strlen(str);
	if(!str_size)
		return NULL;

	Value *root= NULL, *currSt = NULL;
	Stack ancestors;
	Stack_init(&ancestors, 32);
	size_t level = 0;
	XmlTokenizer* tok = XmlTokenizer_new(str, str_size);
	const char* token=0;
	while((token=XmlTokenizer_next(tok))!=0) if(token[0]==OPN) { // new tag found
		if(currSt)
			currSt = childNodes(currSt, &ancestors);
		else if(root)
			break;

		Value* newValue = Value_new(VALUE_MAP, NULL);
		if(!root)
			root = newValue;
		else
			Value_append(currSt, newValue);
		currSt = newValue;
		Value_set(currSt, "tagName", Value_str(XmlTokenizer_next(tok)));

		Value* attributes = NULL;
		while(((token = XmlTokenizer_next(tok))!=0) && (token[0]!=CLS) && (token[0]!=ESC)) { // parse tag header
			if(!attributes)
				attributes = Value_new(VALUE_MAP, NULL);
			size_t sepPos=find(token, "=", 0);
			if(token[sepPos]) { // regular attribute
				char* key = malloc(sepPos+1);
				memcpy(key, token, sepPos);
				key[sepPos] = 0;
				
				const char* aVal =token+sepPos+2;
				size_t lenVal = strlen(aVal);
				char* value = lenVal ? Xml_decode(aVal, lenVal-1, customCodes) : NULL;
				if(value)
					Value_set(attributes, key, Value_str(value));
				free(key);
				free(value);
			}
		}
		if(attributes)
			Value_set(currSt, "attributes", attributes);

		if(!token||(token[0]==ESC)) // tag has no content
			currSt = Stack_top(&ancestors);
		else
			++level;
	}
	else if(token[0]==ESC) { // previous tag is over
		if(!ancestors.sz)
			break;
		if(ancestors.sz > --level)
			Stack_pop(&ancestors);
		currSt = Stack_top(&ancestors);
	}
	else { // read elements
		currSt = childNodes(currSt, &ancestors);
		char* value = Xml_decode(token, strlen(token), customCodes);
		Value_append(currSt, Value_str(value));
		free(value);
	}
	XmlTokenizer_delete(tok);
	Stack_delete(&ancestors);
	return root;
}

#endif //  VALUE_IMPLEMENTATION
