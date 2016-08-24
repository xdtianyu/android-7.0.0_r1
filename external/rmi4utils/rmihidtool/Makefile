CXX ?= g++
CPPFLAGS += -I../include -I./include -I../rmidevice
CXXFLAGS += -Wall
LDFLAGS += -L.
LIBS =  -lrmidevice
LIBDIR = ../rmidevice
LIBNAME = librmidevice.a
RMIHIDTOOLSRC = main.cpp
RMIHIDTOOLOBJ = $(RMIHIDTOOLSRC:.cpp=.o)
PROGNAME = rmihidtool
STATIC_BUILD ?= y
ifeq ($(STATIC_BUILD),y)
LDFLAGS += -static
endif

all: $(PROGNAME)

$(PROGNAME): $(RMIHIDTOOLOBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(RMIHIDTOOLOBJ) -L$(LIBDIR) $(LIBS) -o $(PROGNAME)

clean:
	rm -f $(RMIHIDTOOLOBJ) $(PROGNAME)
