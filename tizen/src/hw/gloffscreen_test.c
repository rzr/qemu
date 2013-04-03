/*
 *  Offscreen OpenGL abstraction layer
 *
 *  Copyright (c) 2010 Intel Corporation
 *  Written by: 
 *    Gordon Williams <gordon.williams@collabora.co.uk>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "gloffscreen_test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#elif __APPLE__
#include <OpenGL/gl.h>
#include <sys/time.h>
#else
#include <GL/gl.h>
#include <sys/time.h>
#endif

#ifdef MANGLE_OPENGL_SYMBOLS
#include "gl_mangled.h"
#endif

// ---------------------------------------------------
//  Copied from glx.h as we need them in windows too
/*
 * Tokens for glXChooseVisual and glXGetConfig:
 */
#define GLX_USE_GL      1
#define GLX_BUFFER_SIZE     2
#define GLX_LEVEL       3
#define GLX_RGBA        4
#define GLX_DOUBLEBUFFER    5
#define GLX_STEREO      6
#define GLX_AUX_BUFFERS     7
#define GLX_RED_SIZE        8
#define GLX_GREEN_SIZE      9
#define GLX_BLUE_SIZE       10
#define GLX_ALPHA_SIZE      11
#define GLX_DEPTH_SIZE      12
#define GLX_STENCIL_SIZE    13
#define GLX_ACCUM_RED_SIZE  14
#define GLX_ACCUM_GREEN_SIZE    15
#define GLX_ACCUM_BLUE_SIZE 16
#define GLX_ACCUM_ALPHA_SIZE    17
// ---------------------------------------------------

#define TX (32)
#define TY (32)

#define CHECK_SUCCESS	0
#define CHECK_FAIL		1
int gl_acceleration_capability_check (void) {
    //int test_failure = 0;
    GloContext *context;
    GloSurface *surface;
    unsigned char *datain;
    unsigned char *datain_flip;
    unsigned char *dataout;
    unsigned char *p;
    int x,y;
    const int bufferAttributes[] = {
            GLX_RED_SIZE,      8,
            GLX_GREEN_SIZE,    8,
            GLX_BLUE_SIZE,     8,
            GLX_ALPHA_SIZE,    8,
            GLX_DEPTH_SIZE,    0,
            GLX_STENCIL_SIZE,  0,
            0,
        };

#ifdef MANGLE_OPENGL_SYMBOLS
    if (mgl_load_symbols("libGL.so.1")) {
		return CHECK_FAIL;
    }
#endif

    int bufferFlags = glo_flags_get_from_glx(bufferAttributes, 0);
    int bpp = glo_flags_get_bytes_per_pixel(bufferFlags);
    int glFormat, glType;
/*
    if (glo_sanity_test () != 0) {
        // test failed.
        return 1;
    }
*/ 
	datain = (unsigned char *)malloc(4*TX*TY);
	datain_flip = (unsigned char *)malloc(4*TX*TY);

    memset(datain, 0, TX*TY*4);
    memset(datain_flip, 0, TX*TY*4);

    p = datain;
    for (y=0;y<TY;y++) {
      for (x=0;x<TX;x++) {
        p[0] = x;
        p[1] = y;
        //if (y&1) { p[0]=0; p[1]=0; }
        if (bpp>2) p[2] = 0;
        if (bpp>3) p[3] = 0xFF;
        p+=bpp;
      }
      memcpy(&datain_flip[((TY-1)-y)*bpp*TX], &datain[y*bpp*TX], bpp*TX);
    }

    if (glo_init() != 0) {
        printf ("Host does not have GL hardware acceleration!(glo_init() failed)\n");
		free (datain);
		free (datain_flip);
		return CHECK_FAIL;
    }

    // new surface
    context = glo_context_create(bufferFlags, 0);
	if (context == NULL) {
        printf ("Host does not have GL hardware acceleration!(context_create() failed)\n");
		free (datain);
		free (datain_flip);
		return CHECK_FAIL;
    }

    surface = glo_surface_create(TX, TY, context);
	if (surface == NULL) {
        printf ("Host does not have GL hardware acceleration!(surface_create() failed)\n");
		free (datain);
		free (datain_flip);
		return CHECK_FAIL;
    }

    glo_surface_makecurrent(surface);
    printf("GL VENDOR %s\n", glGetString(GL_VENDOR));
    printf("GL RENDERER %s\n", glGetString(GL_RENDERER));
    printf("GL VERSION %s\n", glGetString(GL_VERSION));
    //printf("GLSL VERSION %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    if (strstr((const char*)glGetString(GL_RENDERER), "Software")) {
        printf ("Host does not have GL hardware acceleration!(No host gl driver)\n");
		free (datain);
		free (datain_flip);
		return CHECK_FAIL;
    }

    // fill with stuff (in correctly ordered way)
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,TX, 0,TY, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos2f(0,0);
    glo_flags_get_readpixel_type(bufferFlags, &glFormat, &glType);
    printf("glFormat: 0x%08X glType: 0x%08X\n", glFormat, glType);
    glDrawPixels(TX,TY,glFormat, glType, datain_flip);
    glFlush();
	
	dataout = (unsigned char *)malloc(4*TX*TY);
    memset(dataout, 0, bpp*TX*TY);

    glo_surface_getcontents(surface, TX*bpp, bpp*8, dataout);

    // destroy surface
    glo_surface_destroy(surface);
    glo_context_destroy(context);
    // compare
    if (memcmp(datain, dataout, bpp*TX*TY)!=0) {
        printf ("Host does not have GL hardware acceleration!(datain != dataout)\n");
		free (datain);
		free (datain_flip);
		free (dataout);
		return CHECK_FAIL;
    }

    //glo_kill();
    //printf ("Testing %s\n", (test_failure ? "FAILED" : "PASSED"));
    free (datain);
    free (datain_flip);
    free (dataout);

	return CHECK_SUCCESS;
}
