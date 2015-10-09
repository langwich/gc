#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "gc.h"

#define DEBUG 0

#define MAX_ROOTS		100
#define MAX_OBJECTS 	200

static heap_object **_roots[MAX_ROOTS];
static int num_roots = 0; /* index of next free space in _roots for a root */

static int heap_size;
static byte *start_of_heap;
static byte *end_of_heap;
static byte *next_free;

// temp array; result of mark operation
static heap_object *live_objects[MAX_OBJECTS];
static int num_live_objects = 0;

static void gc_mark_live();
static void gc_mark_object(heap_object *p);
static void gc_sweep();

static int  gc_object_size(heap_object *p);
static void gc_compact_object_list();
static void *gc_alloc_space(int size);
static void gc_dump();
static bool gc_in_heap(heap_object *p);
static char *gc_viz_heap();
static void gc_free_object(heap_object *p);
static void print_ptr(heap_object *p);
static void print_addr_array(heap_object **array, int len);
static char *long_array_to_str(unsigned long *array, int len);
static unsigned long gc_rel_addr(heap_object *p);
static int addrcmp(const void *a, const void *b);
static void unmark_objects();
static char *ptr_to_str(heap_object *p);

/* Initialize a heap with a certain size for use with the garbage collector */
void gc_init(int size) {
    heap_size = size;
    start_of_heap = malloc((size_t)size); //TODO: should this be morecore()?
    end_of_heap = start_of_heap + size - 1;
    next_free = start_of_heap;
    num_live_objects = num_roots = 0;
}

/* Announce you are done with the heap managed by the garbage collector */
void gc_done() {
    free(start_of_heap);
}

void gc_add_addr_of_root(heap_object **p)
{
    _roots[num_roots++] = p;
}

/* Perform a mark-and-compact garbage collection, moving all live objects
 * to the start of the heap. Anything that we don't mark is dead. Unlike
 * mark-n-sweep, we do not walk the garbage. The mark operation
 * results in a temporary array called live_objects with objects in random
 * address order. We sort by address order from low to high in order to
 * compact the heap without stepping on a live object. During one of the
 * passes over the live object list, reset p->marked = 0. I do it here in #5.
 *
 * 1. Walk object graph, marking live objects as with mark-sweep.
 *
 * 2. Then sort live object list by address. We have to compact by walking
 *    the objects in address order, low to high.
 *
 * 3. Next we walk all live objects and compute their forwarding addresses.
 *
 * 4. Alter all roots pointing to live objects to point at forwarding address.
 *
 * 5. Walk the live objects and alter all non-NULL managed pointer fields
 *    to point to the forwarding addresses.
 *
 * 6. Move all live objects to the start of the heap in ascending address order.
 */
void gc() {
    if (DEBUG) printf("gc_compact\n");
    gc_mark_live(); // fills live_objects

    // sort objects by address
    qsort(live_objects, num_live_objects, sizeof(heap_object *), addrcmp);


    // compute forwarding addresses
    next_free = start_of_heap; // reset to have no allocated space then realloc at start
    int i;
    for (i = 0; i < num_live_objects; i++) {
        heap_object *p = live_objects[i];
        p->header.forwarded = gc_alloc_space(gc_object_size(p));
    }

    // alter roots that point to live objects
    for (i = 0; i < num_roots; i++) {
        if (DEBUG) printf("move root[%d]=%p\n", i, _roots[i]);
        heap_object *p = *_roots[i];
        if (p != NULL && p->header.marked) {
            *_roots[i] = p->header.forwarded; // move root to new address
        }
    }

    // alter fields; walk all live objects and set their ptr fields
    for (i = 0; i < num_live_objects; i++) {
        heap_object *p = live_objects[i];
        p->header.marked = 0;
        int f;
        if (DEBUG) printf("move ptr fields of %s@%p\n", p->header.metaclass->name, p);
        for (f = 0; f < p->header.metaclass->num_fields; f++) {
            int offset_of_ptr_field = p->header.metaclass->field_offsets[f];
            byte *ptr_to_ptr_field = ((byte *) p) + offset_of_ptr_field;
            heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
            heap_object *target_obj = *ptr_to_obj_ptr_field;
            if (target_obj != NULL) {
                *ptr_to_obj_ptr_field = target_obj->header.forwarded;
            }
        }
    }

    // move objects to compact heap
    for (i = 0; i < num_live_objects; i++) {
        heap_object *p = live_objects[i];
        memcpy(p->header.forwarded, p, gc_object_size(p));
    }
}

heap_object *gc_alloc(ClassDescriptor *class) {
    heap_object *p = gc_alloc_space(class->size);
    memset(p, 0, class->size);
    p->header.metaclass = class;
    return p; // spend hour looking for bug; forgot this
}

String *gc_alloc_string(int size) {
    String *s;
    /* size for struct String, the String itself, and null char */
    s = (String *) gc_alloc_space(sizeof (String) + size + 1);
    s->header.metaclass = &String_metaclass;
    s->header.marked = 0;
    memset(s->str, 0, size);
    s->length = size;
    return s;
}

int gc_num_roots() {
    return num_roots;
}

void gc_set_num_roots(int roots)
{
    num_roots = roots;
}

char *gc_get_state() {
    gc_mark_live(); // fill live_objects[]
    qsort(live_objects, num_live_objects, sizeof (heap_object *), addrcmp);
    charbuf state = charbuf_new(1000);
    char buf[1000];
    sprintf(buf, "next_free=%ld\n", gc_rel_addr((heap_object *) next_free));
    charbuf_add_str(&state, buf);
    sprintf(buf, "objects:\n");
    charbuf_add_str(&state, buf);
    int i;
    for (i = 0; i < num_live_objects; i++) {
        heap_object *p = live_objects[i];
        if (p == NULL) {
            charbuf_add_str(&state, "  NULL\n");
        }
        else {
            charbuf_add_str(&state, "  ");
            char *s = ptr_to_str(p);
            s[strlen(s)-1] = '\0'; // strip \n
            charbuf_add_str(&state, s);
            free(s);
            {
                // print ptr fields
                if ( p->header.metaclass->num_fields>0 ) {
                    charbuf_add_str(&state, "->[");
                    int i;
                    for (i = 0; i < p->header.metaclass->num_fields; i++) {
                        int offset_of_ptr_field = p->header.metaclass->field_offsets[i];
                        byte *ptr_to_ptr_field = ((byte *) p) + offset_of_ptr_field;
                        heap_object **obj_ptr_to_ptr_field = (heap_object **) ptr_to_ptr_field;
                        heap_object *target_obj = *obj_ptr_to_ptr_field;
                        if ( i>0 ) charbuf_add(&state, ',');
                        if ( target_obj!=NULL ) {
                            sprintf(buf, "%ld", gc_rel_addr((heap_object *) target_obj));
                            charbuf_add_str(&state, buf);
                        }
                        else {
                            charbuf_add_str(&state, "NULL");
                        }
                    }
                    charbuf_add_str(&state, "]");
                }
                charbuf_add(&state, '\n');
            }
        }
    }
    char *s = charbuf_to_str(state);
    charbuf_free(state);
    unmark_objects();
    return s;
}




/* Walk all roots and traverse object graph. Mark all p->header.mark=true for
   reachable p.  Fill live_objects[], leaving num_live_objects set at number of live.
 */
static void gc_mark_live() {
    num_live_objects = 0;
    for (int i = 0; i < num_roots; i++) {
        if (DEBUG) printf("root[%d]=%p\n", i, _roots[i]);
        heap_object *p = *_roots[i];
        if (p != NULL) {
            if (DEBUG) printf("root=%s@%p\n", p->header.metaclass->name, p);
            if ( gc_in_heap(p) ) {
                gc_mark_object(p);
            }
        }
    }
}

/* recursively walk object graph starting from p. */
static void gc_mark_object(heap_object *p) {
    if (!p->header.marked) {
        int i;
        if (DEBUG) printf("mark %s@%p\n", p->header.metaclass->name, p);
        p->header.marked = 1;
        live_objects[num_live_objects++] = p; // track live
        // check for tracked heap ptrs in this object
        for (i = 0; i < p->header.metaclass->num_fields; i++) {
            int offset_of_ptr_field = p->header.metaclass->field_offsets[i];
            byte *ptr_to_ptr_field = ((byte *) p) + offset_of_ptr_field;
            heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
            heap_object *target_obj = *ptr_to_obj_ptr_field;
            if (target_obj != NULL) {
                gc_mark_object(target_obj);
            }
        }
    }
}

static bool gc_in_heap(heap_object *p) {
    return p >= (heap_object *) start_of_heap && p <= (heap_object *) end_of_heap;
}

/** Allocate size bytes in the heap; if full, gc() */
static void *gc_alloc_space(int size) {
    if (next_free + size > end_of_heap) {
        gc(); // try to collect        
        if (next_free + size > end_of_heap) { // try again
            return NULL;                      // oh well, no room. puke
        }
    }

    void *p = next_free;
    next_free += size;
    return p;
}

static void print_ptr(heap_object *p) {
    if (p->header.metaclass == &String_metaclass) {
        printf("%s[%d]@%ld\n", p->header.metaclass->name,
                ((String *) p)->length, gc_rel_addr(p));
    } else printf("%s@%ld\n", p->header.metaclass->name, gc_rel_addr(p));
}

static char *ptr_to_str(heap_object *p) {
    char *buf = malloc(200);
    if (p->header.metaclass == &String_metaclass) {
        String *s = (String *) p;
        sprintf(buf, "%04ld:String[%ld+%d]=\"%s\"\n",
                gc_rel_addr(p), sizeof (String), ((String *) p)->length + 1, s->str);
    }
    else {
        sprintf(buf, "%04ld:%s[%d]\n", gc_rel_addr(p), p->header.metaclass->name, gc_object_size(p));
    }
    return buf;
}

static int gc_object_size(heap_object *p) {
    if (p->header.metaclass == &String_metaclass) {
        int n = ((String *) p)->length;
        return p->header.metaclass->size + n + 1;
    }
    return p->header.metaclass->size;
}

static void gc_dump() {
    printf("--------------\n");
    char *s = gc_get_state();
    printf("%s", s);
    free(s);
}

static void print_addr_array(heap_object **array, int len) {
    int i;

    for (i = 0; i < len; i++) printf("%ld ", gc_rel_addr(array[i]));
    putchar('\n');
}

static char *long_array_to_str(unsigned long *array, int len) {
    char *buffer = (char *) malloc(len * sizeof (unsigned long) *2 + len);
    int i;
    for (i = 0; i < len; i++) {
        sprintf(buffer, "%ld", array[i]);
    }
    sprintf(buffer, "\n");
    printf("bbbbb=%s\n", buffer);
    return buffer;
}

/* Get a string with a char per byte of heap. _=live, .=free.
   Chop down to avoid empty space after last live object.
 */
static char *gc_viz_heap() {
    gc_mark_live(); // fill live_objects[]
    char *map = malloc(heap_size);
    memset(map, '.', heap_size);
    int i;
    int last = 0;
    for (i = 0; i < num_live_objects; i++) {
        heap_object *p = live_objects[i];
        if (p == NULL) continue;
        int start = (int) gc_rel_addr(p);
        int j;
        map[start] = '[';
        int n = gc_object_size(p);
        if (start >= heap_size || start + n >= heap_size) {
            printf("object straddles end of heap; start=%d, size=%d, heap_size=%d\n",
                    start, n, heap_size);
            continue;
        }
        char *name = p->header.metaclass->name;
        int nlen = (int) strlen(p->header.metaclass->name);
        int min = nlen < n ? nlen : n;
        strncpy(&map[start + 1], name, min);
        for (j = start + 1 + nlen; j < start + n - 1; j++) {
            map[j] = '_';
        }
        map[start + n - 1] = ']';
        last = start + n;
    }
    map[last] = '\0';
    unmark_objects();
    return map;
}

/** Convert pointers in heap to 0..heapsize-1 */
static unsigned long gc_rel_addr(heap_object *p) {
    if ( p==NULL ) return 0;
    return ((byte *) p)-start_of_heap;
}

/* qsort address comparison function */
static int addrcmp(const void *a, const void *b) {
    void **p = (void **) a;
    void **q = (void **) b;
    return *p - *q;
}

static void unmark_objects() {
    // done with live_objects, wack it
    int i;
    for (i = 0; i < num_live_objects; i++) {
        live_objects[i]->header.marked = 0; // turn off bits set during bogus gc_mark
    }
    num_live_objects = 0;
}