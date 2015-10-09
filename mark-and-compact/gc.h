#ifndef GC_H_
#define GC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* stuff that every instance in the heap must have at the beginning */
typedef struct _heap_object {
	uint32_t size;  // 31 bits for size and 1 bit for inuse/free; size includes header data
	uint8_t marked;	// used during the mark phase of garbage collection
	void (*chase_ptrs)(struct _heap_object *p); // how to chase pointer fields of this type of object
	struct heap_object *forwarded; 				// where we've moved this object during collection
	unsigned char mem[]; // nothing allocated; just a label to location of actual instance data
} heap_object;

// GC interface

/* Initialize a heap with a certain size for use with the garbage collector */
extern void gc_init(int size);

/* Announce you are done with the heap managed by the garbage collector */
extern void gc_done();

/* Perform a mark-and-compact garbage collection, moving all live objects
 * to the start of the heap.
 */
extern void gc();
extern heap_object *gc_alloc(size_t size, void (*chase_ptrs)(struct _heap_object *p));
extern void gc_add_addr_of_root(heap_object **p);

#define gc_begin_func()		int __save = gc_num_roots()
#define gc_end_func()		gc_set_num_roots(__save)
#define gc_add_root(p)		gc_add_addr_of_root((heap_object **)&(p));

// peek into internals for testing and hidden use in macros

extern char *gc_get_state();
extern long gc_heap_highwater();
extern int gc_num_roots();
extern void gc_set_num_roots(int roots);

static const size_t WORD_SIZE_IN_BYTES = sizeof(void *);
static const size_t ALIGN_MASK = WORD_SIZE_IN_BYTES - 1;

/* Pad size n to include header */
static inline size_t size_with_header(size_t n) {
	return n + sizeof(heap_object);
}

/* Align n to nearest word size boundary (4 or 8) */
static inline size_t align_to_word_boundary(size_t n) {
	return (n & ALIGN_MASK) == 0 ? n : (n + WORD_SIZE_IN_BYTES) & ~ALIGN_MASK;
}

/* Convert a user request for n bytes into a size in bytes that has all necessary
 * header room and is word aligned.
 */
static inline size_t request2size(size_t n) {
	return align_to_word_boundary(size_with_header(n));
}

#ifdef __cplusplus
}
#endif

#endif