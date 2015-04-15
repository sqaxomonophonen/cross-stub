#include <stdlib.h>
#include <string.h>

#include "a.h"
#include "shader.h"

static int shader_attr_type_floats(enum shader_attr_type type)
{
	switch (type) {
		case SHADER_ATTR_FLOAT: return 1;
		case SHADER_ATTR_VEC2: return 2;
		case SHADER_ATTR_VEC3: return 3;
		case SHADER_ATTR_VEC4: return 4;
		default: return -1;
	}
}

static GLuint create_shader(GLenum type, const char* src)
{
	GLuint shader = glCreateShader(type); CHKGL;
	glShaderSource(shader, 1, &src, 0);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint msglen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msglen);
		GLchar* msg = (GLchar*) malloc(msglen + 1);
		glGetShaderInfoLog(shader, msglen, NULL, msg);
		const char* stype = type == GL_VERTEX_SHADER ? "vertex" : type == GL_FRAGMENT_SHADER ? "fragment" : "waaaat";
		arghf("%s shader error: %s -- source:\n%s", stype, msg, src);
	}
	return shader;
}

void shader_init(struct shader* shader, const char* vert_src, const char* frag_src, struct shader_attr_spec* attr_specs)
{
	memset(shader, 0, sizeof(*shader));

	GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vert_src);
	GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, frag_src);

	shader->program = glCreateProgram(); CHKGL;

	glAttachShader(shader->program, vertex_shader);
	glAttachShader(shader->program, fragment_shader);

	glLinkProgram(shader->program);

	GLint status;
	glGetProgramiv(shader->program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint msglen;
		glGetProgramiv(shader->program, GL_INFO_LOG_LENGTH, &msglen);
		GLchar* msg = (GLchar*) malloc(msglen + 1);
		glGetProgramInfoLog(shader->program, msglen, NULL, msg);
		arghf("shader link error: %s", msg);
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	int i = 0;
	size_t stride = 0;
	for (struct shader_attr_spec* spec = attr_specs; spec->symbol != NULL; spec++, i++) {
		shader->attr_locations[i] = glGetAttribLocation(shader->program, spec->symbol); CHKGL;
		shader->attr_types[i] = spec->type;
		stride += shader_attr_type_floats(shader->attr_types[i]) * sizeof(float);
	}
	shader->n_attrs = i;
	shader->stride = stride;
}

void shader_use(struct shader* shader)
{
	glUseProgram(shader->program);
}

void shader_set_attrib_pointers(struct shader* shader)
{
	char* offset = 0;
	for (int i = 0; i < shader->n_attrs; i++) {
		int n_floats = shader_attr_type_floats(shader->attr_types[i]);
		glVertexAttribPointer(shader->attr_locations[i], n_floats, GL_FLOAT, GL_FALSE, shader->stride, offset); CHKGL;
		offset += n_floats * sizeof(float);
	}
}

void shader_enable_arrays(struct shader* shader)
{
	for (int i = 0; i < shader->n_attrs; i++) {
		glEnableVertexAttribArray(shader->attr_locations[i]); CHKGL;
	}
}

void shader_disable_arrays(struct shader* shader)
{
	for (int i = 0; i < shader->n_attrs; i++) {
		glDisableVertexAttribArray(shader->attr_locations[i]); CHKGL;
	}
}

void shader_uniform_mat44(struct shader* shader, const char* name, struct mat44* m)
{
	GLint location = glGetUniformLocation(shader->program, name); CHKGL;
	glUniformMatrix4fv(location, 1, GL_FALSE, m->s); CHKGL;
}

void shader_uniform_texture2D(struct shader* shader, const char* name, GLuint texture)
{
	GLint location = glGetUniformLocation(shader->program, name); CHKGL;
	glUniform1i(location, texture);
}

