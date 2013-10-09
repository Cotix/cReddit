# CREDDIT MAKEFILE
#
# Normal usage: make; make install
#
# 'make' will default to compiling creddit in ./build/creddit and libreddit as
# ./build/libreddit.so (shared object)
# To compile libreddit as a shared library separately, use 'make libreddit'
#
# To compile libreddit as a static library, compile with STATIC=y
# Ex. make STATIC=y
#
# You can also compile individual parts or individual files on their own, by
# specifying their object name.
# Ex. 'make build/src/main.o'

QUIETLY:=@

# Set some basic program-wide compile settings
# In the future these shouldn't be assumed.
CC:=$(QUIETLY)gcc
PROJCFLAGS:=-O2 -Wall -I'./include'
PROJLDFLAGS:=-fvisibility=hidden
LD:=$(QUIETLY)ld
AR:=$(QUIETLY)ar
INSTALL:=$(QUIETLY)install
OBJCOPY:=$(QUIETLY)objcopy
BUILD_DIR:=build

MKDIR:=$(QUIETLY)mkdir -p
ECHO:=$(QUIETLY)echo
RM:=$(QUIETLY)rm

ifndef PREFIX
	PREFIX=/usr
endif

all: real-all

include ./common.mk

# Set initial values for 'targets'
CLEAN_TARGETS:=
COMPILE_TARGETS:=
INSTALL_TARGETS:=

# Include mk files from any subdirectories
# These files add onto the 'targets'
include ./libreddit/libreddit.mk
include ./src/creddit.mk

# Add a few ending values for the main program
CLEAN_TARGETS +=build_clean

.PHONY: all real-all install clean $(CLEAN_TARGETS) $(INSTALL_TARGETS)

real-all: $(EXECUTABLE_FULL)

install: $(INSTALL_TARGETS)

$(BUILD_DIR):
	$(ECHO) " MKDIR $(BUILD_DIR)"
	$(MKDIR) $(BUILD_DIR)

# Calls all the clean targets to be run, so we can clean up the build directory
clean: $(CLEAN_TARGETS)

# Top level clean -- deletes whole build directory
build_clean:
	$(ECHO) " RM $(BUILD_DIR)"
	$(RM) -fr $(BUILD_DIR)

