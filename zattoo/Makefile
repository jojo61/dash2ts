# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
TARGET_EXEC := zattood
TARGET_HANDLER := zattoostream
# use either the default include for KODI addon header files
# or overwrite the include directory on the command line
#KODI_ADDON_INCLUDE ?= src/addons/kodi-dev-kit/include

BUILD_DIR := ./build
SRC_DIRS := ./src

LDFLAGS :=  -ltinyxml2 -lcurl -lsqlite3
# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

all: $(BUILD_DIR)/$(TARGET_HANDLER) $(BUILD_DIR)/$(TARGET_EXEC)

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
#OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
OBJS := ./build/./src/zattood.cpp.o\
		./build/./src/Session.cpp.o\
		./build/./src/http/HttpClient.cpp.o\
		./build/./src/sql/SQLConnection.cpp.o\
		./build/./src/Utils.cpp.o\
		./build/./src/http/Curl.cpp.o\
		./build/./src/sql/ParameterDB.cpp.o\
		./build/./src/Settings.cpp.o\
		./build/./src/ZatData.cpp.o\
		./build/./src/epg/ZattooEpgProvider.cpp.o\
		./build/./src/epg/EpgProvider.cpp.o\
		./build/./src/http/Cache.cpp.o\
		./build/./src/md5.cpp.o

HANDLER := ./build/./src/zattoostream.cpp.o

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := -I$(KODI_ADDON_INCLUDE) -I/usr/local/include 
# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP -ggdb

# The final build step.

$(BUILD_DIR)/$(TARGET_HANDLER): $(HANDLER)
	$(CXX) $(HANDLER) -o $@

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean all
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
#-include $(DEPS)
