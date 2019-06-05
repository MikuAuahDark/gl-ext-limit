/**
 * Copyright (C) 2019 Miku AuahDark
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstdint>
#include <cstdio>

#include <vector>
#include <string>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <GL/gl.h>

struct OpenGLExtension
{
	std::string name;
	std::string altName;
	int32_t version[4];
	uint16_t year;
};

static std::vector<OpenGLExtension> extensions = {
	{"", "", {0, 0, 0, 0}, 0}
#define EXT(name_str, driver_cap, gll_ver, glc_ver, gles_ver, gles2_ver, yyyy) \
	,{"GL_" #name_str, "GL_" #driver_cap, {gll_ver, gles_ver, gles2_ver, glc_ver}, yyyy}
#include "extensions_table.h"
#undef EXT
};

namespace wrapGL
{
extern "C"
{
#define WRAPGL_DISABLE_glGetString
#include "wrap.h"
#undef WRAPGL_DISABLE_glGetString

__declspec(dllexport) const GLubyte *__stdcall glGetString(GLenum name)
{
	if (name == GL_EXTENSIONS)
	{
		static std::string extensionString;
		static bool extensionInit = false;
		static const char *oldExt = nullptr;
		const char *ext = (const char *) glGetString_t(GL_EXTENSIONS);

		// Null extension, probably no GL context yet
		if (ext == nullptr)
		{
			extensionInit = false;
			return nullptr;
		}
		oldExt = ext;

		// Different extensions, probably new context with different GPU
		if (oldExt != ext && strcmp(oldExt, ext) != 0)
			extensionInit = false;

		if (extensionInit == false)
		{
			const char *year = nullptr;
			const char *length = nullptr;

			uint16_t maxYear = ~0;
			uint32_t maxLength = ~0;
			std::stringstream strbuilder;

			if ((year = getenv("WRAPGL_EXTENSIONS_MAX_YEAR")) != nullptr)
				maxYear = atoi(year);
			
			if ((length = getenv("WRAPGL_EXTENSIONS_MAX_LENGTH")) != nullptr)
				maxLength = (uint32_t) atoi(length);
			
			try
			{
				bool prependSpace = false;
				for (auto &x: extensions)
				{
					if (prependSpace)
					{
						strbuilder.write(" ", 1);
						prependSpace = false;
					}

					if (maxYear < x.year)
						continue;

					strbuilder.seekp(0, std::ios::end);
					size_t len1 = strbuilder.tellp();
					size_t len2 = x.name.length();
					size_t len3 = x.altName.length();

					if (strstr(ext, x.name.c_str()) != nullptr)
					{
						if (len1 + len2 + 2 >= maxLength)
							continue;

						strbuilder.write(x.name.c_str(), len2);
						prependSpace = true;
					}
					else if (strstr(ext, x.altName.c_str()) != nullptr)
					{
						if (len1 + len3 + 2 >= maxLength)
							continue;

						strbuilder.write(x.altName.c_str(), len3);
						prependSpace = true;
					}
				}
			}
			catch (std::exception &e)
			{
				fprintf(stderr, "Exception caught: %s", e.what());
				return nullptr;
			}

			extensionString = strbuilder.str();
			// I don't know but sometime there's space here
			if (extensionString[0] == ' ')
				extensionString = extensionString.substr(1);
			extensionInit = true;
		}

		return (const GLubyte *) extensionString.c_str();
	}
	else
		return glGetString_t(name);
}

// These functions are undocumented and I took them from Mesa3D
decltype(::ChoosePixelFormat) *wglChoosePixelFormat_t = nullptr;
__declspec(dllexport) int __stdcall wglChoosePixelFormat(HDC a, CONST PIXELFORMATDESCRIPTOR* b)
{
	return wglChoosePixelFormat_t(a, b);
}

decltype(::DescribePixelFormat) *wglDescribePixelFormat_t = nullptr;
__declspec(dllexport) int __stdcall wglDescribePixelFormat(HDC a, int b, UINT c, LPPIXELFORMATDESCRIPTOR d)
{
	return wglDescribePixelFormat_t(a, b, c, d);
}

decltype(::GetPixelFormat) *wglGetPixelFormat_t = nullptr;
__declspec(dllexport) int __stdcall wglGetPixelFormat(HDC a)
{
	return wglGetPixelFormat_t(a);
}

decltype(::SetPixelFormat) *wglSetPixelFormat_t = nullptr;
__declspec(dllexport) BOOL __stdcall wglSetPixelFormat(HDC a, int b,CONST PIXELFORMATDESCRIPTOR *c)
{
	return wglSetPixelFormat_t(a, b, c);
}

decltype(::SwapBuffers) *wglSwapBuffers_t = nullptr;
__declspec(dllexport) BOOL __stdcall wglSwapBuffers(HDC a)
{
	return wglSwapBuffers_t(a);
}

}
}

static HMODULE opengl32 = nullptr;

BOOL WINAPI DllMain(HINSTANCE dll, DWORD why, LPVOID)
{
	if (why == DLL_PROCESS_ATTACH)
	{
		const char *gl32 = getenv("WRAPGL_OPENGL32");
		std::string otherGL32;

		if (gl32 == nullptr)
			otherGL32 = std::string(getenv("WINDIR")) + "\\System32\\opengl32.dll";
		else
			otherGL32 = gl32;
		
		opengl32 = LoadLibraryA(otherGL32.c_str());

		if (opengl32 == nullptr)
			return 0;
		
		wrapGL::wglChoosePixelFormat_t = (decltype(::ChoosePixelFormat) *) GetProcAddress(opengl32, "wglChoosePixelFormat");
		wrapGL::wglDescribePixelFormat_t = (decltype(::DescribePixelFormat) *) GetProcAddress(opengl32, "wglDescribePixelFormat");
		wrapGL::wglGetPixelFormat_t = (decltype(::GetPixelFormat) *) GetProcAddress(opengl32, "wglGetPixelFormat");
		wrapGL::wglSetPixelFormat_t = (decltype(::SetPixelFormat) *) GetProcAddress(opengl32, "wglSetPixelFormat");
		wrapGL::wglSwapBuffers_t = (decltype(::SwapBuffers) *) GetProcAddress(opengl32, "wglSwapBuffers");
		return (BOOL) (
			wrapGL::wglChoosePixelFormat_t != nullptr &&
			wrapGL::wglDescribePixelFormat_t != nullptr &&
			wrapGL::wglGetPixelFormat_t != nullptr &&
			wrapGL::wglSetPixelFormat_t != nullptr &&
			wrapGL::wglSwapBuffers_t != nullptr &&
			wrapGL::initialize(opengl32)
		);
	}
	else if (why == DLL_PROCESS_DETACH)
	{
		if (opengl32 != nullptr)
			FreeLibrary(opengl32);
	}

	return 1;
}
