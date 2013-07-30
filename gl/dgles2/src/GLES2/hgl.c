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
#include "../EGL/degl.h"

// Load the host renderer OpenGL functions.
HGL* hglLoad()
{
	HGL* hglp = malloc(sizeof(*hglp));
	
	Dprintf("Loading GL functions...\n");

#	if(CONFIG_STATIC == 1)
#		define HGL_FUNC(ret, name, attr) \
			if(degl_handle == (void*)-1) \
			{ \
				extern ret GL_APIENTRY mgl##name attr; \
				hglp->name = &mgl##name; \
			} \
			else if((hglp->name = dglGetHostProcAddress("gl" #name)) == NULL) \
			{ \
				fprintf(stderr, "Function gl" #name " not found!\n"); \
			}
#	else
#		define HGL_FUNC(ret, name, attr) \
			if((hglp->name = dglGetHostProcAddress("gl" #name)) == NULL) \
			{ \
				fprintf(stderr, "Function gl" #name " not found!\n"); \
			}
#	endif // CONFIG_STATIC != 1

	HGL_FUNCS

#undef HGL_FUNC

#	if(CONFIG_STATIC == 1)
#		define HGL_FUNC(ret, name, attr) \
			if(degl_handle == (void*)-1) \
			{ \
				extern ret GL_APIENTRY mgl##name attr; \
				hglp->name = &mgl##name; \
			} \
			else if((hglp->name = dglGetHostProcAddress("gl" #name)) == NULL) \
			{ \
				Dprintf("Optional function gl" #name " not found!\n"); \
			}
#	else
#		define HGL_FUNC(ret, name, attr) \
			if((hglp->name = dglGetHostProcAddress("gl" #name)) == NULL) \
			{ \
				Dprintf("Optional function gl" #name " not found!\n"); \
			}
#	endif // CONFIG_STATIC != 1
    
	HGL_OPTIONAL_FUNCS
    
#undef HGL_FUNC

	return hglp;
}

void hglActivate(HGL* hglp)
{
	Dprintf("OpenGL ES 2.0 Emulation: Activating previous GL State.\n");
	degl_tls *tls = deglGetTLS();
	if(!tls->hgl)
	{
		tls->hgl = malloc(sizeof(HGL));
	}
	HGL *h = tls->hgl;

#define HGL_FUNC(ret, name, attr) \
	h->name = hglp ? hglp->name : 0;
	
	HGL_FUNCS
	HGL_OPTIONAL_FUNCS
	
#undef HGL_FUNC
}
