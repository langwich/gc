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
#include <string.h>
#include "gc_mns.h"

#define ASSERT(EXPECTED, RESULT)\
  if(EXPECTED != RESULT) { printf("\n%-30s failure on line %d; expecting %d found %d\n", \
		__func__, __LINE__, EXPECTED, RESULT); }

#define TEST(t) printf("TESTING %s\n", #t);t();


void test_empty() {
	gc_init(1000);
	ASSERT(0, gc_num_roots());
	ASSERT(0, gc_num_object());
	ASSERT(0, gc_num_live_object());
	gc_done();
}

void test_mark_then_allocate() {
	gc_init(120);
	String *a;
	a = gc_alloc_string(80);
	ASSERT(1,gc_num_object());
	String *b;
	b = gc_alloc_string(52);
	ASSERT(2,gc_num_object());
}

void test_allocate_from_free_chunk() {
	gc_init(1000);
	Vector *v;
	v = gc_alloc_vector(5);
	double *data = (double []){1,2,3,4,5};
	memcpy(v->data, data, 5 * sizeof(double));
	gc_add_root(v);
	void *freechunk_addr = get_freechunk_addr();

	String *s;
	s = gc_alloc_string(10);
	strcpy(s->str, "hello");
	gc_add_root(s);
	void *freechunk_addr2 = freechunk_addr + sizeof (String) + s->header.size + 1;

	ASSERT(freechunk_addr2,get_freechunk_addr());
	ASSERT(2,gc_num_roots());
}

int main(int argc, char *argv[]) {
	TEST(test_empty);
	TEST(test_mark_then_allocate);
	TEST(test_allocate_from_free_chunk);
	return 0;
}

