GL Extensions Limiter
=====================

Limits extensions that is reported by the `glGetString(GL_EXTENSIONS)`. Mainly used to workaround
some old games which copies the string to fixed-size buffer which result in buffer overflow and
result in crash. Only for Windows.

Usage
-----

It's controlled by these 2 environment variables:

* `WRAPGL_EXTENSIONS_MAX_YEAR` - returns only extensions at specified year. See https://www.mesa3d.org/application-issues.html

* `WRAPGL_EXTENSIONS_MAX_LENGTH` - returns only at most specified string extensions. May have unintended consequences. It's
recommended to use this along with `WRAPGL_EXTENSIONS_MAX_YEAR` to have better control on extensions.

Compile
-------

Prerequisites:

* Windows.

* Lua interpreter (LuaJIT, Lua5.1-Lua5.3) to generate the necessary files.

* CMake 3.0 to generate the project. Tested in CMake 3.14.1.

* Windows SDK, should come with Visual Studio. Required for gl.h and WinGDI.h

Using CMake. VS2017 + Windows 10 SDK 10.0.17134 is known to work. MinGW is not tested. Cygwin and other OS is not supported.

Make sure to do out-of-tree build. The CMake script will make sure you do so by preventing in-tree build.

License
-------

MIT License. However this project also uses parts of Mesa3D source [`extensions_table.h`](https://github.com/mesa3d/mesa/blob/45ca7798dc32c1cb7da8f94af9a7d7400ee9bc12/src/mesa/main/extensions_table.h). See that file in this repository for license details.
