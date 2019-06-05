# Copyright (C) 2019 Miku AuahDark
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.0.2)

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
	message(FATAL_ERROR "Prevented in-tree build!")
endif()

if (POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()

project(glwrapper LANGUAGES C CXX)

# C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If wrap.h is not found, try to create new (Protip: it doesn't work)
#if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/wrap.h)
#	message(FATAL_ERROR "wrap.h not found. Please generate wrap.h using Lua")
#endif()

if (NOT WIN32 OR CYGWIN)
	message(FATAL_ERROR "Non-Win32 is not supported")
endif()

# Find usable Lua
find_program(LUA_EXECUTABLE
	NAMES luajit lua5.1 lua5.2 lua5.3 lua
)
if (NOT LUA_EXECUTABLE)
	message(FATAL_ERROR "Missing Lua executable to generate wrap.h and wrap.def")
endif()
message(STATUS "Lua used to generate wrap.h and wrap.def: ${LUA_EXECUTABLE}")

set(_PF86 "ProgramFiles(x86)")
if (DEFINED ENV{${_PF86}})
	file(TO_CMAKE_PATH $ENV{${_PF86}} _PROGRAMFILES)
else()
	file(TO_CMAKE_PATH $ENV{ProgramFiles} _PROGRAMFILES)
endif()

if (DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
	set(_WSDK_INCLUDE "${_PROGRAMFILES}/Windows Kits/10/Include/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um")
else()
	set(_WSDK_INCLUDE
		"${_PROGRAMFILES}/Windows Kits/10/Include/10.0.10150.0/um"
		"${_PROGRAMFILES}/Windows Kits/10/Include/10.0.10056.0/um"
		"${_PROGRAMFILES}/Windows Kits/8.1/Include/um"
		"${_PROGRAMFILES}/Windows Kits/8.0/Include/um"
		"${_PROGRAMFILES}/Microsoft SDKs/Windows/v7.1A/Include"
		"${_PROGRAMFILES}/Microsoft SDKs/Windows/v8.0A/Include"
		"${_PROGRAMFILES}/Microsoft SDKs/Windows/v8.1/Include"
		"${_PROGRAMFILES}/Microsoft SDKs/Windows/v8.1A/Include"
		"${_PROGRAMFILES}/Microsoft SDKs/Windows/v10.0A/Include"
	)
endif()

# Find gl.h
find_file(OPENGL_GL_H
	NAMES GL/gl.h
	PATHS ${_WSDK_INCLUDE}
)
if (NOT OPENGL_GL_H)
	message(FATAL_ERROR "Cannot find gl.h")
endif()
message(STATUS "gl.h: ${OPENGL_GL_H}")

# Find WinGDI.h
find_file(WINGDI_H
	NAMES WinGDI.h wingdi.h
	PATHS ${_WSDK_INCLUDE}
)
if (NOT WINGDI_H)
	message(FATAL_ERROR "Cannot find WinGDI.h")
endif()
message(STATUS "WinGDI.h: ${WINGDI_H}")

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/wrap.h ${CMAKE_CURRENT_BINARY_DIR}/wrap.def
	COMMAND "${LUA_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/create_wrapper.lua" "${OPENGL_GL_H}" "${WINGDI_H}" "${CMAKE_CURRENT_BINARY_DIR}/wrap.h" "${CMAKE_CURRENT_BINARY_DIR}/wrap.def"
	VERBATIM
)

add_library(opengl32 SHARED Main.cpp ${CMAKE_CURRENT_BINARY_DIR}/wrap.def)
target_include_directories(opengl32 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(opengl32 PRIVATE _CRT_SECURE_NO_WARNINGS)
set_target_properties(opengl32 PROPERTIES PREFIX "")
install(TARGETS opengl32 DESTINATION bin)
