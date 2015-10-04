/*
The MIT License (MIT)

Copyright (c) 2015 Terence Parr, Hanzhou Shi, Shuai Yuan, Yuanyuan Zhang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef GC_GC_MS_H
#define GC_GC_MS_H

#include <stdbool.h>

typedef unsigned char byte;

typedef struct ClassDescriptor {
	char *name;
	int size;
	int num_fields;
	int *field_offsets;
} ClassDescriptor;

typedef struct GC_Fields {
	struct ClassDescriptor *metaclass;
	byte marked;
} GC_Fields;

typedef struct Object {
	GC_Fields header;
} Object;

typedef struct String {
	GC_Fields header;
	int length;
	char str[];
}String;

typedef struct _Free_Header {
	int size;
	struct _Free_Header *next;
} Free_Header;

extern ClassDescriptor String_metaclass;

extern void gc_init(int size);
extern void gc_done();

extern void gc_ms();
extern Object *gc_alloc(ClassDescriptor *class);
extern String *gc_alloc_string(int size);
extern void gc_add_addr_of_root(Object **p);
extern void gc_add_objects(Object *p);
extern int gc_num_roots();
extern int gc_num_live_object();
extern int gc_num_object();
extern void gc_set_num_roots(int roots);
extern void *get_next_free_addr();

#define gc_begin_func()		int __save = gc_num_roots()
#define gc_end_func()		gc_set_num_roots(__save)
#define gc_add_root(p)		gc_add_addr_of_root((Object **)&(p));
#endif //GC_GC_MS_H
