# Copyright (C) 2010  Nokia Corporation All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

include $(T)/common.mak

CFLAGS += -Wall -Wextra -std=c99 -I../../include -I../include -Wno-unused -Wno-return-type
CFLAGS += -DBUILD_EGL

ifneq ($(strip $(CONFIG_STATIC)),1)
ifneq ($(strip $(OS)),MINGW32)
	LDFLAGS += -ldl
	CFLAGS += -fPIC
endif
	LDFLAGS += -lpthread
endif

MODULE := EGL
libEGL_so_MAJOR := 1
libEGL_so_MINOR := 4
libEGL_so_PATCH := 2
libEGL_so_VER := $(libEGL_so_MAJOR).$(libEGL_so_MINOR).$(libEGL_so_PATCH)
libEGL_so_SRC = \
	api.c \
	config.c \
	context.c \
	display.c \
	surface.c \
	degl.c
CFLAGS += -DDEGL_VERSION="\"$(libEGL_so_VER)\""

ifeq ($(strip $(CONFIG_OSMESA)),1)
	libEGL_so_SRC += hosmesa.c
endif
ifeq ($(strip $(CONFIG_GLX)),1)
	libEGL_so_SRC += hglx.c
	LDFLAGS += -lX11 -lXext
endif
ifeq ($(strip $(CONFIG_WGL)),1)
	libEGL_so_SRC += hwgl.c
	LDFLAGS += -lgdi32
endif
ifeq ($(strip $(CONFIG_COCOA)),1)
	libEGL_so_SRC += hcocoa.m
	LDFLAGS += -framework OpenGL -framework AppKit -lobjc
endif

# Pathify.
P_O := $(call pathify,$(O))
P_S := $(call pathify,$(S))
P_T := $(call pathify,$(T))
P_I := $(call pathify,$(I))

libEGL_so_OBJ = $(addprefix $(P_O)/$(MODULE)/,$(patsubst %.m,%.o,$(libEGL_so_SRC:.c=.o)))

ifeq ($(strip $(OS)),MINGW32)
ifeq ($(strip $(CONFIG_STATIC)),1)
libEGL_so_LIB0 := $(P_O)/libEGL.$(SOEXT)
else
libEGL_so_LIB0 := $(P_O)/EGL.$(SOEXT)
endif
libEGL_so_LIB1 :=
libEGL_so_LIB3 :=

all:	$(libEGL_so_LIB0)

$(libEGL_so_LIB0): $(libEGL_so_OBJ)
ifeq ($(strip $(CONFIG_STATIC)),1)
	$(AR) -cq $@ $^
else
	$(CC) -shared $^ $(LDFLAGS) -o $@
endif # !CONFIG_STATIC

else
ifeq ($(strip $(OS)),Darwin)
libEGL_so_LIB0 := $(P_O)/libEGL.$(SOEXT)
libEGL_so_LIB1 := $(P_O)/libEGL.$(libEGL_so_MAJOR).$(SOEXT)
libEGL_so_LIB3 := $(P_O)/libEGL.$(libEGL_so_VER).$(SOEXT)

all:	$(libEGL_so_LIB0)

$(libEGL_so_LIB0):	$(libEGL_so_LIB1)
	ln -sf $(<F) $@

$(libEGL_so_LIB1):	$(libEGL_so_LIB3)
	ln -sf $(<F) $@

$(libEGL_so_LIB3):	$(libEGL_so_OBJ)
ifeq ($(strip $(CONFIG_STATIC)),1)
	$(AR) -cq $@ $^
else
	$(CC) -dynamiclib -Wl,-dylib_install_name -Wl,$(notdir $(libEGL_so_LIB1)) $^ $(LDFLAGS) -o $@
endif # !CONFIG_STATIC

else
libEGL_so_LIB0 := $(P_O)/libEGL.$(SOEXT)
libEGL_so_LIB1 := $(P_O)/libEGL.$(SOEXT).$(libEGL_so_MAJOR)
libEGL_so_LIB3 := $(P_O)/libEGL.$(SOEXT).$(libEGL_so_VER)

all:	$(libEGL_so_LIB0)

$(libEGL_so_LIB0):	$(libEGL_so_LIB1)
	ln -sf $(<F) $@

$(libEGL_so_LIB1):	$(libEGL_so_LIB3)
	ln -sf $(<F) $@

$(libEGL_so_LIB3):	$(libEGL_so_OBJ)
ifeq ($(strip $(CONFIG_STATIC)),1)
	$(AR) -cq $@ $^
else
	$(CC) -shared -Wl,-soname=$(notdir $(libEGL_so_LIB1)) $^ $(LDFLAGS) -o $@
endif # !CONFIG_STATIC

endif # !Darwin
endif # !MINGW32

INSTALL_TARGETS := $(libEGL_so_LIB0) $(libEGL_so_LIB1) $(libEGL_so_LIB3)

$(P_O)/$(MODULE).dir:
	mkdir -p $(P_O)/$(MODULE)
	touch $(P_O)/$(MODULE).dir

$(libEGL_so_OBJ):$(P_O)/$(MODULE)/%.o: $(P_O)/$(MODULE).dir
$(P_O)/$(MODULE)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
$(P_O)/$(MODULE)/%.o: %.m
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(libEGL_so_OBJ) $(libEGL_so_LIB0) $(libEGL_so_LIB1) $(libEGL_so_LIB3)
	test ! -d $(P_O)/$(MODULE) || rmdir $(P_O)/$(MODULE)
	$(RM) $(P_O)/$(MODULE).dir

install:	all
	mkdir -p $(P_I)/pkgconfig
	sed -e 's,@INSTALL_DIR@,$(P_I),' -e 's,@DEGL_VERSION@,$(libEGL_so_VER),' egl.pc.in > $(P_I)/pkgconfig/egl.pc
	$(RM) $(foreach file,$(INSTALL_TARGETS),$(P_I)/$(notdir $(file)))
	$(CP) $(INSTALL_TARGETS) $(P_I)

.PHONY: all clean install
