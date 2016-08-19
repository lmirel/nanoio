include $(TOPDIR)/env.mk
include $(TOPDIR)/rules.mk

BIN_NAME := nanoclient

OBJS_$(BIN_NAME) := \
		    nanoclient.o

all: $(BIN_NAME) Makefile

$(BIN_NAME): $(OBJS_$(BIN_NAME)) $(TOPDIR)/src/lib/$(LIB_NAME).so
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	@$(RM) $(OBJS_$(BIN_NAME)) $(BIN_NAME)

install: $(BIN_NAME)
	$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
	$(INSTALL) -m 700 $(BIN_NAME) $(DESTDIR)$(bindir)
