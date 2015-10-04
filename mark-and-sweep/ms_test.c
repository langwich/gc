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


typedef struct User /* extends Object */ {
	GC_Fields header;

	int userid;
	int parking_sport;
	float salary;
	String *name;
} User;

ClassDescriptor User_class = {
		"User",
		sizeof (struct User),
		1, /* name field */
		(int []) {offsetof(struct User, name)} /* offset of 2nd field ignoring class descr. */
};

typedef struct Employee /* extends Object */ {
	GC_Fields header;

	int ID;
	String *name;
	struct Employee *mgr;
} Employee;

ClassDescriptor Employee_class = {
		"Employee",
		sizeof (Employee),
		2, /* name, mgr fields */
		(int []) {
				offsetof(Employee, name), /* offset of 2nd field ignoring class descr. */
				offsetof(Employee, mgr)  // 3rd field
		}
};

void test_empty() {
	gc_init(1000);
	ASSERT(0, gc_num_roots());
	ASSERT(0, gc_num_object());
	ASSERT(0, gc_num_live_object());
	gc_ms();
	gc_done();
}
void test_alloc_str_gc_compact_does_nothing() {
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

void test_alloc_str_set_null_gc() {
	gc_init(1000);
	String *a;
	gc_add_root(a);
	ASSERT(1, gc_num_roots());
	a = gc_alloc_string(10);
	void * a_addr = a;
	strcpy(a->str, "hi mom");
	ASSERT(1, gc_num_object());
	a = NULL;

	gc_ms();
	ASSERT(0, gc_num_live_object());
	ASSERT(a_addr,get_next_free_addr());
	gc_done();
}

void test_alloc_user_after_string() {
	gc_init(1000);

	String * s = gc_alloc_string(20);
	gc_add_root(s);
	strcpy(s->str, "parrt");

	User *u = (User *) gc_alloc(&User_class);
	void *last_sweep_addr = u;
	gc_add_root(u);
	u->name = s;

	u = NULL; // should free user but NOT string

	gc_ms();
	ASSERT(2,gc_num_object());
	ASSERT(1,gc_num_live_object());
    ASSERT(last_sweep_addr,get_next_free_addr());
	gc_done();
}

void test_alloc_obj_with_two_ptr_fields() {
	gc_init(1000);

	Employee *tombu = (Employee *) gc_alloc(&Employee_class);
	String *s = gc_alloc_string(3);
	strcpy(s->str, "Tom");
	tombu->name = s;

	Employee *parrt = (Employee *) gc_alloc(&Employee_class);
	parrt->name = gc_alloc_string(10);
	strcpy(parrt->name->str, "Terence");
	parrt->mgr = tombu;

	gc_add_root(parrt); // just one root
	gc_ms();

	ASSERT(4,gc_num_live_object());
	gc_done();
}

void test_alloc_obj_kill_mgr_ptr() {
	gc_init(1000);

	Employee *tombu = (Employee *) gc_alloc(&Employee_class);
	String *s = gc_alloc_string(3);
	strcpy(s->str, "Tom");
	tombu->name = s;

	Employee *parrt = (Employee *) gc_alloc(&Employee_class);
	parrt->name = gc_alloc_string(10);
	strcpy(parrt->name->str, "Terence");
	parrt->mgr = tombu;

	gc_add_root(parrt); // just one root

    void *last_sweep_addr =s;

	parrt->mgr = NULL; // 2 objects live

	gc_ms();

	ASSERT(2,gc_num_live_object());
    ASSERT(last_sweep_addr,get_next_free_addr());
	gc_done();
}
void test_mgr_cycle() {
	gc_init(1000);

	Employee *tombu = (Employee *) gc_alloc(&Employee_class);
	String *s = gc_alloc_string(3);
	strcpy(s->str, "Tom");
	tombu->name = s;

	Employee *parrt = (Employee *) gc_alloc(&Employee_class);
	parrt->name = gc_alloc_string(10);
	strcpy(parrt->name->str, "Terence");

	// CYCLE
	parrt->mgr = tombu;
	tombu->mgr = parrt;

	gc_add_root(parrt); // just one root; can it find everyone and not freak out?
	ASSERT(1,gc_num_roots());
	ASSERT(4,gc_num_object());
	gc_ms();
	ASSERT(4,gc_num_live_object());

	gc_done();
}

void test_mgr_cycle_kill_one_link() {
	gc_init(1000);

	Employee *tombu = (Employee *) gc_alloc(&Employee_class);
	String *s = gc_alloc_string(3);
	strcpy(s->str, "Tom");
	tombu->name = s;

	Employee *parrt = (Employee *) gc_alloc(&Employee_class);
	parrt->name = gc_alloc_string(10);
	strcpy(parrt->name->str, "Terence");

	// CYCLE
	parrt->mgr = tombu;
	tombu->mgr = parrt;

	gc_add_root(parrt); // just one root; can it find everyone and not freak out?

	void *last_sweep_addr =s;
    parrt->mgr = NULL;  // can't see tombu from anywhere

	gc_ms();
	ASSERT(4,gc_num_object());
	ASSERT(2,gc_num_live_object());
    ASSERT(last_sweep_addr,get_next_free_addr());

	gc_done();
}

static Employee *_e1;
void test_global() {
	gc_init(1000);
	static Employee *_e2;
	_e1 = (Employee *)gc_alloc(&Employee_class);
	_e2 = (Employee *)gc_alloc(&Employee_class);
	gc_add_root(_e1);
	gc_add_root(_e2);
	ASSERT(2,gc_num_object());
	ASSERT(2, gc_num_roots());
	gc_ms();
	ASSERT(2,gc_num_live_object())
    void *last_sweep_addr = _e1;
	_e1 = NULL;
	gc_ms();
	ASSERT(1,gc_num_live_object());
    ASSERT(last_sweep_addr,get_next_free_addr());
	gc_done();
}

static void f()
{
	String *a;
	Employee *b;
	gc_begin_func();

	a = gc_alloc_string(10);
	strcpy(a->str, "parrt");
	b = (Employee *)gc_alloc(&Employee_class);
	gc_add_root(a);
	gc_add_root(b);

	gc_end_func(); // should deallocate a,b automagically
}

void test_local_roots_in_called_func() {
	gc_init(1000);

	// start with a global root
	_e1 = (Employee *)gc_alloc(&Employee_class);
	gc_add_root(_e1);
	ASSERT(1,gc_num_roots());
	// now call function with locals as roots
	f();

	// all of the locals from f() should have gone away
	gc_ms();

	ASSERT(1,gc_num_live_object());

	_e1 = NULL;
	gc_ms();

	ASSERT(0,gc_num_live_object());

	gc_done();
}

int main(int argc, char *argv[]) {
	TEST(test_empty);
	TEST(test_alloc_str_gc_compact_does_nothing);
	TEST(test_alloc_str_set_null_gc);
	TEST(test_alloc_user_after_string);
	TEST(test_alloc_obj_with_two_ptr_fields);
	TEST(test_alloc_obj_kill_mgr_ptr);
	TEST(test_mgr_cycle);
	TEST(test_mgr_cycle_kill_one_link);
	TEST(test_global);
	TEST(test_local_roots_in_called_func);
	return 0;
}

