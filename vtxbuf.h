#ifndef VTXBUF_H

#include "platform.h"
#include "shader.h"

struct vtxbuf {
	GLuint buffer;
	size_t sz, used;
	float* data;
	struct shader* shader;
	GLenum mode;
};

void vtxbuf_init(struct vtxbuf* vb, size_t sz);
void vtxbuf_begin(struct vtxbuf* vb, struct shader* shader, GLenum mode);
void vtxbuf_flush(struct vtxbuf* vb);
void vtxbuf_end(struct vtxbuf* vb);
void vtxbuf_element(struct vtxbuf* vb, float* data, size_t sz);

#define VTXBUF_H
#endif
