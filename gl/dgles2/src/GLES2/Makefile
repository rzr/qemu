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
CFLAGS += -DBUILD_GLES2

ifneq ($(strip $(CONFIG_STATIC)),1)
	LDFLAGS += -lpthread -L$(P_O) -lEGL
ifneq ($(strip $(OS)),MINGW32)
	CFLAGS += -fPIC
endif
endif

MODULE := GLES2
libGLESv2_so_MAJOR := 2
libGLESv2_so_MINOR := 0
libGLESv2_so_PATCH := 0
libGLESv2_so_VER := $(libGLESv2_so_MAJOR).$(libGLESv2_so_MINOR).$(libGLESv2_so_PATCH)
libGLESv2_so_SRC = \
	common.c \
	shader.c \
	vertex.c \
	dgl.c \
	hgl.c

P_O := $(call pathify,$(O))
P_S := $(call pathify,$(S))
P_T := $(call pathify,$(T))
P_I := $(call pathify,$(I))

libGLESv2_so_OBJ = $(addprefix $(P_O)/$(MODULE)/,$(libGLESv2_so_SRC:.c=.o))


ifeq ($(strip $(OS)),MINGW32)
ifeq ($(strip $(CONFIG_STATIC)),1)
libGLESv2_so_LIB0 := $(P_O)/libGLESv2.$(SOEXT)
else
libGLESv2_so_LIB0 := $(P_O)/GLESv2.$(SOEXT)
endif
libGLESv2_so_LIB1 :=
libGLESv2_so_LIB3 :=

all:	$(libGLESv2_so_LIB0)

$(libGLESv2_so_LIB0): $(libGLESv2_so_OBJ)
ifeq ($(strip $(CONFIG_STATIC)),1)
	$(AR) -cq $@ $^
else
	$(CC) -shared ./GLESv2.def $^ $(LDFLAGS) -o $@
endif # !CONFIG_STATIC

else
ifeq ($(strip $(OS)),Darwin)
libGLESv2_so_LIB0 := $(P_O)/libGLESv2.$(SOEXT)
libGLESv2_so_LIB1 := $(P_O)/libGLESv2.$(libGLESv2_so_MAJOR).$(SOEXT)
libGLESv2_so_LIB3 := $(P_O)/libGLESv2.$(libGLESv2_so_VER).$(SOEXT)

all:	$(libGLESv2_so_LIB0)

$(libGLESv2_so_LIB0):	$(libGLESv2_so_LIB1)
	ln -sf $(<F) $@

$(libGLESv2_so_LIB1):	$(libGLESv2_so_LIB3)
	ln -sf $(<F) $@

$(libGLESv2_so_LIB3):	$(libGLESv2_so_OBJ)
ifeq ($(strip $(CONFIG_STATIC)),1)
	$(AR) -cq $@ $^
else
	$(CC) -dynamiclib -Wl,-dylib_install_name -Wl,$(notdir $(libGLESv2_so_LIB1)) $^ $(LDFLAGS) -o $@
endif # !CONFIG_STATIC

else
libGLESv2_so_LIB0 := $(P_O)/libGLESv2.$(SOEXT)
libGLESv2_so_LIB1 := $(P_O)/libGLESv2.$(SOEXT).$(libGLESv2_so_MAJOR)
libGLESv2_so_LIB3 := $(P_O)/libGLESv2.$(SOEXT).$(libGLESv2_so_VER)

all:	$(libGLESv2_so_LIB0)

$(libGLESv2_so_LIB0):	$(libGLESv2_so_LIB1)
	ln -sf $(<F) $@

$(libGLESv2_so_LIB1):	$(libGLESv2_so_LIB3)
	ln -sf $(<F) $@

$(libGLESv2_so_LIB3):	$(libGLESv2_so_OBJ)
ifeq ($(strip $(CONFIG_STATIC)),1)
	$(AR) -cq $@ $^
else
	$(CC) -shared -Wl,-soname=$(notdir $(libGLESv2_so_LIB1)) $^ $(LDFLAGS) -o $@
endif # !CONFIG_STATIC

endif # !Darwin
endif # !MINGW32

INSTALL_TARGETS := $(libGLESv2_so_LIB0) $(libGLESv2_so_LIB1) $(libGLESv2_so_LIB3)

$(P_O)/$(MODULE).dir:	
	mkdir -p $(P_O)/$(MODULE)
	touch $(P_O)/$(MODULE).dir

$(libGLESv2_so_OBJ):$(P_O)/$(MODULE)/%.o: $(P_O)/$(MODULE).dir
$(libGLESv2_so_OBJ):$(P_O)/$(MODULE)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(libGLESv2_so_OBJ) $(libGLESv2_so_LIB0) $(libGLESv2_so_LIB1) $(libGLESv2_so_LIB3)
	test ! -d $(P_O)/$(MODULE) || rmdir $(P_O)/$(MODULE)
	$(RM) $(P_O)/$(MODULE).dir

install:	all
	mkdir -p $(P_I)/pkgconfig
	sed -e 's,@INSTALL_DIR@,$(P_I),' -e 's,@GLESV2_VERSION@,$(libGLESv2_so_VER),' glesv2.pc.in > $(P_I)/pkgconfig/glesv2.pc
	$(RM) $(foreach file,$(INSTALL_TARGETS),$(P_I)/$(notdir $(file)))
	$(CP) $(INSTALL_TARGETS) $(P_I)

.PHONY: clean all install
