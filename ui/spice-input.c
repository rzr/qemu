/*
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <spice.h>
#include <spice/enums.h>

#include "qemu-common.h"
#include "ui/qemu-spice.h"
#include "ui/console.h"

#ifdef CONFIG_MARU
extern int get_emul_vm_base_port(void);
#endif

/* keyboard bits */

typedef struct QemuSpiceKbd {
    SpiceKbdInstance sin;
    int ledstate;
} QemuSpiceKbd;

static void kbd_push_key(SpiceKbdInstance *sin, uint8_t frag);
static uint8_t kbd_get_leds(SpiceKbdInstance *sin);
static void kbd_leds(void *opaque, int l);

static const SpiceKbdInterface kbd_interface = {
    .base.type          = SPICE_INTERFACE_KEYBOARD,
    .base.description   = "qemu keyboard",
    .base.major_version = SPICE_INTERFACE_KEYBOARD_MAJOR,
    .base.minor_version = SPICE_INTERFACE_KEYBOARD_MINOR,
    .push_scan_freg     = kbd_push_key,
    .get_leds           = kbd_get_leds,
};

static void kbd_push_key(SpiceKbdInstance *sin, uint8_t frag)
{
    kbd_put_keycode(frag);
}

static uint8_t kbd_get_leds(SpiceKbdInstance *sin)
{
    QemuSpiceKbd *kbd = container_of(sin, QemuSpiceKbd, sin);
    return kbd->ledstate;
}

static void kbd_leds(void *opaque, int ledstate)
{
    QemuSpiceKbd *kbd = opaque;

    kbd->ledstate = 0;
    if (ledstate & QEMU_SCROLL_LOCK_LED) {
        kbd->ledstate |= SPICE_KEYBOARD_MODIFIER_FLAGS_SCROLL_LOCK;
    }
    if (ledstate & QEMU_NUM_LOCK_LED) {
        kbd->ledstate |= SPICE_KEYBOARD_MODIFIER_FLAGS_NUM_LOCK;
    }
    if (ledstate & QEMU_CAPS_LOCK_LED) {
        kbd->ledstate |= SPICE_KEYBOARD_MODIFIER_FLAGS_CAPS_LOCK;
    }
    spice_server_kbd_leds(&kbd->sin, ledstate);
}

#ifdef CONFIG_MARU
/* hwkey bits */

typedef struct QemuSpiceHwkey {
    SpiceHwkeyInstance sin;
} QemuSpiceHwkey;

static void hwkey_push_key(SpiceHwkeyInstance *sin, uint8_t type, uint8_t keycode);

static const SpiceHwkeyInterface hwkey_interface = {
    .base.type          = SPICE_INTERFACE_HWKEY,
    .base.description   = "qemu hwkey",
    .base.major_version = SPICE_INTERFACE_HWKEY_MAJOR,
    .base.minor_version = SPICE_INTERFACE_HWKEY_MINOR,
    .push_scan_freg     = hwkey_push_key,
};

static void hwkey_push_key(SpiceHwkeyInstance *sin, uint8_t type, uint8_t keycode)
{
    hwkey_put_keycode(type, keycode);
}

/* rotation bits */
typedef struct QemuSpiceRotation {
    SpiceRotationInstance sin;
} QemuSpiceRotation;

static void rotation_push_type(SpiceRotationInstance *sin, uint8_t type);

static const SpiceRotationInterface rotation_interface = {
    .base.type          = SPICE_INTERFACE_ROTATION,
    .base.description   = "qemu rotation",
    .base.major_version = SPICE_INTERFACE_ROTATION_MAJOR,
    .base.minor_version = SPICE_INTERFACE_ROTATION_MINOR,
    .push_scan_freg     = rotation_push_type,
};

static void rotation_push_type(SpiceRotationInstance *sin, uint8_t type)
{
    rotation_put_type(type);
}

/* hostkbd bits */
typedef struct QemuSpiceHostkbd {
    SpiceHostkbdInstance sin;
} QemuSpiceHostkbd;

static void hostkbd_push_type(SpiceHostkbdInstance *sin, uint8_t type);

static const SpiceHostkbdInterface hostkbd_interface = {
    .base.type          = SPICE_INTERFACE_HOSTKBD,
    .base.description   = "qemu hostkbd onoff",
    .base.major_version = SPICE_INTERFACE_HOSTKBD_MAJOR,
    .base.minor_version = SPICE_INTERFACE_HOSTKBD_MINOR,
    .push_scan_freg     = hostkbd_push_type,
};

static void hostkbd_push_type(SpiceHostkbdInstance *sin, uint8_t type)
{
    hostkbd_put_type(type);
}

/* tizen close bits */
typedef struct QemuSpiceTizenClose {
    SpiceTizenCloseInstance sin;
} QemuSpiceTizenClose;

static void tizen_close_push_type(SpiceTizenCloseInstance *sin, uint8_t type);

static const SpiceTizenCloseInterface tizen_close_interface = {
    .base.type          = SPICE_INTERFACE_TIZEN_CLOSE,
    .base.description   = "qemu tizen close",
    .base.major_version = SPICE_INTERFACE_TIZEN_CLOSE_MAJOR,
    .base.minor_version = SPICE_INTERFACE_TIZEN_CLOSE_MINOR,
    .push_scan_freg     = tizen_close_push_type,
};

static void tizen_close_push_type(SpiceTizenCloseInstance *sin, uint8_t type)
{
    tizen_close_put_type(type);
}

/* vmname bits */
typedef struct QemuSpiceSdbPort {
    SpiceSdbPortInstance sin;
} QemuSpiceSdbPort;

static void qemu_get_sdb_port(SpiceSdbPortInstance *sin, int* sdb_port);

static const SpiceSdbPortInterface sdbport_interface = {
    .base.type          = SPICE_INTERFACE_SDBPORT,
    .base.description   = "qemu sdb port",
    .base.major_version = SPICE_INTERFACE_SDBPORT_MAJOR,
    .base.minor_version = SPICE_INTERFACE_SDBPORT_MINOR,
    .push_scan_freg     = qemu_get_sdb_port,
};

static void qemu_get_sdb_port(SpiceSdbPortInstance *sin, int* sdb_port)
{
    printf("%d\n", get_emul_vm_base_port());
    *sdb_port = get_emul_vm_base_port();
}
#endif

/* mouse bits */

typedef struct QemuSpicePointer {
    SpiceMouseInstance  mouse;
    SpiceTabletInstance tablet;
    int width, height, x, y;
    Notifier mouse_mode;
    bool absolute;
} QemuSpicePointer;

static int map_buttons(int spice_buttons)
{
    int qemu_buttons = 0;

    /*
     * Note: SPICE_MOUSE_BUTTON_* specifies the wire protocol but this
     * isn't what we get passed in via interface callbacks for the
     * middle and right button ...
     */
    if (spice_buttons & SPICE_MOUSE_BUTTON_MASK_LEFT) {
        qemu_buttons |= MOUSE_EVENT_LBUTTON;
    }
    if (spice_buttons & 0x04 /* SPICE_MOUSE_BUTTON_MASK_MIDDLE */) {
        qemu_buttons |= MOUSE_EVENT_MBUTTON;
    }
    if (spice_buttons & 0x02 /* SPICE_MOUSE_BUTTON_MASK_RIGHT */) {
        qemu_buttons |= MOUSE_EVENT_RBUTTON;
    }
    return qemu_buttons;
}

static void mouse_motion(SpiceMouseInstance *sin, int dx, int dy, int dz,
                         uint32_t buttons_state)
{
    kbd_mouse_event(dx, dy, dz, map_buttons(buttons_state));
}

static void mouse_buttons(SpiceMouseInstance *sin, uint32_t buttons_state)
{
    kbd_mouse_event(0, 0, 0, map_buttons(buttons_state));
}

static const SpiceMouseInterface mouse_interface = {
    .base.type          = SPICE_INTERFACE_MOUSE,
    .base.description   = "mouse",
    .base.major_version = SPICE_INTERFACE_MOUSE_MAJOR,
    .base.minor_version = SPICE_INTERFACE_MOUSE_MINOR,
    .motion             = mouse_motion,
    .buttons            = mouse_buttons,
};

static void tablet_set_logical_size(SpiceTabletInstance* sin, int width, int height)
{
    QemuSpicePointer *pointer = container_of(sin, QemuSpicePointer, tablet);

    if (height < 16) {
        height = 16;
    }
    if (width < 16) {
        width = 16;
    }
    pointer->width  = width;
    pointer->height = height;
}

static void tablet_position(SpiceTabletInstance* sin, int x, int y,
                            uint32_t buttons_state)
{
    QemuSpicePointer *pointer = container_of(sin, QemuSpicePointer, tablet);

    pointer->x = x * 0x7FFF / (pointer->width - 1);
    pointer->y = y * 0x7FFF / (pointer->height - 1);
    kbd_mouse_event(pointer->x, pointer->y, 0, map_buttons(buttons_state));
}


static void tablet_wheel(SpiceTabletInstance* sin, int wheel,
                         uint32_t buttons_state)
{
    QemuSpicePointer *pointer = container_of(sin, QemuSpicePointer, tablet);

    kbd_mouse_event(pointer->x, pointer->y, wheel, map_buttons(buttons_state));
}

static void tablet_buttons(SpiceTabletInstance *sin,
                           uint32_t buttons_state)
{
    QemuSpicePointer *pointer = container_of(sin, QemuSpicePointer, tablet);

    kbd_mouse_event(pointer->x, pointer->y, 0, map_buttons(buttons_state));
}

static const SpiceTabletInterface tablet_interface = {
    .base.type          = SPICE_INTERFACE_TABLET,
    .base.description   = "tablet",
    .base.major_version = SPICE_INTERFACE_TABLET_MAJOR,
    .base.minor_version = SPICE_INTERFACE_TABLET_MINOR,
    .set_logical_size   = tablet_set_logical_size,
    .position           = tablet_position,
    .wheel              = tablet_wheel,
    .buttons            = tablet_buttons,
};

static void mouse_mode_notifier(Notifier *notifier, void *data)
{
    QemuSpicePointer *pointer = container_of(notifier, QemuSpicePointer, mouse_mode);
    bool is_absolute  = kbd_mouse_is_absolute();

    if (pointer->absolute == is_absolute) {
        return;
    }

    if (is_absolute) {
        qemu_spice_add_interface(&pointer->tablet.base);
    } else {
        spice_server_remove_interface(&pointer->tablet.base);
    }
    pointer->absolute = is_absolute;
}

void qemu_spice_input_init(void)
{
    QemuSpiceKbd *kbd;
    QemuSpicePointer *pointer;

#ifdef CONFIG_MARU
    QemuSpiceHwkey *hwkey;
    QemuSpiceRotation *rotation;
    QemuSpiceHostkbd *hostkbd;
    QemuSpiceTizenClose *close;
    QemuSpiceSdbPort *sdb_port;
#endif

    kbd = g_malloc0(sizeof(*kbd));
    kbd->sin.base.sif = &kbd_interface.base;
    qemu_spice_add_interface(&kbd->sin.base);
    qemu_add_led_event_handler(kbd_leds, kbd);

#ifdef CONFIG_MARU
    hwkey = g_malloc0(sizeof(*hwkey));
    hwkey->sin.base.sif = &hwkey_interface.base;
    qemu_spice_add_interface(&hwkey->sin.base);

    rotation = g_malloc0(sizeof(*rotation));
    rotation->sin.base.sif = &rotation_interface.base;
    qemu_spice_add_interface(&rotation->sin.base);

    hostkbd = g_malloc0(sizeof(*hostkbd));
    hostkbd->sin.base.sif = &hostkbd_interface.base;
    qemu_spice_add_interface(&hostkbd->sin.base);

    close = g_malloc0(sizeof(*close));
    close->sin.base.sif = &tizen_close_interface.base;
    qemu_spice_add_interface(&close->sin.base);

    sdb_port = g_malloc0(sizeof(*sdb_port));
    sdb_port->sin.base.sif = &sdbport_interface.base;
    qemu_spice_add_interface(&sdb_port->sin.base);
#endif

    pointer = g_malloc0(sizeof(*pointer));
    pointer->mouse.base.sif  = &mouse_interface.base;
    pointer->tablet.base.sif = &tablet_interface.base;
    qemu_spice_add_interface(&pointer->mouse.base);

    pointer->absolute = false;
    pointer->mouse_mode.notify = mouse_mode_notifier;
    qemu_add_mouse_mode_change_notifier(&pointer->mouse_mode);
    mouse_mode_notifier(&pointer->mouse_mode, NULL);
}
