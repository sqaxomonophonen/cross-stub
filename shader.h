#ifndef SHADER_H

#include "platform.h"
#include "m.h"

#define SHADER_MAX_ATTRS (16)

enum shader_attr_type {
	SHADER_ATTR_FLOAT = 1,
	SHADER_ATTR_VEC2,
	SHADER_ATTR_VEC3,
	SHADER_ATTR_VEC4,
};

struct shader_attr_spec {
	const char* symbol;
	enum shader_attr_type type;
};

struct shader {
	GLuint program;
	int n_attrs;
	GLuint attr_locations[SHADER_MAX_ATTRS];
	enum shader_attr_type attr_types[SHADER_MAX_ATTRS];
	size_t stride;
};

void shader_init(struct shader* shader, const char* vert_src, const char* frag_src, struct shader_attr_spec* attr_specs);
void shader_use(struct shader* shader);
void shader_set_attrib_pointers(struct shader* shader);
void shader_enable_arrays(struct shader* shader);
void shader_disable_arrays(struct shader* shader);
void shader_uniform_mat44(struct shader* shader, const char* name, struct mat44* m);
void shader_uniform_texture2D(struct shader* shader, const char* name, GLuint texture);

#define SHADER_H
#endif
