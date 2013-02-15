include $(T)/common.mak

SUBDIRS=EGL GLES GLES2

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
	$(MAKE) -C $(1) all O=../$(P_O) T=../$(P_T)
$(1)-clean:
	$(MAKE) -C $(1) clean O=../$(P_O) T=../$(P_T)
$(1)-distclean:
	$(MAKE) -C $(1) distclean O=../$(P_O) T=../$(P_T)
$(1)-depend:
	$(MAKE) -C $(1) depend O=../$(P_O) T=../$(P_T)
$(1)-install:
	$(MAKE) -C $(1) install O=../$(P_O) T=../$(P_T) I=$(P_I)/lib/$(CONFIG_LIB_SUBDIR)
endef

$(foreach SUBDIR,$(SUBDIRS),$(eval $(call SUBDIR_template,$(SUBDIR))))
