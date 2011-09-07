/*
 * Emulator
 *
 * Copyright (C) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * DoHyung Hong <don.hong@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * JinKyu Kim <fredrick.kim@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YuYeon Oh <yuyeon.oh@samsung.com>
 * WooJin Jung <woojin2.jung@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#include "qemu_gtk_widget.h"
#include "utils.h"
#include <pthread.h>

#ifndef _WIN32
//extern void opengl_exec_set_parent_window(Display* _dpy, Window _parent_window);
#endif

extern void sdl_display_set_window_size(int w, int h);
extern void sdl_display_force_refresh(void);

static void qemu_widget_class_init (gpointer g_class, gpointer class_data);
static void qemu_widget_init (GTypeInstance *instance, gpointer g_class);
static void qemu_widget_realize (GtkWidget	*widget);
static void qemu_widget_size_allocate	(GtkWidget	*widget, GtkAllocation *allocation);
static void qemu_update (qemu_state_t *qemu_state);
static void qemu_sdl_init(qemu_state_t *qemu_state);

qemu_state_t *qemu_state;
static int widget_exposed;
static pthread_mutex_t sdl_mutex = PTHREAD_MUTEX_INITIALIZER;

int use_qemu_display = 0;

static void qemu_ds_update(DisplayState *ds, int x, int y, int w, int h)
{
	//log_msg(MSGL_DBG2, "qemu_ds_update\n");
	/* call sdl update */
	qemu_update(qemu_state);
}

static void qemu_ds_resize(DisplayState *ds)
{
	log_msg(MSGL_DEBUG, "%d, %d\n",
		ds_get_width(qemu_state->ds),
		ds_get_height(qemu_state->ds));

	if (ds_get_width(qemu_state->ds) == 720 && ds_get_height(qemu_state->ds) == 400) {
		log_msg(MSGL_DEBUG, "blanking BIOS\n");
		qemu_state->surface_qemu = NULL;
		return;
	}

	pthread_mutex_lock(&sdl_mutex);

	/* create surface_qemu */
	qemu_state->surface_qemu = SDL_CreateRGBSurfaceFrom(ds_get_data(qemu_state->ds),
					ds_get_width(qemu_state->ds),
					ds_get_height(qemu_state->ds),
					ds_get_bits_per_pixel(qemu_state->ds),
					ds_get_linesize(qemu_state->ds),
					qemu_state->ds->surface->pf.rmask,
					qemu_state->ds->surface->pf.gmask,
					qemu_state->ds->surface->pf.bmask,
					qemu_state->ds->surface->pf.amask);

	pthread_mutex_unlock(&sdl_mutex);

	if (!qemu_state->surface_qemu) {
		log_msg(MSGL_ERROR, "Unable to set the RGBSurface: %s", SDL_GetError() );
		return;
	}

}


static void qemu_ds_refresh(DisplayState *ds)
{
	vga_hw_update();

	if (widget_exposed) {
		qemu_update(qemu_state);
		widget_exposed--;
	}
}


static const GTypeInfo qemu_widget_info = {
	.class_size = sizeof (qemu_class_t),
	.base_init = NULL,
	.base_finalize = NULL,
	.class_init = qemu_widget_class_init,
	.class_finalize = NULL,
	.class_data = NULL,
	.instance_size = sizeof (qemu_state_t),
	.n_preallocs = 0,
	.instance_init = qemu_widget_init,
	.value_table = NULL,
};

static GType qemu_widget_type;

static GType qemu_get_type(void)
{
	if (!qemu_widget_type)
		qemu_widget_type = g_type_register_static(GTK_TYPE_WIDGET, "Qemu", &qemu_widget_info, 0);

	return qemu_widget_type;
}


static void qemu_widget_class_init(gpointer g_class, gpointer class_data)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (g_class);

	widget_class->realize = qemu_widget_realize;
	widget_class->size_allocate = qemu_widget_size_allocate;
	widget_class->expose_event = qemu_widget_expose;

	log_msg(MSGL_DEBUG, "qemu class init\n");
}


static void qemu_widget_init(GTypeInstance *instance, gpointer g_class)
{
	log_msg(MSGL_DEBUG, "qemu initialize\n");
}


void qemu_display_init (DisplayState *ds)
{
	log_msg(MSGL_WARN, "qemu_display_init\n");
	/*  graphics context information */
	DisplayChangeListener *dcl;

	qemu_state->ds = ds;

	dcl = qemu_mallocz(sizeof(DisplayChangeListener));
	dcl->dpy_update = qemu_ds_update;
	dcl->dpy_resize = qemu_ds_resize;
	dcl->dpy_refresh = qemu_ds_refresh;

	register_displaychangelistener(qemu_state->ds, dcl);
}



int intermediate_section = 12;

gint qemu_widget_new (GtkWidget **widget)
{
	lcd_list_data *lcd = &PHONE.mode[UISTATE.current_mode].lcd_list[0];

	if (!qemu_state)
		qemu_state = g_object_new(qemu_get_type(), NULL);

	qemu_state->scale = lcd->lcd_region.s;
	qemu_state->width = lcd->lcd_region.w / qemu_state->scale;
	qemu_state->height = lcd->lcd_region.h / qemu_state->scale;
	qemu_state->bpp = lcd->bitsperpixel;
	qemu_state->flags = SDL_HWSURFACE|SDL_ASYNCBLIT|SDL_HWACCEL|SDL_NOFRAME;
	if(PHONE.dual_display == 1){
		intermediate_section = lcd->lcd_region.split;
		//printf("rotation intermediate_section %d\n", intermediate_section);
		if(UISTATE.current_mode ==0 || UISTATE.current_mode ==2)
		{
			qemu_state->width=qemu_state->width+intermediate_section;
			qemu_widget_size (qemu_state, qemu_state->width, qemu_state->height);
		}
		if(UISTATE.current_mode ==1 || UISTATE.current_mode ==3)
		{
			qemu_state->height=qemu_state->height+intermediate_section;
			qemu_widget_size (qemu_state, qemu_state->width, qemu_state->height);
		}
	}
	else
		qemu_widget_size (qemu_state, qemu_state->width, qemu_state->height);

	log_msg(MSGL_DEBUG, "qemu widget size is width = %d, height = %d\n",
		qemu_state->width, qemu_state->height);

	*widget = GTK_WIDGET (qemu_state);

	return TRUE;
}


void qemu_widget_size (qemu_state_t *qemu_state, gint width, gint height)
{
	GtkWidget *widget = GTK_WIDGET(qemu_state);

	g_return_if_fail (GTK_IS_QEMU (qemu_state));

	GTK_WIDGET (qemu_state)->requisition.width = width;
	GTK_WIDGET (qemu_state)->requisition.height = height;

	gtk_widget_queue_resize (GTK_WIDGET (qemu_state));

	log_msg(MSGL_INFO, "qemu_qemu_state size if width = %d, height = %d\n", width, height);

	sdl_display_set_rotation((4 - UISTATE.current_mode)%4);
	sdl_display_set_window_size(width, height);

#ifdef GTK_WIDGET_REALIZED
	if (GTK_WIDGET_REALIZED (widget))
#else
	if (gtk_widget_get_realized(widget))
#endif
	{
		pthread_mutex_lock(&sdl_mutex);
		qemu_sdl_init(GTK_QEMU(widget));
		pthread_mutex_unlock(&sdl_mutex);
	}
}


static void qemu_widget_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_QEMU (widget));
#if GTK_CHECK_VERSION(2,20,0)
	gtk_widget_set_realized(widget, TRUE);
#else
	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
#endif

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget) ;
	attributes.event_mask |= GDK_EXPOSURE_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, widget);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

	pthread_mutex_lock(&sdl_mutex);
	qemu_sdl_init(GTK_QEMU(widget));
	pthread_mutex_unlock(&sdl_mutex);

	log_msg(MSGL_DEBUG, "qemu realize success\n");
}


static void qemu_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_QEMU (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;
	/* FIXME, TODO-1.3: back out the MAX() statements */
	widget->allocation.width = MAX (1, widget->allocation.width);
	widget->allocation.height = MAX (1, widget->allocation.height);

#ifdef GTK_WIDGET_REALIZED
	if (GTK_WIDGET_REALIZED (widget))
#else
	if (gtk_widget_get_realized(widget))
#endif
	{
		gdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);
	}

	log_msg(MSGL_DEBUG, "qemu_state size allocated\n");
}


static void qemu_sdl_init(qemu_state_t *qemu_state)
{
	GtkWidget *qw = GTK_WIDGET(qemu_state);
	gchar SDL_windowhack[32];
	SDL_SysWMinfo info;
	long window;

	if (use_qemu_display)
		return;

#ifndef _WIN32
	window = GDK_WINDOW_XWINDOW(qw->window);;
#else
	window = (long)GDK_WINDOW_HWND(qw->window);;
#endif
	sprintf(SDL_windowhack, "%ld", window);
#ifndef _WIN32
	XSync(GDK_DISPLAY(), FALSE);
#endif
	g_setenv("SDL_WINDOWID", SDL_windowhack, 1);

	if (SDL_InitSubSystem (SDL_INIT_VIDEO) < 0 ) {
		log_msg(MSGL_ERROR, "unable to init SDL: %s", SDL_GetError() );
		exit(1);
	}

	qemu_state->surface_screen = SDL_SetVideoMode(qemu_state->width,
					qemu_state->height, 0, qemu_state->flags);

#ifndef _WIN32
	SDL_VERSION(&info.version);
	SDL_GetWMInfo(&info);
//	opengl_exec_set_parent_window(info.info.x11.display, info.info.x11.window);
#endif
}

gint qemu_widget_expose (GtkWidget *widget, GdkEventExpose *event)
{
	if (!qemu_state->ds) {
		sdl_display_force_refresh();
		return FALSE;
	}

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (GTK_IS_QEMU (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	log_msg(MSGL_DBG2, "qemu_expose\n");
	widget_exposed++;

	return FALSE;
}

static void qemu_update (qemu_state_t *qemu_state)
{
	SDL_Surface *surface  = NULL;

	if (!qemu_state->ds)
		return;

	g_return_if_fail (qemu_state != NULL);
	g_return_if_fail (GTK_IS_QEMU (qemu_state));

	pthread_mutex_lock(&sdl_mutex);

	surface = SDL_GetVideoSurface ();

	/* if dbi_file have scale option, we use zoomdown function.*/
	if (PHONE.dual_display == 1) {
		SDL_Surface *down_screen = NULL;
		SDL_Rect fb_first_half;
		SDL_Rect fb_second_half;
		SDL_Rect on_screen_first_half;
		SDL_Rect on_screen_second_half;

		SDL_Rect *ptr_fb_first_half;
		SDL_Rect *ptr_fb_second_half;
		SDL_Rect *ptr_on_screen_first_half;
		SDL_Rect *ptr_on_screen_second_half;
		SDL_Surface *bar  = NULL;

		down_screen = rotozoomSurface(qemu_state->surface_qemu, UISTATE.current_mode * 90,
				 1 / qemu_state->scale, SMOOTHING_ON);

		if(UISTATE.current_mode==0 ||  UISTATE.current_mode==2) /* 0 and 180 degree rotation*/
		{
			if (down_screen && qemu_state->scale ==2)
			{
				/*load the splitted area with image*/
				bar=SDL_LoadBMP(PHONE.mode[ UISTATE.current_mode].image_list.splitted_area_image);
				/*firt half of the framebuffer data (SDLSurface1)*/
				fb_first_half.x=0;
				fb_first_half.y=0;
				fb_first_half.w= down_screen->w/2;
				fb_first_half.h= down_screen->h;

				/*second half of the framebuffer data(SDL_Surface2)*/
				fb_second_half.x=down_screen->w/2;
				fb_second_half.y=0;
				fb_second_half.w= down_screen->w/2;
				fb_second_half.h= down_screen->h;

				/*on screen position for SDLSurface1*/
				on_screen_first_half.x=0;
				on_screen_first_half.y=0;
				on_screen_first_half.w=down_screen->w/2;
				on_screen_first_half.h= down_screen->h;

				/*on screen position for SDLSurface2*/
				on_screen_second_half.x=down_screen->w/2+intermediate_section;
				on_screen_second_half.y=0;
				on_screen_second_half.w= down_screen->w/2;
				on_screen_second_half.h=down_screen->h;

				ptr_fb_first_half=&fb_first_half;
				ptr_fb_second_half=&fb_second_half;
				ptr_on_screen_first_half=&on_screen_first_half;
				ptr_on_screen_second_half=&on_screen_second_half;
			}
			if ((qemu_state->scale == 1) && (UISTATE.current_mode == 0))
			{
				SDL_BlitSurface(qemu_state->surface_qemu, NULL,
					 qemu_state->surface_screen, NULL);
			}
			else
			{
				SDL_BlitSurface(down_screen, ptr_fb_first_half,
					qemu_state->surface_screen, ptr_on_screen_first_half);
				SDL_BlitSurface(down_screen, ptr_fb_second_half,
					qemu_state->surface_screen, ptr_on_screen_second_half);
				SDL_BlitSurface(bar,NULL, qemu_state->surface_screen, ptr_fb_second_half);
			}
		}
		if(UISTATE.current_mode==1 || UISTATE.current_mode==3)/*90 and 270 degree Rotation */
		{
			if (down_screen && qemu_state->scale ==2)
			{
				/*load the splitted area with image*/
				bar=SDL_LoadBMP(PHONE.mode[ UISTATE.current_mode].image_list.splitted_area_image);

				/*firt half of the framebuffer data (SDLSurface1)*/
				fb_first_half.x=0;
				fb_first_half.y=0;
				fb_first_half.w= down_screen->w;
				fb_first_half.h= down_screen->h/2;
				/*second half of the framebuffer data(SDL_Surface2)*/
				fb_second_half.x=0;
				fb_second_half.y=down_screen->h/2;
				fb_second_half.w= down_screen->w;
				fb_second_half.h= down_screen->h/2;

				/*on screen position for SDLSurface1*/
				on_screen_first_half.x=0;
				on_screen_first_half.y=0;
				on_screen_first_half.w= down_screen->w;
				on_screen_first_half.h= down_screen->h/2;

				/*on screen position for SDLSurface2*/
				on_screen_second_half.x=0;
				on_screen_second_half.y= (down_screen->h/2)+intermediate_section;
				on_screen_second_half.w= down_screen->w;
				on_screen_second_half.h= down_screen->h/2;

				ptr_fb_first_half=&fb_first_half;
				ptr_fb_second_half=&fb_second_half;
				ptr_on_screen_first_half=&on_screen_first_half;
				ptr_on_screen_second_half=&on_screen_second_half;
			}
			if ((qemu_state->scale == 1) && (UISTATE.current_mode == 0))
			{
				SDL_BlitSurface(qemu_state->surface_qemu, NULL,
					qemu_state->surface_screen, NULL);
			}
			else
			{
				SDL_BlitSurface(down_screen, ptr_fb_first_half,
					qemu_state->surface_screen, ptr_on_screen_first_half);
				SDL_BlitSurface(down_screen, ptr_fb_second_half,
					qemu_state->surface_screen, ptr_on_screen_second_half);
				SDL_BlitSurface(bar, NULL,
					qemu_state->surface_screen, ptr_fb_second_half);
			}
		}
	}
	else
	{
		if ((qemu_state->scale == 1) && (UISTATE.current_mode == 0))
			SDL_BlitSurface(qemu_state->surface_qemu, NULL, qemu_state->surface_screen, NULL);
		else
		{
			SDL_Surface *down_screen;
			down_screen = rotozoomSurface(qemu_state->surface_qemu,
				 UISTATE.current_mode * 90, 1 / qemu_state->scale, SMOOTHING_ON);
			SDL_BlitSurface(down_screen, NULL, qemu_state->surface_screen, NULL);
			SDL_FreeSurface(down_screen);
		}
	}

	/* If 'x', 'y', 'w' and 'h' are all 0, SDL_UpdateRect will update the entire screen.*/

	SDL_UpdateRect(qemu_state->surface_screen, 0, 0, 0, 0);

	pthread_mutex_unlock(&sdl_mutex);
}
