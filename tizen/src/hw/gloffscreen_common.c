/*
 *  Offscreen OpenGL abstraction layer - Common utilities
 *
 *  Copyright (c) 2010 Intel Corporation
 *  Written by:
 *    Gordon Williams <gordon.williams@collabora.co.uk>
 *    Ian Molton <ian.molton@collabora.co.uk>
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
#include "gloffscreen.h"

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include <GL/gl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *g_malloc(size_t size);
void *g_realloc(void *ptr, size_t size);
void g_free(void *ptr);

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
/*
 * GLX 1.3 and later:
 */
#define GLX_CONFIG_CAVEAT		0x20
#define GLX_DONT_CARE			0xFFFFFFFF
#define GLX_X_VISUAL_TYPE		0x22
#define GLX_TRANSPARENT_TYPE		0x23
#define GLX_TRANSPARENT_INDEX_VALUE	0x24
#define GLX_TRANSPARENT_RED_VALUE	0x25
#define GLX_TRANSPARENT_GREEN_VALUE	0x26
#define GLX_TRANSPARENT_BLUE_VALUE	0x27
#define GLX_TRANSPARENT_ALPHA_VALUE	0x28
#define GLX_WINDOW_BIT			0x00000001
#define GLX_NONE			0x8000
#define GLX_TRUE_COLOR			0x8002
#define GLX_VISUAL_ID			0x800B
#define GLX_DRAWABLE_TYPE		0x8010
#define GLX_RENDER_TYPE			0x8011
#define GLX_X_RENDERABLE		0x8012
#define GLX_FBCONFIG_ID			0x8013
#define GLX_RGBA_TYPE			0x8014
#define GLX_MAX_PBUFFER_WIDTH		0x8016
#define GLX_MAX_PBUFFER_HEIGHT		0x8017
#define GLX_MAX_PBUFFER_PIXELS		0x8018
#define GLX_LARGEST_PBUFFER		0x801C
#define GLX_WIDTH			0x801D
#define GLX_HEIGHT			0x801E
#define GLX_RGBA_BIT			0x00000001

// ---------------------------------------------------

extern void glo_surface_getcontents_readpixels(int formatFlags, int stride,
                                    int bpp, int width, int height, void *data);

// ---------------------------------------------------

int glo_flags_get_depth_bits(int formatFlags) {
    switch ( formatFlags & GLO_FF_DEPTH_MASK ) {
        case GLO_FF_DEPTH_16: return 16;
        case GLO_FF_DEPTH_24: return 24;
        case GLO_FF_DEPTH_32: return 32;
        default: return 0;
    }
}

int glo_flags_get_stencil_bits(int formatFlags) {
    switch ( formatFlags & GLO_FF_STENCIL_MASK ) {
        case GLO_FF_STENCIL_8: return 8;
        default: return 0;
    }
}

void glo_flags_get_rgba_bits(int formatFlags, int *rgba) {
    int alpha = (formatFlags & GLO_FF_ALPHA) != 0;
    switch ( formatFlags & GLO_FF_BITS_MASK ) {
        case GLO_FF_BITS_16:
            rgba[0] = alpha ? 4 : 5;
            rgba[1] = alpha ? 4 : 6;
            rgba[2] = alpha ? 4 : 5;
            rgba[3] = alpha ? 4 : 0;
            break;
        case GLO_FF_BITS_24:
            // ignore alpha
            rgba[0] = 8;
            rgba[1] = 8;
            rgba[2] = 8;
            rgba[3] = 0;
            break;
        case GLO_FF_BITS_32:
            rgba[0] = 8;
            rgba[1] = 8;
            rgba[2] = 8;
            rgba[3] = 8;
            break;
        default:
            rgba[0] = 8;
            rgba[1] = 8;
            rgba[2] = 8;
            rgba[3] = 0;
            break;
      }
}

int glo_flags_get_bytes_per_pixel(int formatFlags) {
    switch ( formatFlags & GLO_FF_BITS_MASK ) {
        case GLO_FF_BITS_16: return 2;
        case GLO_FF_BITS_24: return 3;
        case GLO_FF_BITS_32: return 4;
        default: return 3;
    }
}

void glo_flags_get_readpixel_type(int formatFlags, int *glFormat, int *glType) {
    GLenum gFormat, gType;

    if (formatFlags & GLO_FF_ALPHA) {
        switch ( formatFlags & GLO_FF_BITS_MASK ) {
            case GLO_FF_BITS_16:
                gFormat = GL_RGBA;
                gType = GL_UNSIGNED_SHORT_4_4_4_4;
                break;
            case GLO_FF_BITS_24:
            case GLO_FF_BITS_32:
            default:
                gFormat = GL_BGRA;
                gType = GL_UNSIGNED_BYTE;
                break;
        }
    } else {
        switch ( formatFlags & GLO_FF_BITS_MASK ) {
            case GLO_FF_BITS_16:
                gFormat = GL_RGB;
                gType = GL_UNSIGNED_SHORT_5_6_5;
                break;
            case GLO_FF_BITS_24:
            case GLO_FF_BITS_32:
            default:
                gFormat = GL_BGR;
                gType = GL_UNSIGNED_BYTE;
                break;
        }
    }

    if (glFormat)
        *glFormat = gFormat;
    if (glType)
        *glType = gType;
}

#if 0
// seems wrong below.
int glo_flags_score(int formatFlagsExpected, int formatFlagsReal) {

    if (formatFlagsExpected == formatFlagsReal)
        return 0;

    int score = 1;

    // we wanted alpha, but we didn't get it
    if ((formatFlagsExpected&GLO_FF_ALPHA_MASK) <
        (formatFlagsReal&GLO_FF_ALPHA_MASK))
        score++;
    // less bits than we expected
    if ((formatFlagsExpected&GLO_FF_BITS_MASK) <
        !(formatFlagsReal&GLO_FF_BITS_MASK))
        score++;
    // less depth bits than we expected
    if ((formatFlagsExpected&GLO_FF_DEPTH_MASK) <
        !(formatFlagsReal&GLO_FF_DEPTH_MASK))
        score++;
    // less stencil bits than we expected
    if ((formatFlagsExpected&GLO_FF_STENCIL_MASK) <
        !(formatFlagsReal&GLO_FF_STENCIL_MASK))
        score++;

    return score;
}
#else
int glo_flags_score(int formatFlagsExpected, int formatFlagsReal) {

    if (formatFlagsExpected == formatFlagsReal)
        return 0;

    int score = 1;

    // we wanted alpha, but we didn't get it
    if ((formatFlagsReal & GLO_FF_ALPHA_MASK) <
        (formatFlagsExpected  & GLO_FF_ALPHA_MASK))
        score++;
    // less bits than we expected
    if ((formatFlagsReal & GLO_FF_BITS_MASK) <
        (formatFlagsExpected & GLO_FF_BITS_MASK))
        score++;
    // less depth bits than we expected
    if ((formatFlagsReal & GLO_FF_DEPTH_MASK) <
        (formatFlagsExpected & GLO_FF_DEPTH_MASK))
        score++;
    // less stencil bits than we expected
    if ((formatFlagsReal & GLO_FF_STENCIL_MASK) <
        (formatFlagsExpected & GLO_FF_STENCIL_MASK))
        score++;

    return score;
}
#endif

int glo_flags_get_from_glx(const int *fbConfig, int assumeBooleans) {
    int bufferSize = 0;
    int depthSize = 0;
    int stencilSize = 0;
    int rgbaSize[] = {0,0,0,0};
    int flags = 0;

    while (*fbConfig) {
        int isSingle = 0;
        switch (*fbConfig) {
            case GLX_USE_GL:
                isSingle = 1;
                break;
            case GLX_BUFFER_SIZE:
                bufferSize = fbConfig[1];
                break;
            case GLX_LEVEL:
                break;
            case GLX_RGBA:
                flags |= GLO_FF_ALPHA;
                break;
            case GLX_DOUBLEBUFFER:
                isSingle = 1;
                break;
            case GLX_STEREO:
                isSingle = 1;
                break;
            case GLX_AUX_BUFFERS:
                break;
            case GLX_RED_SIZE:
                rgbaSize[0] = fbConfig[1];
                break;
            case GLX_GREEN_SIZE:
                rgbaSize[1] = fbConfig[1];
                break;
            case GLX_BLUE_SIZE:
                rgbaSize[2] = fbConfig[1];
                break;
            case GLX_ALPHA_SIZE:
                rgbaSize[3] = fbConfig[1];
                break;
            case GLX_DEPTH_SIZE:
                depthSize = fbConfig[1];
                break;
            case GLX_STENCIL_SIZE:
                stencilSize = fbConfig[1];
                break;
            case GLX_ACCUM_RED_SIZE:
            case GLX_ACCUM_GREEN_SIZE:
            case GLX_ACCUM_BLUE_SIZE:
            case GLX_ACCUM_ALPHA_SIZE:
                break;
        }

        // go to next
        if (isSingle && assumeBooleans)
            fbConfig++;
        else
            fbConfig+=2;
    }

    if (rgbaSize[3])
        flags |= GLO_FF_ALPHA;
    // ensure we have room for *some* alpha
    if ((flags & GLO_FF_ALPHA) && (rgbaSize[3]==0))
        rgbaSize[3] = 1;
    // Buffer size flag
    if (bufferSize==0)
        bufferSize = rgbaSize[0]+rgbaSize[1]+rgbaSize[2]+rgbaSize[3];
    if (bufferSize==0)
        bufferSize = (flags & GLO_FF_ALPHA) ? 32 : 24;
    if (bufferSize<=16)
        flags |= GLO_FF_BITS_16;
    else if (bufferSize<=24)
        flags |= GLO_FF_BITS_24;
    else
        flags |= GLO_FF_BITS_32;

    // Depth
    if (depthSize<=16)
        flags |= GLO_FF_DEPTH_16;
    else if (depthSize<=24)
        flags |= GLO_FF_DEPTH_24;
    else
        flags |= GLO_FF_DEPTH_32;
    // Stencil
    if (stencilSize>0)
        flags |= GLO_FF_STENCIL_8;

    return flags;
}

void glo_geometry_get_from_glx(const int* attrib_list, int* width, int* height)
{
    while ( *attrib_list )
    {
        switch (*attrib_list)
        {
            case GLX_WIDTH:
                *width = attrib_list[1];
                break;
            case GLX_HEIGHT:
                *height = attrib_list[1];
                break;
            default:
                fprintf(stderr, "Should not pass any attribs except for width and height for glXCreiatePixmap. \n");
        }
        attrib_list += 2;
    }
}

void glo_surface_getcontents_readpixels(int formatFlags, int stride, int bpp,
                             int width, int height, void *data) {
    int glFormat, glType, rl, pa;
    static int once;

    glo_flags_get_readpixel_type(formatFlags, &glFormat, &glType);
    switch(bpp) {
        case 24:
            if(glFormat != GL_BGR) {
                if(!once) {
                    fprintf(stderr, "Warning: compressing alpha\n");
                    once = 1;
                }
                glFormat = GL_BGR;
            }
            break;
        case 32:
            if(glFormat != GL_BGRA) {
                fprintf(stderr, "Warning: expanding alpha!\n");
                glFormat = GL_BGRA;
            }
            break;
        default:
            fprintf(stderr, "Warning: unsupported colourdepth\n");
            break;
    }

    // Save guest processes GL state before we ReadPixels()
    glGetIntegerv(GL_PACK_ROW_LENGTH, &rl);
    glGetIntegerv(GL_PACK_ALIGNMENT, &pa);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

#ifdef GETCONTENTS_INDIVIDUAL
    GLubyte *b = (GLubyte *)data;
    int irow;
    for(irow = height-1 ; irow >= 0 ; irow--) {
        glReadPixels(0, irow, width, 1, glFormat, glType, b);
        b += stride;
    }
#else
    // Faster buffer flip
    GLubyte *b = (GLubyte *)data;
    GLubyte *c = &((GLubyte *)data)[stride*(height-1)];
    GLubyte *tmp = (GLubyte*)g_malloc(stride);
    int irow;

    glReadPixels(0, 0, width, height, glFormat, glType, data);

    for(irow = 0; irow < height/2; irow++) {
        memcpy(tmp, b, stride);
        memcpy(b, c, stride);
        memcpy(c, tmp, stride);
        b += stride;
        c -= stride;
    }
    g_free(tmp);

#endif

    // Restore GL state
    glPixelStorei(GL_PACK_ROW_LENGTH, rl);
    glPixelStorei(GL_PACK_ALIGNMENT, pa);
}

int glo_get_glx_from_flags(int formatFlags, int glxEnum) {
    int rgba[4];
    glo_flags_get_rgba_bits(formatFlags, rgba);

    switch (glxEnum) {
        case GLX_USE_GL: return 1;
        case GLX_BUFFER_SIZE:
            return glo_flags_get_bytes_per_pixel(formatFlags)*8;
        case GLX_LEVEL: return 0;
        case GLX_RGBA: return formatFlags & GLO_FF_ALPHA;
        case GLX_DOUBLEBUFFER: return 1;
        case GLX_STEREO: return 0;
        case GLX_AUX_BUFFERS: return 0;
        case GLX_RED_SIZE: return rgba[0];
        case GLX_GREEN_SIZE: return rgba[1];
        case GLX_BLUE_SIZE: return rgba[2];
        case GLX_ALPHA_SIZE: return rgba[3];
        case GLX_DEPTH_SIZE: return glo_flags_get_depth_bits(formatFlags);
        case GLX_STENCIL_SIZE: return glo_flags_get_stencil_bits(formatFlags);
        case GLX_ACCUM_RED_SIZE:
        case GLX_ACCUM_GREEN_SIZE:
        case GLX_ACCUM_BLUE_SIZE:
        case GLX_ACCUM_ALPHA_SIZE:
            return 0;
        //the attributes for glXGetFBConfigAttrib
        case GLX_FBCONFIG_ID: return 0;
        case GLX_RENDER_TYPE: return GLX_RGBA_BIT;
        case GLX_DRAWABLE_TYPE: return GLX_WINDOW_BIT;
        case GLX_X_RENDERABLE: return 1;
        case GLX_VISUAL_ID:
        case GLX_X_VISUAL_TYPE:
            // the real value is obtained in client side.
            return 0;
        case GLX_CONFIG_CAVEAT: return GLX_NONE;
        case GLX_TRANSPARENT_TYPE: return GLX_NONE;
        case GLX_TRANSPARENT_INDEX_VALUE:
        case GLX_TRANSPARENT_RED_VALUE:
        case GLX_TRANSPARENT_GREEN_VALUE:
        case GLX_TRANSPARENT_BLUE_VALUE:
        case GLX_TRANSPARENT_ALPHA_VALUE:
            return 0;
        case GLX_MAX_PBUFFER_WIDTH:
        case GLX_MAX_PBUFFER_HEIGHT:
        case GLX_MAX_PBUFFER_PIXELS:
            return 0;
    }
    return 0;
}

