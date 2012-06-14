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
#include "display.h"
#include "config.h"
#include "degl.h"

#if(CONFIG_OFFSCREEN == 1)
static DEGLOffscreenConfig const degl_configs[] =
{
	{{ 32, 8, 8, 8, 8, EGL_NONE, 0x0d, 24, 0, EGL_TRUE, 0, 0, 8, EGL_WINDOW_BIT | EGL_PIXMAP_BIT, EGL_NONE, 0, 0, 0 }, 0 },
	{{ 32, 8, 8, 8, 8, EGL_NONE, 0x0e,  0, 0, EGL_TRUE, 0, 0, 0, EGL_WINDOW_BIT | EGL_PIXMAP_BIT, EGL_NONE, 0, 0, 0 }, 0 },
	{{ 32, 8, 8, 8, 8, EGL_NONE, 0x0d, 24, 0, EGL_TRUE, 0, 0, 8, EGL_WINDOW_BIT | EGL_PIXMAP_BIT, EGL_NONE, 0, 0, 0 }, 1 },
	{{ 32, 8, 8, 8, 8, EGL_NONE, 0x0e,  0, 0, EGL_TRUE, 0, 0, 0, EGL_WINDOW_BIT | EGL_PIXMAP_BIT, EGL_NONE, 0, 0, 0 }, 1 },
};
#endif // CONFIG_OFFSCREEN == 1

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy,
	EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	DEGLDisplay* display = dpy;
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
	if(!num_config)
	{
		Dprintf("num_config == NULL!\n");
		deglSetError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		*num_config = sizeof(degl_configs)/sizeof(degl_configs[0]);
		if(!configs)
			return EGL_TRUE;

		if(config_size < *num_config)
			*num_config = config_size;

		for(EGLint i = 0; i < *num_config; ++i)
			configs[i] = (EGLConfig)(degl_configs + i);

		return EGL_TRUE;
	}
#	endif // CONFIG_OFFSCREEN == 1

	*num_config = display->config_count;
	if(!configs)
	{
		return EGL_TRUE;
	}
	if(config_size < *num_config)
	{
		*num_config = config_size;
	}

	for(EGLint i = 0; i < *num_config; i++)
	{
		switch(degl_frontend)
		{
#			if(CONFIG_GLX == 1)
			case DEGLFrontend_x11: configs[i] = &display->x11_config[i]; break;
#			endif // CONFIG_GLX == 1

#			if(CONFIG_COCOA==1)
			case DEGLFrontend_cocoa: configs[i] = &display->cocoa_config[i]; break;
#			endif // CONFIG_COCOA == 1

			default: deglSetError(EGL_BAD_DISPLAY); return EGL_FALSE;
		}
	}

	return EGL_TRUE;
}

#if(CONFIG_DEBUG == 1)
static struct degl_attrib_name_s
{
	EGLint attrib;
	const char *name;
}
const degl_attrib_name_table[] =
{
#	define DEGL_PAIR(value) { value, #value },
	DEGL_PAIR(EGL_BUFFER_SIZE)
	DEGL_PAIR(EGL_RED_SIZE)
	DEGL_PAIR(EGL_GREEN_SIZE)
	DEGL_PAIR(EGL_BLUE_SIZE)
	DEGL_PAIR(EGL_LUMINANCE_SIZE)
	DEGL_PAIR(EGL_ALPHA_SIZE)
	DEGL_PAIR(EGL_ALPHA_MASK_SIZE)
	DEGL_PAIR(EGL_BIND_TO_TEXTURE_RGB)
	DEGL_PAIR(EGL_BIND_TO_TEXTURE_RGBA)
	DEGL_PAIR(EGL_COLOR_BUFFER_TYPE)
	DEGL_PAIR(EGL_CONFIG_CAVEAT)
	DEGL_PAIR(EGL_CONFIG_ID)
	DEGL_PAIR(EGL_CONFORMANT)
	DEGL_PAIR(EGL_DEPTH_SIZE)
	DEGL_PAIR(EGL_LEVEL)
	DEGL_PAIR(EGL_MATCH_NATIVE_PIXMAP)
	DEGL_PAIR(EGL_MAX_PBUFFER_WIDTH)
	DEGL_PAIR(EGL_MAX_PBUFFER_HEIGHT)
	DEGL_PAIR(EGL_MAX_PBUFFER_PIXELS)
	DEGL_PAIR(EGL_MAX_SWAP_INTERVAL)
	DEGL_PAIR(EGL_MIN_SWAP_INTERVAL)
	DEGL_PAIR(EGL_NATIVE_RENDERABLE)
	DEGL_PAIR(EGL_NATIVE_VISUAL_ID)
	DEGL_PAIR(EGL_NATIVE_VISUAL_TYPE)
	DEGL_PAIR(EGL_RENDERABLE_TYPE)
	DEGL_PAIR(EGL_SAMPLE_BUFFERS)
	DEGL_PAIR(EGL_SAMPLES)
	DEGL_PAIR(EGL_STENCIL_SIZE)
	DEGL_PAIR(EGL_SURFACE_TYPE)
	DEGL_PAIR(EGL_TRANSPARENT_TYPE)
	DEGL_PAIR(EGL_TRANSPARENT_RED_VALUE)
	DEGL_PAIR(EGL_TRANSPARENT_GREEN_VALUE)
	DEGL_PAIR(EGL_TRANSPARENT_BLUE_VALUE)
#	ifdef EGL_NOK_texture_from_pixmap
	DEGL_PAIR(EGL_Y_INVERTED_NOK)
#	endif // def EGL_NOK_texture_from_pixmap
#	undef DEGL_PAIR
};
static const char *degl_attrib_name(EGLint attribute)
{
	const unsigned degl_attrib_table_size = sizeof(degl_attrib_name_table) / sizeof(degl_attrib_name_table[0]);
	unsigned j = 0;
	for(; j < degl_attrib_table_size; ++j)
		if(degl_attrib_name_table[j].attrib == attribute)
			return degl_attrib_name_table[j].name;
	return NULL;
}
#endif // CONFIG_DEBUG == 1

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy,
	const EGLint *attrib_list, EGLConfig *configs, EGLint config_size,
	EGLint *num_config)
{
	DEGLDisplay* display = dpy;
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
	if(!num_config)
	{
		Dprintf("num_config is NULL!\n");
		deglSetError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	int attrib_list_len = attrib_list ? 1 : 0;
	for(EGLint const* p = attrib_list; p && *p != EGL_NONE; p+=2, attrib_list_len+=2);

	Dprintf("%d attributes..\n", (attrib_list_len - 1) / 2);
#	if(CONFIG_DEBUG == 1)
	for(int i = 0; i < attrib_list_len - 1; i += 2)
	{
		const char *p = degl_attrib_name(attrib_list[i]);
		if(p)
		{
			Dprintf("\t%s = 0x%x\n", p, attrib_list[i + 1]);
		}
		else
		{
			Dprintf("\tunknown(0x%x) = 0x%x\n", attrib_list[i], attrib_list[i + 1]);
		}
	}
#	endif // (CONFIG_DEBUG == 1)

#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		EGLint depth = 0;
		EGLint stencil = 0;
		EGLint invert_y = 0;

		for(int i = 0; i < attrib_list_len; i += 2)
		{
			switch(attrib_list[i])
			{
#				ifdef EGL_NOK_texture_from_pixmap
				case EGL_Y_INVERTED_NOK:
					invert_y = attrib_list[i + 1];
					if(invert_y)
						Dprintf("Inverted y-axis requested...\n");
					break;
#				endif // def EGL_NOK_texture_from_pixmap
				case EGL_DEPTH_SIZE:
					depth = attrib_list[i + 1];
					Dprintf("Depth %d requested...\n", depth);
					break;
				case EGL_STENCIL_SIZE:
					stencil = attrib_list[i + 1];
					Dprintf("Stencil %d requested...\n", stencil);
					break;
				default:
					break;
			}
		}

		// FIXME: For more configs, we need a little more advanced way.
		if(!configs)
		{
			*num_config = (depth || stencil) ? 2 : 1;
		}
		else
		{
			*num_config = 0;

			if((depth || stencil) && *num_config < config_size)
			{
				if(configs)
					configs[*num_config] = (EGLConfig)(degl_configs + 0 + invert_y*2);
				*num_config += 1;
			}

			if(*num_config < config_size)
			{
				if(configs)
					configs[*num_config] = (EGLConfig)(degl_configs + 1 + invert_y*2);
				*num_config += 1;
			}
		}
		Dprintf("%d modes returned.\n", *num_config);
		return EGL_TRUE;
	}
#	endif // (CONFIG_OFFSCREEN == 1)

	DEGLConfig crit =
	{
		.buffer_bits = 0,
		.red_bits = 0,
		.green_bits = 0,
		.blue_bits = 0,
		.alpha_bits = 0,
		.caveat = EGL_DONT_CARE,
		.id = EGL_DONT_CARE,
		.depth_bits = 0,
		.level = 0,
		.native_renderable = EGL_DONT_CARE,
		.sample_buffers = 0,
		.samples = 0,
		.stencil_bits = 0,
		.surface_type = EGL_WINDOW_BIT,
		.transparency_type = EGL_NONE,
		.transparent_red = EGL_DONT_CARE,
		.transparent_green = EGL_DONT_CARE,
		.transparent_blue = EGL_DONT_CARE,
	};
	int pixmap_bits = EGL_DONT_CARE;
	int i, j;
	for(i = 0; i < attrib_list_len - 1; i += 2)
	{
		int value = attrib_list[i + 1];
		switch(attrib_list[i])
		{
			case EGL_BUFFER_SIZE: crit.buffer_bits = value; break;
			case EGL_RED_SIZE: crit.red_bits = value; break;
			case EGL_GREEN_SIZE: crit.green_bits = value; break;
			case EGL_BLUE_SIZE: crit.blue_bits = value; break;
			case EGL_LUMINANCE_SIZE: /* ignored */ break;
			case EGL_ALPHA_SIZE: crit.alpha_bits = value; break;
			case EGL_ALPHA_MASK_SIZE: /* ignored */ break;
			case EGL_BIND_TO_TEXTURE_RGB: /* ignored */ break;
			case EGL_BIND_TO_TEXTURE_RGBA: /* ignored */ break;
			case EGL_COLOR_BUFFER_TYPE:
				// we have RGB buffers available only
				if(value != EGL_DONT_CARE && value != EGL_RGB_BUFFER)
				{
					*num_config = 0;
					return EGL_TRUE;
				}
				break;
			case EGL_CONFIG_CAVEAT: crit.caveat = value; break;
			case EGL_CONFIG_ID: crit.id = value; break;
			case EGL_CONFORMANT: /* ignored */ break;
			case EGL_DEPTH_SIZE: crit.depth_bits = value; break;
			case EGL_LEVEL: crit.level = value; break;
			case EGL_MATCH_NATIVE_PIXMAP:
#				if(CONFIG_GLX == 1)
				if(degl_frontend == DEGLFrontend_x11)
				{
					Window dummy_win = 0;
					int dummy_int = 0;
					unsigned int dummy_uint = 0;
					XGetGeometry(display->dpy, (Pixmap)value, &dummy_win, &dummy_int,
						&dummy_int, &dummy_uint, &dummy_uint, &dummy_uint,
						(unsigned int *)&pixmap_bits);
				}
#				endif // CONFIG_GLX == 1
				break;
			case EGL_MAX_PBUFFER_WIDTH: /* ignored */ break;
			case EGL_MAX_PBUFFER_HEIGHT: /* ignored */ break;
			case EGL_MAX_PBUFFER_PIXELS: /* ignored */ break;
			case EGL_MAX_SWAP_INTERVAL: /* ignored */ break;
			case EGL_MIN_SWAP_INTERVAL: /* ignored */ break;
			case EGL_NATIVE_RENDERABLE: crit.native_renderable = value; break;
			case EGL_NATIVE_VISUAL_ID: /* ignored */ break;
			case EGL_NATIVE_VISUAL_TYPE:
				/* TODO */
				Dprintf("EGL_NATIVE_VISUAL_TYPE attribute ignored!\n");
				break;
			case EGL_RENDERABLE_TYPE: /* ignored */ break;
			case EGL_SAMPLE_BUFFERS: crit.sample_buffers = value; break;
			case EGL_SAMPLES: crit.samples = value; break;
			case EGL_STENCIL_SIZE: crit.stencil_bits = value; break;
			case EGL_SURFACE_TYPE: crit.surface_type = value; break;
			case EGL_TRANSPARENT_TYPE: crit.transparency_type = value; break;
			case EGL_TRANSPARENT_RED_VALUE: crit.transparent_red = value; break;
			case EGL_TRANSPARENT_GREEN_VALUE: crit.transparent_green = value; break;
			case EGL_TRANSPARENT_BLUE_VALUE: crit.transparent_blue = value; break;
			default:
				Dprintf("unknown attribute 0x%x!\n", attrib_list[i]);
				deglSetError(EGL_BAD_ATTRIBUTE);
				return EGL_FALSE;
		}
	}
	for(i = 0, *num_config = 0; i < display->config_count; i++)
	{
		DEGLConfig *c;
		switch(degl_frontend)
		{
#			if(CONFIG_GLX == 1)
			case DEGLFrontend_x11: c = &display->x11_config[i].config; break;
#			endif // CONFIG_GLX == 1

#			if(CONFIG_COCOA == 1)
			case DEGLFrontend_cocoa: c = &display->cocoa_config[i].config; break;
#			endif // CONFIG_COCOA == 1

			default: deglSetError(EGL_BAD_DISPLAY); return EGL_FALSE;
		}
		if((crit.red_bits != EGL_DONT_CARE && c->red_bits < crit.red_bits) ||
			(crit.green_bits != EGL_DONT_CARE && c->green_bits < crit.green_bits) ||
			(crit.blue_bits != EGL_DONT_CARE && c->blue_bits < crit.blue_bits) ||
			(crit.alpha_bits != EGL_DONT_CARE && c->alpha_bits < crit.alpha_bits) ||
			(crit.buffer_bits != EGL_DONT_CARE && c->buffer_bits < crit.buffer_bits) ||
			(crit.depth_bits != EGL_DONT_CARE && c->depth_bits < crit.depth_bits) ||
			(crit.stencil_bits != EGL_DONT_CARE && c->stencil_bits < crit.stencil_bits) ||
			!(c->surface_type & crit.surface_type) ||
			(crit.id != EGL_DONT_CARE && c->id != crit.id) ||
			(crit.caveat != EGL_DONT_CARE && c->caveat != crit.caveat) ||
			(crit.transparency_type != EGL_DONT_CARE && crit.transparency_type != c->transparency_type) ||
			(crit.transparent_red != EGL_DONT_CARE && c->transparent_red != crit.transparent_red) ||
			(crit.transparent_green != EGL_DONT_CARE && c->transparent_green != crit.transparent_green) ||
			(crit.transparent_blue != EGL_DONT_CARE && c->transparent_blue != crit.transparent_blue) ||
			(crit.level != c->level) ||
			(crit.native_renderable != EGL_DONT_CARE && c->native_renderable != crit.native_renderable) ||
			(crit.sample_buffers != EGL_DONT_CARE && c->sample_buffers < crit.sample_buffers) ||
			(crit.samples != EGL_DONT_CARE && c->samples < crit.samples) ||
			(pixmap_bits != EGL_DONT_CARE && (c->buffer_bits != pixmap_bits || !(c->surface_type & EGL_PIXMAP_BIT))))
		{
			//Dprintf("config 0x%x does not meet minimum requirements\n", c->id);
			continue;
		}
		if(configs)
		{
			int color_bits = ((crit.red_bits != EGL_DONT_CARE && crit.red_bits) ? c->red_bits : 0) +
				((crit.green_bits != EGL_DONT_CARE && crit.green_bits) ? c->green_bits : 0) +
				((crit.blue_bits != EGL_DONT_CARE && crit.blue_bits) ? c->blue_bits : 0) +
				((crit.alpha_bits != EGL_DONT_CARE && crit.alpha_bits) ? c->alpha_bits : 0);
			for(j = 0; j < *num_config; j++)
			{
				DEGLConfig *refc = configs[j];
				if(crit.caveat == EGL_DONT_CARE && c->caveat != refc->caveat)
				{
					if(c->caveat == EGL_NONE) break;
					if(refc->caveat == EGL_NONE) continue;
					if(c->caveat == EGL_SLOW_CONFIG) break;
					if(refc->caveat == EGL_SLOW_CONFIG) continue;
				}
				EGLint ref = ((crit.red_bits != EGL_DONT_CARE && crit.red_bits) ? refc->red_bits : 0) +
					((crit.green_bits != EGL_DONT_CARE && crit.green_bits) ? refc->green_bits : 0) +
					((crit.blue_bits != EGL_DONT_CARE && crit.blue_bits) ? refc->blue_bits : 0) +
					((crit.alpha_bits != EGL_DONT_CARE && crit.alpha_bits) ? refc->alpha_bits : 0);
				if(color_bits > ref) break;
				if(color_bits < ref) continue;
				if(c->buffer_bits < refc->buffer_bits) break;
				if(c->buffer_bits > refc->buffer_bits) continue;
				if(c->sample_buffers < refc->sample_buffers) break;
				if(c->sample_buffers > refc->sample_buffers) continue;
				if(c->samples < refc->samples) break;
				if(c->samples > refc->samples) continue;
				if(c->depth_bits < refc->depth_bits) break;
				if(c->depth_bits > refc->depth_bits) continue;
				if(c->stencil_bits < refc->stencil_bits) break;
				if(c->stencil_bits > refc->stencil_bits) continue;
				if(c->id < refc->id) break;
				if(c->id > refc->id) continue;
			}
			if(j < config_size)
			{
				if(j < (*num_config) - 1)
				{
					memcpy(&configs[j + 1], &configs[j], (*num_config - j) * sizeof(EGLConfig));
				}
				if(*num_config < config_size)
				{
					(*num_config)++;
				}
				configs[j] = c;
			}
		}
		else
		{
			(*num_config)++;
		}
	}
	return EGL_TRUE;
}

EGLAPI_BUILD EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config_,
	EGLint attribute, EGLint *value)
{
	DEGLDisplay* display = dpy;
	DEGLConfig const* config = config_;

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
	if(!config)
	{
		Dprintf("bad config!\n");
		deglSetError(EGL_BAD_CONFIG);
		return EGL_FALSE;
	}

	// first handle common attributes found in our EGL config structure
	switch(attribute)
	{
		case EGL_BUFFER_SIZE:             *value = degl_no_alpha ? config->buffer_bits - config->alpha_bits : config->buffer_bits;  return EGL_TRUE;
		case EGL_RED_SIZE:                *value = config->red_bits;          return EGL_TRUE;
		case EGL_GREEN_SIZE:              *value = config->green_bits;        return EGL_TRUE;
		case EGL_BLUE_SIZE:               *value = config->blue_bits;         return EGL_TRUE;
		case EGL_ALPHA_SIZE:              *value = degl_no_alpha ? 0 : config->alpha_bits; return EGL_TRUE;
		case EGL_CONFIG_CAVEAT:           *value = config->caveat;            return EGL_TRUE;
		case EGL_CONFIG_ID:               *value = config->id;                return EGL_TRUE;
		case EGL_DEPTH_SIZE:              *value = config->depth_bits;        return EGL_TRUE;
		case EGL_LEVEL:                   *value = config->level;             return EGL_TRUE;
		case EGL_NATIVE_RENDERABLE:       *value = config->native_renderable; return EGL_TRUE;
		case EGL_SAMPLE_BUFFERS:          *value = config->sample_buffers;    return EGL_TRUE;
		case EGL_SAMPLES:                 *value = config->samples;           return EGL_TRUE;
		case EGL_STENCIL_SIZE:            *value = config->stencil_bits;      return EGL_TRUE;
		case EGL_SURFACE_TYPE:            *value = config->surface_type;      return EGL_TRUE;
		case EGL_TRANSPARENT_TYPE:        *value = config->transparency_type; return EGL_TRUE;
		case EGL_TRANSPARENT_RED_VALUE:   *value = config->transparent_red;   return EGL_TRUE;
		case EGL_TRANSPARENT_GREEN_VALUE: *value = config->transparent_green; return EGL_TRUE;
		case EGL_TRANSPARENT_BLUE_VALUE:  *value = config->transparent_blue;  return EGL_TRUE;
		default: break;
	}

	// backend/frontend specific attribute handling
#	if(CONFIG_OFFSCREEN == 1)
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		const DEGLOffscreenConfig *oc = config_;
#		ifdef EGL_NOK_texture_from_pixmap
		if(attribute == EGL_INVERTED_Y_NOK)
		{
			*value = oc->invert_y;
			return EGL_TRUE;
		}
#		endif // EGL_NOK_texture_from_pixmap
	}
#	endif // CONFIG_OFFSCREEN == 1

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		const DEGLX11Config *xc = config_;
		int glx_attr = 0;
		switch(attribute)
		{
			case EGL_MAX_PBUFFER_WIDTH:  glx_attr = GLX_MAX_PBUFFER_WIDTH;  break;
			case EGL_MAX_PBUFFER_HEIGHT: glx_attr = GLX_MAX_PBUFFER_HEIGHT; break;
			case EGL_MAX_PBUFFER_PIXELS: glx_attr = GLX_MAX_PBUFFER_PIXELS; break;
			case EGL_NATIVE_VISUAL_ID:   *value = (EGLint)xc->visual_id;    return EGL_TRUE;
			case EGL_NATIVE_VISUAL_TYPE: *value = (EGLint)xc->visual_type;  return EGL_TRUE;
			default: break;
		}
		if(glx_attr)
		{
			hglX.GetFBConfigAttrib(display->dpy, xc->fb_config[0], glx_attr, value);
			return EGL_TRUE;
		}
	}
#	endif // CONFIG_GLX == 1

#	if(CONFIG_COCOA == 1)
	if(degl_frontend == DEGLFrontend_cocoa)
	{
		const DEGLCocoaConfig *ac = config_;
		switch(attribute)
		{
			case EGL_MAX_SWAP_INTERVAL: *value = 1; return EGL_TRUE;
			case EGL_MIN_SWAP_INTERVAL: *value = 0; return EGL_TRUE;
			default: break;
		}
	}
#	endif // CONFIG_COCOA == 1

	// attribute not recognized yet, make a guess
	switch(attribute)
	{
		case EGL_LUMINANCE_SIZE:          *value = 0;                    break;
		case EGL_ALPHA_MASK_SIZE:         *value = 0;                    break;
		case EGL_BIND_TO_TEXTURE_RGB:     *value = EGL_TRUE;             break;
		case EGL_BIND_TO_TEXTURE_RGBA:    *value = EGL_TRUE;             break;
		case EGL_COLOR_BUFFER_TYPE:       *value = EGL_RGB_BUFFER;       break;
		case EGL_CONFORMANT:              *value = EGL_OPENGL_ES_BIT |
		                                           EGL_OPENGL_ES2_BIT;   break;
		case EGL_MAX_PBUFFER_WIDTH:       *value = 0;                    break;
		case EGL_MAX_PBUFFER_HEIGHT:      *value = 0;                    break;
		case EGL_MAX_PBUFFER_PIXELS:      *value = 0;                    break;
		case EGL_MAX_SWAP_INTERVAL:       *value = 1;                    break;
		case EGL_MIN_SWAP_INTERVAL:       *value = 1;                    break;
		case EGL_NATIVE_VISUAL_ID:        *value = 0;                    break;
		case EGL_NATIVE_VISUAL_TYPE:      *value = 0;                    break;
		case EGL_RENDERABLE_TYPE:         *value = EGL_OPENGL_ES_BIT |
		                                           EGL_OPENGL_ES2_BIT;   break;
		default:
			Dprintf("unknown attribute 0x%x!\n", attribute);
			deglSetError(EGL_BAD_ATTRIBUTE);
			return EGL_FALSE;
	}

	return EGL_TRUE;
}
