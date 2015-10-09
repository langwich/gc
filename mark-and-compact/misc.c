#include <stdlib.h>
#include <memory.h>
#include "misc.h"

/* A simple character buffer implementation */

charbuf charbuf_new(int size) {
	charbuf a;
	a.data = calloc(size, sizeof(char));
    a.next = 0;
	a.size = size;
	return a;
}

void charbuf_free(charbuf a) {
	free(a.data);
}

void charbuf_add(charbuf *list, char v) {
    if ( list->next >= list->size ) {
        char *old = list->data;
        list->data = calloc(list->size*2, sizeof(char));
        memcpy(list->data, old, list->size);
        free(old);
        list->size *= 2;
    }
    list->data[list->next++] = v;
}

void charbuf_add_str(charbuf *list, char *v) {
    if ( v==NULL ) return;
    while ( *v!='\0' ) {
        charbuf_add(list, *v);
        v++;
    }
}

/* Do two charbufs have same elements? */
int charbuf_eq(charbuf a, charbuf b) {
    int i;
    if ( a.next!=b.next ) return 0;
    int n = a.next > b.next ? a.next : b.next; // get max
    if ( n<=0 ) return 0;
    for (i=0; i<n; i++) {
        if ( a.data[i]!=b.data[i] ) return 0;
    }
    return 1;
}

char *charbuf_to_str(charbuf list) {
    char *s = (char *)malloc(list.size+1);
    memcpy(s, list.data, list.size);
    s[list.size] = '\0';
    return s;
}
