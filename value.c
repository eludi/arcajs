#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
Value* Value_int(signed long long value) {
	Value* v = (Value*)malloc(sizeof(Value));
	v->type = VALUE_INT;
	v->next = NULL;
	v->i = value;
	return v;
}
Value* Value_bool(int value) {
	Value* v = (Value*)malloc(sizeof(Value));
	v->type = VALUE_BOOL;
	v->next = NULL;
	v->i = value;
	return v;
}
Value* Value_float(double value) {
	Value* v = (Value*)malloc(sizeof(Value));
	v->type = VALUE_FLOAT;
	v->next = NULL;
	v->f = value;
	return v;
}

void Value_delete(Value* v, int deleteChain) {
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

void Value_set(Value* parent, const char* key, Value* value) {
	if(parent->type != VALUE_MAP)
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

const Value* Value_get(const Value* parent, const char* key) {
	if(!parent || parent->type != VALUE_MAP)
		return 0;
	const Value *pKey = parent->child;
	while(pKey) {
		const Value* pValue = pKey->next;
		if(strcmp(key, pKey->str)==0)
			return pValue;
		pKey = pValue->next;
	}
	return 0;
}

static Value* Value_getn(Value* parent, const char* key, size_t n) {
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

Value* Value_at(Value* parent, unsigned n) {
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

Value* Value_atPath(Value* parent, const char* path) {
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
		return parent;
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
	if(tok->type == VALUE_SYMBOL) {
		if((tok->m_token_size==4 && strncmp(tok->s, "true", tok->m_token_size)==0)
			|| (tok->m_token_size==5 && strncmp(tok->s, "false", tok->m_token_size)==0))
			tok->type = VALUE_BOOL;
	}
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
	return tok->m_token;
}

//--- Stack --------------------------------------------------------

typedef struct Stack {
	void** data;
	size_t sz;
	size_t capacity;
} Stack;

static void Stack_init(Stack * stack, size_t capacity) {
	stack->sz=0;
	stack->capacity = capacity;
	stack->data = (void**)malloc(capacity*sizeof(void*));
}
static void* Stack_top(Stack* stack) {
	return stack->sz ? stack->data[stack->sz-1] : 0;
}
static void Stack_push(Stack * stack, void * data) {
	if(stack->sz==stack->capacity) {
		stack->capacity*=2;
		stack->data = (void**)realloc(stack->data, stack->capacity*sizeof(void*));
	}
	stack->data[stack->sz] = data;
	++stack->sz;
}
static void Stack_pop(Stack * stack) {
	if(!stack->sz)
		return;
	stack->sz--;
	stack->data[stack->sz]=0;
}
static void Stack_delete(Stack* stack) {
	free(stack->data);
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

static void Value_print2(FILE* stream, Value* v, int indent, const char* tail) {
	for(int i=0; i<indent; ++i)
		fprintf(stream, "\t");
	switch(v->type) {
		case VALUE_SYMBOL:
			fprintf(stream, "%s%s", v->str, tail);
			break;
		case VALUE_STRING:
			fprintf(stream, "\"%s\"%s", v->str, tail);
			break;
		case VALUE_INT:
#ifdef __MINGW32__
			fprintf(stream, "%I64d%s", v->i, tail);
#else
			fprintf(stream, "%lld%s", v->i, tail);
#endif
			break;
		case VALUE_BOOL:
			fprintf(stream, v->i ? "true%s" : "false$s", tail);
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
				Value_print2(stream, c, indent+1, c->next ? ",\n" :"\n");
			for(int i=0; i<indent; ++i)
				fprintf(stream, "\t");
			fprintf(stream, "]%s", tail);
			break;
		case VALUE_MAP:
			fprintf(stream, "{\n");
			for(Value* c=v->child; c!=NULL; c=c->next) {
				Value_print2(stream, c, indent+1, ": ");
				c = c->next;
				if(c)
					Value_print2(stream, c, 0, c->next ? ",\n" :"\n");
			}
			for(int i=0; i<indent; ++i)
				fprintf(stream, "\t");
			fprintf(stream, "}%s", tail);
			break;
		default:
			fprintf(stream, "%c%s", v->type, tail);
	}
}

void Value_print(Value* v, FILE* stream) {
	if(v) {
		Value_print2(stream, v, 0, v->next ? ",\n" :"\n");
		if(v->next)
			Value_print(v->next, stream);
	}
}
