
# Set some basic program-wide compile settings
# In the future these shouldn't be assumed.
CC:=gcc
PROJCFLAGS:=-O2 -Wall -I'./include'
LD:=ld
AR:=ar
INSTALL:=install
BUILD_DIR:=build
EXECUTABLE_NAME:=creddit
EXECUTABLE_FULL:=$(BUILD_DIR)/$(EXECUTABLE_NAME)

MKDIR:=@mkdir -p
ECHO:=@echo
RM:=@rm

LDFLAGS:=-lc -lncurses `curl-config --cflags` `curl-config --libs` -L$(BUILD_DIR) -lreddit

SOURCES:=main.c
OBJECTS:=$(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

ifndef PREFIX
	PREFIX=/usr
endif

all: real-all

# Set initial values for 'targets'
CLEAN_TARGETS:=
COMPILE_TARGETS:=

# Include mk files from any subdirectories
# These files add onto the 'targets'
include ./libreddit/libreddit.mk

# Add a few ending values for the main program
CLEAN_TARGETS +=build_clean

.SILENT:
.PHONY: all real-all install clean $(CLEAN_TARGETS)

real-all: $(EXECUTABLE_FULL)

# Compiles the cReddit executable via linking together all the object files
# and archives
$(EXECUTABLE_FULL): $(COMPILE_TARGETS) $(OBJECTS)
	$(ECHO) " CC $(EXECUTABLE_FULL)"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE_FULL)

install: $(EXECUTABLE_FULL)
	$(ECHO) " INSTALL $(EXECUTABLE_FULL)"
	$(INSTALL) -m 0755 $(EXECUTABLE_FULL) $(PREFIX)/bin/


$(BUILD_DIR):
	$(ECHO) " MKDIR $(BUILD_DIR)"
	$(MKDIR) $(BUILD_DIR)

# Builds top-level c files -- currently just main.c
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(ECHO) " CC $<"
	$(CC) $(PROJCFLAGS) -c $< -o $@

# Calls all the clean targets to be run, so we can clean up the build directory
clean: $(CLEAN_TARGETS)

# Top level clean -- deletes whole build directory
build_clean:
	$(ECHO) " RM $(BUILD_DIR)"
	$(RM) -fr $(BUILD_DIR)

