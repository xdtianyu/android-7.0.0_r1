# Default options
USE_BSDIFF ?= y

BINARIES-y = bspatch
BINARIES-$(USE_BSDIFF) += bsdiff

BINARIES += $(BINARIES-y)

INSTALL = install
CFLAGS += -O3 -Wall -Werror
CXXFLAGS += -std=c++11

DESTDIR ?=
PREFIX = /usr
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share
MANDIR = $(DATADIR)/man
MAN1DIR = $(MANDIR)/man1
INSTALL_PROGRAM ?= $(INSTALL) -c -m 755
INSTALL_MAN ?= $(INSTALL) -c -m 444

.PHONY: all test clean
all: $(BINARIES)
test: unittests
clean:
	rm -f *.o $(BINARIES) unittests

BSDIFF_LIBS = -lbz2 -ldivsufsort -ldivsufsort64
BSDIFF_OBJS = \
  bsdiff.o

BSPATCH_LIBS = -lbz2
BSPATCH_OBJS = \
  bspatch.o \
  extents.o \
  extents_file.o \
  file.o

UNITTEST_LIBS = -lgmock -lgtest
UNITTEST_OBJS = \
  bsdiff_unittest.o \
  extents_file_unittest.o \
  extents_unittest.o \
  test_utils.o \
  testrunner.o

bsdiff: $(BSDIFF_OBJS) bsdiff_main.o
bsdiff: LDLIBS += $(BSDIFF_LIBS)

bspatch: $(BSPATCH_OBJS) bspatch_main.o
bspatch: LDLIBS += $(BSPATCH_LIBS)

unittests: LDLIBS += $(BSDIFF_LIBS) $(BSPATCH_LIBS) $(UNITTEST_LIBS)
unittests: $(BSPATCH_OBJS) $(BSDIFF_OBJS) $(UNITTEST_OBJS)

unittests bsdiff bspatch:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

# Source file dependencies.
bsdiff.o: bsdiff.cc
bsdiff_main.o: bsdiff_main.cc bsdiff.h
bsdiff_unittest.o: bsdiff_unittest.cc bsdiff.h test_utils.h
bspatch.o: bspatch.cc bspatch.h extents.h extents_file.h file_interface.h \
 file.h
bspatch_main.o: bspatch_main.cc bspatch.h
extents.o: extents.cc extents.h extents_file.h file_interface.h
extents_file.o: extents_file.cc extents_file.h file_interface.h
extents_file_unittest.o: extents_file_unittest.cc extents_file.h \
 file_interface.h
extents_unittest.o: extents_unittest.cc extents.h extents_file.h \
 file_interface.h
file.o: file.cc file.h file_interface.h
testrunner.o: testrunner.cc
test_utils.o: test_utils.cc test_utils.h

install:
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(MAN1DIR)
	$(INSTALL_PROGRAM) $(BINARIES) $(DESTDIR)$(BINDIR)
ifndef WITHOUT_MAN
	$(INSTALL_MAN) $(BINARIES:=.1) $(DESTDIR)$(MAN1DIR)
endif
