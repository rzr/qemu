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

#include "hgl.h"
#include "common-dgl.h"
#include "common-vertex.h"
#include <stdlib.h>

// Initialization routine, called by first valid eglMakeCurrent.
GL_APICALL_BUILD void* dglActivateContext(int version, void* data)
{
	HGL* hglp = (HGL*)data;

	if(!hglp)
	{
		Dprintf("OpenGL ES 1.1 Emulation: Context initialization!\n");

		hglp = hglLoad();

		hglActivate(hglp);

		Dprintf("OpenGL context information:\n");
		Dprintf("\tVendor      : %s\n", (char const*)HGL_TLS.GetString(GL_VENDOR));
		Dprintf("\tRenderer    : %s\n", (char const*)HGL_TLS.GetString(GL_RENDERER));
		Dprintf("\tVersion     : %s\n", (char const*)HGL_TLS.GetString(GL_VERSION));
		Dprintf("\tExtensions:\n\t%s\n", (char const*)HGL_TLS.GetString(GL_EXTENSIONS));

        dglVertexInit();
	}
	hglActivate(hglp);

	return hglp;
}
