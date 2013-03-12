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

SUBDIRS=EGL GLES GLES2 KHR

# Pathify.
P_O := $(call pathify,$(O))
P_S := $(call pathify,$(S))
P_T := $(call pathify,$(T))
P_I := $(call pathify,$(I))

all:	$(addsuffix -all, $(SUBDIRS))
clean:	$(addsuffix -clean, $(SUBDIRS))
distclean:	$(addsuffix -distclean, $(SUBDIRS))
depend:	$(addsuffix -depend, $(SUBDIRS))
install:	$(addsuffix -install, $(SUBDIRS))

define SUBDIR_template
$(1)-all:
	make -C $(1) all O=../$(P_O) T=../$(P_T)
$(1)-clean:
	make -C $(1) clean O=../$(P_O) T=../$(P_T)
$(1)-distclean:
	make -C $(1) distclean O=../$(P_O) T=../$(P_T)
$(1)-depend:
	make -C $(1) depend O=../$(P_O) T=../$(P_T)
$(1)-install:
	make -C $(1) install O=../$(P_O) I=$(P_I) T=../$(P_T)
endef

$(foreach SUBDIR,$(SUBDIRS),$(eval $(call SUBDIR_template,$(SUBDIR))))
