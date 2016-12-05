include $(TOPDIR)/version.mk

SUBDIRS := lib nanowrite nanoread nanoclient nanoserver

.DEFAULT_GOAL := all
.PHONE: all clean install

all clean install:
	@for x in $(SUBDIRS); do $(MAKE) -C $$x $@ || exit $?; done
