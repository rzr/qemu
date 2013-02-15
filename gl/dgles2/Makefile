# Copyright (C) 2010-2011  Nokia Corporation All Rights Reserved.
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

export OS = $(shell uname -s | cut -d_ -f1)

CONFIG := config-host
CONFIGS := $(CONFIG).mak $(CONFIG).h
-include $(CONFIG).mak
export ARCH = $(CONFIG_ARCH)

ifeq ($(strip $(OS)),MINGW32)
	CFLAGS += -mthreads
	LDFLAGS += -mthreads
	SOEXT = dll
	CP = cp -a
else
ifeq ($(strip $(OS)),Linux)
	SOEXT = so
	CP = cp -a
else
ifeq ($(strip $(OS)),Darwin)
	SOEXT = dylib
	CP = cp -pPR
else
$(error "Unknown OS!")
endif
endif
endif

ifeq ($(strip $(CONFIG_STATIC)),1)
	SOEXT = a
endif
export SOEXT CP

CFLAGS  += -DARCH=$(ARCH) -DCONFIG_H="\"../../config-host.h\""

ifeq ($(strip $(ARCH)),x86_64)
	CFLAGS += -m64
	LDFLAGS += -m64
else
	CFLAGS += -m32
	LDFLAGS += -m32
endif

ifeq ($(strip $(CONFIG_DEBUG)),1)
	CFLAGS += -g3 -O0
else
	CFLAGS += -g -O2
endif

export CFLAGS LDFLAGS

O := $(O)/objs-$(ARCH)
T := .
I = $(CONFIG_PREFIX)

include $(T)/common.mak

# Pathify.
P_O := $(call pathify,$(O))
P_S := $(call pathify,$(S))
P_T := $(call pathify,$(T))
P_I := $(call pathify,$(I))

all: $(CONFIGS)
	mkdir -p $(P_O)
	$(MAKE) -C $(P_S)/include all O=../$(P_O) T=../$(P_T)
	$(MAKE) -C $(P_S)/src all O=../$(P_O) T=../$(P_T)
clean: $(CONFIGS)
	$(MAKE) -C $(P_S)/include clean O=../$(P_O) T=../$(P_T)
	$(MAKE) -C $(P_S)/src clean O=../$(P_O) T=../$(P_T)
	test ! -d $(P_O) || rmdir $(P_O)
distclean:
	$(RM) -r objs-*
	$(RM) $(CONFIGS)
install: $(CONFIGS)
	$(MAKE) -C $(P_S)/include install O=../$(P_O) T=../$(P_T) I=$(P_I)
	$(MAKE) -C $(P_S)/src install O=../$(P_O) T=../$(P_T) I=$(P_I)
