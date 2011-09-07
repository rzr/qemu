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

/* This is a modified and simplified version of original sdl.c in qemu */
/*
 * QEMU System Emulator
 *
 * Modified by Samsung Electronics Co., LTD. 2007-2011.
 * Copyright (C) 2007 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Copyright (c) 2003-2008 Fabrice Bellard
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


#include "event_handler.h"
#include "utils.h"
#include "tools.h"

typedef enum
{
	gtk_shift_left = 1 << 0,
	gtk_shift_right = 1 << 1,
	gtk_control_left = 1 << 2,
	gtk_control_right = 1 << 3,
	gtk_alt_left = 1 << 4,
	gtk_alt_right = 1 << 5,
	gtk_caps_lock = 1 << 6
} gtk_key_mod;

//#define DEBUG_TOUCH_EVENT

static uint8_t modifiers_state[256];
static int gui_grab_code = gtk_alt_left | gtk_control_left;
static int gui_key_modifier_pressed;
static int gui_keysym;
static kbd_layout_t *kbd_layout = NULL;


static uint8_t gtk_keyevent_to_keycode_generic(const GdkEventKey *event)
{
	int keysym;

	keysym = event->keyval;

	if (keysym == 0 && event->hardware_keycode == 113) {
		keysym = GDK_Mode_switch;
	}

	return keysym2scancode(kbd_layout, keysym);
}

/* specific keyboard conversions from scan codes */

#if defined(_WIN32)

#include <windows.h>

static UINT vk2scan(UINT vk)
{
	return MapVirtualKey(vk,0);
}


static uint8_t gtk_keyevent_to_keycode(const GdkEventKey *event)
{
	return (uint8_t)vk2scan((UINT)(event->hardware_keycode));
}

#else

static const uint8_t x_keycode_to_pc_keycode[61] = {
   0xc7,      /*  97  Home   */
   0xc8,      /*  98  Up     */
   0xc9,      /*  99  PgUp   */
   0xcb,      /* 100  Left   */
   0x4c,        /* 101  KP-5   */
   0xcd,      /* 102  Right  */
   0xcf,      /* 103  End    */
   0xd0,      /* 104  Down   */
   0xd1,      /* 105  PgDn   */
   0xd2,      /* 106  Ins    */
   0xd3,      /* 107  Del    */
   0x9c,      /* 108  Enter  */
   0x9d,      /* 109  Ctrl-R */
   0x0,       /* 110  Pause  */
   0xb7,      /* 111  Print  */
   0xb5,      /* 112  Divide */
   0xb8,      /* 113  Alt-R  */
   0xc6,      /* 114  Break  */
   0x0,         /* 115 */
   0x0,         /* 116 */
   0x0,         /* 117 */
   0x0,         /* 118 */
   0x0,         /* 119 */
   0x70,         /* 120 Hiragana_Katakana */
   0x0,         /* 121 */
   0x0,         /* 122 */
   0x73,         /* 123 backslash */
   0x0,         /* 124 */
   0x0,         /* 125 */
   0x0,         /* 126 */
   0x0,         /* 127 */
   0x0,         /* 128 */
   0x79,         /* 129 Henkan */
   0x0,         /* 130 */
   0x7b,         /* 131 Muhenkan */
   0x0,         /* 132 */
   0x7d,         /* 133 Yen */
   0x0,         /* 134 */
   0x0,         /* 135 */
   0x47,         /* 136 KP_7 */
   0x48,         /* 137 KP_8 */
   0x49,         /* 138 KP_9 */
   0x4b,         /* 139 KP_4 */
   0x4c,         /* 140 KP_5 */
   0x4d,         /* 141 KP_6 */
   0x4f,         /* 142 KP_1 */
   0x50,         /* 143 KP_2 */
   0x51,         /* 144 KP_3 */
   0x52,         /* 145 KP_0 */
   0x53,         /* 146 KP_. */
   0x47,         /* 147 KP_HOME */
   0x48,         /* 148 KP_UP */
   0x49,         /* 149 KP_PgUp */
   0x4b,         /* 150 KP_Left */
   0x4c,         /* 151 KP_ */
   0x4d,         /* 152 KP_Right */
   0x4f,         /* 153 KP_End */
   0x50,         /* 154 KP_Down */
   0x51,         /* 155 KP_PgDn */
   0x52,         /* 156 KP_Ins */
   0x53,         /* 157 KP_Del */
};

/* This table is generated based off the xfree86 -> scancode mapping above
 * and the keycode mappings in /usr/share/X11/xkb/keycodes/evdev
 * and  /usr/share/X11/xkb/keycodes/xfree86
 */

static const uint8_t evdev_keycode_to_pc_keycode[61] = {
    0,         /*  97 EVDEV - RO   ("Internet" Keyboards) */
    0,         /*  98 EVDEV - KATA (Katakana) */
    0,         /*  99 EVDEV - HIRA (Hiragana) */
    0x79,      /* 100 EVDEV - HENK (Henkan) */
    0x70,      /* 101 EVDEV - HKTG (Hiragana/Katakana toggle) */
    0x7b,      /* 102 EVDEV - MUHE (Muhenkan) */
    0,         /* 103 EVDEV - JPCM (KPJPComma) */
    0x9c,      /* 104 KPEN */
    0x9d,      /* 105 RCTL */
    0xb5,      /* 106 KPDV */
    0xb7,      /* 107 PRSC */
    0xb8,      /* 108 RALT */
    0,         /* 109 EVDEV - LNFD ("Internet" Keyboards) */
    0xc7,      /* 110 HOME */
    0xc8,      /* 111 UP */
    0xc9,      /* 112 PGUP */
    0xcb,      /* 113 LEFT */
    0xcd,      /* 114 RGHT */
    0xcf,      /* 115 END */
    0xd0,      /* 116 DOWN */
    0xd1,      /* 117 PGDN */
    0xd2,      /* 118 INS */
    0xd3,      /* 119 DELE */
    0,         /* 120 EVDEV - I120 ("Internet" Keyboards) */
    0,         /* 121 EVDEV - MUTE */
    0,         /* 122 EVDEV - VOL- */
    0,         /* 123 EVDEV - VOL+ */
    0,         /* 124 EVDEV - POWR */
    0,         /* 125 EVDEV - KPEQ */
    0,         /* 126 EVDEV - I126 ("Internet" Keyboards) */
    0,         /* 127 EVDEV - PAUS */
    0,         /* 128 EVDEV - ???? */
    0,         /* 129 EVDEV - I129 ("Internet" Keyboards) */
    0xf1,      /* 130 EVDEV - HNGL (Korean Hangul Latin toggle) */
    0xf2,      /* 131 EVDEV - HJCV (Korean Hangul Hanja toggle) */
    0x7d,      /* 132 AE13 (Yen)*/
    0xdb,      /* 133 EVDEV - LWIN */
    0xdc,      /* 134 EVDEV - RWIN */
    0xdd,      /* 135 EVDEV - MENU */
    0,         /* 136 EVDEV - STOP */
    0,         /* 137 EVDEV - AGAI */
    0,         /* 138 EVDEV - PROP */
    0,         /* 139 EVDEV - UNDO */
    0,         /* 140 EVDEV - FRNT */
    0,         /* 141 EVDEV - COPY */
    0,         /* 142 EVDEV - OPEN */
    0,         /* 143 EVDEV - PAST */
    0,         /* 144 EVDEV - FIND */
    0,         /* 145 EVDEV - CUT  */
    0,         /* 146 EVDEV - HELP */
    0,         /* 147 EVDEV - I147 */
    0,         /* 148 EVDEV - I148 */
    0,         /* 149 EVDEV - I149 */
    0,         /* 150 EVDEV - I150 */
    0,         /* 151 EVDEV - I151 */
    0,         /* 152 EVDEV - I152 */
    0,         /* 153 EVDEV - I153 */
    0,         /* 154 EVDEV - I154 */
    0,         /* 155 EVDEV - I156 */
    0,         /* 156 EVDEV - I157 */
    0,         /* 157 EVDEV - I158 */
};

#include <X11/XKBlib.h>

static int check_for_evdev(void)
{
    SDL_SysWMinfo info;
    XkbDescPtr desc = NULL;
    int has_evdev = 0;
    char *keycodes = NULL;

    SDL_VERSION(&info.version);
    if (!SDL_GetWMInfo(&info)) {
        return 0;
    }
    desc = XkbGetKeyboard(info.info.x11.display,
                          XkbGBN_AllComponentsMask,
                          XkbUseCoreKbd);
    if (desc && desc->names) {
        keycodes = XGetAtomName(info.info.x11.display, desc->names->keycodes);
        if (keycodes == NULL) {
            fprintf(stderr, "could not lookup keycode name\n");
        } else if (strstart(keycodes, "evdev", NULL)) {
            has_evdev = 1;
        } else if (!strstart(keycodes, "xfree86", NULL)) {
            fprintf(stderr, "unknown keycodes `%s', please report to "
                    "qemu-devel@nongnu.org\n", keycodes);
        }
    }

    if (desc) {
        XkbFreeKeyboard(desc, XkbGBN_AllComponentsMask, True);
    }
    if (keycodes) {
        XFree(keycodes);
    }
    return has_evdev;
}

static uint8_t gtk_keyevent_to_keycode(const GdkEventKey *event)
{
	int keycode;
	static int has_evdev = -1;

	if (has_evdev == -1)
		has_evdev = check_for_evdev();

	keycode = event->hardware_keycode;

	if (keycode < 9)
		keycode = 0;
	else if (keycode < 97) {
		keycode -= 8; /* just an offset */
	}
	else if (keycode < 158) {
		/* use conversion table */
		if (has_evdev)	
			keycode = evdev_keycode_to_pc_keycode[keycode - 97];
		else
			keycode = x_keycode_to_pc_keycode[keycode - 97];
	}
	else {
		keycode = 0;
	}

	return keycode;
}
#endif


static void reset_keys(void)
{
	int i;

	for (i = 0; i < 256; i++) {

		if (modifiers_state[i]) {
			if (i & 0x80) {
				kbd_put_keycode(0xe0);
			}

			kbd_put_keycode(i | 0x80);
			modifiers_state[i] = 0;
		}
	}
}


static void gtk_process_key(GdkEventKey *event)
{
	int keycode, v;

	if (event->keyval == GDK_Pause) {
		/* specific case */
		v = 0;

		if (event->type == GDK_KEY_RELEASE)
		v |= 0x80;

		kbd_put_keycode(0xe1);
		kbd_put_keycode(0x1d | v);
		kbd_put_keycode(0x45 | v);
		return;
	}

	if (kbd_layout)
		keycode = gtk_keyevent_to_keycode_generic(event);
	else
		keycode = gtk_keyevent_to_keycode(event);

	switch(keycode) {
	case 0x00:
		/* sent when leaving window: reset the modifiers state */
		reset_keys();
		return;
	case 0x2a:                          /* Left Shift */
	case 0x36:                          /* Right Shift */
	case 0x1d:                          /* Left CTRL */
	case 0x9d:                          /* Right CTRL */
	case 0x38:                          /* Left ALT */
	case 0xb8:                         /* Right ALT */
		if (event->type == GDK_KEY_RELEASE)
			modifiers_state[keycode] = 0;
		else
			modifiers_state[keycode] = 1;
		break;
	case 0x45: /* num lock */
	case 0x3a: /* caps lock */
		/* GTK does send the key up event, so we dont generate it */
		/*kbd_put_keycode(keycode);
		kbd_put_keycode(keycode | 0x80);
		return;*/
		break;
	}

	/* now send the key code */

	if (keycode & 0x80) {
		kbd_put_keycode(0xe0);
	}

	if (event->type == GDK_KEY_RELEASE) {
		kbd_put_keycode(keycode | 0x80);
	}

	else {
		kbd_put_keycode(keycode & 0x7f);
	}
}


/* convert GDK modifiers and invoke ugly hack to distinguish
between left and right shift/control/alt */
static guint gtk_GetModState(const GdkEventKey *event)
{
	guint key = 0, keyval = event->keyval, state = event->state;

	switch(keyval)
	{
	case GDK_Shift_L:
		if (event->type != GDK_KEY_RELEASE)
			key |= gtk_shift_left;
		keyval = 1;
		break;
	case GDK_Shift_R:
		if (event->type != GDK_KEY_RELEASE)
			key |= gtk_shift_right;
		keyval = 1;
		break;
	case GDK_Control_L:
		if (event->type != GDK_KEY_RELEASE)
			key |= gtk_control_left;
		keyval = 2;
		break;
	case GDK_Control_R:
		if (event->type != GDK_KEY_RELEASE)
			key |= gtk_control_right;
		keyval = 2;
		break;
	case GDK_Alt_L:
		if (event->type != GDK_KEY_RELEASE)
			key |= gtk_alt_left;
		keyval = 3;
		break;
	case GDK_Alt_R:
		if (event->type != GDK_KEY_RELEASE)
			key |= gtk_alt_right;
		keyval = 3;
		break;
	case GDK_Caps_Lock:
		if (event->type != GDK_KEY_RELEASE)
			key |= gtk_caps_lock;
		keyval = 4;
		break;
	default:
		keyval = 0;
		break;
	}

	if (keyval != 1 && (state & GDK_SHIFT_MASK)) {
		if (modifiers_state[0x2a])
			key |= gtk_shift_left;
		if (modifiers_state[0x36])
			key |= gtk_shift_right;
	}

	if (keyval != 2 && (state & GDK_CONTROL_MASK)) {
		if (modifiers_state[0x1d])
			key |= gtk_control_left;
		if (modifiers_state[0x9d])
			key |= gtk_control_right;
	}

	/* fixme: need to do a check to make sure that alt is mapped to GDK_MOD1_MASK in the GDK_Keymap */

	if (keyval != 3 && (state & GDK_MOD1_MASK)) {
		if (modifiers_state[0x38])
			key |= gtk_alt_left;
		if (modifiers_state[0xb8])
			key |= gtk_alt_right;
	}

	if (keyval != 4 && (state & GDK_LOCK_MASK))
		key |= gtk_caps_lock;

	return key;
}


/**
  * @brief	handler to process keyboard press event
  * @param	widget:	event generate widget
  * @param	event_type: keyboard event type
  * @param	data: user event pointer
  * @return	void
  */
gboolean key_event_handler (GtkWidget *wid, GdkEventKey *event)
{
	int mod_state;

	if (event->type == GDK_KEY_PRESS) {

		mod_state = (gtk_GetModState(event) & (int)gui_grab_code) == (int)gui_grab_code;
		gui_key_modifier_pressed = mod_state;

		if (gui_key_modifier_pressed) {

			int keycode;
		    keycode = gtk_keyevent_to_keycode(event);

		    switch( keycode ) {
		    case 0x21: /* 'f' key on US keyboard */
				gui_keysym = 1;
				break;
		    case 0x02 ... 0x0a: /* '1' to '9' keys */
//				console_select(keycode - 0x02);
				gui_keysym = 1;
				break;
		    default:
				break;
			}
		}

		else if (!is_graphic_console()) {

			int keysym;
			keysym = 0;

			if (event->state & GDK_CONTROL_MASK) {

				switch(event->keyval) {
				case GDK_Up: keysym = QEMU_KEY_CTRL_UP; break;
				case GDK_Down: keysym = QEMU_KEY_CTRL_DOWN; break;
				case GDK_Left: keysym = QEMU_KEY_CTRL_LEFT; break;
				case GDK_Right: keysym = QEMU_KEY_CTRL_RIGHT; break;
				case GDK_Home: keysym = QEMU_KEY_CTRL_HOME; break;
				case GDK_End: keysym = QEMU_KEY_CTRL_END; break;
				case GDK_Page_Up: keysym = QEMU_KEY_CTRL_PAGEUP; break;
				case GDK_Page_Down: keysym = QEMU_KEY_CTRL_PAGEDOWN; break;
				default: break;
				}
			}
			else {
				switch(event->keyval) {
				case GDK_Up: keysym = QEMU_KEY_UP; break;
				case GDK_Down: keysym = QEMU_KEY_DOWN; break;
				case GDK_Left: keysym = QEMU_KEY_LEFT; break;
				case GDK_Right: keysym = QEMU_KEY_RIGHT; break;
				case GDK_Home: keysym = QEMU_KEY_HOME; break;
				case GDK_End: keysym = QEMU_KEY_END; break;
				case GDK_Page_Up: keysym = QEMU_KEY_PAGEUP; break;
				case GDK_Page_Down: keysym = QEMU_KEY_PAGEDOWN; break;
				case GDK_BackSpace: keysym = QEMU_KEY_BACKSPACE; break;
				case GDK_Delete: keysym = QEMU_KEY_DELETE; break;
				default: break;
				}
		    }

		    if (keysym)
				kbd_put_keysym(keysym);
		}
	}

	else if (event->type == GDK_KEY_RELEASE) {

		mod_state = (gtk_GetModState(event) & gui_grab_code);

		if (!mod_state) {
			if (gui_key_modifier_pressed) {
				if (gui_keysym == 0) {
					/* SDL does not send back all the
					  modifiers key, so we must correct it */
					reset_keys();
					return TRUE;
				}

				gui_key_modifier_pressed = 0;
				gui_keysym = 0;
		    }
		}
	}

	if (is_graphic_console())
		gtk_process_key(event);

	return TRUE;
}

static int button_status = -1;
static void touch_shoot_for_type(int x, int y, int lcd_status, int touch_type)
{
	int dx, dy, dz = 0, lcd_height, lcd_width;
	GtkWidget *pWidget = NULL;
	GtkWidget *popup_menu = get_widget(EMULATOR_ID, POPUP_MENU);
	lcd_height = (int)(PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.h);
	lcd_width = (int)(PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.w);

#if 0
	if (qemu_arch_is_arm()) {
	  	dx = x * 0x7FFF / (lcd_width - 1);
   		dy = y * 0x7FFF / (lcd_height - 1);
	}
	else {
		if(UISTATE.current_mode == 0){
			dx = (x * 0x7FFF) / 5040;
			dy = (y * 0x7FFF) / 3780;
		}
		else if(UISTATE.current_mode == 1){
			/* In this mode x becomes y and y -(lcd_height + lcd_region.y) becomes  x, the idea is to
			     convert all rotation events(90,180,270) to normal rotation event(0)
			  */
			dx = (((y -  (lcd_height  + PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.y))*-1) * 0x7FFF) / 5040;	
			dy = (x * 0x7FFF) / 3780;
		}
		else if(UISTATE.current_mode == 2){
			/*Similar to above comment*/
			dx = (((x -  (lcd_width  + PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.x))*-1) * 0x7FFF) / 5040;	
			dy = (((y -  (lcd_height))*-1) * 0x7FFF) / 3780;
		}
		else if(UISTATE.current_mode == 3){
			/*Similar to above comment*/
			dx = (y * 0x7FFF) / 5040;
			dy = (((x -  (lcd_width))*-1) * 0x7FFF) / 3780;
		}

	}
#endif

	if (qemu_arch_is_arm()) {
	  	dx = x * 0x7FFF / (lcd_width - 1);
   		dy = y * 0x7FFF / (lcd_height - 1);
	}
	else {
		if(UISTATE.current_mode == 0){
			dx = (x * 0x7FFF) / 5040;
			dy = (y * 0x7FFF) / 3780;
		}
		else if(UISTATE.current_mode == 1){
			/* In this mode x becomes y and y -(lcd_height + lcd_region.y) becomes  x, the idea is to
			     convert all rotation events(90,180,270) to normal rotation event(0)
			  */
			dx = ((lcd_height - y) * 0x7FFF) / 5040;
			dy = (x * 0x7FFF) / 3780;
		}
		else if(UISTATE.current_mode == 2){
			/*Similar to above comment*/
			dx = ((lcd_width - x ) * 0x7FFF) / 5040;
			dy = ((lcd_height - y) * 0x7FFF) / 3780;
		}
		else if(UISTATE.current_mode == 3){
			/*Similar to above comment*/
			dx = (y * 0x7FFF) / 5040;
			dy = ((lcd_width - x) * 0x7FFF) / 3780;
		}

	}

#ifdef DEBUG_TOUCH_EVENT 
	printf("dx %d dy %d x %d y %d lcd_width %d lcd_height %d\n",dx,dy,x,y,lcd_width,lcd_height);
	printf("lcd_region.x = %d, lcd_region.y = %d\n",
			PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.x,
			PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.y);
#endif /* DEBUG_TOUCH_EVENT */

	/* when rotate 0 (normal) */
	pWidget = g_object_get_data((GObject *) popup_menu, ROTATE_NORMAL);
	if(pWidget && GTK_IS_CHECK_MENU_ITEM(pWidget)) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE)  /* touch drag process */
			kbd_mouse_event(dx, dy, dz, touch_type);
	}
	else
		kbd_mouse_event(dx, dy, dz, touch_type);
	/* when rotate 90 */
	pWidget = g_object_get_data((GObject *) popup_menu, ROTATE_90);
	if(pWidget && GTK_IS_CHECK_MENU_ITEM(pWidget)) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
			if (qemu_arch_is_arm()){
				/* here dx comes dy and since it is (dy - 0x7FFF - lcd_height - 1) * -1) becomes dx
				   am subtracting 0x7fff because it was multiplied earlier, No idea why 0x7fff is used
				   Similar comments for other rotations
				*/
				kbd_mouse_event(((dy - 0x7FFF - lcd_height - 1) * -1) , dx, dz, touch_type);
			}
			else
			{
				kbd_mouse_event(dx, dy, dz, touch_type);
			}
		}
	}

	/* when rotate 180 */
	pWidget = g_object_get_data((GObject *) popup_menu, ROTATE_180);
	if(pWidget && GTK_IS_CHECK_MENU_ITEM(pWidget)) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
				if (qemu_arch_is_arm())
					kbd_mouse_event((dx - 0x7FFF - lcd_width - 1)*-1, (dy -0x7FFF) * -1, dz, touch_type);
				else
					kbd_mouse_event(dx, dy, dz, touch_type);
		}
	}

	/* when rotate 270 */
	pWidget = g_object_get_data((GObject *) popup_menu, ROTATE_270);
	if(pWidget && GTK_IS_CHECK_MENU_ITEM(pWidget)) {
		if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(pWidget)) == TRUE) {
				if (qemu_arch_is_arm())
					kbd_mouse_event(dy, (dx - 0x7FFF)*-1, dz, touch_type);
				else
					kbd_mouse_event(dx, dy, dz, touch_type);
		}
	}
}


/**
  * @brief 	draw image of pressed button
  * @param 	widget: widget to draw
  * @param 	event:	pointer to event generated by widget
  * @param 	nPressKey: pressed keycode
  * @param 	pSkinData: data code for skin 
  */
static void draw_mapping(GtkWidget * widget, GdkEventButton * event, int nPressKey, PHONEMODELINFO * device)
{
	int x, y, nW, nH = 0;

	if (nPressKey >= 0 && nPressKey < MAX_KEY_NUM) {

		x = PHONE.mode[UISTATE.current_mode].key_map_list[nPressKey].key_map_region.x;
		y = PHONE.mode[UISTATE.current_mode].key_map_list[nPressKey].key_map_region.y;
		nW = PHONE.mode[UISTATE.current_mode].key_map_list[nPressKey].key_map_region.w;
		nH = PHONE.mode[UISTATE.current_mode].key_map_list[nPressKey].key_map_region.h;
		gdk_draw_pixbuf(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], PHONE.mode_SkinImg[UISTATE.current_mode].pPixImg_P, x, y, x, y, nW, nH, GDK_RGB_DITHER_MAX, 0, 0);
		gdk_flush();
	}
}


/**
  * @brief	handler to process event generated by mouse
  * @param 	widget: pointer to widget generating event
  * @param 	event: pointer to event
  * @param 	data 
  * @return	success: TRUE	failure:FALSE
  */
gint motion_notify_event_handler(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	int x, y, dx, dy, lcd_status, j, keycode = 0;
 	int old_button_status = -1;
	static gboolean lcd_press_flag = FALSE; /* LCD press check flag */
	GdkColor color;
	x = event->x;
	y = event->y;

	old_button_status = button_status;	
	button_status = check_region_button(x, y, &PHONE);
	lcd_status = check_region_lcd(x, y, &PHONE);
	if(PHONE.dual_display == 1){
		int curr_rotation = UISTATE.current_mode;
		extern int intermediate_section;
		/* 0 */
		if(curr_rotation == 0 && lcd_status != NON_LCD_REGION){
			int value = PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.x + ((int)(PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.w/PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.s)/2);
			/* 0------------n   n+1-----------
			     --------------||--------------
			     --------------||--------------
			     --------------||--------------
			     the middle bar is of size intermediate_section so n+1 is actuall n + intermediate_section + 1 
			     so there is a need to subtract intermediate_section.
			     Similarly you can derive for other rotation modes.
			 */
			if(x >= value)
				x = x - intermediate_section;
		}
		/* 90 */
		else if(curr_rotation == 1 && lcd_status != NON_LCD_REGION){
			int value = PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.y + ((int)(PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.h/PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.s)/2) + intermediate_section;
			if(y <= value)
				y = y + intermediate_section;
		}
		/* 180 */
		else if(curr_rotation == 2 && lcd_status != NON_LCD_REGION){
			int value = PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.x + ((int)(PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.w/PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.s)/2) + intermediate_section;
			if(x <= value)
				x = x + intermediate_section;
		}
		/* 270 */
		else if(curr_rotation == 3 && lcd_status != NON_LCD_REGION){
			int value = PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.y + ((int)(PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.h/PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.s)/2);
			if(y > value)
				y = y - intermediate_section;
		}
	}
	dx = (x - (PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.x)) * PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.s;
	dy = (y - (PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.y)) * PHONE.mode[UISTATE.current_mode].lcd_list[lcd_status].lcd_region.s;	

	/* 1. button animation */
	
	if(button_status >= 0) {
		color.pixel = gdk_rgb_xpixel_from_rgb(0xf0f0f0); /* gray:f0 */
		gdk_gc_set_foreground(widget->style->fg_gc[GTK_STATE_NORMAL], &color);

		if (button_status != old_button_status) {
			for(j = 0; j < PHONE.mode[UISTATE.current_mode].key_map_list_cnt; j++) {

				gtk_widget_queue_draw_area(widget, PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.x,	
								   PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.y,
								   PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.w,
								   PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.h);			
			}

		}
		
		gdk_draw_rectangle(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], FALSE, 
						PHONE.mode[UISTATE.current_mode].key_map_list[button_status].key_map_region.x, 
						PHONE.mode[UISTATE.current_mode].key_map_list[button_status].key_map_region.y, 
						PHONE.mode[UISTATE.current_mode].key_map_list[button_status].key_map_region.w-1,
						PHONE.mode[UISTATE.current_mode].key_map_list[button_status].key_map_region.h-1);
		
	}

	else {
		color.pixel = gdk_rgb_xpixel_from_rgb(0x000000); /* black:00 */
		gdk_gc_set_foreground(widget->style->fg_gc[GTK_STATE_NORMAL], &color);

		for(j = 0; j < PHONE.mode[UISTATE.current_mode].key_map_list_cnt; j++) {
			gtk_widget_queue_draw_area(widget, PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.x,	
											   PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.y,
											   PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.w,
											   PHONE.mode[UISTATE.current_mode].key_map_list[j].key_map_region.h);
		}
	}

	/* 2. if clicked right mouse button, create popup_menu */
	
	GtkWidget *popup_menu = get_widget(EMULATOR_ID, POPUP_MENU);
	if (event->button == 3) {
		color.pixel = gdk_rgb_xpixel_from_rgb(0x3f3f3f); /* gray:0f */
		gdk_gc_set_foreground(widget->style->fg_gc[GTK_STATE_NORMAL], &color);
		gtk_menu_popup (GTK_MENU (popup_menu), NULL, NULL, NULL, NULL, event->button, event->time);
		return TRUE;
	}
	
	/* 3. LCD draging in LCD region */

	if (lcd_press_flag == TRUE) {
		if (lcd_status != LCD_REGION) {
			event->type = GDK_BUTTON_RELEASE;
			lcd_status = LCD_REGION;
		}

		if (event->type == GDK_MOTION_NOTIFY) 
			if (lcd_status == LCD_REGION || lcd_status == DUAL_LCD_REGION) 	
				touch_shoot_for_type(dx, dy, lcd_status, TOUCH_DRAG);
	}
	
	/* 4. when event press */

	if (event->type == GDK_BUTTON_PRESS) {

		/* 4.1  when event is in lcd region (touch region) */
		
		if (lcd_status == LCD_REGION || lcd_status == DUAL_LCD_REGION) {	

			lcd_press_flag = TRUE;			
			touch_shoot_for_type(dx, dy, lcd_status, TOUCH_PRESS);
		}

		/* 5.2  when event is not in lcd region (keycode region) */

		else {
			lcd_press_flag = FALSE;			
			UISTATE.button_press_flag = button_status;

			/* int a = kbd_mouse_is_absolute(); */

			if (button_status != NON_BUTTON_REGION) {
				keycode = PHONE.mode[UISTATE.current_mode].key_map_list[button_status].event_info[0].event_value[0].key_code;

				if (keycode == 169) keycode = 100;
				else if (keycode == 132) keycode = 101;
				else if (keycode == 174) keycode = 102;
				else if (keycode == 212) keycode = 103;
				else if (keycode == 217) keycode = 104;
				else if (keycode == 356) keycode = 105;

				if (kbd_mouse_is_absolute()) {
					log_msg(MSGL_DEBUG, "press parsing keycode = %d, result = %d\n", keycode, keycode & 0x7f);
					ps2kbd_put_keycode(keycode & 0x7f);				
				}
			}
			else {
				UISTATE.button_press_flag = -1;
				gtk_window_begin_move_drag(GTK_WINDOW(widget), event->button, 
								event->x_root, event->y_root, event->time);
			}

			draw_mapping(widget, event, UISTATE.button_press_flag, &PHONE);
		}
	}
	
	/* 5. when button release */
	
	else if (event->type == GDK_BUTTON_RELEASE) {

		/* 5.1  when event is in lcd region (touch region) */		

		if (lcd_status == LCD_REGION || lcd_status == DUAL_LCD_REGION) 	
			touch_shoot_for_type(dx, dy, lcd_status, TOUCH_RELEASE);

		/* 5.2  when event is not in lcd region (keycode region) */

		else {
			gtk_widget_queue_draw_area(widget, PHONE.mode[UISTATE.current_mode].key_map_list[UISTATE.button_press_flag].key_map_region.x,
										   PHONE.mode[UISTATE.current_mode].key_map_list[UISTATE.button_press_flag].key_map_region.y,
										   PHONE.mode[UISTATE.current_mode].key_map_list[UISTATE.button_press_flag].key_map_region.w,
										   PHONE.mode[UISTATE.current_mode].key_map_list[UISTATE.button_press_flag].key_map_region.h);

			if (button_status != NON_BUTTON_REGION) {
				keycode = PHONE.mode[UISTATE.current_mode].key_map_list[button_status].event_info[0].event_value[0].key_code;
				
				if (keycode == 169) keycode = 100;
				else if (keycode == 132) keycode = 101;
				else if (keycode == 174) keycode = 102;
				else if (keycode == 212) keycode = 103;
				else if (keycode == 217) keycode = 104;
				else if (keycode == 356) keycode = 105;

				if (kbd_mouse_is_absolute()) {
					log_msg(MSGL_DEBUG, "release parsing keycode = %d, result = %d\n", keycode, keycode| 0x80);
					ps2kbd_put_keycode(keycode | 0x80);	
				}
			}
		}

		UISTATE.button_press_flag = -1;
		lcd_press_flag = FALSE;
	}

	color.pixel = gdk_rgb_xpixel_from_rgb(0x000000); /* black:f0 */
	gdk_gc_set_foreground(widget->style->fg_gc[GTK_STATE_NORMAL], &color);
	
	return TRUE;
}


/**
  * @brief		event handle function when the screen size is changed
  * @param 	widget: event generation widget
  * @param 	event: pointer to event structure
  * @return	success : TRUE	failure	: FALSE
  */
gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	/* just save new values in configuration structure */

	configuration.main_x = event->x;
	configuration.main_y = event->y;

	return TRUE;
}

