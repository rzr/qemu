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

#include <stdlib.h>
#include "common.h"
#include "context.h"
#include "degl.h"
#include "display.h"
#include "surface.h"
#include "config.h"

static void deglRefContext(DEGLContext* context)
{
	Dprintf("Make reference counting threadsafe!\n");
	if(context)
		context->bound++;
}

static void deglUnrefContext(EGLDisplay dpy, DEGLContext** context)
{
	if(*context && !--(*context)->bound && (*context)->destroy)
	{
		eglDestroyContext(dpy, *context);
		*context = 0;
	}
}

EGLAPI_BUILD EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy,
	EGLConfig config_, EGLContext share_, const EGLint *attrib_list)
{
	DEGLDisplay* display = dpy;
	DEGLConfig* config = config_;
	DEGLContext* share_context = (DEGLContext *)share_;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_NO_CONTEXT;
	}
	if(!display->initialized)
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_NO_CONTEXT;
	}
	if(!config)
	{
		Dprintf("bad config!\n");
		deglSetError(EGL_BAD_CONFIG);
		return EGL_NO_CONTEXT;
	}

	DEGLContext* context = malloc(sizeof(*context));
	context->display    = dpy;
	context->config     = config;
	context->version    = 1;
	context->clientlib  = 0;
	context->clientdata = 0;
	context->glesfuncs  = malloc(sizeof(*(context->glesfuncs)));
	context->bound      = 0;
	context->destroy    = 0;
	context->draw       = 0;
	context->read       = 0;
	context->ebo_bound  = 0;

	// Figure the context version..
	if(attrib_list)
	{
		for(unsigned i = 0; attrib_list[i] != EGL_NONE; i += 2)
		{
			if(attrib_list[i] == EGL_CONTEXT_CLIENT_VERSION)
				context->version = attrib_list[i + 1];
		}
	}
	Dprintf("Context version %d creation requested.\n", context->version);

#	if(CONFIG_OSMESA == 1)
	if(degl_backend == DEGLBackend_osmesa)
	{
		GLenum format;
		switch(config->buffer_bits)
		{
			//case 16: format = OSMESA_RGB_565; break;
			case 24: format = OSMESA_BGR; break;
			case 32: format = OSMESA_BGRA; /*OSMESA_ARGB;*/ break;
			default:
				Dprintf("ERROR: Unsupported depth %d!\n", config->buffer_bits);
				free(context);
				deglSetError(EGL_BAD_CONFIG);
				return EGL_NO_CONTEXT;
		}
		if(context != EGL_NO_CONTEXT)
		{
			context->osmesa.ctx = hOSMesa.CreateContextExt(format,
				config->depth_bits, config->stencil_bits, 0,
				share_context ? share_context->osmesa.ctx : NULL);

			if(!context->osmesa.ctx)
			{
				Dprintf("WARNING: OSMesaCreateContextExt failed!\n");
				free(context);
				deglSetError(EGL_BAD_ALLOC);
				return EGL_NO_CONTEXT;
			}
			else
			{
				Dprintf("Created context %p.\n", context->osmesa.ctx);
			}
		}
		return context;
	}
#	endif

#	if(CONFIG_GLX == 1)
	if(degl_backend == DEGLBackend_glx)
	{
		if(degl_frontend == DEGLFrontend_x11)
		{
			context->glx.config = ((DEGLX11Config *)config_)->fb_config;
		}

#		if(CONFIG_OFFSCREEN == 1)
		if(degl_frontend == DEGLFrontend_offscreen)
		{
			int const attribs[] =
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

			int num_config;
			if(!(context->glx.config = hglX.ChooseFBConfig(display->dpy,
				XScreenNumberOfScreen(display->scr), attribs, &num_config))
			|| !num_config)
			{
				Dprintf("No FBConfigs found for requested config.\n");
				free(context);
				deglSetError(EGL_BAD_CONFIG);
				return EGL_NO_CONTEXT;
			}
		}
#		endif // CONFIG_OFFSCREEN == 1

		context->glx.ctx = hglX.CreateNewContext(display->dpy,
			context->glx.config[0], GLX_RGBA_TYPE,
			share_context ? share_context->glx.ctx : NULL,
			degl_glx_direct ? True : False);
		if(!context->glx.ctx)
		{
			Dprintf("glXCreateNewContext returned NULL!\n");
#			if(CONFIG_OFFSCREEN == 1)
			if(degl_frontend == DEGLFrontend_offscreen)
			{
				XFree(context->glx.config);
				context->glx.config = 0;
			}
#			endif // CONFIG_OFFSCREEN == 1
			free(context);
			deglSetError(EGL_BAD_ALLOC);
			return EGL_NO_CONTEXT;
		}
		return context;
	}
#	endif // CONFIG_GLX == 1

#	if(CONFIG_WGL == 1)
	if(degl_backend == DEGLBackend_wgl)
	{
		int const attribs[] =
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
			0,
		};

		int format;
		UINT num_formats;

		pthread_mutex_lock(&hwgl.haxlock);
		context->wgl.haxown = 0;
		if(hwgl.ChoosePixelFormatARB &&
			hwgl.ChoosePixelFormatARB(hwgl.pbuffer.haxdc, attribs, NULL, 1, &format, &num_formats) &&
			num_formats != 0)
		{
			if(format != hwgl.pbuffer.haxpf)
			{
				const int pattribs[] = { 0 };
				context->wgl.haxbuf = hwgl.CreatePbufferARB(hwgl.pbuffer.haxdc, format, 1, 1, pattribs);
				if(!context->wgl.haxbuf)
				{
					Dprintf("wglCreatePbufferARB failed for format %d\n", format);
				}
				else
				{
					context->wgl.haxdc = hwgl.GetPbufferDCARB(context->wgl.haxbuf);
					if(!context->wgl.haxdc)
					{
						Dprintf("wglGetPbufferDCARB failed for pbuffer with format %d\n", format);
						hwgl.DestroyPbufferARB(context->wgl.haxbuf);
					}
					else
					{
						context->wgl.haxown = 1;
						context->wgl.config = format;
						Dprintf("created new temporary pbuffer context with pixel format %d\n", context->wgl.config);
					}
				}
			}
		}
		else
		{
			if(hwgl.ChoosePixelFormatARB)
			{
				Dprintf("WARNING: Couldn't use wglChoosePixelFormatARB, ERROR = %x.\n",
					(unsigned)GetLastError());
			}
		}
		if(!context->wgl.haxown)
		{
			context->wgl.config = hwgl.pbuffer.haxpf;
			context->wgl.haxbuf = hwgl.pbuffer.haxbuf;
			context->wgl.haxdc = hwgl.pbuffer.haxdc;
		}
		context->wgl.ctx = hwgl.CreateContext(context->wgl.haxdc);
		if(share_context)
		{
			hwgl.ShareLists(share_context->wgl.ctx, context->wgl.ctx);
		}
		pthread_mutex_unlock(&hwgl.haxlock);
		return context;
	}
#	endif // CONFIG_WGL == 1

#	if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		if(degl_frontend == DEGLFrontend_cocoa)
		{
			CGLError err = CGLCreateContext(((DEGLCocoaConfig *)config_)->pf,
				share_context ? share_context->cocoa.ctx : NULL, &context->cocoa.ctx);
			if(err != kCGLNoError)
			{
				Dprintf("CGLCreateContext failed: %s\n", CGLErrorString(err));
				free(context);
				deglSetError(EGL_BAD_ALLOC);
				return EGL_NO_CONTEXT;
			}
		}

#		if(CONFIG_OFFSCREEN == 1)
		if(degl_frontend == DEGLFrontend_offscreen)
		{
			const CGLPixelFormatAttribute attribs1[] =
			{
				kCGLPFAPBuffer,
				kCGLPFAAccelerated,
				kCGLPFANoRecovery,
				kCGLPFAClosestPolicy,
				kCGLPFAColorSize, config->red_bits + config->blue_bits + config->green_bits + config->alpha_bits,
				kCGLPFAAlphaSize, config->alpha_bits,
				kCGLPFAStencilSize, config->stencil_bits,
				kCGLPFADepthSize, config->depth_bits,
				0
			};
			const CGLPixelFormatAttribute attribs2[] =
			{
				kCGLPFAOffScreen,
				kCGLPFAClosestPolicy,
				kCGLPFAColorSize, config->red_bits + config->blue_bits + config->green_bits + config->alpha_bits,
				kCGLPFAAlphaSize, config->alpha_bits,
				kCGLPFAStencilSize, config->stencil_bits,
				kCGLPFADepthSize, config->depth_bits,
				0
			};

			CGLPixelFormatObj cpf;
			GLint npix = 0;
			if(CGLChoosePixelFormat(attribs1, &cpf, &npix) != kCGLNoError || npix < 1)
			{
				CGLError err = CGLChoosePixelFormat(attribs2, &cpf, &npix);
				if(err != kCGLNoError || npix < 1)
				{
					Dprintf("CGLChoosePixelFormat failed: %s\n",
						CGLErrorString(err));
					context->cocoa.ctx = NULL;
				}
				else
				{
					context->cocoa.pbuffer_mode = 0;
				}
			}
			else
			{
				context->cocoa.pbuffer_mode = 1;
			}
			if(cpf)
			{
				CGLError err = CGLCreateContext(cpf,
					share_context ? share_context->cocoa.ctx : NULL,
					&context->cocoa.ctx);
				CGLDestroyPixelFormat(cpf);
				if(err != kCGLNoError)
				{
					Dprintf("CGLCreateContext failed: %s\n",
						CGLErrorString(err));
					free(context);
					deglSetError(EGL_BAD_ALLOC);
					return EGL_NO_CONTEXT;
				}
			}
			else
			{
				free(context);
				deglSetError(EGL_BAD_CONFIG);
				return EGL_NO_CONTEXT;
			}
		}
#		endif // CONFIG_OFFSCREEN == 1

		CGLRetainContext(context->cocoa.ctx);
		context->cocoa.nsctx = NULL;
		return context;
	}
#	endif // CONFIG_COCOA == 1

	deglSetError(EGL_BAD_DISPLAY);
	return EGL_NO_CONTEXT;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	DEGLDisplay* display = dpy;
	DEGLContext* context = ctx;

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
	if(context->bound)
	{
		context->destroy = 1;
		return EGL_TRUE;
	}

	switch(degl_backend)
	{
#		if(CONFIG_OSMESA == 1)
		case DEGLBackend_osmesa:
			hOSMesa.DestroyContext(context->osmesa.ctx);
			context->osmesa.ctx = 0;
			break;
#		endif // (CONFIG_OSMESA == 1)

#		if(CONFIG_COCOA == 1)
		case DEGLBackend_cocoa:
			if(context->cocoa.nsctx)
			{
				hcocoa.DestroyContext(context->cocoa.nsctx);
				context->cocoa.nsctx = NULL;
			}
			CGLClearDrawable(context->cocoa.ctx);
			CGLReleaseContext(context->cocoa.ctx);
			context->cocoa.ctx = 0;
			break;
#		endif // (CONFIG_COCOA == 1)

#		if(CONFIG_WGL == 1)
		case DEGLBackend_wgl:
			hwgl.DeleteContext(context->wgl.ctx);
			context->wgl.ctx = 0;
			context->wgl.config = 0;
			if(context->wgl.haxown)
			{
				hwgl.ReleasePbufferDCARB(context->wgl.haxbuf, context->wgl.haxdc);
				hwgl.DestroyPbufferARB(context->wgl.haxbuf);
				context->wgl.haxdc = 0;
				context->wgl.haxbuf = 0;
				context->wgl.haxown = 0;
			}
			break;
#		endif // (CONFIG_OSMESA == 1)

#		if(CONFIG_GLX == 1)
		case DEGLBackend_glx:
			hglX.DestroyContext(display->dpy, context->glx.ctx);
#			if(CONFIG_OFFSCREEN == 1)
			if(degl_frontend == DEGLFrontend_offscreen)
			{
				XFree(context->glx.config);
			}
#			endif // CONFIG_OFFSCREEN == 1
			context->glx.ctx = 0;
			context->glx.config = 0;
			break;
#		endif // (CONFIG_OSMESA == 1)

		default:
			break;
	}
	deglDeactivateClientAPI(context);
	free(context);
	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
			  EGLSurface read, EGLContext ctx)
{
	DEGLDisplay* display = dpy;
	DEGLSurface* draw_surface = draw;
	DEGLSurface* read_surface = read;
	DEGLContext* context = ctx;

	if(!display)
	{
		Dprintf("bad display!\n");
		deglSetError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}
	if(!display->initialized && (ctx != EGL_NO_CONTEXT ||
		draw != EGL_NO_SURFACE || read != EGL_NO_SURFACE))
	{
		Dprintf("display not initialized!\n");
		deglSetError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}
	if((ctx == EGL_NO_CONTEXT && draw != EGL_NO_SURFACE && read != EGL_NO_SURFACE) ||
		(ctx != EGL_NO_CONTEXT && draw == EGL_NO_SURFACE && read == EGL_NO_SURFACE))
	{
		Dprintf("bad match!\n");
		deglSetError(EGL_BAD_MATCH);
		return EGL_FALSE;
	}

	DEGLContext* old_context = (DEGLContext*)eglGetCurrentContext();
	Dprintf("draw=%p, read=%p, context=%p (current context %p)\n",
		draw, read, ctx, old_context);

#	if(CONFIG_OSMESA == 1)
	if(degl_backend == DEGLBackend_osmesa)
	{
		GLenum type = GL_UNSIGNED_BYTE;

		if(draw_surface && draw_surface->bpp == 2)
			type = GL_UNSIGNED_SHORT_5_6_5;

		if(draw_surface != read_surface)
		{
			Dprintf("WARNING: Draw and read surfaces are different!\n");
		}

#		if(CONFIG_OFFSCREEN == 1)
		if(degl_frontend == DEGLFrontend_offscreen)
		{
			if(context && draw_surface && !hOSMesa.MakeCurrent(context->osmesa.ctx,
				draw_surface->ddraw->pixels, type,
				draw_surface->width, draw_surface->height))
			{
				Dprintf("ctx = %p, bpp = %d, width = %d, height = %d\n",
					context->osmesa.ctx, draw_surface->bpp,
					draw_surface->width, draw_surface->height);
				Dprintf("ERROR: OSMesaMakeCurrent failed!\n");
				deglSetError(EGL_BAD_ALLOC);
				return EGL_FALSE;
			}
#			if(CONFIG_DEBUG == 1)
			if(draw_surface)
				Dprintf("PIXELS: Bind %p as current!\n", draw_surface->ddraw->pixels);
			else
				Dprintf("PIXELS: Bind NULL!\n");
#			endif // CONFIG_DEBUG == 1
		}
#		endif // CONFIG_OFFSCREEN == 1

		if(context)
			hOSMesa.PixelStore(OSMESA_Y_UP, 0);
	}
#	endif // (CONFIG_OSMESA == 1)

#	if(CONFIG_GLX == 1)
	if(degl_backend == DEGLBackend_glx)
	{
		if(!hglX.MakeContextCurrent(display->dpy,
			draw_surface ? draw_surface->glx.drawable : 0,
			read_surface ? read_surface->glx.drawable : 0,
			context ? context->glx.ctx : NULL))
		{
			Dprintf("Couldn't make context current!\n");
			deglSetError(EGL_BAD_ALLOC);
			return EGL_FALSE;
		}
	}
#	endif // (CONFIG_GLX == 1)

#	if(CONFIG_WGL == 1)
	if(degl_backend ==  DEGLBackend_wgl)
	{
		if(!draw_surface || !read_surface)
		{
			hwgl.MakeCurrent(NULL, NULL);
		}
		else
		{
			if (draw_surface != read_surface)
			{
				Dprintf("WARNING: Draw and read surfaces are different!\n");
			}
			Dprintf("surface %d x %d x %d, format %d (context format %d)\n",
				draw_surface->width, draw_surface->height, draw_surface->bpp,
				draw_surface->wgl.format, context->wgl.config);
			if(!hwgl.MakeCurrent(draw_surface->wgl.dc, context->wgl.ctx))
			{
				Dprintf("Couldn't make context current!\n");
				deglSetError(EGL_BAD_ALLOC);
				return EGL_FALSE;
			}
		}
	}
#	endif // (CONFIG_WGL == 1)

#	if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		if(draw_surface != read_surface)
		{
			Dprintf("WARNING: Draw and read surfaces are different!\n");
		}
		CGLError err = CGLSetCurrentContext(context ? context->cocoa.ctx : NULL);
		if(err != kCGLNoError)
		{
			Dprintf("CGLSetCurrentContext failed: %s\n", CGLErrorString(err));
			deglSetError(EGL_BAD_ALLOC);
			return EGL_FALSE;
		}
		if(context && draw_surface)
		{
			if(degl_frontend == DEGLFrontend_cocoa)
			{
				GLint screen = 0;
				switch(draw_surface->type)
				{
					case DEGLSurface_window:
						if(context->cocoa.nsctx == NULL)
						{
							context->cocoa.nsctx = hcocoa.CreateContext(context->cocoa.ctx);
						}
						hcocoa.SetView(context->cocoa.nsctx,
							draw_surface->cocoa.nsview);
						break;
					case DEGLSurface_pbuffer:
						err = CGLGetVirtualScreen(context->cocoa.ctx, &screen);
						if (err == kCGLNoError)
						{
							err = CGLSetPBuffer(context->cocoa.ctx,
								draw_surface->cocoa.pbuffer, 0, 0, screen);
						}
						break;
					default:
						Dprintf("unsupported cocoa surface type %d\n",
							draw_surface->type);
						break;
				}
			}
#			if(CONFIG_OFFSCREEN == 1)
			if(degl_frontend == DEGLFrontend_offscreen)
			{
				Dprintf("cocoa context %p, surface %d x %d x %d, pixels at %p\n",
					context->cocoa.ctx, draw_surface->width, draw_surface->height,
					draw_surface->bpp, draw_surface->ddraw->pixels);
				if(context->cocoa.pbuffer_mode)
				{
					GLint screen = 0;
					err = CGLGetVirtualScreen(context->cocoa.ctx, &screen);
					if (err == kCGLNoError)
					{
						err = CGLSetPBuffer(context->cocoa.ctx,
							draw_surface->cocoa.pbuffer, 0, 0, screen);
					}
				}
				else
				{
					err = CGLSetOffScreen(context->cocoa.ctx,
						draw_surface->width, draw_surface->height,
						draw_surface->width * draw_surface->bpp,
						draw_surface->ddraw->pixels);
				}
			}
#			endif // CONFIG_OFFSCREEN == 1
			if(err != kCGLNoError)
			{
				Dprintf("CGL drawable activation: %s\n", CGLErrorString(err));
				deglSetError(EGL_BAD_ALLOC);
				return EGL_FALSE;
			}
		}
	}
#	endif // (CONFIG_COCOA == 1)

	if(ctx != EGL_NO_CONTEXT)
	{
		deglRefContext(context);
		deglRefSurface(draw_surface);
		deglRefSurface(read_surface);

		deglActivateClientAPI(context);
	}

	if(old_context != EGL_NO_CONTEXT && old_context )
	{
		deglUnrefSurface(dpy, &old_context->draw);
		deglUnrefSurface(dpy, &old_context->read);
		deglUnrefContext(dpy, &old_context);
	}

	if(ctx != EGL_NO_CONTEXT)
	{
		context->draw = draw_surface;
		context->read = read_surface;
	}

	Dprintf("Setting %p as current.\n", ctx);
	deglGetTLS()->current_context = ctx;
	return EGL_TRUE;
}

EGLAPI_BUILD EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
	return deglGetTLS()->current_context;
}

EGLAPI_BUILD EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
	EGLContext ctx = eglGetCurrentContext();
	if(ctx == EGL_NO_CONTEXT)
	{
		return EGL_NO_DISPLAY;
	}
	return ((DEGLContext *)ctx)->display;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
			   EGLint attribute, EGLint *value)
{
	DEGLDisplay *display = (DEGLDisplay *)dpy;
	DEGLContext *context = (DEGLContext *)ctx;

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
	if(!ctx)
	{
		Dprintf("bad context!\n");
		deglSetError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}

	switch(attribute)
	{
		case EGL_CONFIG_ID:
			if(value)
			{
				*value = ((DEGLConfig *)(context->config))->id;
			}
			break;
		case EGL_CONTEXT_CLIENT_TYPE:
			if(value)
			{
				*value = EGL_OPENGL_ES_API;
			}
			break;
		case EGL_CONTEXT_CLIENT_VERSION:
			if(value)
			{
				*value = context->version;
			}
			break;
		case EGL_RENDER_BUFFER:
			if(value)
			{
				if(context->draw)
				{
					DEGLSurface *surface = context->draw;
					switch(surface->type)
					{
						case DEGLSurface_window: *value = EGL_SINGLE_BUFFER; break;
						case DEGLSurface_pixmap: *value = EGL_SINGLE_BUFFER; break;
						case DEGLSurface_pbuffer: *value = EGL_BACK_BUFFER; break;
						default: *value = EGL_NONE; break;
					}
				}
				else
				{
					*value = EGL_NONE;
				}
			}
			break;
		default:
			Dprintf("unknown attribute 0x%x\n", attribute);
			deglSetError(EGL_BAD_ATTRIBUTE);
			return EGL_FALSE;
	}

	return EGL_TRUE;
}
