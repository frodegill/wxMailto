#
# Based on Makefile from <URL: http://hak5.org/forums/index.php?showtopic=2077&p=27959 >

PROGRAM = wxMailto

############# Main application #################
all:    $(PROGRAM)
.PHONY: all

# source files
PCH = pch.h
WXWIDGETS_VERSION = 3.0
DEBUG_INFO = YES
SOURCES = $(shell find . -name '*.cpp')
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.dep)

######## compiler- and linker settings #########
WX_CONFIG := wx-config --version=$(WXWIDGETS_VERSION)
CXX = $(shell $(WX_CONFIG) --cxx)
CFLAGS = -DHAVE_INTTYPES_H $(shell pkg-config gnutls --cflags) $(shell gpgme-config --thread=pthread --cflags) $(shell pkg-config libidn --cflags) $(shell pkg-config libgsasl --cflags)
CXXFLAGS = $(shell $(WX_CONFIG) --cxxflags)
CPPFLAGS += -W -Wall -Werror -pipe
LIBSFLAGS = $(shell $(WX_CONFIG) --libs std) $(shell pkg-config gnutls --libs) -lgnutlsxx $(shell gpgme-config --thread=pthread --libs) $(shell pkg-config libidn --libs) -lmimetic $(shell pkg-config libgsasl --libs) -lutil
ifdef PCH
 CPPFLAGS += -DWX_PRECOMP
endif
ifdef DEBUG_INFO
 CPPFLAGS += -g
 LIBSFLAGS += -lPocoODBCd -lPocoMySQLd -lPocoDatad -lPocoFoundationd
else
 CPPFLAGS += -O
 LIBSFLAGS += -lPocoODBC -lPocoMySQL -lPocoData -lPocoFoundation
endif

ifdef PCH
%.o: %.cpp $(PCH).gch
else
%.o: %.cpp
endif
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CFLAGS) -o $@ -c $<

%.dep: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CFLAGS) -MM $< -MT $(<:.cpp=.o) > $@

############ Precompiled header ################
ifdef PCH
$(PCH).gch: $(PCH)
	$(CXX) -x c++ -c $(PCH) -o $(PCH).gch $(CXXFLAGS) $(CPPFLAGS) $(CFLAGS)
endif

############# Main application #################
$(PROGRAM):	$(OBJECTS) $(DEPS)
	$(CXX) -o $@ $(OBJECTS) $(LIBSFLAGS)

################ Dependencies ##################
ifneq ($(MAKECMDGOALS),clean)
include $(DEPS)
endif

################### Clean ######################
clean:
	find . -name '*~' -delete
	-rm -f $(PROGRAM) $(OBJECTS) $(DEPS)
ifdef PCH
	-rm -f $(PCH).gch
endif
