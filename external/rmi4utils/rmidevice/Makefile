CXX ?= g++
AR ?= ar
RANLIB ?= ranlib
CPPFLAGS += -I../include -I./include
CXXFLAGS += -fPIC -Wall
RMIDEVICESRC = rmifunction.cpp rmidevice.cpp hiddevice.cpp
RMIDEVICEOBJ = $(RMIDEVICESRC:.cpp=.o)
LIBNAME = librmidevice.so
STATIC_LIBNAME = librmidevice.a
LDFLAGS = -shared -Wl,-soname,$(LIBNAME)

all: $(LIBNAME) $(STATIC_LIBNAME)

$(LIBNAME): $(RMIDEVICEOBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(STATIC_LIBNAME): $(RMIDEVICEOBJ)
	$(AR) crv $(STATIC_LIBNAME) $^
	$(RANLIB) $(STATIC_LIBNAME)

clean:
	rm -f $(RMIDEVICEOBJ) $(LIBNAME)* $(STATIC_LIBNAME)*
