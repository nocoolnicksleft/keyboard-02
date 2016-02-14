/* module for circular buffers
 * -- Baegsch 2005
 */
#include <16F877.h>
#ifndef EINVAL
#define EINVAL 22
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memcpy */
#include "circular-buffer.h"

static int buffer_count;
static int buffer_size_count;

struct buffer *buf_alloc(int size)
{
	struct buffer *buf;

	if(size < 1)
		return NULL;

	buf = malloc(sizeof(*buf));
	if(!buf)
		return NULL;

	buf->bs = malloc(size);
	if(!buf->bs) {
		buf_free(buf);
		return NULL;
	}

	buf->size = size;
	buf->get = buf->put = buf->bs;

	buffer_count++;
	buffer_size_count += size;
	return buf;
}

void buf_free(struct buffer *buf)
{
	if(buf) {
		if(buf->bs) {
			buffer_size_count -= buf->size;
			buffer_count--;
			free(buf->bs);
			free(buf);
		}
	}

	return;
}

int buf_used(struct buffer *buf)
{
	if(buf)
		return (buf->size + (buf->put - buf->get)) % buf->size;
	else
		return -EINVAL;
}
	
int buf_space(struct buffer *buf)
{
	if(buf)
		return (buf->size - (buf->put - buf->get)-1) % buf->size;
	else
		return -EINVAL;
}

int buf_get(struct buffer *buf, void *dest, int size)
{
	int count;

	if(size < 1 || !dest || !buf)
		return -EINVAL;

	if(size > buf_used(buf))
		size = buf_used(buf);

	if(size == 0)
		return 0;

	count = buf->bs + buf->size - buf->get;
	if(size > count) {
		memcpy(dest, buf->get, count);
		memcpy(dest+count, buf->bs, size-count);
		buf->get = buf->bs + size-count;
	} else {
		memcpy(dest, buf->get, size);
		buf->get += size;
	}

	return size;
}

int buf_put(struct buffer *buf, void *data, int size)
{
	int count;

	if(!buf || size<1 || !data)
		return -EINVAL;

	if(size > buf_space(buf))
		size = buf_space(buf);

	count = buf->bs + buf->size - buf->put;
	if(size > count) {
		memcpy(buf->put, data, count);
		memcpy(buf->bs, data+count, size-count);
		buf->put = buf->bs + size-count;
	} else {
		memcpy(buf->put, data, size);
		buf->put += size;
	}

	return size;
}

void buf_flush(struct buffer *buf)
{
	buf->get = buf->put;
	return;
}

void buf_dbg(struct buffer *buf)
{
	printf("used: %d free: %d get@%d put@%d\n",
		buf_used(buf), buf_space(buf),
		buf->get - buf->bs, buf->put - buf->bs);

	return;
}

void buf_print_stats(void)
{
	printf("allocated %d buffers using %d bytes\n",
		buffer_count, buffer_size_count);
	return;
}
