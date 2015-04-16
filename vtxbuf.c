#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vtxbuf.h"

void vtxbuf_init(struct vtxbuf* vb, size_t sz)
{
	memset(vb, 0, sizeof(*vb));
	vb->sz = sz;
	vb->data = malloc(sz);
	AN(vb->data);
	glGenBuffers(1, &vb->buffer); CHKGL;
	glBindBuffer(GL_ARRAY_BUFFER, vb->buffer); CHKGL;
	glBufferData(GL_ARRAY_BUFFER, sz, vb->data, GL_STREAM_DRAW); CHKGL;
}

void vtxbuf_begin(struct vtxbuf* vb, struct shader* shader, GLenum mode)
{
	vb->shader = shader;
	vb->mode = mode;
	vb->used = 0;
	shader_use(shader);
	shader_enable_arrays(shader);
}

void vtxbuf_flush(struct vtxbuf* vb)
{
	if (vb->used == 0) return;

	glBindBuffer(GL_ARRAY_BUFFER, vb->buffer); CHKGL;
	glBufferSubData(GL_ARRAY_BUFFER, 0, vb->used, vb->data); CHKGL;

	shader_set_attrib_pointers(vb->shader);
	glBindBuffer(GL_ARRAY_BUFFER, vb->buffer); CHKGL;
	glDrawArrays(vb->mode, 0, vb->used / vb->shader->stride);
	vb->used = 0;
}

void vtxbuf_end(struct vtxbuf* vb)
{
	vtxbuf_flush(vb);
	shader_disable_arrays(vb->shader);
}

void vtxbuf_element(struct vtxbuf* vb, float* data, size_t sz)
{
	if ((vb->used + sz) > vb->sz) vtxbuf_flush(vb);
	if ((vb->used + sz) > vb->sz) WRONG("not enough room for even one element");
	memcpy(((uint8_t*)vb->data) + vb->used, data, sz);
	vb->used += sz;
}

