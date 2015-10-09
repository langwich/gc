#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "gc.h"

#define ASSERT(EXPECTED, RESULT)\
  if(EXPECTED != RESULT) { printf("\n%-30s failure on line %d; expecting %d found %d\n", \
        __func__, __LINE__, EXPECTED, RESULT); }
#define STR_ASSERT(EXPECTED, RESULT)\
  if(strcmp(EXPECTED,RESULT)!=0) { printf("\n%-30s failure on line %d; expecting:\n%s\nfound:\n%s\n", \
        __func__, __LINE__, EXPECTED, RESULT); }

#define TEST(t) printf("TESTING %s\n", #t); t();

#define check(expected) \
    {\
    char *found = gc_get_state(); \
    STR_ASSERT(expected, found);\
    free(found); \
    }

typedef enum {String_t, User_t, Employee_t} TYPE;

typedef struct {
    heap_object header;

} String;

typedef struct {
    heap_object header;

    int userid;
    int parking_sport;
    float salary;
    String *name;
} User;

typedef struct {
    heap_object header;

    int ID;
    String *name;
    struct Employee *mgr;
} Employee;


void test_empty() {
    gc_init(1000);
    ASSERT(0, gc_num_roots());
    char *expected =
        "next_free=0\n"
        "objects:\n";
    char *found = gc_get_state();
    STR_ASSERT(expected, found);
    free(found);
    gc();
    gc_done();
}

void test_alloc_str_gc_compact_does_nothing() {
    gc_init(1000);
    String *a;

    gc_add_root(a);
    ASSERT(1, gc_num_roots());

    a = gc_alloc_string(10);
    strcpy(a->str, "hi mom");

    check("next_free=43\n"
                "objects:\n"
                "  0000:String[32+11]=\"hi mom\"\n");

    gc();

    check("next_free=43\n"
                "objects:\n"
                "  0000:String[32+11]=\"hi mom\"\n");

    gc_done();
}

void test_alloc_str_set_null_gc() {
    gc_init(1000);
    String *a;
    gc_add_root(a);
    ASSERT(1, gc_num_roots());

    a = gc_alloc_string(10);
    strcpy(a->str, "hi mom");

    check("next_free=43\n"
        "objects:\n"
        "  0000:String[32+11]=\"hi mom\"\n");

    a = NULL;

    gc();

    check("next_free=0\n"
          "objects:\n");

    gc_done();
}

void test_alloc_2_str_overwrite_first_one_gc() {
    gc_init(1000);
    String *a;
    gc_add_root(a);
    ASSERT(1, gc_num_roots());

    a = gc_alloc_string(10);
    strcpy(a->str, "hi mom");

    check("next_free=43\n"
                "objects:\n"
                "  0000:String[32+11]=\"hi mom\"\n");

    a = gc_alloc_string(10);
    strcpy(a->str,"hi dad");

    gc();

    check("next_free=43\n"
                "objects:\n"
                "  0000:String[32+11]=\"hi dad\"\n");


    gc_done();
}

// user->name points at string

void test_alloc_user() {
    gc_init(1000);
    
    User *u = (User *) gc_alloc(&User_class);
    gc_add_root(u);

    u->name = gc_alloc_string(20);
    strcpy(u->name->str, "parrt");

    check("next_free=101\n"
                "objects:\n"
                "  0000:User[48]->[48]\n"
                "  0048:String[32+21]=\"parrt\"\n");

    u = NULL; // should free user and string

    gc();

    check("next_free=0\n"
                "objects:\n");

    gc_done();
}

void test_alloc_user_after_string() {
    gc_init(1000);

    String * s = gc_alloc_string(20);
    gc_add_root(s);
    strcpy(s->str, "parrt");

    User *u = (User *) gc_alloc(&User_class);
    gc_add_root(u);
    u->name = s;

    check("next_free=101\n"
                "objects:\n"
                "  0000:String[32+21]=\"parrt\"\n"
                "  0053:User[48]->[0]\n");

    u = NULL; // should free user but NOT string

    gc();

    check("next_free=53\n"
                "objects:\n"
                "  0000:String[32+21]=\"parrt\"\n");

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
    
    gc();

    check("next_free=175\n"
            "objects:\n"
            "  0000:Employee[48]->[48,NULL]\n"
            "  0048:String[32+4]=\"Tom\"\n"
            "  0084:Employee[48]->[132,0]\n"
            "  0132:String[32+11]=\"Terence\"\n");

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
    
    parrt->mgr = NULL; // 2 objects live
    
    gc();

    check("next_free=91\n"
            "objects:\n"
            "  0000:Employee[48]->[48,NULL]\n"
            "  0048:String[32+11]=\"Terence\"\n");

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
    
    gc();

    check("next_free=175\n"
        "objects:\n"
        "  0000:Employee[48]->[48,84]\n"
        "  0048:String[32+4]=\"Tom\"\n"
        "  0084:Employee[48]->[132,0]\n"
        "  0132:String[32+11]=\"Terence\"\n");

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
    
    parrt->mgr = NULL;  // can't see tombu from anywhere
    
    gc();

    check("next_free=91\n"
            "objects:\n"
            "  0000:Employee[48]->[48,NULL]\n"
            "  0048:String[32+11]=\"Terence\"\n");

    gc_done();
}

void test_big_loop_doesnt_run_out_of_memory() {
    gc_init(1000);

    Employee *tombu;
    gc_add_root(tombu);
    
    int i = 0;
    while ( i < 10000000 ) {
        tombu = (Employee *) gc_alloc(&Employee_class);
        String *s = gc_alloc_string(3);
        strcpy(s->str, "Tom");
        tombu->name = s;
        i++;
        if ( i % 1000000 == 0 ) {
/*
            char *found = gc_get_state();
            printf("%s\n", found);
            free(found);            
*/
        }
    }
    
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
    ASSERT(2, gc_num_roots());

    check("next_free=96\n"
          "objects:\n"
          "  0000:Employee[48]->[NULL,NULL]\n"
          "  0048:Employee[48]->[NULL,NULL]\n");

    gc();

    check("next_free=96\n"
              "objects:\n"
              "  0000:Employee[48]->[NULL,NULL]\n"
              "  0048:Employee[48]->[NULL,NULL]\n");

    _e1 = NULL;
    gc();

    check("next_free=48\n"
              "objects:\n"
              "  0000:Employee[48]->[NULL,NULL]\n"); // gets moved

    gc_done();
}

static void f()
{
    String *a;
    Employee *b;
    gc_begin_func();

    // just 1 global
    check("next_free=48\n"
          "objects:\n"
          "  0000:Employee[48]->[NULL,NULL]\n");

    a = gc_alloc_string(10);
    strcpy(a->str, "parrt");
    b = (Employee *)gc_alloc(&Employee_class);
    gc_add_root(a);
    gc_add_root(b);

    check("next_free=139\n"
          "objects:\n"
          "  0000:Employee[48]->[NULL,NULL]\n"
          "  0048:String[32+11]=\"parrt\"\n"
          "  0091:Employee[48]->[NULL,NULL]\n");

    gc_end_func(); // should deallocate a,b automagically
}

void test_local_roots_in_called_func() {
    gc_init(1000);

    // start with a global root
    _e1 = (Employee *)gc_alloc(&Employee_class);
    gc_add_root(_e1);

    // now call function with locals as roots
    f();

    // all of the locals from f() should have gone away

    check("next_free=139\n"  // we haven't called gc() yet
              "objects:\n"
              "  0000:Employee[48]->[NULL,NULL]\n");
    gc();

    check("next_free=48\n"
              "objects:\n"
              "  0000:Employee[48]->[NULL,NULL]\n");

    _e1 = NULL;
    gc();
    check("next_free=0\n"
              "objects:\n");

    gc_done();
}

void test_template() {
    gc_init(1000);
    // gc_add_root(s);
    gc_done();
}

int main(int argc, char *argv[]) {
    TEST(test_empty);
    TEST(test_alloc_str_gc_compact_does_nothing);
    TEST(test_alloc_str_set_null_gc);
    TEST(test_alloc_2_str_overwrite_first_one_gc);
    TEST(test_alloc_user);
    TEST(test_alloc_user_after_string);
    TEST(test_alloc_obj_with_two_ptr_fields);
    TEST(test_alloc_obj_kill_mgr_ptr);
    TEST(test_mgr_cycle);
    TEST(test_mgr_cycle_kill_one_link);

    TEST(test_global);
    TEST(test_local_roots_in_called_func);

    TEST(test_big_loop_doesnt_run_out_of_memory);
    
    return 0;
}
