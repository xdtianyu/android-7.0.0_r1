-include .config

MAKEFLAGS += --no-print-directory

PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

MKDIR ?= mkdir -p
INSTALL ?= install
CC ?= "gcc"
PKG_CONFIG ?= pkg-config

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration

OBJS = iwcap.o
ALL = iwcap

NL1FOUND := $(shell $(PKG_CONFIG) --atleast-version=1 libnl-1 && echo Y)
NL2FOUND := $(shell $(PKG_CONFIG) --atleast-version=2 libnl-2.0 && echo Y)

ifeq ($(NL1FOUND),Y)
NLLIBNAME = libnl-1
endif

ifeq ($(NL2FOUND),Y)
CFLAGS += -DCONFIG_LIBNL20
LIBS += -lnl-genl
NLLIBNAME = libnl-2.0
endif

ifeq ($(NLLIBNAME),)
$(error Cannot find development files for any supported version of libnl)
endif

LIBS += $(shell $(PKG_CONFIG) --libs $(NLLIBNAME))
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(NLLIBNAME))

ifeq ($(V),1)
Q=
NQ=true
else
Q=@
NQ=echo
endif

all: version_check $(ALL)

version_check:
ifeq ($(NL2FOUND),Y)
else
ifeq ($(NL1FOUND),Y)
else
	$(error No libnl found)
endif
endif


%.o: %.c nl80211.h
	@$(NQ) ' CC  ' $@
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

iwcap:	$(OBJS)
	@$(NQ) ' CC  ' iwcap
	$(Q)$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o iwcap

check:
	$(Q)$(MAKE) all CC="REAL_CC=$(CC) CHECK=\"sparse -Wall\" cgcc"

%.gz: %
	@$(NQ) ' GZIP' $<
	$(Q)gzip < $< > $@

install: iwcap
	@$(NQ) ' INST iwcap'
	$(Q)$(MKDIR) $(DESTDIR)$(BINDIR)
	$(Q)$(INSTALL) -m 755 -t $(DESTDIR)$(BINDIR) iwcap

clean:
	$(Q)rm -f iwcap *.o *~ *.gz version.c *-stamp
