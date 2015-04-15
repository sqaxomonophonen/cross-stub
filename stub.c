#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "platform.h"

#include "a.h"
#include "m.h"
#include "shader.h"
#include "vtxbuf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "stb_vorbis.c"

#include <SDL.h>

static void gl_viewport_from_sdl_window(SDL_Window* window)
{
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h); CHKGL;
}

#define ASSET_PATH_MAX_LENGTH (1500)
static char assets_base[ASSET_PATH_MAX_LENGTH];
static char assets_tmp[ASSET_PATH_MAX_LENGTH + 256];

static char* asset_path(const char* asset)
{
	strcpy(assets_tmp, assets_base);
	#if BUILD_MINGW32
	const char* postfix = "assets\\";
	#else
	const char* postfix = "assets/";
	#endif
	strcpy(assets_tmp + strlen(assets_tmp), postfix);
	strcpy(assets_tmp + strlen(assets_tmp), asset);
	return assets_tmp;
}

struct music {
	SDL_AudioDeviceID device;
	stb_vorbis* vorbis;
	float* song;
};

static void audio_callback(void* userdata, Uint8* stream_u8, int n)
{
	struct music* music = userdata;
	float* stream = (float*)stream_u8;
	int na = stb_vorbis_get_samples_float_interleaved(music->vorbis, 2, stream, n / (sizeof(float)));
	if (na == 0) stb_vorbis_seek_start(music->vorbis);
}

static void music_init(struct music* music)
{
	int vorbis_error;
	music->vorbis = stb_vorbis_open_filename(asset_path("music.ogg"), &vorbis_error, NULL);
	if (music->vorbis == NULL) arghf("stb_vorbis_open_filename() failed (%d)", vorbis_error);

	SDL_AudioSpec want, have;
	want.freq = 44100;
	want.format = AUDIO_F32;
	want.channels = 2;
	want.samples = 1024;
	want.callback = audio_callback;
	want.userdata = music;

	music->device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (music->device == 0) arghf("SDL_OpenAudioDevice: %s", SDL_GetError());

	//printf("freq: %d\n", have.freq);

	SDL_PauseAudioDevice(music->device, 0);
}

static void music_stop(struct music* music)
{
	SDL_CloseAudioDevice(music->device);
}


struct texture {
	unsigned char* data;
	int width;
	int height;
	int bpp;
	GLuint texture;
};

static void texture_load(struct texture* t, char* asset)
{
	t->data = stbi_load(asset_path(asset), &t->width, &t->height, &t->bpp, 4);
	AN(t->data);

	glGenTextures(1, &t->texture); CHKGL;
	glBindTexture(GL_TEXTURE_2D, t->texture); CHKGL;
	int level = 0;
	int border = 0;
	glTexImage2D(GL_TEXTURE_2D, level, 4, t->width, t->height, border, GL_RGBA, GL_UNSIGNED_BYTE, t->data); CHKGL;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHKGL;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHKGL;
}


struct render {
	SDL_Window* window;
	struct texture texture;
	struct mat44 projection;
	struct shader shader0;
	struct vtxbuf vtxbuf;
	float scroll_offset;
};

static void render_update_projection(struct render* render)
{
	int width, height;
	SDL_GetWindowSize(render->window, &width, &height);
	float fovy = 65;
	float aspect = (float)width / (float)height;
	mat44_set_perspective(&render->projection, fovy, aspect, 0.01f, 409.6f);
}

static void render_init(struct render* render, SDL_Window* window)
{
	memset(render, 0, sizeof(*render));

	render->window = window;

	texture_load(&render->texture, "v.png");

	#include "shader0.glsl.inc"
	struct shader_attr_spec specs[] = {
		{"a_position", SHADER_ATTR_VEC3},
		{"a_uv", SHADER_ATTR_VEC2},
		{NULL}
	};
	shader_init(&render->shader0, shader0_vert_src, shader0_frag_src, specs);

	render_update_projection(render);

	vtxbuf_init(&render->vtxbuf, 65536);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

static void render_frame(struct render* render, struct mat44* view)
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); CHKGL;

	glClearColor(0.0, 0.1, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	vtxbuf_begin(&render->vtxbuf, &render->shader0, GL_QUADS);
	shader_uniform_mat44(&render->shader0, "u_projection", &render->projection);
	shader_uniform_mat44(&render->shader0, "u_view", view);
	shader_uniform_texture2D(&render->shader0, "u_texture", 0);

	for (int i = 0; i < 1000; i++) {
		float quad[5*4];
		for (int j = 0; j < 4; j++) {
			int dx = (j&1)^((j&2)>>1);
			int dy = (j&2)>>1;
			quad[j*5 + 0] = (i-500) * 2 + dx + render->scroll_offset;
			quad[j*5 + 1] = -dy + 0.5;
			quad[j*5 + 2] = -1;
			quad[j*5 + 3] = dx;
			quad[j*5 + 4] = dy;
		}
		vtxbuf_element(&render->vtxbuf, quad, sizeof(quad));
	}

	render->scroll_offset += 0.01f;
	if (render->scroll_offset > 2.0f) render->scroll_offset -= 2.0f;

	vtxbuf_end(&render->vtxbuf);
}

struct cam {
	struct vec3 position;
	float pitch, yaw;
};

static void cam_init(struct cam* cam)
{
	memset(cam, 0, sizeof(*cam));
}

int main(int argc, char** argv)
{
	SAZ(SDL_Init(SDL_INIT_EVERYTHING));
	atexit(SDL_Quit);

	{
		char* sdl_base_path = SDL_GetBasePath();
		if (sdl_base_path) {
			if (strlen(sdl_base_path) > (sizeof(assets_base)-1)) arghf("SDL_GetBasePath() too long");
			strcpy(assets_base, sdl_base_path);
			SDL_free(sdl_base_path);
		} else {
			arghf("SDL_GetBasePath() = NULL");
		}
	}

	int bitmask = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL;
	SDL_Window* window = SDL_CreateWindow(
			"stub",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			0, 0,
			bitmask);
	SAN(window);

	SDL_GLContext glctx = SDL_GL_CreateContext(window);
	SAN(glctx);

	SAZ(SDL_GL_SetSwapInterval(1)); // or -1, "late swap tearing"?

	#ifdef USE_GLEW
	{
		GLenum err = glewInit();
		if (err != GLEW_OK) arghf("glewInit() failed: %s", glewGetErrorString(err));
	}
	#endif

	gl_viewport_from_sdl_window(window);

	struct music music;
	music_init(&music);

	struct render render;
	render_init(&render, window);

	struct cam cam;
	cam_init(&cam);

	SDL_SetRelativeMouseMode(SDL_TRUE);

	int exiting = 0;
	while (!exiting) {
		SDL_Event e;
		int mdx = 0;
		int mdy = 0;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) exiting = 1;
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					exiting = 1;
				}
			}

			 if (e.type == SDL_MOUSEMOTION) {
				 mdx += e.motion.xrel;
				 mdy += e.motion.yrel;
			 }
		}

		{
			float sensitivity = 0.1f;
			cam.yaw += (float)mdx * sensitivity;
			cam.pitch += (float)mdy * sensitivity;
			float pitch_limit = 90;
			if (cam.pitch > pitch_limit) cam.pitch = pitch_limit;
			if (cam.pitch < -pitch_limit) cam.pitch = -pitch_limit;
		}


		struct mat44 view;
		mat44_set_identity(&view);
		mat44_rotate_x(&view, cam.pitch);
		mat44_rotate_y(&view, cam.yaw);

		struct vec3 translate;
		vec3_scale(&translate, &cam.position, -1);
		mat44_translate(&view, &translate);

		render_frame(&render, &view);

		SDL_GL_SwapWindow(window);
	}

	music_stop(&music);

	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(glctx);

	return EXIT_SUCCESS;
}
