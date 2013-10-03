# Add libreddit's targets
CLEAN_TARGETS +=libreddit_clean

# libreddit currently just compiles with the default settings
LIBREDDIT_CFLAGS :=$(PROJCFLAGS)

# The directory to store the object files in
LIBREDDIT_DIR :=libreddit
LIBREDDIT_CMP_DIR :=$(BUILD_DIR)/$(LIBREDDIT_DIR)

LIBREDDIT_STATIC := $(BUILD_DIR)/libreddit.a

COMPILE_TARGETS += $(LIBREDDIT_STATIC)

# Find source files, and make a list of the object files to be compiled
LIBREDDIT_SOURCES := $(patsubst $(LIBREDDIT_DIR)/%,%,$(wildcard $(LIBREDDIT_DIR)/*.c))
LIBREDDIT_OBJECTS := $(patsubst %,$(LIBREDDIT_CMP_DIR)/%,$(LIBREDDIT_SOURCES:.c=.o))

libreddit: $(LIBREDDIT_STATIC)

$(LIBREDDIT_CMP_DIR): | $(BUILD_DIR)
	$(ECHO) " MKDIR $(LIBREDDIT_CMP_DIR)"
	$(MKDIR) $(LIBREDDIT_CMP_DIR)

# Removes the libreddit folder
libreddit_clean:
	$(ECHO) " RM $(LIBREDDIT_CMP_DIR)"
	$(RM) -fr $(LIBREDDIT_CMP_DIR)

$(LIBREDDIT_STATIC): $(LIBREDDIT_OBJECTS)
	$(ECHO) " AR $(LIBREDDIT_STATIC)"
	$(AR) rcs $(LIBREDDIT_STATIC) $(LIBREDDIT_OBJECTS)

# Compiles a single c file into a coresponding .o file
$(LIBREDDIT_CMP_DIR)/%.o: $(LIBREDDIT_DIR)/%.c | $(LIBREDDIT_CMP_DIR)
	$(ECHO) " CC $@"
	$(CC) $(LIBREDDIT_CFLAGS) -c $< -o $@

