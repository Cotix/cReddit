# CREDDIT MAKEFILE
#
# To compile the creddit executable either use 'make', or 'make creddit'
# To compile libreddit as a static library only, use 'make libreddit'
#
# You can also compile individual parts or individual files on their own, by
# specifying their object name.
# Ex. 'make build/src/main.o'

QUIETLY:=@

# Set some basic program-wide compile settings
# In the future these shouldn't be assumed.
CC:=gcc
PROJCFLAGS:=-O2 -Wall -I'./include'
LD:=ld
AR:=ar
INSTALL:=install
OBJCOPY:=objcopy
BUILD_DIR:=build

MKDIR:=mkdir -p
ECHO:=echo
RM:=rm

ifndef PREFIX
	PREFIX=/usr
endif

all: real-all

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
	$(QUIETLY)$(ECHO) " MKDIR $(BUILD_DIR)"
	$(QUIETLY)$(MKDIR) $(BUILD_DIR)

# Calls all the clean targets to be run, so we can clean up the build directory
clean: $(CLEAN_TARGETS)

# Top level clean -- deletes whole build directory
build_clean:
	$(QUIETLY)$(ECHO) " RM $(BUILD_DIR)"
	$(QUIETLY)$(RM) -fr $(BUILD_DIR)

