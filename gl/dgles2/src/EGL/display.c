/* Copyright (C) 2010-2011  Nokia Corporation All Rights Reserved.
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

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "common.h"
#include "display.h"
#include "degl.h"
#include "surface.h"
#include "context.h"

pthread_once_t degl_init_control = PTHREAD_ONCE_INIT;
DEGLDisplay* degl_display = NULL;

void deglInitDisplayInternal(EGLNativeDisplayType display_id)
{
    degl_display = malloc(sizeof(DEGLDisplay));
    degl_display->initialized = 0;
    degl_display->config_count = 0;

#   if(CONFIG_GLX == 1)
    if(degl_backend == DEGLBackend_glx)
    {
        int display_opened = 0;
        if(display_id == EGL_DEFAULT_DISPLAY)
        {
            if(!(display_id = XOpenDisplay(NULL)))
            {
                Dprintf("Can't open default display!\n");
                free(degl_display);
                degl_display = NULL;
                return;
            }
            display_opened = 1;
        }

        Dprintf("Opened X11 display %p!\n", display_id);

        int error_base = 0, event_base = 0, glx_major = 0, glx_minor = 0;
        Bool glx_ok = hglX.QueryExtension(display_id, &error_base, &event_base);
        if(glx_ok == False)
        {
            Dprintf("glXQueryExtension failed!\n");
        }
        else
        {
            glx_ok = hglX.QueryVersion(display_id, &glx_major, &glx_minor);
            if(glx_ok == False)
            {
                Dprintf("glXQueryVersion failed!\n");
            }
            else
            {
                if(glx_major < 1 || (glx_major == 1 && glx_minor < 1))
                {
                    Dprintf("Insufficient GLX version (%d.%d)!\n",
                        glx_major, glx_minor);
                    glx_ok = False;
                }
                else
                {
                    Dprintf("GLX version %d.%d\n", glx_major, glx_minor);
                }
            }
        }
        if(glx_ok == False)
        {
            if(display_opened)
            {
                XCloseDisplay(display_id);
            }
            free(degl_display);
            degl_display = NULL;
            return;
        }

        degl_display->dpy = display_id;
        degl_display->scr = (Screen*)-1;
    }
    if(degl_frontend == DEGLFrontend_x11)
    {
        degl_display->x11_config = NULL;
    }
#   endif // (CONFIG_GLX == 1)

#   if(CONFIG_COCOA == 1)
    if(degl_frontend == DEGLFrontend_cocoa)
    {
        degl_display->cocoa_config = NULL;
        if(degl_display_id == EGL_DEFAULT_DISPLAY)
        {
            degl_display->id = CGMainDisplayID();
        }
        else
        {
            degl_display->id = (CGDirectDisplayID)(uintptr_t)display_id;
        }
    }
#   endif // (CONFIG_COCOA == 1)
}

EGLAPI_BUILD EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
    /*
     * We always get default display
     */

    assert(display_id == EGL_DEFAULT_DISPLAY);

	extern void deglInit(void);
	pthread_once(&degl_init_control, deglInit);

	return degl_display;
}

#if(CONFIG_COCOA == 1)
static int deglGetNextColorMode(GLint *flags, GLint *bpp, GLint *r, GLint *g,
	GLint *b, GLint *a)
{
	if (((*flags) & kCGLRGB444Bit))       { *flags &= ~kCGLRGB444Bit;       *bpp = 16;  *r = 4;  *g = 4;  *b = 4;  *a = 0;  return 1; }
	if (((*flags) & kCGLARGB4444Bit))     { *flags &= ~kCGLARGB4444Bit;     *bpp = 16;  *r = 4;  *g = 4;  *b = 4;  *a = 4;  return 1; }
	if (((*flags) & kCGLRGB555Bit))       { *flags &= ~kCGLRGB555Bit;       *bpp = 16;  *r = 5;  *g = 5;  *b = 5;  *a = 0;  return 1; }
	if (((*flags) & kCGLARGB1555Bit))     { *flags &= ~kCGLARGB1555Bit;     *bpp = 16;  *r = 5;  *g = 5;  *b = 5;  *a = 1;  return 1; }
	if (((*flags) & kCGLRGB565Bit))       { *flags &= ~kCGLRGB565Bit;       *bpp = 16;  *r = 5;  *g = 6;  *b = 5;  *a = 0;  return 1; }
	if (((*flags) & kCGLRGB888Bit))       { *flags &= ~kCGLRGB888Bit;       *bpp = 32;  *r = 8;  *g = 8;  *b = 8;  *a = 0;  return 1; }
	if (((*flags) & kCGLARGB8888Bit))     { *flags &= ~kCGLARGB8888Bit;     *bpp = 32;  *r = 8;  *g = 8;  *b = 8;  *a = 8;  return 1; }
	if (((*flags) & kCGLRGB101010Bit))    { *flags &= ~kCGLRGB101010Bit;    *bpp = 32;  *r = 10; *g = 10; *b = 10; *a = 0;  return 1; }
	if (((*flags) & kCGLARGB2101010Bit))  { *flags &= ~kCGLARGB2101010Bit;  *bpp = 32;  *r = 10; *g = 10; *b = 10; *a = 2;  return 1; }
	if (((*flags) & kCGLRGB121212Bit))    { *flags &= ~kCGLRGB121212Bit;    *bpp = 48;  *r = 12; *g = 12; *b = 12; *a = 0;  return 1; }
	if (((*flags) & kCGLARGB12121212Bit)) { *flags &= ~kCGLARGB12121212Bit; *bpp = 48;  *r = 12; *g = 12; *b = 12; *a = 12; return 1; }
	if (((*flags) & kCGLRGB161616Bit))    { *flags &= ~kCGLRGB161616Bit;    *bpp = 64;  *r = 16; *g = 16; *b = 16; *a = 0;  return 1; }
	if (((*flags) & kCGLRGBA16161616Bit)) { *flags &= ~kCGLRGBA16161616Bit; *bpp = 64;  *r = 16; *g = 16; *b = 16; *a = 16; return 1; }
	/* skip other formats */
	*flags = 0; return 0;
}

static GLint deglGetNextBitDepth(GLint *flags)
{
	if (((*flags) & kCGL0Bit)) { *flags &= ~kCGL0Bit; return 0; }
	if (((*flags) & kCGL1Bit)) { *flags &= ~kCGL1Bit; return 1; }
	if (((*flags) & kCGL2Bit)) { *flags &= ~kCGL2Bit; return 2; }
	if (((*flags) & kCGL3Bit)) { *flags &= ~kCGL3Bit; return 3; }
	if (((*flags) & kCGL4Bit)) { *flags &= ~kCGL4Bit; return 4; }
	if (((*flags) & kCGL5Bit)) { *flags &= ~kCGL5Bit; return 5; }
	if (((*flags) & kCGL6Bit)) { *flags &= ~kCGL6Bit; return 6; }
	if (((*flags) & kCGL8Bit)) { *flags &= ~kCGL8Bit; return 8; }
	if (((*flags) & kCGL10Bit)) { *flags &= ~kCGL10Bit; return 10; }
	if (((*flags) & kCGL12Bit)) { *flags &= ~kCGL12Bit; return 12; }
	if (((*flags) & kCGL16Bit)) { *flags &= ~kCGL16Bit; return 16; }
	if (((*flags) & kCGL24Bit)) { *flags &= ~kCGL24Bit; return 24; }
	if (((*flags) & kCGL32Bit)) { *flags &= ~kCGL32Bit; return 32; }
	if (((*flags) & kCGL48Bit)) { *flags &= ~kCGL48Bit; return 48; }
	if (((*flags) & kCGL64Bit)) { *flags &= ~kCGL64Bit; return 64; }
	if (((*flags) & kCGL96Bit)) { *flags &= ~kCGL96Bit; return 96; }
	if (((*flags) & kCGL128Bit)) { *flags &= ~kCGL128Bit; return 128; }
	*flags = 0; return 0;
}

#define CALLCGL(f, p, a) \
	do { \
		if ((err = f p) != kCGLNoError) { \
			Dprintf(#f " failed for " #a ": %s\n", CGLErrorString(err)); \
		} \
	} while(0)
#define RINFO(n, v) CALLCGL(CGLDescribeRenderer, (renderer_info, renderer_index, n, &v), v)
#define PFINFO(n, v) CALLCGL(CGLDescribePixelFormat, (c.pf, 0, n, &v), v)

static void deglAddRendererConfigs(DEGLDisplay *display,
	CGLRendererInfoObj renderer_info, int renderer_index)
{
	CGLError err;
	GLint renderer_id = 0, accelerated = 0;
	RINFO(kCGLRPRendererID, renderer_id);
	RINFO(kCGLRPAccelerated, accelerated);
	GLint window = 0, pbuffer = 1;
	GLint colormode_flags = 0, buffermode_flags = 0;
	GLint depth_flags = 0, stencil_flags = 0;
	RINFO(kCGLRPWindow, window);
	RINFO(kCGLRPColorModes, colormode_flags);
	RINFO(kCGLRPBufferModes, buffermode_flags);
	RINFO(kCGLRPDepthModes, depth_flags);
	RINFO(kCGLRPStencilModes, stencil_flags);
	Dprintf("0x%x: generating usable pixel formats:\n", renderer_id);
	DEGLCocoaConfig c;
	c.renderer_id = renderer_id;
	c.config.caveat = accelerated ? EGL_NONE : EGL_SLOW_CONFIG;
	c.config.level = 0;
	c.config.native_renderable = window ? EGL_TRUE : EGL_FALSE;
	c.config.transparency_type = EGL_NONE;
	c.config.transparent_red = 0;
	c.config.transparent_green = 0;
	c.config.transparent_blue = 0;
	while (deglGetNextColorMode(&colormode_flags, &c.config.buffer_bits, &c.config.red_bits,
		&c.config.green_bits, &c.config.blue_bits, &c.config.alpha_bits))
	{
		GLint dp = depth_flags;
		do
		{
			c.config.depth_bits = deglGetNextBitDepth(&dp);
			GLint st = stencil_flags;
			do
			{
				c.config.stencil_bits = deglGetNextBitDepth(&st);
				CGLPixelFormatAttribute attribs[64], i = 0;
				attribs[i++] = kCGLPFAClosestPolicy;
				if (accelerated) attribs[i++] = kCGLPFAAccelerated;
				if (accelerated) attribs[i++] = kCGLPFANoRecovery;
				if (window)      attribs[i++] = kCGLPFAWindow;
				if (pbuffer)     attribs[i++] = kCGLPFAPBuffer;
				if ((buffermode_flags & kCGLDoubleBufferBit)) attribs[i++] = kCGLPFADoubleBuffer;
				attribs[i++] = kCGLPFAColorSize;   attribs[i++] = c.config.red_bits + c.config.green_bits + c.config.blue_bits + c.config.alpha_bits;
				attribs[i++] = kCGLPFAAlphaSize;   attribs[i++] = c.config.alpha_bits;
				attribs[i++] = kCGLPFADepthSize;   attribs[i++] = c.config.depth_bits;
				attribs[i++] = kCGLPFAStencilSize; attribs[i++] = c.config.stencil_bits;
				attribs[i++] = kCGLPFARendererID;  attribs[i++] = renderer_id;
				attribs[i++] = 0;
				GLint npix = 0;
				err = CGLChoosePixelFormat(attribs, &c.pf, &npix);
				if (err == kCGLNoError && npix > 0)
				{
					GLint cs = -1, a = -1, d = -1, s = -1, pb = 0, win = 0;
					PFINFO(kCGLPFAColorSize, cs);
					PFINFO(kCGLPFAAlphaSize, a);
					PFINFO(kCGLPFADepthSize, d);
					PFINFO(kCGLPFAStencilSize, s);
					PFINFO(kCGLPFAPBuffer, pb);
					PFINFO(kCGLPFAWindow, win);
					if (cs == c.config.red_bits + c.config.green_bits + c.config.blue_bits + c.config.alpha_bits &&
						a == c.config.alpha_bits && d == c.config.depth_bits &&
						s == c.config.stencil_bits && (pb | win))
					{
						CGLRetainPixelFormat(c.pf);
						c.config.id = ++display->config_count;
						c.config.surface_type = 0;
						if (pb)  c.config.surface_type |= EGL_PBUFFER_BIT;
						if (win) c.config.surface_type |= EGL_WINDOW_BIT;
						PFINFO(kCGLPFASampleBuffers, c.config.sample_buffers);
						PFINFO(kCGLPFASamples, c.config.samples);
						Dprintf("\t0x%02x: %dbpp (%d-%d-%d-%d) depth %-2d stencil %d %s%s\n",
							c.config.id, c.config.buffer_bits, c.config.red_bits,
							c.config.green_bits, c.config.blue_bits, c.config.alpha_bits,
							c.config.depth_bits, c.config.stencil_bits,
							(c.config.surface_type & EGL_WINDOW_BIT) ? "win " : "",
							(c.config.surface_type & EGL_PBUFFER_BIT) ? "pbuf" : "");
						display->cocoa_config = realloc(display->cocoa_config,
							display->config_count * sizeof(DEGLCocoaConfig));
						display->cocoa_config[display->config_count - 1] = c;
					}
					else
					{
						// we didn't get what we asked for, trash it
						CGLReleasePixelFormat(c.pf);
					}
				}
				else if (err != kCGLNoError)
				{
					Dprintf("CGLChoosePixelFormat failed: %s\n",
						CGLErrorString(err));
				}
			}
			while (st);
		}
		while (dp);
	}
}
#undef RINFO
#undef CALLCGL
#endif // CONFIG_COCOA == 1

#if(CONFIG_GLX == 1)
static void deglAddFBConfig(DEGLDisplay *display, GLXFBConfig fb_config,
	int force_window)
{
	DEGLX11Config c;
	int id = 0, glx_visual_type = 0;
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_FBCONFIG_ID, &id);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_X_VISUAL_TYPE, &glx_visual_type);
 	// accept TrueColor and DirectColor visuals only
	switch(glx_visual_type)
	{
		case GLX_TRUE_COLOR:   c.visual_type = TrueColor; break;
		case GLX_DIRECT_COLOR: c.visual_type = DirectColor; break;
		default: 
			Dprintf("FBconfig 0x%x has visual type 0x%x, skipping\n",
				id, glx_visual_type);
			return;
	}
	XVisualInfo *xvisual = hglX.GetVisualFromFBConfig(display->dpy, fb_config);
	if(xvisual)
	{
		c.visual_id = xvisual->visual->visualid;
		XFree(xvisual);
		xvisual = 0;
	}
	else
	{
		c.visual_id = 0;
	}
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_BUFFER_SIZE, &c.config.buffer_bits);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_RED_SIZE, &c.config.red_bits);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_GREEN_SIZE, &c.config.green_bits);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_BLUE_SIZE, &c.config.blue_bits);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_ALPHA_SIZE, &c.config.alpha_bits);
	int x = 0;
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_CONFIG_CAVEAT, &x);
	switch(x)
	{
		case GLX_NONE: c.config.caveat = EGL_NONE; break;
		case GLX_SLOW_CONFIG: c.config.caveat = EGL_SLOW_CONFIG; break;
		case GLX_NON_CONFORMANT_CONFIG: c.config.caveat = EGL_NON_CONFORMANT_CONFIG; break;
		default: Dprintf("unknown GLX caveat 0x%x, skipping config 0x%x\n", x, c.config.id); return;
	}
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_DEPTH_SIZE, &c.config.depth_bits);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_LEVEL, &c.config.level);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_X_RENDERABLE, &x);
	c.config.native_renderable = (x == True) ? EGL_TRUE : EGL_FALSE;
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_SAMPLE_BUFFERS, &c.config.sample_buffers);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_SAMPLES, &c.config.samples);
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_STENCIL_SIZE, &c.config.stencil_bits);
	c.config.surface_type = 0;
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_DRAWABLE_TYPE, &x);
	if(c.visual_id && (force_window || (x & GLX_WINDOW_BIT))) c.config.surface_type |= EGL_WINDOW_BIT;
	if(x & GLX_PIXMAP_BIT)  c.config.surface_type |= EGL_PIXMAP_BIT;
	if(x & GLX_PBUFFER_BIT) c.config.surface_type |= EGL_PBUFFER_BIT;
	if(!c.config.surface_type)
	{
		return;
	}
	hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_TRANSPARENT_TYPE, &x);
	if(x == GLX_TRANSPARENT_RGB)
	{
		c.config.transparency_type = EGL_TRANSPARENT_RGB;
		hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_TRANSPARENT_RED_VALUE, &c.config.transparent_red);
		hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_TRANSPARENT_GREEN_VALUE, &c.config.transparent_green);
		hglX.GetFBConfigAttrib(display->dpy, fb_config, GLX_TRANSPARENT_BLUE_VALUE, &c.config.transparent_blue);
	}
	else if(x == GLX_NONE)
	{
		c.config.transparency_type = EGL_NONE;
		c.config.transparent_red = 0;
		c.config.transparent_green = 0;
		c.config.transparent_blue = 0;
	}
	else
	{
		// don't accept index transparency
		return;
	}
	// fetch a fresh handle to the GLXFBConfig so we own it
	const int attribs[] = { GLX_FBCONFIG_ID, id, None, None };
	c.fb_config = hglX.ChooseFBConfig(display->dpy, XScreenNumberOfScreen(display->scr), attribs, &x);
	if(!c.fb_config)
	{
		Dprintf("failed to obtain a new handle to config 0x%x, skipping\n", c.config.id);
		return;
	}
	c.config.id = ++display->config_count;
	display->x11_config = realloc(display->x11_config, display->config_count * sizeof(DEGLX11Config));
	display->x11_config[display->config_count - 1] = c;
	Dprintf("0x%02x: %2dbpp (%d-%d-%d-%d) d%-2d s%d %s %s%s%s (fbconfig 0x%x)\n",
		c.config.id, c.config.buffer_bits, c.config.red_bits,
		c.config.green_bits, c.config.blue_bits, c.config.alpha_bits,
		c.config.depth_bits, c.config.stencil_bits,
		(c.config.transparency_type == EGL_TRANSPARENT_RGB) ? "trns" : "opaq",
		(c.config.surface_type & EGL_WINDOW_BIT) ? "win " : "",
		(c.config.surface_type & EGL_PIXMAP_BIT) ? "pix " : "",
		(c.config.surface_type & EGL_PBUFFER_BIT) ? "pbuf" : "",
		id);
}
#endif // CONFIG_GLX == 1

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	DEGLDisplay* display = dpy;
	if(display == NULL)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(!display->initialized)
	{
		display->major = 0;
		display->minor = 0;

#		if(CONFIG_GLX == 1)
		if(degl_backend == DEGLBackend_glx)
		{
			display->scr = XDefaultScreenOfDisplay(display->dpy);

			Dprintf("X11 Screen is %p.\n", display->scr);

			const char *vendor = hglX.QueryServerString(display->dpy, XScreenNumberOfScreen(display->scr), GLX_VENDOR);
			const char* version = hglX.QueryServerString(display->dpy, XScreenNumberOfScreen(display->scr), GLX_VERSION);
			const char *exts = hglX.QueryServerString(display->dpy, XScreenNumberOfScreen(display->scr), GLX_EXTENSIONS);
			const char *cvendor = hglX.GetClientString(display->dpy, GLX_VENDOR);
			const char *cversion = hglX.GetClientString(display->dpy, GLX_VERSION);
			const char *cexts = hglX.GetClientString(display->dpy, GLX_EXTENSIONS);
			if(!vendor || !version || !exts || !cvendor || !cversion || !cexts)
			{
				fprintf(stderr, "ERROR: GLX client/server version queries failed!\n");
				deglSetError(EGL_NOT_INITIALIZED);
				return EGL_FALSE;
			}
			Dprintf("GLX server vendor string: %s\n", vendor);
			Dprintf("GLX server version string: %s\n", version);
			Dprintf("GLX server extensions string: %s\n", exts);
			Dprintf("GLX client vendor string: %s\n", cvendor);
			Dprintf("GLX client version string: %s\n", cversion);
			Dprintf("GLX client extensions string: %s\n", cexts);

			sscanf(version, "%d.%d", &display->major, &display->minor);
			if(display->major < 1 || (display->major == 1 && display->minor < 2))
			{
				Dprintf("ERROR: Server GLX too old (%d.%d < 1.2)!\n", display->major, display->minor);
				deglSetError(EGL_NOT_INITIALIZED);
				return EGL_FALSE;
			}
		}
		if(degl_frontend == DEGLFrontend_x11)
		{
			if(!display->x11_config)
			{
				const int attribs[] =
				{
					GLX_RENDER_TYPE,      GLX_RGBA_BIT,
					GLX_DOUBLEBUFFER,     True,
					GLX_DRAWABLE_TYPE,    GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT,
					GLX_TRANSPARENT_TYPE, GLX_DONT_CARE,
					None, None
				};
				int fbconfig_count = 0, force_window = 0;
				GLXFBConfig *fbconfigs = NULL;
				fbconfigs = hglX.ChooseFBConfig(display->dpy,
					XScreenNumberOfScreen(display->scr), attribs,
					&fbconfig_count);
				if(!fbconfigs || !fbconfig_count)
				{
					if(fbconfigs)
					{
						XFree(fbconfigs);
						fbconfigs = NULL;
					}
					// some implementations don't like GLX_DRAWABLE_TYPE
					// attribute, so try again without it -- according to
					// GLX spec, all possibly returned FBConfigs will in
					// this case support drawing to a window so we can
					// force this assumption
					force_window = 1;
					const int attribs2[] =
					{
						GLX_RENDER_TYPE,      GLX_RGBA_BIT,
						GLX_DOUBLEBUFFER,     True,
						GLX_TRANSPARENT_TYPE, GLX_DONT_CARE,
						None, None
					};
					fbconfigs = hglX.ChooseFBConfig(display->dpy,
						XScreenNumberOfScreen(display->scr), attribs2,
						&fbconfig_count);
				}
				if(!fbconfigs || !fbconfig_count)
				{
					Dprintf("No FB configs found!\n");
					if(fbconfigs)
					{
						XFree(fbconfigs);
					}
					deglSetError(EGL_NOT_INITIALIZED);
					return EGL_FALSE;
				}
				Dprintf("%d suitable FBConfig candidates found, picking:\n", fbconfig_count);
				int i = 0;
				for(; i < fbconfig_count; i++)
				{
					deglAddFBConfig(display, fbconfigs[i], force_window);
				}
				Dprintf("%d FBConfigs selected.\n", display->config_count);
				XFree(fbconfigs);
			}
		}
#		endif // CONFIG_GLX == 1

#		if(CONFIG_COCOA == 1)
		if(degl_backend == DEGLBackend_cocoa)
		{
			CGLGetVersion(&display->major, &display->minor);
			Dprintf("CGL library version %d.%d\n", display->major, display->minor);
		}
		if(degl_frontend == DEGLFrontend_cocoa)
		{
			if(!display->cocoa_config)
			{
				display->config_count = 0;
				CGLRendererInfoObj renderers;
				GLint nrend = 0;
				CGLError err = CGLQueryRendererInfo(CGDisplayIDToOpenGLDisplayMask(display->id), &renderers, &nrend);
				if (err == kCGLNoError)
				{
					Dprintf("available renderers:\n");
					GLint i = 0;
					for(; i < nrend; i++)
					{
						deglAddRendererConfigs(display, renderers, i);
					}
					CGLDestroyRendererInfo(renderers);
				}
				else
				{
					Dprintf("CGLQueryRendererInfo failed: %s\n", CGLErrorString(err));
				}
			}
		}
#		endif // (CONFIG_COCOA == 1)

		display->initialized = 1;
	}

	if(major) *major = 1;
	if(minor) *minor = 4;

	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
	DEGLDisplay* display = dpy;
	if(display == NULL)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(display->initialized)
	{
		display->initialized = 0;

#		if(CONFIG_GLX == 1)
		if(degl_frontend == DEGLFrontend_x11)
		{
			if(display->x11_config)
			{
				int i;
				for(i = 0; i < display->config_count; i++)
				{
					XFree(display->x11_config[i].fb_config);
					display->x11_config[i].fb_config = 0;
				}
				free(display->x11_config);
				display->x11_config = NULL;
			}
		}
		if(degl_backend == DEGLBackend_glx)
		{
			// TODO: figure out how/when we could close the X display since
			// we cannot do it here because the EGL spec requires that
			// eglInitialize can be called on the display again without
			// having to call eglGetDisplay -- for now we're just leaking
		}
#		endif // (CONFIG_GLX == 1)

#		if(CONFIG_COCOA == 1)
		if(degl_frontend == DEGLFrontend_cocoa)
		{
			if(display->cocoa_config)
			{
				int i;
				for(i = 0; i < display->config_count; i++)
				{
					CGLReleasePixelFormat(display->cocoa_config[i].pf);
					display->cocoa_config[i].pf = 0;
				}
				free(display->cocoa_config);
				display->cocoa_config = NULL;
			}
		}
#		endif // CONFIG_COCOA == 1

		display->config_count = 0;
	}

	return EGL_TRUE;
}

EGLAPI_BUILD const char * EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
	DEGLDisplay *display = dpy;
	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
	}
	else if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
	}
	else
	{
		static const char vendor[] = "Nokia Corporation";
		static const char version[] = "1.4 DGLES2-" DEGL_VERSION;
		static const char extensions[] = "";
		static const char client_apis[] = "OpenGL_ES";
		switch(name)
		{
			case EGL_VENDOR:
				return vendor;
			case EGL_VERSION:
				return version;
			case EGL_EXTENSIONS:
				return extensions;
			case EGL_CLIENT_APIS:
				return client_apis;
			default:
				deglSetError(EGL_BAD_PARAMETER);
				break;
		}
	}
	return NULL;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	DEGLDisplay *display = dpy;
	if(display == NULL)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
	}
	else if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
	}
	else
	{
		DEGLContext *context = (DEGLContext *)eglGetCurrentContext();
		if(context == EGL_NO_CONTEXT)
		{
			Dprintf("bad context!\n");
			deglSetError(EGL_BAD_CONTEXT);
		}
		else
		{
			if(eglGetCurrentSurface(EGL_DRAW) == EGL_NO_SURFACE)
			{
				Dprintf("bad surface!\n");
				deglSetError(EGL_BAD_SURFACE);
			}
			else
			{
#				if(CONFIG_COCOA==1)
				if(degl_frontend == DEGLFrontend_cocoa)
				{
					if (CGLSetParameter(context->cocoa.ctx, kCGLCPSwapInterval,
						&interval) != kCGLNoError)
					{
						Dprintf("CGLSetParameter failed for kCGLCPSwapInterval\n");
					}
				}
#				endif // CONFIG_COCOA == 1
				/* ignored */
				return EGL_TRUE;
			}
		}
	}
	return EGL_FALSE;
}
