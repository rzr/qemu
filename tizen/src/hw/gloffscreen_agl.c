/*Offscreen OpenGL abstraction layer - AGL specific 
* 
*  Copyright (c) 2010 Intel Corporation 
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
#ifdef __APPLE__ 
  
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <inttypes.h>
#include <dlfcn.h>
/*hack due to conflicting typedefs in qemu's softfloat.h*/
#define __SECURITYHI__ 1
  
#include <OpenGL/OpenGL.h> 
#include <AGL/agl.h> 
#include "gloffscreen.h" 
 
#ifdef GL_DEBUG 
#define TRACE(fmt, ...) printf("%s@%d: " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__) 
#else 
#define TRACE(...) 
#endif 
 
struct _GloContext 
{ 
     GLuint         formatFlags; 
     AGLPixelFormat pixelFormat; 
     AGLContext     context; 
 }; 
  
struct _GloSurface 
{ 
     GLuint     width; 
     GLuint     height;   
     GloContext *context; 
     AGLPbuffer pbuffer; 
 }; 
  
extern void glo_surface_getcontents_readpixels(int formatFlags, int stride, 
                                                int bpp, int width, int height, 
                                                void *data); 


  
/* Initialise gloffscreen */ 
int glo_init(void) 
{
	int major, minor;
	aglGetVersion(&major, &minor);
	fprintf(stdout, "%s----AGL version %d.%d\n",__FUNCTION__,  major, minor);
	return 0;
 } 
  
/* Uninitialise gloffscreen */ 
void glo_kill(void) 
{ 
 
} 
 
const char *glo_glXQueryExtensionsString(void) 
{ 
     return ""; 
 } 
  
/* Like wglGetProcAddress/glxGetProcAddress */ 
 void *glo_getprocaddress(const char *procName) 
 { 
    void *ret = NULL;

    if (procName) 
    {
         if (!strncmp(procName, "glX", 3))
             ret = (void *)1;
         else 
            ret = dlsym(RTLD_NEXT, procName);
     }

#if 0//standard code for mac to look up procaddress, but seems slow that above.
	NSSymbol symbol;
	char *symbolname;
	symbolname = malloc(strlen(procName) +2);
	strcpy(symbolname +1, procName);
	symbolname[0] = '_';
	symbol = NULL;
	if(NSIsSymbolNameDefined(symbolname))
		symbol = NSLookupAndBindSymbol(symbolname);
	free(symbolname);
	ret = symbol ? NSAddressOfSymbol (symbol) : NULL;
#endif    
    TRACE("'%s' --> %p", procName, ret); 
    return ret; 
} 
 
/* Create an OpenGL context for a certain pixel format. 
* formatflags are from the GLO_ constants 
*/

GloContext *__glo_context_create(int formatFlags)
{
	int rgba[4];
    	glo_flags_get_rgba_bits(formatFlags, rgba);
    	const int attribs[] = {
        AGL_RGBA,
        AGL_MINIMUM_POLICY,
        AGL_PIXEL_SIZE, glo_flags_get_bytes_per_pixel(formatFlags) * 8,
        AGL_RED_SIZE, rgba[0],
        AGL_GREEN_SIZE, rgba[1],
        AGL_BLUE_SIZE, rgba[2],
        AGL_ALPHA_SIZE, rgba[3],
        AGL_DEPTH_SIZE, glo_flags_get_depth_bits(formatFlags),
        AGL_STENCIL_SIZE, glo_flags_get_stencil_bits(formatFlags),
        AGL_ACCELERATED,
        AGL_NO_RECOVERY,
        AGL_NONE
    };

    TRACE("req %dbpp %d-%d-%d-%d depth %d stencil %d",
          glo_flags_get_bytes_per_pixel(formatFlags) * 8,
          rgba[0], rgba[1], rgba[2], rgba[3],
          glo_flags_get_depth_bits(formatFlags),
          glo_flags_get_stencil_bits(formatFlags));

    	AGLPixelFormat pf = aglChoosePixelFormat(NULL, 0, attribs);
    if (pf == NULL)
        fprintf(stderr, "No matching pixelformat found.");
    else 
    {
        GLint bpp = 0, a = 0, d = 0, s = 0;
        aglDescribePixelFormat(pf, AGL_PIXEL_SIZE, &bpp);
        aglDescribePixelFormat(pf, AGL_ALPHA_SIZE, &a);
        aglDescribePixelFormat(pf, AGL_DEPTH_SIZE, &d);
        aglDescribePixelFormat(pf, AGL_STENCIL_SIZE, &s);

        formatFlags &= ~(GLO_FF_ALPHA_MASK | GLO_FF_BITS_MASK | GLO_FF_DEPTH_MASK | GLO_FF_STENCIL_MASK);
        switch (bpp) {
	    case 16: formatFlags |= GLO_FF_BITS_16; break;
            case 24: formatFlags |= GLO_FF_BITS_24; break;
            case 32: formatFlags |= GLO_FF_BITS_32; break;
            default: fprintf(stderr, "got unsupported bpp %d", bpp); break;
     }

         if (a > 0) 
	{
            formatFlags |= GLO_FF_ALPHA;
        }

        switch (d) {
            case 0: break;
            case 16: formatFlags |= GLO_FF_DEPTH_16; break;
            case 24: formatFlags |= GLO_FF_DEPTH_24; break;
            case 32: formatFlags |= GLO_FF_DEPTH_32; break;
            default: fprintf(stderr, "got unsupported depth %d", d); break;
        }

        switch (s) {
            case 0: break;
             case 8: formatFlags |= GLO_FF_STENCIL_8; break;
            default: fprintf(stderr, "got unsupported stencil %d", s); break;
         }
     }
    GloContext *context = (GloContext *)malloc(sizeof(*context));
    memset(context, 0, sizeof(*context));
    context->formatFlags = formatFlags;
    context->pixelFormat = pf;

    return context;

}
 
GloContext *glo_context_create(int formatFlags, GloContext *shareLists) 
{ 
	GloContext *context = __glo_context_create(formatFlags);
	if(!context)
		return NULL;

    	context->context = aglCreateContext(context->pixelFormat, shareLists ? shareLists->context : NULL); 
   	if (context->context == NULL) 
	{ 
        fprintf(stderr, "aglCreateContext failed: %s", aglErrorString(aglGetError())); 
    	} 

    	TRACE("context=%p", context); 
    	return context; 
} 
  
/* Destroy a previously created OpenGL context */ 
void glo_context_destroy(GloContext *context) 
{ 
     TRACE("context=%p", context); 
     if (context) 
     { 
         aglDestroyContext(context->context); 
         aglDestroyPixelFormat(context->pixelFormat); 
         context->context = NULL; 
         context->pixelFormat = NULL; 
         free(context); 
     } 
 }

int glo_surface_update_context(GloSurface *surface, GloContext *context)
{

    int prev_context_valid = 0;

    if ( surface->context )
    {
        prev_context_valid = (surface->context->context != 0);
        if ( !prev_context_valid ) /* light-weight context */
            g_free(surface->context);
    }
    surface->context = context;

    return prev_context_valid;
}

 
  
/* Create a surface with given width and height, formatflags are from the 
  * GLO_ constants */ 
GloSurface *glo_surface_create(int width, int height, GloContext *context) 
{ 
     GloSurface *surface = NULL; 
     if (context) 
     { 
         surface = (GloSurface *)malloc(sizeof(*surface)); 
         memset(surface, 0, sizeof(*surface)); 
         surface->width = width; 
         surface->height = height; 
         surface->context = context; 
 
        TRACE("%dx%d", surface->width, surface->height); 
       if (aglCreatePBuffer(width, height, GL_TEXTURE_2D, GL_RGBA, 0, &surface->pbuffer) == GL_FALSE)
 	           fprintf(stderr, "aglCreatePbuffer failed: %s", aglErrorString(aglGetError())); 

    } 
    TRACE("surface=%p", surface); 
    return surface; 
} 
 
/* Destroy the given surface */ 
void glo_surface_destroy(GloSurface *surface) 
{ 
    TRACE("surface=%p", surface); 
    if (surface) 
    { 
       aglDestroyPBuffer(surface->pbuffer); 
       surface->pbuffer = NULL; 
       free(surface); 
    } 
} 
 
/* Make the given surface current */ 
int glo_surface_makecurrent(GloSurface *surface) 
{ 
    int ret = GL_FALSE; 
    TRACE("surface=%p", surface); 
    if (surface) 
   { 
        if (aglSetPBuffer(surface->context->context, surface->pbuffer, 0, 0, 0) == GL_FALSE)
      	      fprintf(stderr, "aglSetPbuffer failed: %s", aglErrorString(aglGetError())); 
        ret = aglSetCurrentContext(surface->context->context); 
   } else 
   { 
        ret = aglSetCurrentContext(NULL); 
    } 

    if (ret == GL_FALSE)
    	    fprintf(stderr, "aglSetCurrentContext failed: %s",  aglErrorString(aglGetError())); 
    
     TRACE("Return ret=%d\n", ret);
     return ret; 
 } 

void glo_surface_updatecontents(GloSurface *surface)
{
	const GLint swap_interval = 1;

	if(!surface)
		return;
	aglSwapBuffers(surface->context->context);
        aglSetInteger(surface->context->context, AGL_SWAP_INTERVAL, &swap_interval);
}

 
/* Get the contents of the given surface */ 
void glo_surface_getcontents(GloSurface *surface, int stride, int bpp, 
                             void *data) 
{
     const GLint swap_interval = 1; 
     if (surface)
     { 
        aglSwapBuffers(surface->context->context); 	
	aglSetInteger(surface->context->context, AGL_SWAP_INTERVAL, &swap_interval);
        glo_surface_getcontents_readpixels(surface->context->formatFlags, stride, bpp, surface->width, surface->height, data); 
    } 
} 
  
/* Return the width and height of the given surface */ 
void glo_surface_get_size(GloSurface *surface, int *width, int *height) 
{ 
    if (width)
    { 
         *width = surface->width; 
    } 

    if (height)
    { 
        *height = surface->height; 
    } 
} 
 
/* Bind the surface as texture */
void glo_surface_as_texture(GloSurface *surface)
{
	//Not QUit sure about this function;
	int glFormat, glType;
	glo_surface_updatecontents(surface);
	/*XXX: changet the fixed target: GL_TEXTURE_2D*/
	glo_flags_get_readpixel_type(surface->context->formatFlags, &glFormat, &glType);
    fprintf(stderr, "surface_as_texture:teximage:width=%d,height=%d, glFormat=0x%x, glType=0x%x.\n", surface->width, surface->height, glFormat, glType);
    /* glTexImage2D use different RGB order than the contexts in the pixmap surface */
/*    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->width, surface->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->image->data);*/
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->width, surface->height, 0, glFormat, glType, surface->pbuffer);

	
}

void glo_surface_release_texture(GloSurface *surface)
{

}
 
#endif 

