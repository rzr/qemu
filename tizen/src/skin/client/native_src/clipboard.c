/**
 * Copy framebuffer to clipboard by JNI
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * GiWoong Kim <giwoong.kim@samsung.com>
 * SangHo Park <sangho1206.park@samsung.com>
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

#include <jni.h>
#include <stdio.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "org_tizen_emulator_skin_screenshot_ScreenShotDialog.h"

JNIEXPORT jint JNICALL Java_org_tizen_emulator_skin_screenshot_ScreenShotDialog_copyToClipboard
    (JNIEnv *env, jobject obj, jint width, jint height, jbyteArray pixels)
{
    int len = (*env)->GetArrayLength(env, pixels);
    if (len <= 0) {
        fprintf(stderr, "clipboard.c: failed to get length\n");
        fflush(stderr);
        return -1;
    }

    fprintf(stdout, "clipboard.c: copy to clipboard (%dx%d:%d)\n", width, height, len);
    fflush(stdout);

    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    if (clipboard == NULL) {
        fprintf(stderr, "clipboard.c: failed to get clipboard\n");
        fflush(stderr);
        return -1;
    }

    char *data = (char *)g_malloc(len);
    if (data == NULL) {
        fprintf(stderr, "clipboard.c: failed to g_malloc\n");
        fflush(stderr);
        return -1;
    }
    (*env)->GetByteArrayRegion(env, pixels, 0, len, data);

    GdkPixbuf *image = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, FALSE, 8,
        width, height, width * 3, NULL, NULL);
    if (image == NULL) {
        fprintf(stderr, "clipboard.c: failed to get pixbuf\n");
        fflush(stderr);

        g_free(data);
        return -1;
    }

    gtk_clipboard_set_image(clipboard, image);

    g_object_unref(image);

    return 0;
}

