typedef struct charbuf {
    int next; // next spot to add something
	int size; // how big is array
	char *data;
} charbuf;

extern charbuf charbuf_new(int size);
extern void charbuf_free(charbuf list);
extern void charbuf_add(charbuf *list, char v);
extern void charbuf_add_str(charbuf *list, char *v);
extern int charbuf_eq(charbuf a, charbuf b);
extern char *charbuf_to_str(charbuf list);
