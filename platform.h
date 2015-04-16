#ifndef PLATFORM_H

#if BUILD_LINUX || BUILD_MINGW32
	#define USE_GLEW
	#include <GL/glew.h>
#elif BUILD_OSX
	#include <gl.h>
#else
	#error "you must define a build platform"
#endif

#ifndef BUILD_MINGW32
#include <alloca.h>
#endif

#define PLATFORM_H
#endif
