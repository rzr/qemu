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

#include <pthread.h>
#include "common.h"
#include "surface.h"
#include "degl.h"
#include "display.h"
#include "config.h"
#include "context.h"
#define GL_BGRA                           0x80E1

#define TIME_TRANSFERS 0

void deglRefSurface(DEGLSurface* surface)
{
	Dprintf("Make reference counting threadsafe!\n");
	if(surface)
		surface->bound++;
}

void deglUnrefSurface(EGLDisplay dpy, DEGLSurface** surface)
{
	if(*surface && !--(*surface)->bound && (*surface)->destroy)
	{
		eglDestroySurface(dpy, *surface),
		*surface = 0;
	}
}

#if(CONFIG_WGL == 1)
static int deglCreateWGLPbufferSurface(DEGLDisplay *display,
	DEGLConfig *config, DEGLSurface *surface)
{
	int format;
	UINT num_formats;
	HDC savedc = hwgl.GetCurrentDC();
	HGLRC savectx = hwgl.GetCurrentContext();

	pthread_mutex_lock(&hwgl.haxlock);
	if(!hwgl.MakeCurrent(hwgl.pbuffer.haxdc, hwgl.pbuffer.haxctx))
	{
		Dprintf("Couldn't make hax context current, ERROR = %x.\n",
			(unsigned)GetLastError());
		pthread_mutex_unlock(&hwgl.haxlock);
		deglSetError(EGL_BAD_ALLOC);
		return 0;
	}

	const int attribs[] =
	{
		WGL_PIXEL_TYPE_ARB,      WGL_TYPE_RGBA_ARB,
		WGL_DRAW_TO_PBUFFER_ARB, GL_TRUE,
		WGL_COLOR_BITS_ARB,      config->buffer_bits,
		WGL_RED_BITS_ARB,        config->red_bits,
		WGL_GREEN_BITS_ARB,      config->green_bits,
		WGL_BLUE_BITS_ARB,       config->blue_bits,
		WGL_ALPHA_BITS_ARB,      config->alpha_bits,
		WGL_STENCIL_BITS_ARB,    config->stencil_bits,
		WGL_DEPTH_BITS_ARB,      config->depth_bits,

		WGL_SUPPORT_OPENGL_ARB,  GL_TRUE,
		WGL_ACCELERATION_ARB,    WGL_FULL_ACCELERATION_ARB,
		WGL_DOUBLE_BUFFER_ARB,   GL_FALSE,
		0
	};
	if(hwgl.ChoosePixelFormatARB && hwgl.ChoosePixelFormatARB(hwgl.pbuffer.haxdc,
		attribs, NULL, 1, &format, &num_formats) && num_formats != 0)
	{
		surface->wgl.format = format;
	}
	else
	{
		if(hwgl.ChoosePixelFormatARB)
		{
			Dprintf("Couldn't use wglChoosePixelFormatARB, ERROR = %x.\n",
				(unsigned)GetLastError());
		}
		surface->wgl.format = hwgl.pbuffer.haxpf;
	}

	const int pattribs[] = { 0 };
	Dprintf("Creating pixel buffer (%p, %d, %dx%d)...\n", hwgl.pbuffer.haxdc,
		surface->wgl.format, surface->width, surface->height);
	if(!(surface->wgl.pbuffer = hwgl.CreatePbufferARB(hwgl.pbuffer.haxdc,
		surface->wgl.format, surface->width, surface->height, pattribs)))
	{
		Dprintf("Failed to create PBuffer, ERROR = %x!\n",
			(unsigned)GetLastError());
		pthread_mutex_unlock(&hwgl.haxlock);
		deglSetError(EGL_BAD_ALLOC);
		return 0;
	}

	Dprintf("Getting pixel buffer (%p) DC...\n", surface->wgl.pbuffer);
	surface->wgl.dc = hwgl.GetPbufferDCARB(surface->wgl.pbuffer);

	hwgl.MakeCurrent(savedc, savectx);
	pthread_mutex_unlock(&hwgl.haxlock);
	return 1;
}
#endif // CONFIG_WGL == 1

#if(CONFIG_OFFSCREEN == 1)
#if(CONFIG_GLX == 1)
static int deglCreateGLXPbufferSurface(DEGLDisplay *display,
	DEGLConfig *config, DEGLSurface *surface)
{
	const int attribs[] =
	{
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		GLX_BUFFER_SIZE,   config->buffer_bits,
		GLX_RED_SIZE,      config->red_bits,
		GLX_GREEN_SIZE,    config->green_bits,
		GLX_BLUE_SIZE,     config->blue_bits,
		GLX_ALPHA_SIZE,    config->alpha_bits,
		GLX_STENCIL_SIZE,  config->stencil_bits,
		GLX_DEPTH_SIZE,    config->depth_bits,
		None, None
	};
	int num_config = 0;
	if(!(surface->glx.config = hglX.ChooseFBConfig(display->dpy,
		XScreenNumberOfScreen(display->scr), attribs, &num_config))
		|| !num_config)
	{
		Dprintf("No FBConfigs found for requested config.\n");
		if(surface->glx.config)
		{
			XFree(surface->glx.config);
			surface->glx.config = 0;
		}
		deglSetError(EGL_BAD_MATCH);
		return 0;
	}
	const int pattribs[] =
	{
		GLX_PBUFFER_WIDTH, surface->width,
		GLX_PBUFFER_HEIGHT, surface->height,
		GLX_LARGEST_PBUFFER, False,
		GLX_PRESERVED_CONTENTS, True,
		None
	};
	surface->glx.drawable = hglX.CreatePbuffer(display->dpy,
		surface->glx.config[0], pattribs);
	return 1;
}
#endif // CONFIG_GLX == 1

EGLAPI_BUILD EGLSurface EGLAPIENTRY eglCreateOffscreenSurfaceDGLES(EGLDisplay dpy,
	EGLConfig cfg, DEGLDrawable *ddraw)
{
	DEGLDisplay *display = dpy;
	DEGLConfig *config = cfg;
	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_NO_SURFACE;
	}
	if(!config)
	{
		Dprintf("bad config!\n");
		deglSetError(EGL_BAD_CONFIG);
		return EGL_NO_SURFACE;
	}
	if(!ddraw)
	{
		Dprintf("bad drawable!\n");
		deglSetError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}
	if(degl_frontend != DEGLFrontend_offscreen)
	{
		Dprintf("offscreen frontend not active!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}
	if(ddraw->bpp != config->buffer_bits >> 3)
	{
		Dprintf("Surface bpp(%d) != config bpp(%d)!\n",
			ddraw->bpp, config->buffer_bits);
		deglSetError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	DEGLSurface *surface = malloc(sizeof(*surface));
	surface->bound   = 0;
	surface->destroy = 0;
	surface->width   = ddraw->width;
	surface->height  = ddraw->height;
	surface->depth   = ddraw->depth;
	surface->bpp     = ddraw->bpp;
	surface->config  = config;
	surface->type    = DEGLSurface_offscreen;
	surface->ddraw   = ddraw;
	surface->pbo     = 0;

#	if(CONFIG_GLX == 1)
	if(degl_backend == DEGLBackend_glx)
	{
		if(!deglCreateGLXPbufferSurface(display, config, surface))
		{
			free(surface);
			return EGL_NO_SURFACE;
		}
	}
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_WGL == 1)
	if(degl_backend == DEGLBackend_wgl)
	{
		if(!deglCreateWGLPbufferSurface(display, config, surface))
		{
			free(surface);
			return EGL_NO_SURFACE;
		}
	}
#	endif // (CONFIG_WGL == 1)

#	if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		surface->cocoa.pbuffer = NULL;
		CGLError err = CGLCreatePBuffer(surface->width, surface->height,
			GL_TEXTURE_RECTANGLE_EXT, config->alpha_bits ? GL_RGBA : GL_RGB, 0,
			&surface->cocoa.pbuffer);
		if (err != kCGLNoError)
		{
			Dprintf("CGLCreatePBuffer failed: %s\n", CGLErrorString(err));
			free(surface);
			deglSetError(EGL_BAD_ALLOC);
			return EGL_NO_SURFACE;
		}
		CGLRetainPBuffer(surface->cocoa.pbuffer);
	}
#	endif // (CONFIG_COCOA == 1)

	Dprintf("Created offscreen surface [%p] %ux%u@%u(%u), pixels at %p...\n",
		surface, surface->width, surface->height, surface->depth, surface->bpp,
		surface->ddraw->pixels);
	return surface;
}
#endif // (CONFIG_OFFSCREEN == 1)

EGLAPI_BUILD EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy,
	EGLConfig config_, EGLNativeWindowType win, const EGLint *attrib_list)
{
	DEGLDisplay* display = dpy;
	DEGLConfig* config = config_;
	DEGLSurface *surface = NULL;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_NO_SURFACE;
	}
	if(!config)
	{
		Dprintf("bad config!\n");
		deglSetError(EGL_BAD_CONFIG);
		return EGL_NO_SURFACE;
	}
	if(!(config->surface_type & EGL_WINDOW_BIT))
	{
		Dprintf("bad match!\n");
		deglSetError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}
	if(!win)
	{
		Dprintf("bad native window!\n");
		deglSetError(EGL_BAD_NATIVE_WINDOW);
		return EGL_NO_SURFACE;
	}

	surface = malloc(sizeof(*surface));
	if(!surface)
	{
		Dprintf("malloc failed!\n");
		deglSetError(EGL_BAD_ALLOC);
		return EGL_NO_SURFACE;
	}
	surface->bound = 0;
	surface->destroy = 0;
	surface->type = DEGLSurface_window;
	surface->config = config;

#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		Dprintf("offscreen frontend doesn't support window surfaces!\n");
		deglSetError(EGL_BAD_NATIVE_WINDOW);
	}
#	endif // (CONFIG_OFFSCREEN == 1)

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		XWindowAttributes xattr;
		if(!XGetWindowAttributes(display->dpy, (Window)win, &xattr))
		{
			Dprintf("XGetWindowAttributes failed!\n");
			deglSetError(EGL_BAD_NATIVE_WINDOW);
		}
		else
		{
			Dprintf("window %dx%d depth %d\n", xattr.width, xattr.height, xattr.depth);
			Dprintf("using config 0x%x, RGBA %d-%d-%d-%d depth %d stencil %d %s%s%s\n",
				config->id, config->red_bits, config->green_bits, config->blue_bits,
				config->alpha_bits, config->depth_bits, config->stencil_bits,
				(config->surface_type & EGL_WINDOW_BIT) ? "win " : "",
				(config->surface_type & EGL_PIXMAP_BIT) ? "pix " : "",
				(config->surface_type & EGL_PBUFFER_BIT) ? "pbuf" : "");
			DEGLX11Config *x11config = (DEGLX11Config *)config_;
			if(x11config->visual_id != xattr.visual->visualid)
			{
				Dprintf("WARNING: FBconfig visual id (0x%lx) != window visual id (0x%lx)!\n",
					x11config->visual_id, xattr.visual->visualid);
				//FIXME same visual is too strict, check specs
				deglSetError(EGL_BAD_MATCH);
				free(surface); return EGL_NO_SURFACE;
			}
			surface->glx.drawable = hglX.CreateWindow(display->dpy,
				x11config->fb_config[0], (Window)win, NULL);
			surface->glx.x_drawable = (Window)win;
			surface->glx.config = x11config->fb_config;
			surface->width = xattr.width;
			surface->height = xattr.height;
			surface->depth = xattr.depth;
			surface->bpp = (surface->depth >> 3) + ((surface->depth & 7) ? 1 : 0);
			return surface;
		}
	}
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_COCOA == 1)
	if(degl_frontend == DEGLFrontend_cocoa)
	{
		surface->depth = config->buffer_bits;
		surface->bpp = (surface->depth >> 3) + ((surface->depth & 7) ? 1 : 0);
		surface->cocoa.nsview = hcocoa.CreateView(surface, win,
			&surface->width, &surface->height);
		return surface;
	}
#	endif // CONFIG_COCOA == 1

	free(surface);
	return EGL_NO_SURFACE;
}

EGLAPI_BUILD EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy,
	EGLConfig cfg, const EGLint *attrib_list)
{
	DEGLDisplay *display = (DEGLDisplay *)dpy;
	DEGLConfig *config = (DEGLConfig *)cfg;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_NO_SURFACE;
	}
	if(!config)
	{
		Dprintf("bad config!\n");
		deglSetError(EGL_BAD_CONFIG);
		return EGL_NO_SURFACE;
	}
	if(!(config->surface_type & EGL_PBUFFER_BIT))
	{
		Dprintf("bad match!\n");
		deglSetError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	EGLint width = 0, height = 0;
	EGLint format = EGL_NO_TEXTURE, target = EGL_NO_TEXTURE;
	EGLint mipmap = EGL_FALSE, largest = EGL_FALSE;
	for(; attrib_list && *attrib_list != EGL_NONE; attrib_list++)
	{
		switch(*attrib_list)
		{
			case EGL_WIDTH: width = *(++attrib_list); break;
			case EGL_HEIGHT: height = *(++attrib_list); break;
			case EGL_TEXTURE_FORMAT: format = *(++attrib_list); break;
			case EGL_TEXTURE_TARGET: target = *(++attrib_list); break;
			case EGL_MIPMAP_TEXTURE: mipmap = *(++attrib_list); break;
			case EGL_LARGEST_PBUFFER: largest = *(++attrib_list); break;
			default:
				Dprintf("unknown attribute 0x%x\n", *attrib_list);
				deglSetError(EGL_BAD_ATTRIBUTE);
				return EGL_NO_SURFACE;
		}
	}
	if(width < 0 || height < 0)
	{
		Dprintf("invalid size (%dx%d)!\n", width, height);
		deglSetError(EGL_BAD_PARAMETER);
		return EGL_NO_SURFACE;
	}
	if(format != EGL_NO_TEXTURE && format != EGL_TEXTURE_RGB &&
		format != EGL_TEXTURE_RGBA)
	{
		Dprintf("invalid format (0x%x)!\n", format);
		deglSetError(EGL_BAD_PARAMETER);
		return EGL_NO_SURFACE;
	}
	if(target != EGL_NO_TEXTURE && target != EGL_TEXTURE_2D)
	{
		Dprintf("invalid target (0x%x)!\n", target);
		deglSetError(EGL_BAD_PARAMETER);
		return EGL_NO_SURFACE;
	}
	if((format == EGL_NO_TEXTURE && target != EGL_NO_TEXTURE) ||
		(format != EGL_NO_TEXTURE && target == EGL_NO_TEXTURE))
	{
		Dprintf("invalid format and target combination\n");
		deglSetError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	DEGLSurface *surface = malloc(sizeof(*surface));
	if(!surface)
	{
		Dprintf("malloc failed!\n");
		deglSetError(EGL_BAD_ALLOC);
		return EGL_NO_SURFACE;
	}
	surface->bound = 0;
	surface->destroy = 0;
	surface->type = DEGLSurface_pbuffer;
	surface->width = width;
	surface->height = height;
	surface->depth = config->buffer_bits;
	surface->bpp = (surface->depth >> 3) + ((surface->depth & 7) ? 1 : 0);
	surface->config = config;
	Dprintf("pbuffer %dx%d, largest=%s\n", width, height, largest ? "true" : "false");
	Dprintf("using config 0x%x, RGBA %d-%d-%d-%d depth %d stencil %d type %s%s%s\n",
		config->id, config->red_bits, config->green_bits, config->blue_bits,
		config->alpha_bits, config->depth_bits, config->stencil_bits,
		(config->surface_type & EGL_WINDOW_BIT) ? "win " : "",
		(config->surface_type & EGL_PIXMAP_BIT) ? "pix " : "",
		(config->surface_type & EGL_PBUFFER_BIT) ? "pbuf" : "");

#	if(CONFIG_GLX == 1)
	if(degl_backend == DEGLBackend_glx)
	{
		if(degl_frontend == DEGLFrontend_x11)
		{
			const int xattribs[] =
			{
				GLX_PBUFFER_WIDTH, width,
				GLX_PBUFFER_HEIGHT, height,
				GLX_LARGEST_PBUFFER, largest ? True : False,
				GLX_PRESERVED_CONTENTS, True,
				None
			};
			DEGLX11Config *x11config = (DEGLX11Config *)cfg;
			surface->glx.drawable = hglX.CreatePbuffer(display->dpy,
				x11config->fb_config[0], xattribs);
			surface->glx.x_drawable = 0;
			surface->glx.config = x11config->fb_config;
			return surface;
		}
#		if(CONFIG_OFFSCREEN == 1)
		if(degl_frontend == DEGLFrontend_offscreen)
		{
			if(deglCreateGLXPbufferSurface(display, config, surface))
			{
				return surface;
			}
		}
#		endif // CONFIG_OFFSCREEN == 1
	}
#	endif // CONFIG_GLX == 1

#	if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		CGLError err = CGLCreatePBuffer(surface->width, surface->height,
			GL_TEXTURE_RECTANGLE_EXT,
			(format == EGL_TEXTURE_RGBA) ? GL_RGBA : GL_RGB,
			(mipmap == EGL_TRUE) ? 1 : 0, &surface->cocoa.pbuffer);
		if (err == kCGLNoError)
		{
			CGLRetainPBuffer(surface->cocoa.pbuffer);
			return surface;
		}
	}
#	endif // CONFIG_COCOA == 1

#	if(CONFIG_WGL == 1)
	if(degl_backend == DEGLBackend_wgl)
	{
		if(deglCreateWGLPbufferSurface(display, config, surface))
		{
			return surface;
		}
	}
#	endif // CONFIG_WGL == 1

#	if(CONFIG_OSMESA == 1)
	if(degl_backend == DEGLBackend_osmesa)
	{
		Dprintf("osmesa backend does not support pbuffer surfaces!\n");
		deglSetError(EGL_BAD_ALLOC);
	}
#	endif // CONFIG_OSMESA == 1

	free(surface);
	return EGL_NO_SURFACE;
}

EGLAPI_BUILD EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(
	EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
	EGLConfig config, const EGLint *attrib_list)
{
	/* this is for OpenVG images only, we don't support it */
	Dprintf("OpenVG images are not supported!\n");
	deglSetError(EGL_BAD_PARAMETER);
	return EGL_NO_SURFACE;
}

#if(CONFIG_GLX == 1)
static const int egl_glx_attrib_translation[] =
{
	EGL_TEXTURE_FORMAT, GLX_TEXTURE_FORMAT_EXT,
	EGL_NO_TEXTURE, GLX_TEXTURE_FORMAT_NONE_EXT,
	EGL_TEXTURE_RGB, GLX_TEXTURE_FORMAT_RGB_EXT,
	EGL_TEXTURE_RGBA, GLX_TEXTURE_FORMAT_RGBA_EXT,

	EGL_TEXTURE_TARGET, GLX_TEXTURE_TARGET_EXT,
	EGL_TEXTURE_2D, GLX_TEXTURE_2D_EXT,

	EGL_MIPMAP_TEXTURE, GLX_MIPMAP_TEXTURE_EXT,
	EGL_FALSE, 0,
	EGL_TRUE, 1
};

const int *glx_attrib_for_egl_attrib(EGLint egl_attrib)
{
	const int translation_count = sizeof(egl_glx_attrib_translation) / sizeof(egl_glx_attrib_translation[0]);
	for(int i = 0; i < translation_count; i += 2)
	{
		if(egl_glx_attrib_translation[i] == egl_attrib)
		{
			return &egl_glx_attrib_translation[i+1];
		}
	}
	return 0;
}
#endif

EGLAPI_BUILD EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy,
	EGLConfig config_, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	DEGLDisplay* display = dpy;
	DEGLConfig* config = config_;
	DEGLSurface *surface = NULL;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_NO_SURFACE;
	}
	if(!config)
	{
		Dprintf("bad config!\n");
		deglSetError(EGL_BAD_CONFIG);
		return EGL_NO_SURFACE;
	}
	if(!(config->surface_type & EGL_PIXMAP_BIT))
	{
		Dprintf("bad match!\n");
		deglSetError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	if(!pixmap)
	{
		Dprintf("bad native pixmap!\n");
		deglSetError(EGL_BAD_NATIVE_PIXMAP);
		return EGL_NO_SURFACE;
	}

	surface = malloc(sizeof(*surface));
	if(!surface)
	{
		Dprintf("malloc failed!\n");
		deglSetError(EGL_BAD_ALLOC);
		return EGL_NO_SURFACE;
	}
	surface->bound = 0;
	surface->destroy = 0;
	surface->type = DEGLSurface_pixmap;
	surface->config = config;

#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		Dprintf("offscreen frontend doesn't support pixmap surfaces!\n");
		deglSetError(EGL_BAD_NATIVE_PIXMAP);
	}
#	endif // (CONFIG_OFFSCREEN == 1)

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		Window root = 0;
		int x = 0, y = 0;
		unsigned w = 0, h = 0, bw = 0, d = 0;
		if (!XGetGeometry(display->dpy, (Pixmap)pixmap, &root, &x, &y, &w, &h, &bw, &d))
		{
			Dprintf("XGetGeometry failed!\n");
			deglSetError(EGL_BAD_NATIVE_PIXMAP);
		}
		else
		{
			int attribs[7];
			int attrib_count = 0;
			const int max_attrib_count = sizeof(attribs) / sizeof(attribs[0]);
			while (attrib_list && attrib_list[0] && attrib_list[1])
			{
				const int *glx_attrib_name = glx_attrib_for_egl_attrib(attrib_list[0]);
				const int *glx_attrib_value = glx_attrib_for_egl_attrib(attrib_list[1]);
				if (glx_attrib_name && glx_attrib_value)
				{
					if (attrib_count + 2 >= max_attrib_count)
					{
						Dprintf("unexpected GLX attributes in eglCreatePixmapSurface!\n");
					}
					else
					{
						attribs[attrib_count++] = *glx_attrib_name;
						attribs[attrib_count++] = *glx_attrib_value;
					}
				}
				attrib_list += 2;
			}
			attribs[attrib_count++] = 0;
			Dprintf("pixmap %dx%d depth %d\n", w, h, d);
			Dprintf("using config 0x%x, RGBA %d-%d-%d-%d depth %d stencil %d type %s,%s,%s\n",
				config->id, config->red_bits, config->green_bits, config->blue_bits,
				config->alpha_bits, config->depth_bits, config->stencil_bits,
				(config->surface_type & EGL_WINDOW_BIT) ? "win" : "-",
				(config->surface_type & EGL_PIXMAP_BIT) ? "pix" : "-",
				(config->surface_type & EGL_PBUFFER_BIT) ? "pbuf" : "-");
			DEGLX11Config *x11config = (DEGLX11Config *)config_;
			surface->glx.drawable = hglX.CreatePixmap(display->dpy,
				x11config->fb_config[0], (Pixmap)pixmap, attribs);
			surface->glx.x_drawable = (Pixmap)pixmap;
			surface->glx.config = x11config->fb_config;
			surface->width = w;
			surface->height = h;
			surface->depth = d;
			surface->bpp = (surface->depth >> 3) + ((surface->depth & 7) ? 1 : 0);
			return surface;
		}
	}
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_COCOA == 1)
	if(degl_frontend == DEGLFrontend_cocoa)
	{
		Dprintf("pixmaps are not supported for cocoa!\n");
		deglSetError(EGL_BAD_NATIVE_PIXMAP);
	}
#	endif // CONFIG_COCOA == 1

	free(surface);
	return EGL_NO_SURFACE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy,
	EGLSurface surface_)
{
	DEGLDisplay* display = dpy;
	DEGLSurface* surface = surface_;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}
	if(!surface)
	{
		Dprintf("bad surface!\n");
		deglSetError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}
	if(surface->bound)
	{
		surface->destroy = 1;
		return EGL_TRUE;
	}

#	if(CONFIG_GLX == 1)
	if(degl_backend == DEGLBackend_glx)
	{
		if(surface->glx.drawable)
		{
			switch(surface->type)
			{
				case DEGLSurface_window:
					//don't call GLXDestroyWindow because if an XCloseDisplay call
					//precedes it, it will crash.
					break;
				case DEGLSurface_pixmap:
					hglX.DestroyPixmap(display->dpy, surface->glx.drawable);
					break;
				case DEGLSurface_pbuffer:
#				if(CONFIG_OFFSCREEN == 1)
				case DEGLSurface_offscreen:
					if(degl_frontend == DEGLFrontend_offscreen)
					{
						XFree(surface->glx.config);
					}
#				endif // CONFIG_OFFSCREEN == 1
					hglX.DestroyPbuffer(display->dpy, surface->glx.drawable);
					break;
				default:
					Dprintf("unknown surface type (%d)!\n", surface->type);
					break;
			}
			surface->glx.drawable = 0;
			surface->glx.config = 0;
		}
	}
#	endif // CONFIG_GLX == 1

#	if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		switch(surface->type)
		{
			case DEGLSurface_window:
				if(surface->cocoa.nsview != NULL)
				{
					hcocoa.DestroyView(surface->cocoa.nsview);
					surface->cocoa.nsview = NULL;
				}
				break;
			case DEGLSurface_pbuffer:
#			if(CONFIG_OFFSCREEN == 1)
			case DEGLSurface_offscreen:
#			endif // CONFIG_OFFSCREEN == 1
				if(surface->cocoa.pbuffer != NULL)
				{
					CGLReleasePBuffer(surface->cocoa.pbuffer);
					surface->cocoa.pbuffer = NULL;
				}
				break;
			default:
				Dprintf("unknown surface type (%d)\n", surface->type);
				break;
		}
	}
#	endif // (CONFIG_COCOA == 1)

#	if(CONFIG_WGL == 1)
	if(degl_backend == DEGLBackend_wgl)
	{
		HDC savedc = hwgl.GetCurrentDC();
		HGLRC savectx = hwgl.GetCurrentContext();

		if(savedc == surface->wgl.dc) savedc = 0;

		pthread_mutex_lock(&hwgl.haxlock);
		hwgl.MakeCurrent(hwgl.pbuffer.haxdc, hwgl.pbuffer.haxctx);

		switch(surface->type)
		{
			case DEGLSurface_pbuffer:
#			if(CONFIG_OFFSCREEN == 1)
			case DEGLSurface_offscreen:
#			endif // CONFIG_OFFSCREEN == 1
				hwgl.ReleasePbufferDCARB(surface->wgl.pbuffer, surface->wgl.dc);
				hwgl.DestroyPbufferARB(surface->wgl.pbuffer);
				break;
			default:
				break;
		}
		hwgl.MakeCurrent(savedc, savectx);
		pthread_mutex_unlock(&hwgl.haxlock);
	}
#	endif // (CONFIG_WGL == 1)

#	if(CONFIG_OSMESA == 1)
	if(degl_backend == DEGLBackend_osmesa)
	{
		// nothing to do
	}
#	endif // CONFIG_OSMESA == 1

#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		if(surface->pbo)
		{
			deglRemoveReservedBuffer(surface->pbo);
			DEGLContext *ctx = eglGetCurrentContext();
			if(ctx && ctx->glesfuncs && ctx->glesfuncs->dglDeletePBO)
			{
				ctx->glesfuncs->dglDeletePBO(surface->pbo);
			}
			surface->pbo = 0;
		}
	}
#	endif // CONFIG_OFFSCREEN == 1

	free(surface);
	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy,
	EGLSurface surface_, EGLint attribute, EGLint *value)
{
	DEGLDisplay* display = dpy;
	DEGLSurface* surface = surface_;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}
	if(!surface)
	{
		Dprintf("bad surface!\n");
		deglSetError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	switch(attribute)
	{
		case EGL_CONFIG_ID:
			if(value)
				*value = surface->config->id;
			return EGL_TRUE;
		case EGL_WIDTH:
			if(value)
				*value = surface->width;
			return EGL_TRUE;
		case EGL_HEIGHT:
			if(value)
				*value = surface->height;
			return EGL_TRUE;
		case EGL_HORIZONTAL_RESOLUTION:
		case EGL_VERTICAL_RESOLUTION:
			if(value)
				*value = EGL_UNKNOWN;
			return EGL_TRUE;
		case EGL_PIXEL_ASPECT_RATIO:
			if(value)
				*value = EGL_DISPLAY_SCALING;
			return EGL_TRUE;
		case EGL_RENDER_BUFFER:
			if(value)
			{
#				if(CONFIG_OFFSCREEN == 1)
				if(degl_frontend == DEGLFrontend_offscreen)
					return EGL_BACK_BUFFER;
#				endif // CONFIG_OFFSCREEN == 1
				if(surface->type == DEGLSurface_pbuffer)
					*value = EGL_BACK_BUFFER;
				else
					*value = EGL_SINGLE_BUFFER;
			}
			return EGL_TRUE;
		case EGL_MULTISAMPLE_RESOLVE:
			if(value)
				*value = EGL_MULTISAMPLE_RESOLVE_DEFAULT;
			return EGL_TRUE;
		case EGL_SWAP_BEHAVIOR:
			if(value)
				*value = EGL_BUFFER_PRESERVED;
			return EGL_TRUE;
		case EGL_LARGEST_PBUFFER:
		case EGL_TEXTURE_FORMAT:
		case EGL_TEXTURE_TARGET:
		case EGL_MIPMAP_TEXTURE:
		case EGL_MIPMAP_LEVEL:
			if(surface->type == DEGLSurface_pbuffer)
			{
				Dprintf("TODO: attribute 0x%x!\n", attribute);
				*value = 0;
			}
			return EGL_TRUE;
		default:
			Dprintf("unknown attribute 0x%x!\n", attribute);
			deglSetError(EGL_BAD_ATTRIBUTE);
			break;
	}
	return EGL_FALSE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
			    EGLint attribute, EGLint value)
{
	DUMMY();
}

EGLAPI_BUILD EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
	DEGLContext *context = (DEGLContext *)eglGetCurrentContext();
	if(context)
	{
		switch(readdraw)
		{
			case EGL_READ:
				return context->read;
			case EGL_DRAW:
				return context->draw;
			default:
				break;
		}
		Dprintf("unknown surface type 0x%x!\n", readdraw);
		deglSetError(EGL_BAD_PARAMETER);
	}
	return EGL_NO_SURFACE;
}

#if(CONFIG_OFFSCREEN == 1)
static EGLBoolean deglResizeOffscreenSurface(DEGLDisplay *display, DEGLContext *context,
	DEGLSurface *surface)
{
	surface->width = surface->ddraw->width;
	surface->height = surface->ddraw->height;
	surface->depth = surface->ddraw->depth;
	surface->bpp = surface->ddraw->bpp;

#	if(CONFIG_OSMESA == 1)
	if(degl_backend == DEGLBackend_osmesa)
	{
		eglMakeCurrent(display, surface, surface, context);
	}
#	endif // (CONFIG_OSMESA == 1)

#	if(CONFIG_GLX == 1)
	if(degl_backend == DEGLBackend_glx)
	{
		hglX.DestroyPbuffer(display->dpy, surface->glx.drawable);
		const int attribs[] =
		{
			GLX_PBUFFER_WIDTH, surface->width,
			GLX_PBUFFER_HEIGHT, surface->height,
			GLX_LARGEST_PBUFFER, False,
			GLX_PRESERVED_CONTENTS, True,
			None
		};
		surface->glx.drawable = hglX.CreatePbuffer(display->dpy,
			surface->glx.config[0], attribs);

		eglMakeCurrent(display, surface, surface, context);
	}
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_WGL == 1)
	if(degl_backend == DEGLBackend_wgl)
	{
		HDC savedc = hwgl.GetCurrentDC();
		HGLRC savectx = hwgl.GetCurrentContext();
		int was_current = savedc == surface->wgl.dc;

		pthread_mutex_lock(&hwgl.haxlock);
		hwgl.MakeCurrent(hwgl.pbuffer.haxdc, hwgl.pbuffer.haxctx);

		hwgl.ReleasePbufferDCARB(surface->wgl.pbuffer, surface->wgl.dc);
		hwgl.DestroyPbufferARB(surface->wgl.pbuffer);

		const int pattribs[] = { 0 };

		if(!(surface->wgl.pbuffer = hwgl.CreatePbufferARB(hwgl.pbuffer.haxdc,
			surface->wgl.format, surface->width, surface->height,
			pattribs)))
		{
			Dprintf("Failed to recreate PBuffer, ERROR = %x!\n",
				(unsigned)GetLastError());
			return EGL_FALSE;
		}

		Dprintf("Recreated pbuffer %p (%dx%d).\n", surface->wgl.pbuffer,
			surface->width, surface->height);

		surface->wgl.dc = hwgl.GetPbufferDCARB(surface->wgl.pbuffer);

		hwgl.MakeCurrent(was_current ? surface->wgl.dc : savedc, savectx);
		pthread_mutex_unlock(&hwgl.haxlock);
	}
#	endif // (CONFIG_WGL == 1)

#	if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		if(surface->cocoa.pbuffer != NULL)
		{
			CGLReleasePBuffer(surface->cocoa.pbuffer);
			surface->cocoa.pbuffer = NULL;
		}
		CGLError err = CGLCreatePBuffer(surface->width, surface->height,
			GL_TEXTURE_RECTANGLE_EXT,
			surface->config->alpha_bits ? GL_RGBA : GL_RGB, 0,
			&surface->cocoa.pbuffer);
		if(err != kCGLNoError)
		{
			Dprintf("Unable to recreate pbuffer: %s\n", CGLErrorString(err));
		}
		eglMakeCurrent(display, surface, surface, context);
	}
#	endif // (CONFIG_COCOA == 1)

	return EGL_TRUE;
}
#endif // CONFIG_OFFSCREEN == 1

#if(TIME_TRANSFERS == 1)
#	include <sys/time.h>
static struct timeval deglGetTime()
{
	struct timeval now;
	gettimeofday(&now, 0);
	return now;
}

static long long deglGetTimeDiff(struct timeval start)
{
	struct timeval now;
	gettimeofday(&now, 0);

	long long nowusec = now.tv_sec * 1000000LL + now.tv_usec;
	long long startusec = start.tv_sec * 1000000LL + start.tv_usec;
	return nowusec - startusec;
}
#endif // TIME_TRANSFERS == 1

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface_)
{
	DEGLDisplay* display = dpy;
	DEGLSurface* surface = surface_;
	DEGLContext* context = (DEGLContext*)eglGetCurrentContext();

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}
	if(!context)
	{
		Dprintf("bad context!\n");
		deglSetError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}
	if(!surface)
	{
		Dprintf("bad surface!\n");
		deglSetError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	(*context->glesfuncs->glFlush)();
#	if(TIME_TRANSFERS == 1)
	static struct timeval prev_time;
	float total_diff = deglGetTimeDiff(prev_time)/1000000.f;
	prev_time = deglGetTime();
#	endif // TIME_TRANSFERS == 1

#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		GLuint flags = 1;
		if(surface->width != (unsigned)surface->ddraw->width ||
			surface->height != (unsigned)surface->ddraw->height ||
			surface->depth != (unsigned)surface->ddraw->depth ||
			surface->bpp != (unsigned)surface->ddraw->bpp)
		{
			Dprintf("OFFSCREEN SURFACE RESIZE!\n");
			if(deglResizeOffscreenSurface(display, context, surface) == EGL_FALSE)
			{
				return EGL_FALSE;
			}
			flags |= 2;
		}
#		if(CONFIG_COCOA == 1)
		if(degl_backend == DEGLBackend_cocoa && !context->cocoa.pbuffer_mode)
		{
			CGLError err = CGLFlushDrawable(context->cocoa.ctx);
			if(err != kCGLNoError)
			{
				Dprintf("CGLFlushDrawable failed: %s\n", CGLErrorString(err));
			}
		}
		else
#		endif // (CONFIG_COCOA == 1)
#		if(CONFIG_OSMESA == 1)
		if (degl_backend != DEGLBackend_osmesa)
#		endif // (CONFIG_OSMESA == 1)
		{
			(*context->glesfuncs->dglReadPixels)(surface->ddraw->pixels,
				&surface->pbo, 0, 0, surface->width, surface->height,
				(surface->bpp == 4) ? GL_BGRA : GL_RGB, GL_UNSIGNED_BYTE,
				flags);
		}
	}
#	endif // (CONFIG_OFFSCREEN == 1)

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		if(surface->glx.x_drawable)
		{
			Window root = 0;
			int x = 0, y = 0;
			unsigned w = 0, h = 0, bw = 0, d = 0;
			if (XGetGeometry(display->dpy, surface->glx.x_drawable, &root,
				&x, &y,	&w, &h, &bw, &d))
			{
				if(surface->width != w || surface->height != h ||
					surface->depth != d)
				{
					Dprintf("surface changed from %dx%dx%d to %dx%dx%d\n",
						surface->width, surface->height, surface->depth, w, h, d);
					surface->width = w;
					surface->height = h;
					surface->depth = d;
					surface->bpp = (surface->depth >> 3) + ((surface->depth & 7) ? 1 : 0);
				}
			}
		}
		hglX.SwapBuffers(display->dpy, surface->glx.drawable);
	}
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_COCOA == 1)
	if(degl_frontend == DEGLFrontend_cocoa)
	{
		CGLError err = CGLFlushDrawable(context->cocoa.ctx);
		if(err != kCGLNoError)
		{
			Dprintf("CGLFlushDrawable failed: %s\n", CGLErrorString(err));
		}
	}
#	endif // CONFIG_COCOA == 1

#	if(TIME_TRANSFERS == 1)
	float transfer_diff = deglGetTimeDiff(prev_time)/1000000.f;
	fprintf(stderr, "transfer: %f, total: %f, percent: %.2f%%\n",
		transfer_diff, total_diff, 100.f*(transfer_diff/total_diff));
#	endif // TIME_TRANSFERS == 1
	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy,
	EGLSurface surface_, EGLNativePixmapType target)
{
	DEGLDisplay *display = (DEGLDisplay *)dpy;
	DEGLSurface *surface = (DEGLSurface *)surface_;
	DEGLContext *context = eglGetCurrentContext();

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}

	if(!surface)
	{
		Dprintf("bad surface!\n");
		deglSetError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	if(surface != eglGetCurrentSurface(EGL_DRAW))
	{
		Dprintf("surface is not currently active!\n");
		deglSetError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	if(!target)
	{
		Dprintf("bad native pixmap!\n");
		deglSetError(EGL_BAD_NATIVE_PIXMAP);
		return EGL_FALSE;
	}

	(*context->glesfuncs->glFlush)();

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		Window root = 0;
		int x = 0, y = 0;
		unsigned w = 0, h = 0, bw = 0, d = 0;
		if(!XGetGeometry(display->dpy, (Pixmap)target, &root, &x, &y, &w, &h, &bw, &d))
		{
			Dprintf("XGetGeometry failed!\n");
			deglSetError(EGL_BAD_NATIVE_PIXMAP);
			return EGL_FALSE;
		}
		if(surface->width != w || surface->height != h || surface->depth != d)
		{
			Dprintf("surface dimensions (%dx%dx%d) != pixmap dimensions (%dx%dx%d)\n",
				surface->width, surface->height, surface->depth, w, h, d);
			deglSetError(EGL_BAD_MATCH);
			return EGL_FALSE;
		}
		if(surface->type == DEGLSurface_window || surface->type == DEGLSurface_pixmap)
		{
			XGCValues gcvalues;
			gcvalues.function = GXcopy;
			gcvalues.plane_mask = AllPlanes;
			GC gc = XCreateGC(display->dpy, (Pixmap)target, GCFunction | GCPlaneMask, &gcvalues);
			XCopyArea(display->dpy, surface->glx.x_drawable, (Pixmap)target, gc, 0, 0, w, h, 0, 0);
			XFreeGC(display->dpy, gc);
			return EGL_TRUE;
		}
		if(surface->type == DEGLSurface_pbuffer)
		{
			Dprintf("not implemented for pbuffers yet!\n");
			deglSetError(EGL_BAD_SURFACE);
			return EGL_FALSE;
		}
	}
#	endif // CONFIG_GLX == 1

#	if(CONFIG_COCOA == 1)
	if(degl_frontend == DEGLFrontend_cocoa)
	{
		Dprintf("pixmaps are not supported with cocoa!\n");
		deglSetError(EGL_BAD_NATIVE_PIXMAP);
		return EGL_FALSE;
	}
#	endif // CONFIG_COCOA == 1

	DUMMY();
	return EGL_FALSE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglBindTexImage(EGLDisplay dpy,
	EGLSurface surface_, EGLint buffer)
{
	DEGLDisplay* display = (DEGLDisplay *)dpy;
	DEGLSurface *surface = (DEGLSurface *)surface_;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}
	if(!surface)
	{
		Dprintf("bad surface!\n");
		deglSetError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	DEGLContext* context = (DEGLContext*)eglGetCurrentContext();
	if(!context)
	{
		Dprintf("bad context!\n");
		deglSetError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}

#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen &&
		surface->type == DEGLSurface_offscreen)
	{
		Dprintf("Should use Pixel Buffer Objects?\n");
		(*context->glesfuncs->dglBindTexImage)(surface->ddraw->width,
			surface->ddraw->height, surface->ddraw->pixels);
		return EGL_TRUE;
	}
#	endif // (CONFIG_OFFSCREEN == 1)

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11 && surface->type == DEGLSurface_pixmap && hglX.BindTexImageEXT)
	{
		hglX.BindTexImageEXT(display->dpy, surface->glx.drawable, GLX_FRONT_EXT, 0);
		return EGL_TRUE;
	}
#	endif // CONFIG_GLX == 1

#	if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		switch(surface->type)
		{
			case DEGLSurface_pbuffer:
				{
					CGLError err = CGLTexImagePBuffer(context->cocoa.ctx,
						surface->cocoa.pbuffer, GL_FRONT);
					if(err != kCGLNoError)
					{
						Dprintf("CGLTexImagePBuffer failed: %s\n",
							CGLErrorString(err));
						return EGL_FALSE;
					}
				}
				return EGL_TRUE;
			case DEGLSurface_window:
				if(!context->cocoa.nsctx)
				{
					context->cocoa.nsctx = hcocoa.CreateContext(context->cocoa.ctx);
				}
				hcocoa.BindTexImageFromView(context->cocoa.nsctx,
					surface->cocoa.nsview);
				break;
			default:
				break;
		}
	}
#	endif // CONFIG_COCOA == 1

	EGLSurface current_draw = eglGetCurrentSurface(EGL_DRAW);
	EGLSurface current_read = eglGetCurrentSurface(EGL_READ);
	if(surface_ != current_draw)
	{
		eglMakeCurrent(dpy, surface_, surface_, context);
	}
	(*context->glesfuncs->dglBindTexImage)(surface->width,
		surface->height, NULL);
	if(surface_ != current_draw)
	{
		eglMakeCurrent(dpy, current_draw, current_read, context);
	}
	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglReleaseTexImage(EGLDisplay dpy,
	EGLSurface surface_, EGLint buffer)
{
	DEGLDisplay *display = (DEGLDisplay *)dpy;
	DEGLSurface* surface = (DEGLSurface *)surface_;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}
	if(!surface)
	{
		Dprintf("bad surface!\n");
		deglSetError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11 && surface->type == DEGLSurface_pixmap && hglX.BindTexImageEXT)
	{
		hglX.ReleaseTexImageEXT(display->dpy, surface->glx.drawable, GLX_FRONT_EXT);
		return EGL_TRUE;
	}
#	endif // CONFIG_GLX == 1

	Dprintf("See if image was changed...\n");

	return EGL_TRUE;
}
