# Stub for gamedev cross development

Goal is to ease simultaneous development for Linux, Win32 and Mac OS X. mingw32 is used to build Windows executables.

It's not meant to be used as a library, but as copy-pastable code to get you started quickly.

# Usage
- Linux: `make`
- Win32: `make -f Makefile.mingw32` (on Fedora there are mingw32 packages for all dependencies)
- OS X: `make -f Makefile.osx`

# Done
- Linux, Win32(mingw32), OS X
- SDL2
- OpenGL, texture loading via `stb_image.h`
- Ogg Vorbis music playback via SDL2 and `stb_vorbis.c`

# To do
- `make dist` packaging
- .exe/.app icons
- text rendering via `stb_truetype.h`

Thanks to Sean Barrett for the stb libraries (http://github.com/nothings/stb)
