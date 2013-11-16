# Add libreddit's targets
CLEAN_TARGETS +=libreddit_clean

# Current version of libreddit
LIBREDDIT_VERSION:=0.0.1

# libreddit currently just compiles with the default settings
LIBREDDIT_CFLAGS :=$(PROJCFLAGS) -fvisibility=hidden -DLIBREDDIT_VERSION=$(LIBREDDIT_VERSION)
LIBREDDIT_LDFLAGS :=`curl-config --cflags` `curl-config --libs`

# The directory to store the object files in
LIBREDDIT_DIR :=libreddit
LIBREDDIT_CMP_DIR :=$(BUILD_DIR)/$(LIBREDDIT_DIR)

# if you define SHARED before running make, then LIBREDDIT will be compiled
# as a shared .so library, instead of a static library

ifndef STATIC
    LIBREDDIT_CFLAGS +=-fPIC
    LIBREDDIT_CMP := $(BUILD_DIR)/libreddit.so
else
    LIBREDDIT_CMP := $(BUILD_DIR)/libreddit.a
endif

LIBREDDIT_COMBINED := $(LIBREDDIT_CMP_DIR)/all.o

COMPILE_TARGETS += $(LIBREDDIT_CMP)
INSTALL_TARGETS += libreddit_install

# Find source files, and make a list of the object files to be compiled
LIBREDDIT_SOURCES := $(patsubst $(LIBREDDIT_DIR)/%,%,$(wildcard $(LIBREDDIT_DIR)/*.c))
LIBREDDIT_OBJECTS := $(patsubst %,$(LIBREDDIT_CMP_DIR)/%,$(LIBREDDIT_SOURCES:.c=.o))

libreddit: $(LIBREDDIT_CMP)

$(LIBREDDIT_CMP_DIR): | $(BUILD_DIR)
	$(ECHO) " MKDIR $(LIBREDDIT_CMP_DIR)"
	$(MKDIR) $(LIBREDDIT_CMP_DIR)

# Removes the libreddit folder
libreddit_clean:
	$(ECHO) " RM $(LIBREDDIT_CMP_DIR)"
	$(RM) -fr $(LIBREDDIT_CMP_DIR)

libreddit_install: $(LIBREDDIT_CMP) | $(PREFIX)/lib $(PREFIX)/include
	$(ECHO) " INSTALL $(PREFIX)/lib/libreddit.so"
	$(INSTALL) -m 0755 $(LIBREDDIT_CMP) $(PREFIX)/lib
	$(ECHO) " INSTALL $(PREFIX)/include/reddit.h"
	$(INSTALL) -m 0755 ./include/reddit.h $(PREFIX)/include

ifdef STATIC
$(LIBREDDIT_CMP): $(LIBREDDIT_COMBINED)
	$(ECHO) " AR $(LIBREDDIT_CMP)"
	$(AR) rcs $(LIBREDDIT_CMP) $(LIBREDDIT_COMBINED)
else
$(LIBREDDIT_CMP):  $(LIBREDDIT_COMBINED)
	$(ECHO) " CC $(LIBREDDIT_CMP)"
	$(CC) -shared $(LIBREDDIT_LDFLAGS) $(LIBREDDIT_CFLAGS) $(LIBREDDIT_COMBINED) -o $(LIBREDDIT_CMP)
endif

$(LIBREDDIT_COMBINED): $(LIBREDDIT_OBJECTS)
	$(ECHO) " LD $@"
	$(LD) -r $(LIBREDDIT_OBJECTS) -o $@

# Compiles a single c file into a coresponding .o file
$(LIBREDDIT_CMP_DIR)/%.o: $(LIBREDDIT_DIR)/%.c | $(LIBREDDIT_CMP_DIR)
	$(ECHO) " CC $@"
	$(CC) $(LIBREDDIT_CFLAGS) -c $< -o $@

