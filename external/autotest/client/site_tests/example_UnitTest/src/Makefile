# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

TARGET = unit_test
OBJECTS = main.o
INCLUDES = -I$(GCLIENT_ROOT)/src/common
OPTIONS = -fprofile-arcs -ftest-coverage -fno-inline -O0
LIBS = -lgtest
PACKAGES = 

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OPTIONS) $(LDFLAGS) $(PACKAGES) $(OBJECTS) $(LIBS) \
			-o $(TARGET)
	
-include $(OBJS:.o=.d)

.cc.o:
	$(CXX) $(CXXFLAGS) $(PACKAGES) $(OPTIONS) $(INCLUDES) -c $< -o $@
	$(CXX) -MM $(CXXFLAGS) $(PACKAGES) $(OPTIONS) $(INCLUDES) $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -l | \
			sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp
	
clean:
	$(RM) *.o *.d $(TARGET)
	
