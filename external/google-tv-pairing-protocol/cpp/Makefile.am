## Process this file with automake to produce Makefile.in

# Make sure that when we re-make ./configure, we get the macros we need
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = .

DIST_SUBDIRS = src

# This is so we can #include <package/foo>
AM_CPPFLAGS = -I$(top_srcdir)/src \
  -I$(top_srcdir)/tests \
  -I$(GTEST_DIR) -I$(GTEST_DIR)/include \
  -I$(GMOCK_DIR) -I$(GMOCK_DIR)/include

# These are good warnings to turn on by default
if GCC
AM_CXXFLAGS = -Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare
endif

# The -no-undefined flag allows libtool to generate shared libraries for
# Cygwin and MinGW.  LIBSTDCXX_LA_LINKER_FLAG is used to fix a Solaris bug.
# -version-info tells libtool what to put after the '.so' in the filename.
AM_LDFLAGS = -no-undefined $(LIBSTDCXX_LA_LINKER_FLAG) \
             -version-info @SO_VERSION@

# The libraries (.so's) to build and install.
lib_LTLIBRARIES = libpolo.la

# Libraries that are built but not installed.
noinst_LTLIBRARIES = libgtest.la libgmock.la

## The .h files you want to install (that is, .h files that people
## who install this package can include in their own applications.)

polo_encoding_includedir = $(includedir)/polo/encoding
polo_encoding_include_HEADERS = \
  src/polo/encoding/encodingoption.h \
  src/polo/encoding/hexadecimalencoder.h \
  src/polo/encoding/secretencoder.h

polo_pairing_includedir = $(includedir)/polo/pairing
polo_pairing_include_HEADERS = \
  src/polo/pairing/clientpairingsession.h \
  src/polo/pairing/pairingcontext.h \
  src/polo/pairing/pairinglistener.h \
  src/polo/pairing/pairingsession.h \
  src/polo/pairing/polochallengeresponse.h \
  src/polo/pairing/poloerror.h \
  src/polo/pairing/serverpairingsession.h

polo_pairing_message_includedir = $(includedir)/polo/pairing/message
polo_pairing_message_include_HEADERS = \
  src/polo/pairing/message/configurationackmessage.h \
  src/polo/pairing/message/configurationmessage.h \
  src/polo/pairing/message/messagelistener.h \
  src/polo/pairing/message/optionsmessage.h \
  src/polo/pairing/message/pairingrequestackmessage.h \
  src/polo/pairing/message/pairingrequestmessage.h \
  src/polo/pairing/message/polomessage.h \
  src/polo/pairing/message/secretackmessage.h \
  src/polo/pairing/message/secretmessage.h

polo_util_includedir = $(includedir)/polo/util
polo_util_include_HEADERS = \
  src/polo/util/certificateutil.h \
  src/polo/util/macros.h \
  src/polo/util/poloutil.h

polo_wire_includedir = $(includedir)/polo/wire
polo_wire_include_HEADERS = \
  src/polo/wire/polowireadapter.h \
  src/polo/wire/polowireinterface.h \
  src/polo/wire/polowirelistener.h

polo_wire_protobuf_includedir = $(includedir)/polo/wire/protobuf
polo_wire_protobuf_include_HEADERS = \
  src/polo/wire/protobuf/polo.pb.h \
  src/polo/wire/protobuf/protobufwireadapter.h

docdir = $(prefix)/share/doc/$(PACKAGE)-$(VERSION)
dist_doc_DATA = AUTHORS COPYING INSTALL ChangeLog README NEWS

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = polo.pc

## unittests to run when people type 'make check'.
TESTS = polo-test
TESTS_ENVIRONMENT =
check_SCRIPTS =
# Every time you add a unittest to check_SCRIPTS, add it here too
noinst_SCRIPTS =

## vvvv RULES TO MAKE THE LIBRARIES, BINARIES, AND UNITTESTS

protoc_inputs = \
  ../proto/polo.proto

protoc_outputs = \
  src/polo/wire/protobuf/polo.pb.cc \
  src/polo/wire/protobuf/polo.pb.h

BUILT_SOURCES = $(protoc_outputs)

protoc_gen: $(protoc_inputs)
	$(PROTOC) --proto_path=../proto \
	    --cpp_out=$(srcdir)/src/polo/wire/protobuf \
            $(protoc_inputs)
	touch protoc_gen

$(protoc_outputs): protoc_gen

libpolo_la_CXXFLAGS = $(PROTOBUF_CFLAGS) $(GLOG_CFLAGS) $(SSL_CFLAGS) \
  $(CRYPTO_CFLAGS)

libpolo_la_LIBADD = $(PROTOBUF_LIBS) $(GLOG_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS)
libpolo_la_SOURCES = \
  src/polo/encoding/encodingoption.cc \
  src/polo/encoding/hexadecimalencoder.cc \
  src/polo/pairing/clientpairingsession.cc \
  src/polo/pairing/pairingcontext.cc \
  src/polo/pairing/pairingsession.cc \
  src/polo/pairing/polochallengeresponse.cc \
  src/polo/pairing/serverpairingsession.cc \
  src/polo/pairing/message/configurationackmessage.cc \
  src/polo/pairing/message/configurationmessage.cc \
  src/polo/pairing/message/optionsmessage.cc \
  src/polo/pairing/message/pairingrequestackmessage.cc \
  src/polo/pairing/message/pairingrequestmessage.cc \
  src/polo/pairing/message/polomessage.cc \
  src/polo/pairing/message/secretackmessage.cc \
  src/polo/pairing/message/secretmessage.cc \
  src/polo/util/certificateutil.cc \
  src/polo/util/poloutil.cc \
  src/polo/wire/polowireadapter.cc \
  src/polo/wire/protobuf/polo.pb.cc \
  src/polo/wire/protobuf/protobufwireadapter.cc

polo_test_LDADD = libpolo.la libgtest.la libgmock.la

polo_test_SOURCES = \
  tests/polo/polotests.cc \
  tests/polo/encoding/hexadecimalencodertest.cc \
  tests/polo/pairing/clientpairingsessiontest.cc \
  tests/polo/pairing/pairingsessiontest.cc \
  tests/polo/pairing/polochallengeresponsetest.cc \
  tests/polo/pairing/serverpairingsessiontest.cc \
  tests/polo/util/certificateutiltest.cc \
  tests/polo/util/poloutiltest.cc \
  tests/polo/wire/protobuf/protobufwireadaptertest.cc

libgtest_la_SOURCES = $(GTEST_DIR)/src/gtest-all.cc

libgmock_la_SOURCES = $(GMOCK_DIR)/src/gmock-all.cc

## ^^^^ END OF RULES TO MAKE THE LIBRARIES, BINARIES, AND UNITTESTS


## This should always include $(TESTS), but may also include other
## binaries that you compile but don't want automatically installed.
noinst_PROGRAMS = $(TESTS)

rpm: dist-gzip packages/rpm.sh packages/rpm/rpm.spec
	@cd packages && ./rpm.sh ${PACKAGE} ${VERSION}

deb: dist-gzip packages/deb.sh packages/deb/*
	@cd packages && ./deb.sh ${PACKAGE} ${VERSION}

# Windows wants write permission to .vcproj files and maybe even sln files.
dist-hook:
	test ! -e "$(distdir)/vsprojects" \
	   || chmod -R u+w $(distdir)/*.sln $(distdir)/vsprojects/

EXTRA_DIST = packages/rpm.sh packages/rpm/rpm.spec packages/deb.sh packages/deb \
	autogen.sh $(SCRIPTS) src/solaris/libstdc++.la

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

EXTRA_DIST += libtool

CLEANFILES = $(protoc_ouputs) protoc_gen

