#------------------------------------------------------------------------------#
.PHONY = all meta clean showsetup

#------------------------------------------------------------------------------#
LIBRARY_NAME = n_lib
LIBRARY_AUTHOR = "Nioelumijke"
LIBRARY_DESCRIPTION = "Various Pure Data object"
LIBRARY_LICENSE = "GPL v3"
LIBRARY_VERSION = "0.3"
META_FILE = $(LIBRARY_NAME)-meta.pd

#
SOURCESDIR = .
SOURCES = \
$(SOURCESDIR)/n_array.c \
$(SOURCESDIR)/n_canvas.c \
$(SOURCESDIR)/n_scope~.c \
$(SOURCESDIR)/n_spectr~.c \
$(SOURCESDIR)/n_life.c
LIBS =

#------------------------------------------------------------------------------#
CFLAGS = -DPD -I"$(PD_INCLUDE)" -Wall -W
CFLAGS += -DVERSION='"$(LIBRARY_VERSION)"'
LDFLAGS =

#------------------------------------------------------------------------------#
UNAME := $(shell uname -s)
# Darwin ----------------------------------------------------------------------#
ifeq ($(UNAME),Darwin)
	CPU := $(shell uname -p)
	# iPhone/iPod Touch ---------------------------------------------------#
	ifeq ($(CPU),arm)
		OS = iphoneos
		EXTENSION = pd_darwin
		PD_PATH ?= /Applications/Pd-extended.app/Contents/Resources
		PD_INCLUDE = $(PD_PATH)/include
		IPHONE_BASE=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin
		CC=$(IPHONE_BASE)/gcc
		CPP=$(IPHONE_BASE)/cpp
		CXX=$(IPHONE_BASE)/g++
		ISYSROOT = -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS3.0.sdk
		IPHONE_CFLAGS = -miphoneos-version-min=3.0 $(ISYSROOT) -arch armv6
		OPT_CFLAGS = -fast -funroll-loops -fomit-frame-pointer
		CFLAGS := $(IPHONE_CFLAGS) $(OPT_CFLAGS) $(CFLAGS)
		LDFLAGS += -arch armv6 -bundle -undefined dynamic_lookup $(ISYSROOT)
		LIBS += -lc
		STRIP = strip -x
	# Mac OS X ------------------------------------------------------------#
	else
		OS = macosx
		EXTENSION = pd_darwin
		PD_PATH ?= /Applications/Pd-extended.app/Contents/Resources
		PD_INCLUDE = $(PD_PATH)/include
		OPT_CFLAGS = -ftree-vectorize -ftree-vectorizer-verbose=2 -fast
		# build universal 32-bit on 10.4 and 32/64 on newer
		ifeq ($(shell uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|'), 8)
			FAT_FLAGS = -arch ppc -arch i386 -mmacosx-version-min=10.4
		else
			FAT_FLAGS = -arch ppc -arch i386 -arch x86_64 -mmacosx-version-min=10.4
			SOURCES += $(SOURCES_iphoneos)
		endif
		CFLAGS += $(FAT_FLAGS) -fPIC -I/sw/include
		LDFLAGS += $(FAT_FLAGS) -bundle -undefined dynamic_lookup -L/sw/lib
		# if the 'pd' binary exists, check the linking against it to aid with stripping
		LDFLAGS += $(shell test -e $(PD_PATH)/bin/pd && echo -bundle_loader $(PD_PATH)/bin/pd)
		LIBS += -lc
		STRIP = strip -x
	endif
endif
# Linux -----------------------------------------------------------------------#
ifeq ($(UNAME),Linux)
	OS = linux
	EXTENSION = pd_linux
	CPU := $(shell uname -m)
	PD_PATH ?= /usr
	PD_INCLUDE = $(PD_PATH)/include
	OPT_CFLAGS = -O5 -funroll-loops -fomit-frame-pointer
	CFLAGS += -fPIC
	LDFLAGS += -Wl,--export-dynamic  -shared -fPIC
	LIBS += -lc
	STRIP = strip --strip-unneeded -R .note -R .comment
endif
# Cygwin ----------------------------------------------------------------------#
ifeq (CYGWIN,$(findstring CYGWIN,$(UNAME)))
	OS = cygwin
	EXTENSION = dll
	CPU := $(shell uname -m)
	PD_PATH ?= $(cygpath $(PROGRAMFILES))/pd
	PD_INCLUDE = $(PD_PATH)/include
	OPT_CFLAGS = -O6 -funroll-loops -fomit-frame-pointer
	CFLAGS +=
	LDFLAGS += -Wl,--export-dynamic -shared -L"$(PD_PATH)/src" -L"$(PD_PATH)/bin"
	LIBS += -lc -lpd
	STRIP = strip --strip-unneeded -R .note -R .comment
endif
# Windows ---------------------------------------------------------------------#
ifeq (MINGW,$(findstring MINGW,$(UNAME)))
	OS = windows
	EXTENSION = dll
	CPU := $(shell uname -m)
	PD_PATH ?= $(shell cd "$(PROGRAMFILES)"/pd && pwd)
	PD_INCLUDE = $(PD_PATH)/include
	OPT_CFLAGS = -O3 -funroll-loops -fomit-frame-pointer
	CFLAGS += -mms-bitfields
	LDFLAGS += -s -shared -Wl,--enable-auto-import
	LIBS += -L"$(PD_PATH)/src" -L"$(PD_PATH)/bin" -L"$(PD_PATH)/obj" -lpd -lwsock32 -lkernel32 -luser32 -lgdi32
	STRIP = strip --strip-unneeded -R .note -R .comment
endif

#------------------------------------------------------------------------------#
ALL_CFLAGS := $(ALL_CFLAGS) $(CFLAGS) $(OPT_CFLAGS)
ALL_LDFLAGS := $(LDFLAGS) $(ALL_LDFLAGS)
ALL_LIBS := $(LIBS) $(ALL_LIBS)

#------------------------------------------------------------------------------#
all: $(SOURCES:.c=.$(EXTENSION))
	@echo "done"

#
$(SOURCES:.c=.$(EXTENSION)): %.$(EXTENSION): %.o
	$(CC) $(ALL_LDFLAGS) -o $@  $<  $(ALL_LIBS)
	chmod a-x $@

# obj
%.o: %.c
	$(CC) $(ALL_CFLAGS) -o "$*.o" -c "$*.c"

#------------------------------------------------------------------------------#
meta:
	echo "#N canvas 100 100 360 360 10;" > $(META_FILE)
	echo "#X text 10 10 META this is prototype of a libdir meta file;" >> $(META_FILE)
	echo "#X text 10 30 NAME" $(LIBRARY_NAME) ";" >> $(META_FILE)
	echo "#X text 10 50 AUTHOR" $(LIBRARY_AUTHOR) ";" >> $(META_FILE)
	echo "#X text 10 70 DESCRIPTION" $(LIBRARY_DESCRIPTION) ";" >> $(META_FILE)
	echo "#X text 10 90 LICENSE" $(LIBRARY_LICENSE) ";" >> $(META_FILE)
	echo "#X text 10 110 VERSION" $(LIBRARY_VERSION) ";" >> $(META_FILE)
	@echo "meta done"

#------------------------------------------------------------------------------#
clean:
	-rm -f -- $(SOURCES:.c=.o)
	-rm -f -- $(SOURCES:.c=.$(EXTENSION))
	@echo "clean done"

#------------------------------------------------------------------------------#
showsetup:
	@echo "UNAME               : $(UNAME)"
	@echo "OS                  : $(OS)"
	@echo "CPU                 : $(CPU)"
	@echo "EXTENSION           : $(EXTENSION)"
	@echo "PD_PATH             : $(PD_PATH)"
	@echo "PD_INCLUDE          : $(PD_INCLUDE)"
	@echo "CFLAGS              : $(CFLAGS)"
	@echo "ALL_CFLAGS          : $(ALL_CFLAGS)"
	@echo "LDFLAGS             : $(LDFLAGS)"
	@echo "ALL_LDFLAGS         : $(ALL_LDFLAGS)"
	@echo "LIBS                : $(LIBS)"
	@echo "ALL_LIBS            : $(ALL_LIBS)"
	@echo "LIBRARY_NAME        : $(LIBRARY_NAME)"
	@echo "LIBRARY_AUTHOR      : $(LIBRARY_AUTHOR)"
	@echo "LIBRARY_DESCRIPTION : $(LIBRARY_DESCRIPTION)"
	@echo "LIBRARY_LICENSE     : $(LIBRARY_LICENSE)"
	@echo "LIBRARY_VERSION     : $(LIBRARY_VERSION)"
	@echo "ALLSOURCES          : $(SOURCES)"
