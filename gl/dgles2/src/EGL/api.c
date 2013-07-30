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

#include "degl.h"
#include "common.h"
#include "context.h"
#include "surface.h"


EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
	if(api != EGL_OPENGL_ES_API)
	{
		Dprintf("WARNING: Unsupported API requested 0x%x!\n", api);
		return EGL_FALSE;
	}
	return EGL_TRUE;
}

EGLAPI_BUILD EGLenum EGLAPIENTRY eglQueryAPI(void)
{
	return EGL_OPENGL_ES_API;
}

EGLAPI_BUILD EGLint EGLAPIENTRY eglGetError(void)
{
	degl_tls *tls = deglGetTLS();
	EGLint error = tls->error;
	tls->error = EGL_SUCCESS;
	return error;
}

void deglSetError(EGLint error)
{
	deglGetTLS()->error = error;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
	EGLDisplay display = eglGetCurrentDisplay();
	if(display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT);
	}
	deglFreeTLS();
	return EGL_TRUE;
}

EGLAPI_BUILD __eglMustCastToProperFunctionPointerType EGLAPIENTRY
	eglGetProcAddress(const char *procname)
{
	// TODO: this should work for glPointSizePointerOES at least!
	DUMMY();
	return 0;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglWaitClient(void)
{
	DEGLContext *context = eglGetCurrentContext();
	DEGLSurface *surface = eglGetCurrentSurface(EGL_DRAW);
	if(context)
	{
		if(!surface)
		{
			Dprintf("bad current surface!\n");
			deglSetError(EGL_BAD_CURRENT_SURFACE);
			return EGL_FALSE;
		}
		(*context->glesfuncs->glFinish)();
	}
	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglWaitGL(void)
{
#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		hglX.WaitGL();
		return EGL_TRUE;
	}
#	endif // CONFIG_GLX == 1

	DUMMY();
	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine)
{
#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		hglX.WaitX();
		return EGL_TRUE;
	}
#	endif // CONFIG_GLX == 1

	DUMMY();
	return EGL_TRUE;
}

