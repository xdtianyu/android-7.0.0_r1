CXX ?= g++
CPPFLAGS += -I../include -I./include -I../rmidevice
CXXFLAGS += -Wall
LDFLAGS += -L.
LIBS =  -lrmidevice -lrt
LIBDIR = ../rmidevice
LIBNAME = librmidevice.a
RMI4UPDATESRC = main.cpp firmware_image.cpp rmi4update.cpp updateutil.cpp
RMI4UPDATEOBJ = $(RMI4UPDATESRC:.cpp=.o)
PROGNAME = rmi4update
STATIC_BUILD ?= y
ifeq ($(STATIC_BUILD),y)
LDFLAGS += -static
endif

all: $(PROGNAME)

$(PROGNAME): $(RMI4UPDATEOBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(RMI4UPDATEOBJ) -L$(LIBDIR) $(LIBS) -o $(PROGNAME)

clean:
	rm -f $(RMI4UPDATEOBJ) $(PROGNAME)
