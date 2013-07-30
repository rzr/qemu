/* Copyright (C) 2010  Nokia Corporation All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <string.h>
#include CONFIG_H

// Correct defines for platform detection.
#if(CONFIG_GLX == 1)
#	ifndef __unix__
#		define __unix__
#	endif
#elif(CONFIG_COCOA == 1)
#	ifndef __unix__
#		define __unix__
#	endif
#	ifndef MESA_EGL_NO_X11_HEADERS
#		define MESA_EGL_NO_X11_HEADERS
#	endif
#elif(CONFIG_OFFSCREEN == 1)
#	define degl_offscreen__
#else
#	error "Frontend needs to be enabled!"
#endif

// DLL-mangling.
#ifdef _WIN32
#	define DGLES2_EXPORT __declspec(dllexport)
#	define DGLES2_IMPORT __declspec(dllimport)
#	define DGLES2_CALL	 __stdcall
#else
#	define DGLES2_EXPORT
#	define DGLES2_IMPORT extern
#	define DGLES2_CALL
#endif

// For proper imports and exports.
#if(defined BUILD_EGL)
#	define EGLAPI       extern
#	define EGLAPI_BUILD DGLES2_EXPORT
#elif(defined BUILD_GLES2 | defined BUILD_GLES)
#	define GL_API           extern
#	define GL_APICALL       extern
#	define GL_APICALL_BUILD DGLES2_EXPORT
#else
#	error "Only to be used with EGL or GLES!"
#endif

#define EGLAPIENTRY DGLES2_CALL
#define GL_APIENTRY DGLES2_CALL

// The actual standard headers.
#include "EGL/egl.h"

// For malloc
#ifdef __APPLE__
#	include <stdlib.h>
#else
#	include <malloc.h>
#endif

// Debug location aids.
#define COMMON_STAMP_FMT "%s:%d(%s)"
#define COMMON_STAMP_ARGS ,(strchr(__FILE__, '/')?:__FILE__), __LINE__, __PRETTY_FUNCTION__

#define DUMMY() \
	fprintf(stderr, "\x1b[41mDUMMY\x1b[0m " COMMON_STAMP_FMT ": Unimplemented!\n" COMMON_STAMP_ARGS)
#define STUB_ONCE(format, args...) \
	{ \
		static int once = 1; \
		if(once) \
		{ \
			fprintf(stderr, "\x1b[43mSTUB ONCE\x1b[0m " COMMON_STAMP_FMT ": " format COMMON_STAMP_ARGS, ##args); \
			once = 0; \
		} \
	} (void) 0
#define STUB(format, args...) \
	fprintf(stderr, "STUB " COMMON_STAMP_FMT ": " format COMMON_STAMP_ARGS, ##args)

#if(CONFIG_DEBUG == 1 && !defined NDEBUG)
#	define Dprintf(format, args...) fprintf(stderr, "DEBUG " COMMON_STAMP_FMT ": " format  COMMON_STAMP_ARGS, ##args)
#else // NDEBUG
#	define Dprintf(format, args...) (void)0
#endif // !NDEBUG

#endif /* COMMON_H_ */
