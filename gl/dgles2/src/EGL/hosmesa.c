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

#include "common.h"
#include "hosmesa.h"
#include "degl.h"

HOSMesa hOSMesa;

// Load the OSMesa function pointers from renderer library.
int hOSMesaLoad()
{
	Dprintf("Loading OSMesa functions...\n");
#if(CONFIG_STATIC == 1)
#	define HOSMESA_FUNC(ret, name, attr) \
		if(degl_handle == (void*)-1) \
		{ \
			extern ret GL_APIENTRY OSMesa##name attr; \
			hOSMesa.name = &OSMesa##name; \
		} \
		else if((hOSMesa.name = deglGetHostProcAddress("OSMesa" #name)) == NULL) \
		{ \
			fprintf(stderr, "Function OSMesa" #name " not found!\n"); \
		}
#else
#	define HOSMESA_FUNC(ret, name, attr) \
		if((hOSMesa.name = (HOSMesa##name##_t)deglGetHostProcAddress("OSMesa" #name)) == NULL) \
		{ \
			fprintf(stderr, "Function OSMesa" #name " not found!\n"); \
		}
#endif // CONFIG_STATIC != 1

	HOSMESA_FUNCS

#undef HOSMESA_FUNC

	return 1;
}
