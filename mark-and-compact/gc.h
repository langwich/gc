#ifndef GC_H_
#define GC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;

typedef struct { 	// stuff that every instance in the heap must have at the beginning
	int type; 		// numeric identifier for the type of this heap object
	byte marked;	// used during the mark phase of garbage collection
	struct heap_object *forwarded; // where we've moved this object during collection
	unsigned char mem[]; // nothing allocated; just a label to location after size
} heap_object;

typedef struct {
	uint32_t size;       // 31 bits for size and 1 bit for inuse/free; size includes header data
	void (*mark)(heap_object *p);
} type_metadata;

// GC interface

/* Initialize a heap with a certain size for use with the garbage collector */
extern void gc_init(int size);

/* Announce you are done with the heap managed by the garbage collector */
extern void gc_done();

/* Perform a mark-and-compact garbage collection, moving all live objects
 * to the start of the heap.
 */
extern void gc();
extern heap_object *gc_alloc(int type);
extern void gc_add_addr_of_root(heap_object **p);

#define gc_begin_func()		int __save = gc_num_roots()
#define gc_end_func()		gc_set_num_roots(__save)
#define gc_add_root(p)		gc_add_addr_of_root((heap_object **)&(p));

// peek into internals for testing and hidden use in macros

extern char *gc_get_state();
extern long gc_heap_highwater();
extern int gc_num_roots();
extern void gc_set_num_roots(int roots);

#ifdef __cplusplus
}
#endif

#endif