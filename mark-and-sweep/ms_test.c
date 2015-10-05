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
#include <stddef.h>
#include <string.h>
#include "gc_ms.h"

#define ASSERT(EXPECTED, RESULT)\
  if(EXPECTED != RESULT) { printf("\n%-30s failure on line %d; expecting %d found %d\n", \
		__func__, __LINE__, EXPECTED, RESULT); }

#define TEST(t) printf("TESTING %s\n", #t); t();


void test_empty() {
	gc_init(1000);
	ASSERT(0, gc_num_roots());
	ASSERT(0, gc_num_object());
	ASSERT(0, gc_num_live_object());
	gc_ms();
	gc_done();
}
void test_alloc_str_still_alive_after_sweep() {
	gc_init(1000);
	String *a;

	gc_add_root(a);
	ASSERT(1, gc_num_roots());

	a = gc_alloc_string(10);
	ASSERT(1, gc_num_object());
	strcpy(a->str, "hi mom");
	gc_ms();
	ASSERT(1, gc_num_live_object());
	gc_done();
}

void test_alloc_strs_set_null_gc() {
	gc_init(1000);
	String *a;
	a = gc_alloc_string(10);
	strcpy(a->str, "hi mom");
	String *b;
	b = gc_alloc_string(5);
	strcpy(b->str, "mom");
	gc_add_root(a);

	void * last_sweep_addr = b;
	ASSERT(1, gc_num_roots());
	ASSERT(2, gc_num_object());

	a = NULL;
	gc_ms();
	ASSERT(0, gc_num_live_object());
	ASSERT(last_sweep_addr,get_next_free_addr());
	gc_done();
}

void test_alloc_vector_sweep_nothing(){
	gc_init(1000);
	Vector *v;
	v = gc_alloc_vector(5);
	double *data = (double []){1,2,3,4,5};
	memcpy(v->data, data, 5 * sizeof(double));
	gc_add_root(v);
	ASSERT(1, gc_num_roots());
	ASSERT(1, gc_num_object());
	gc_ms();
	ASSERT(1,gc_num_live_object());
}

void test_alloc_vector_gc_twice() {
	gc_init(1000);
	Vector *v;
	v = gc_alloc_vector(5);
	double *data = (double []){1,2,3,4,5};
	memcpy(v->data, data, 5 * sizeof(double));
	gc_add_root(v);
	ASSERT(1, gc_num_roots());
	ASSERT(1, gc_num_object());
	gc_ms();
	ASSERT(1,gc_num_live_object());
	v = NULL;
	gc_ms();
	ASSERT(0,gc_num_live_object());
}

static void f()
{
	String *a;
	Vector *b;
	gc_begin_func();

	a = gc_alloc_string(10);
	strcpy(a->str, "parrt");
	b = gc_alloc_vector(5);
	double *data = (double []){1,2,3,4,5};
	memcpy(b->data, data, 5 * sizeof(double));
	gc_add_root(a);
	gc_add_root(b);

	gc_end_func(); // should deallocate a,b automagically
}

void test_local_roots_in_called_func() {
	gc_init(1000);

	// start with a global root
	String *c;
	gc_add_root(c);
	c = gc_alloc_string(10);
	strcpy(c->str, "hello");
	void *lastsweepaddr = c;
	ASSERT(1,gc_num_roots());
	// now call function with locals as roots
	f();

	// all of the locals from f() should have gone away
	gc_ms();

	ASSERT(1,gc_num_live_object());

	c = NULL;
	gc_ms();

	ASSERT(0,gc_num_live_object());
	ASSERT(lastsweepaddr,get_next_free_addr());

	gc_done();
}

int main(int argc, char *argv[]) {
	TEST(test_empty);
	TEST(test_alloc_str_still_alive_after_sweep);
	TEST(test_alloc_strs_set_null_gc);
	TEST(test_alloc_vector_sweep_nothing);
	TEST(test_alloc_vector_gc_twice);
	TEST(test_local_roots_in_called_func);
	return 0;
}

