#!/usr/bin/env python

import pygtk
import gtk
import sys

def copy_image(f):
    image = gtk.gdk.pixbuf_new_from_file(f)

    clipboard = gtk.clipboard_get()
    clipboard.set_image(image)
    clipboard.store()


copy_image(sys.argv[1]);
