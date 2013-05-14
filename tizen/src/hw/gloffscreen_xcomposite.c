/*
 *  Offscreen OpenGL abstraction layer - GLX specific
 *
 *  Copyright (c) 2010 Intel
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
#if (!defined _WIN32) && (!defined __APPLE__)
#include "gloffscreen.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xcomposite.h>

#ifdef MANGLE_OPENGL_SYMBOLS
#include "gl_mangled.h"
#endif

void *g_malloc(size_t size);
void *g_realloc(void *ptr, size_t size);
void g_free(void *ptr);

struct GloMain {
    Display *dpy;
    int use_ximage;
};
struct GloMain glo;
int glo_inited = 0;

struct _GloContext {
    GLuint                formatFlags;

    GLXFBConfig           fbConfig;
    GLXContext            context;
};

struct _GloSurface {
    GLuint                width;
    GLuint                height;

    GloContext           *context;
    Window                window;

    // For use by the 'fast' copy code.
    Pixmap		pixmap;
    XImage               *image;
    XShmSegmentInfo       shminfo;
	GLXPixmap			glxPixmap;
};

extern void glo_surface_getcontents_readpixels(int formatFlags, int stride,
                                    int bpp, int width, int height, void *data);
static void glo_test_readback_methods(void);

/* ------------------------------------------------------------------------ */

int glo_initialised(void) {
    return glo_inited;
}

/* 
 * The X error was disabled, otherwise QEMU will abort. Printing more error
 * messages inside below function could help debug potential bugs. On the 
 * other hand, since the X errors could be caused by the GL calls forwarded
 * from client OS side, which does not make sense to abort the whole QEMU,
 * so it is reasonable to redirect error handler here.
 *
 */
static int x_errhandler(Display *dpy, XErrorEvent *e)
{
    //fprintf (stderr, "X Error Happened!\n");
    return 0;
}

/* Sanity test of the host GL capabilities to see whether the gl offscreen
 * could be well supported
 */
int glo_sanity_test (void) {
    return 0;
}

/* Initialise gloffscreen */
int glo_init(void) {
    if (glo_inited) {
        printf( "gloffscreen already inited\n" );
        //exit( EXIT_FAILURE );
		return 1;
    }

    /* Open a connection to the X server */
    glo.dpy = XOpenDisplay( NULL );
    if ( glo.dpy == NULL ) {
        printf( "Unable to open a connection to the X server\n" );
        //exit( EXIT_FAILURE );
		return 1;
    }

    glo_inited = 1; // safe because we are single threaded. Otherwise we cause
                    // recursion on the next call.
    // set the X error handler.
    XSetErrorHandler (x_errhandler);
    glo_test_readback_methods();
	return 0;
}

/* Uninitialise gloffscreen */
void glo_kill(void) {
    XCloseDisplay(glo.dpy);
    glo.dpy = NULL;
}


/* Like wglGetProcAddress/glxGetProcAddress */
void *glo_getprocaddress(const char *procName) {
    if (!glo_inited)
        glo_init();

    return glXGetProcAddressARB((const GLubyte *) procName);
}

/* ------------------------------------------------------------------------ */

/* Create a light-weight context just for creating surface */
GloContext *__glo_context_create(int formatFlags) {

    GLXFBConfig          *fbConfigs;
    int                   numReturned;
    GloContext           *context;
    int                   rgbaBits[4];
    int                   bufferAttributes[] = {
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_RED_SIZE,      8,
        GLX_GREEN_SIZE,    8,
        GLX_BLUE_SIZE,     8,
        GLX_ALPHA_SIZE,    8,
        GLX_DEPTH_SIZE,    0,
        GLX_STENCIL_SIZE,  0,
        None
    };

    if (!glo_inited)
        glo_init();

    // set up the surface format from the flags we were given
    glo_flags_get_rgba_bits(formatFlags, rgbaBits);
    bufferAttributes[5]  = rgbaBits[0];
    bufferAttributes[7]  = rgbaBits[1];
    bufferAttributes[9]  = rgbaBits[2];
    bufferAttributes[11] = rgbaBits[3];
    bufferAttributes[13] = glo_flags_get_depth_bits(formatFlags);
    bufferAttributes[15] = glo_flags_get_stencil_bits(formatFlags);

    fbConfigs = glXChooseFBConfig( glo.dpy, DefaultScreen(glo.dpy),
                                   bufferAttributes, &numReturned );
    if (numReturned==0) {
        printf( "No matching configs found.\n" );
        //exit( EXIT_FAILURE );
		return NULL;
    }
    context = (GloContext*)g_malloc(sizeof(GloContext));
    memset(context, 0, sizeof(GloContext));
    context->formatFlags = formatFlags;
    context->fbConfig = fbConfigs[0];

	return context;
}

/* Create an OpenGL context for a certain pixel format.
      formatflags are from the GLO_ constants */

GloContext *glo_context_create(int formatFlags, GloContext *shareLists) {

	GloContext *context = __glo_context_create(formatFlags);

	if (!context) {
		return NULL;
	}

    /* Create a GLX context for OpenGL rendering */
    context->context = glXCreateNewContext(glo.dpy, context->fbConfig,
                                         GLX_RGBA_TYPE,
                                         shareLists ? shareLists->context: NULL,
                                         True );

    if (!context->context) {
        printf( "glXCreateNewContext failed\n" );
        //exit( EXIT_FAILURE );
		return NULL;
    }

    return context;
}

/* Destroy a previously created OpenGL context */
void glo_context_destroy(GloContext *context) {
    if (!context) return;

    // TODO: check for GloSurfaces using this?
    glXDestroyContext( glo.dpy, context->context);
    g_free(context);
}

static void glo_surface_free_xshm_image(GloSurface *surface) {
    XShmDetach(glo.dpy, &surface->shminfo);
    surface->image->data = NULL;
    XDestroyImage(surface->image);
    shmdt(surface->shminfo.shmaddr);
    shmctl(surface->shminfo.shmid, IPC_RMID, NULL);
}

//FIXMEIM - handle failure to allocate.
static void glo_surface_try_alloc_xshm_image(GloSurface *surface) {

    if(surface->image)
        glo_surface_free_xshm_image(surface);

    surface->image =
        XShmCreateImage(glo.dpy, DefaultVisual(glo.dpy, 0), 24, ZPixmap, NULL,
                        &surface->shminfo, surface->width, surface->height);

    surface->shminfo.shmid = shmget(IPC_PRIVATE,
                                    surface->image->bytes_per_line *
                                    surface->height,
                                    IPC_CREAT | 0777);
    surface->shminfo.shmaddr = shmat(surface->shminfo.shmid, NULL, 0);
    surface->image->data = surface->shminfo.shmaddr;
    surface->shminfo.readOnly = False;
    XShmAttach(glo.dpy, &surface->shminfo);
}

/* ------------------------------------------------------------------------ */

/* Update the context in surface and handle previous context */
void glo_surface_update_context(GloSurface *surface, GloContext *context, int free_flags)
 {
    /* If previous context is light-weight context, just free it. If previous
     * context is valid one binded with surface via MakeCurrent, we need unbind
     * from original glstate */

     if ( surface->context )
    {
		if (free_flags) /* light-weight context */
            g_free(surface->context);
    }
    surface->context = context;
}

/* Create a surface with given width and height, formatflags are from the
 * GLO_ constants */
GloSurface *glo_surface_create(int width, int height, GloContext *context) {
    GloSurface           *surface;
    XSetWindowAttributes attr = { 0 };
    unsigned long mask;
    XVisualInfo *vis;

    if (!context)
      return 0;

    surface = (GloSurface*)g_malloc(sizeof(GloSurface));
    memset(surface, 0, sizeof(GloSurface));
    surface->width = width;
    surface->height = height;
    surface->context = context;

    vis = glXGetVisualFromFBConfig(glo.dpy, ((struct _GloContext*)context)->fbConfig);

    attr.background_pixel = 0xff000000;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(glo.dpy, DefaultRootWindow(glo.dpy),
                                    vis->visual, AllocNone);
    attr.event_mask = 0;
    attr.save_under = True;
    attr.override_redirect = True;
    attr.cursor = None;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask |
           CWOverrideRedirect | CWSaveUnder;

    surface->window = XCreateWindow(glo.dpy, DefaultRootWindow(glo.dpy), 0, 3000, width, height, 0, vis->depth, InputOutput, vis->visual, mask, &attr);

    if (!surface->window) {
        printf( "XCreateWindow failed\n" );
        //exit( EXIT_FAILURE );
		return NULL;
    }

    XMapWindow(glo.dpy, surface->window);
    XCompositeRedirectWindow (glo.dpy, surface->window, CompositeRedirectAutomatic);

    if(glo.use_ximage) {
        surface->pixmap = XCompositeNameWindowPixmap(glo.dpy, surface->window);
    } else {
        surface->pixmap = XCreatePixmap( glo.dpy, DefaultRootWindow(glo.dpy),
                                         width, height,
                                         glo_flags_get_bytes_per_pixel(context->formatFlags)*8);
    }

    if(surface->pixmap == 0) {
        fprintf(stderr, "Failed to allocate pixmap!\n");
        //exit(EXIT_FAILURE);
		return NULL;
    }

    /* Create a GLX pixmap to associate the frame buffer configuration with the
     * created X window */
    /*XXX: need attribute_list? */
    surface->glxPixmap = glXCreatePixmap( glo.dpy, context->fbConfig, surface->pixmap, NULL );
    if (!surface->glxPixmap) {
      printf( "glXCreatePixmap failed\n" );
      //exit( EXIT_FAILURE );
     return NULL;
    }

    XSync(glo.dpy, 0);

    /* set hints and properties */
    {
        XSizeHints sizehints;

        sizehints.x = 0;
        sizehints.y = 0;
        sizehints.width = width;
        sizehints.height = height;
        sizehints.flags = USSize | USPosition;
        XSetWMNormalHints(glo.dpy, surface->window, &sizehints);
        XSetStandardProperties(glo.dpy, surface->window, "", "", None,
                               (char **) NULL, 0, &sizehints);
    }

    XSync(glo.dpy, 0);

    // If we're using XImages to pull the data from the graphics card...
    glo_surface_try_alloc_xshm_image(surface);

    return surface;
}

/* Destroy the given surface */
void glo_surface_destroy(GloSurface *surface) {

    if(surface->pixmap)
        XFreePixmap( glo.dpy, surface->pixmap);

    if(surface->glxPixmap)
	glXDestroyPixmap( glo.dpy, surface->glxPixmap);

    XDestroyWindow( glo.dpy, surface->window);
    if(surface->image)
        glo_surface_free_xshm_image(surface);
    g_free(surface);

}

/* Make the given surface current */
int glo_surface_makecurrent(GloSurface *surface) {
    int ret;

    if (!glo_inited)
        glo_init();

    if (surface)
        ret = glXMakeCurrent(glo.dpy, surface->window,
                             surface->context->context);
    else
        ret = glXMakeCurrent(glo.dpy, 0, NULL);

    return ret;
}

void glo_surface_updatecontents(GloSurface *surface) {
    if (!surface)
        return;

    if(glo.use_ximage) {
        glXWaitGL();

        if(surface->image) {
            XShmGetImage (glo.dpy, surface->pixmap, surface->image, 0, 0, AllPlanes);
        }
        else {
            XGetImage(glo.dpy, surface->pixmap, 0, 0, surface->width, surface->height, AllPlanes, ZPixmap);
        }
    }

}

/* Get the contents of the given surface */
void glo_surface_getcontents(GloSurface *surface, int stride, int bpp, void *data) {
    static int once;
    XImage *img;

    if (!surface)
        return;

    if(glo.use_ximage) {
        glXWaitGL();
    
        if(surface->image) {
            XShmGetImage (glo.dpy, surface->pixmap, surface->image, 0, 0, AllPlanes);
            img = surface->image;
        }
        else {
            img = XGetImage(glo.dpy, surface->pixmap, 0, 0, surface->width, surface->height, AllPlanes, ZPixmap);
        }

        if (img) {
            if(bpp != 32 && bpp != 24 && !once) {
                fprintf(stderr, "Warning: unsupported colourdepth\n");
                once = 1;
            }

            if(bpp == img->bits_per_pixel && stride == img->bytes_per_line)
            {
                 memcpy(data, img->data, stride * surface->height);
            }
            else
            {
                int x, y;
                for(y = 0 ; y < surface->height ; y++) {
                    for(x = 0 ; x < surface->width ; x++) {
                        char *src = ((char*)img->data) +
                                    (x*(img->bits_per_pixel/8)) +
                                    (y*img->bytes_per_line);
                        char *dst = ((char*)data) + x*(bpp/8) + (y*stride);
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        if(bpp == 32)
                              dst[3] = 0xff; // if guest is 32 bit and host is 24
                    }
                }
            }
  
            // If we're not using Shm
            if(!surface->image)
                XDestroyImage(img);

            return;  // We're done.
        } 
    // Uh oh... better fallback. Perhaps get glo.use_ximage to 0?
    }

    // Compatible / fallback method.
    glo_surface_getcontents_readpixels(surface->context->formatFlags,
                                       stride, bpp, surface->width,
                                       surface->height, data);
}

/* Return the width and height of the given surface */
void glo_surface_get_size(GloSurface *surface, int *width, int *height) {
    if (width)
        *width = surface->width;
    if (height)
        *height = surface->height;
}

/* Bind the surface as texture */
void glo_surface_as_texture(GloContext *ctxt, GloSurface *surface)
{
#if 0
    void (*ptr_func_glXBindTexImageEXT) (Display *dpy, GLXDrawable draw, int buffer, int *attrib_list);
    ptr_func_glXBindTexImageEXT =
        (void(*)(Display*, GLXDrawable, int, int*))glo_getprocaddress((const char*)"glXBindTexImageEXT");

    /*XXX: When to call the glXReleaseTexImageEXT?*/
    if (!ptr_func_glXBindTexImageEXT)
    {
        fprintf (stderr, "glXBindTexImageEXT not supported! Can't emulate glEGLImageTargetTexture2DOES!\n");
    }

    fprintf(stderr, "surface_as_texture:error=%d.\n", glGetError());
    ptr_func_glXBindTexImageEXT(glo.dpy, surface->glxPixmap, GLX_FRONT_LEFT_EXT, NULL);
    fprintf(stderr, "surface_as_texture:2:error=%d.\n", glGetError());
#else
	int glFormat, glType;
    glo_surface_updatecontents(surface);
    /*XXX: changet the fixed target: GL_TEXTURE_2D*/
	glo_flags_get_readpixel_type(surface->context->formatFlags, &glFormat, &glType);
    fprintf(stderr, "surface_as_texture:teximage:width=%d,height=%d, glFormat=0x%x, glType=0x%x.\n", surface->width, surface->height, glFormat, glType);
    /* glTexImage2D use different RGB order than the contexts in the pixmap surface */
/*    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->width, surface->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->image->data);*/

    if ((glFormat == GL_RGBA || glFormat == GL_BGRA) && glType == GL_UNSIGNED_BYTE) {
        GLubyte *b = (GLubyte *)surface->image->data;
        int stride = surface->width * 4;
        GLubyte *c = &((GLubyte *)surface->image->data)[stride*(surface->height-1)];
        GLubyte *tmp = (GLubyte*)g_malloc(stride);
        int irow;

        for(irow = 0; irow < surface->height / 2; irow++) {
            memcpy(tmp, b, stride);
            memcpy(b, c, stride);
            memcpy(c, tmp, stride);
            b += stride;
            c -= stride;
        }
        g_free(tmp);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->width, surface->height, 0, glFormat, glType, surface->image->data);
#endif
}

void glo_surface_release_texture(GloSurface *surface)
{
}

 /* Abstract glXQueryExtensionString() */
const char *glo_glXQueryExtensionsString(void) {
    return glXQueryExtensionsString(glo.dpy, 0);
}


#define TX (17)
#define TY (16)

static int glo_can_readback(void) {
    GloContext *context;
    GloSurface *surface;

    unsigned char *datain = (unsigned char *)g_malloc(4*TX*TY);
    unsigned char *datain_flip = (unsigned char *)g_malloc(4*TX*TY); // flipped input data (for GL)
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

    int bufferFlags = glo_flags_get_from_glx(bufferAttributes, 0);
    int bpp = glo_flags_get_bytes_per_pixel(bufferFlags);
    int glFormat, glType;

    memset(datain_flip, 0, TX*TY*4);
    memset(datain, 0, TX*TY*4);

    p = datain;
    for (y=0;y<TY;y++) {
        for (x=0;x<TX;x++) {
            p[0] = x;
            p[1] = y;
            if (bpp>2) p[2] = 0;
            if (bpp>3) p[3] = 0xFF;
            p+=bpp;
        }
        memcpy(&datain_flip[((TY-1)-y)*bpp*TX], &datain[y*bpp*TX], bpp*TX);
    }

    context = glo_context_create(bufferFlags, 0);
	if (context == NULL) {
		g_free(datain);
		g_free(datain_flip);
		return 1;
	}
    surface = glo_surface_create(TX, TY, context);
	if (surface == NULL) {
		g_free(datain);
		g_free(datain_flip);
		return 1;
	}

    glo_surface_makecurrent(surface);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,TX, 0,TY, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos2f(0,0);
    glo_flags_get_readpixel_type(bufferFlags, &glFormat, &glType);
    glDrawPixels(TX,TY,glFormat, glType, datain_flip);
    glFlush();

	dataout = (unsigned char *)g_malloc(4*TX*TY);
    memset(dataout, 0, bpp*TX*TY);

    glo_surface_getcontents(surface, TX*4, bpp*8, dataout);

    glo_surface_destroy(surface);
    glo_context_destroy(context);

    if (memcmp(datain, dataout, bpp*TX*TY)==0) {
		g_free(datain);
		g_free(datain_flip);
		g_free(dataout);
        return 1;
	}

	g_free(datain);
	g_free(datain_flip);
	g_free(dataout);

    return 0;
}

static void glo_test_readback_methods(void) {
    glo.use_ximage = 1;
    if(!glo_can_readback())
        glo.use_ximage = 0;

    //fprintf(stderr, "VM GL: Using %s readback\n", glo.use_ximage?"XImage":"glReadPixels");
}

#endif
