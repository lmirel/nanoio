include $(TOPDIR)/version.mk

LIB_NAME := libnanoio

LDFLAGS +=
CFLAGS += -DNANOIO_VERSION=\"$(NANOIO_VERSION)\"

.DEFAULT_GOAL := all
.PHONE: all clean install
