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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gc_mns.h"

#define DEBUG 1
#define MAX_ROOTS       100
#define MAX_OBJECTS     200

static Object **_roots[MAX_ROOTS];
static int num_roots;
static int heap_size;
static byte *start_of_heap;
static byte *end_of_heap;
void *freechunk;

static Object *objects[MAX_OBJECTS];
static int num_objects;
static int num_live_objects;

static void gc_mark();
static void gc_mark_object(Object *p);
static void gc_clear_mark();
static bool gc_in_heap(Object *p);
static void *gc_alloc(int size);
static void *gc_alloc_space(int size);

/*
 * Implementation:
 * 1.when object is allocated, the mark bit will be set to 1, whatever it is reachable or not.
 * 2.when have new allocate request, first will check never-allocated chunk(named freechunk here)
 *   if freechunk+size < end of heap, allocate and return; otherwise, walk all the objects in heap,
 *   check if there is any fitted gc object (mark =0, objectsize>=size), if find,return (first fit here)
 * 3.if still can't find allocatable chunk by step 2, clear all the objects's mark bit, set to 0
 * 4.walk all reachable objects, set the mark bit to 1. till now all the reachable objects are marked as 1,
 *   all gc objects are marked as 0.
 * 5.walk all objects again, find the fitted object and return
 * 6.If still cannot allocate, return error "memory is full"
 */
void gc_init(int size) {
    heap_size = size;
    start_of_heap = malloc(size);
    end_of_heap = start_of_heap + heap_size -1;
    num_live_objects = 0;
    num_roots = 0;
    num_objects =0;
    freechunk = start_of_heap;
}

Vector *gc_alloc_vector(int size) {
    Vector *v = gc_alloc(sizeof(Vector) + size * sizeof(double)+1);
    if(DEBUG)  printf("gc allocate vector @%p\n",v);
    v->header.marked = 1;
    v->header.size = size;
    v->name = "Vector";
    memset(v->data, 0, size*sizeof(double));
    gc_add_objects(v);
    return v;
}

String *gc_alloc_string(int size) {
    String *s;
    s = (String *) gc_alloc(sizeof (String) + size + 1);
    if(DEBUG)  printf("gc allocate string @%p\n",s);
    s->header.marked = 1;
    memset(s->str, 0, size);
    s->header.size = size;
    s->name = "String";
    gc_add_objects(s);
    return s;
}

static void *gc_alloc(int size) {
    void *o = gc_alloc_space(size);
    if(NULL == o) {
        gc_clear_mark();
        gc_mark();
        o = gc_alloc_space(size);
        if (o == NULL) {
            if (DEBUG) printf("memory is full");
            return NULL;
        }
    }
    return o;
}

static void *gc_alloc_space(int size) {
    if (freechunk + size < end_of_heap) {
        void *p = freechunk;
        freechunk += size;
        return p;
    }
    int i;
    for (i = 0; i < num_objects; i++) {
        Object * o = objects[i];
        if (!o->header.marked && o->header.size >= size) {
            if(DEBUG) printf("release object@%p\n",o);
            return o;
        }
    }
    return NULL;
}

static void gc_mark() {
    int i;
    num_live_objects = 0;
    for (i = 0; i < num_roots; i++) {
        if (DEBUG) printf("root[%d]=%p\n", i, _roots[i]);
        Object *p = *_roots[i];
        if (p != NULL) {
            if (gc_in_heap(p)) {
                gc_mark_object(p);
            }
        }
    }
}

static void gc_mark_object(Object *p) {
    if (!p->header.marked) {
        if (DEBUG) printf("mark %s@%p\n", p->name, p);
        p->header.marked = 1;
        num_live_objects++;
    }
}

static void gc_clear_mark() {
    Object *p = NULL;
    int i;
    for (i =0; i< num_objects; i++) {
        p = objects[i];
        p->header.marked = 0;
    }
}

int gc_num_roots() {
    return num_roots;
}

int gc_num_live_object() {
    return num_live_objects;
}

int gc_num_object() {
    return num_objects;
}

void gc_set_num_roots(int roots)
{
    num_roots = roots;
}

static bool gc_in_heap(Object *p) {
    return p >= (Object *) start_of_heap && p <= (Object *) end_of_heap;
}

void gc_done() {
    free(start_of_heap);
}

void gc_add_addr_of_root(Object **p)
{
    _roots[num_roots++] = p;
}

void gc_add_objects(Object *p) {
    objects[num_objects++] = p;
}

void *get_freechunk_addr(){
    return freechunk;
}