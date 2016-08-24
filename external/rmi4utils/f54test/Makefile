CXX ?= g++
CPPFLAGS += -I../include -I./include -I../rmidevice
CXXFLAGS += -Wall
LDFLAGS += -L.
LIBS =  -lrmidevice -lrt
LIBDIR = ../rmidevice
LIBNAME = librmidevice.a
F54TESTSRC = main.cpp f54test.cpp testutil.cpp display.cpp
F54TESTOBJ = $(F54TESTSRC:.cpp=.o)
PROGNAME = f54test
STATIC_BUILD ?= y
ifeq ($(STATIC_BUILD),y)
LDFLAGS += -static
endif

all: $(PROGNAME)

$(PROGNAME): $(F54TESTOBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(F54TESTOBJ) -L$(LIBDIR) $(LIBS) -o $(PROGNAME)

clean:
	rm -f $(F54TESTOBJ) $(PROGNAME)
