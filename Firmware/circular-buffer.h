#ifndef __CIRCULAR_BUFFERS_H
#define __CIRCULAR_BUFFERS_H

typedef unsigned char __u8;

struct buffer {
	__u8 *bs, *get, *put;
	int size;
};

struct buffer *buf_alloc(int);
void buf_free(struct buffer *);
int buf_used(struct buffer *);
int buf_space(struct buffer *);
int buf_put(struct buffer *, void *, int);
int buf_get(struct buffer *, void *, int);
void buf_dbg(struct buffer *);
void buf_flush(struct buffer *);
void buf_print_stats(void);

#endif
