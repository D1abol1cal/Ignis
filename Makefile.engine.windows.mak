DIR := $(subst /,\,${CURDIR})
BUILD_DIR := bin
OBJ_DIR := obj

ASSEMBLY := engine
EXTENSION := .dll

# C compiler flags
COMPILER_FLAGS := -g -MD -Wall -Werror -Wvla -Wgnu-folding-constant -Wno-missing-braces -fdeclspec #-fPIC

# C++ compiler flags (for ImGui and editor)
CXX_COMPILER_FLAGS := -g -MD -Wall -Wno-missing-braces -fdeclspec -std=c++17

INCLUDE_FLAGS := -Iengine\src -Iengine\src\vendor\imgui -I$(VULKAN_SDK)\include
LINKER_FLAGS := -g -shared -luser32 -lvulkan-1 -L$(VULKAN_SDK)\Lib -L$(OBJ_DIR)\engine -lstdc++
DEFINES := -D_DEBUG -DKEXPORT -D_CRT_SECURE_NO_WARNINGS

# Make does not offer a recursive wildcard function, so here's one:
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# Get all .c and .cpp files
SRC_FILES := $(call rwildcard,$(ASSEMBLY)/,*.c)
CPP_SRC_FILES := $(call rwildcard,$(ASSEMBLY)/,*.cpp)

DIRECTORIES := \$(ASSEMBLY)\src $(subst $(DIR),,$(shell dir $(ASSEMBLY)\src /S /AD /B | findstr /i src)) # Get all directories under src.

# Get compiled object files for both C and C++
OBJ_FILES := $(SRC_FILES:%=$(OBJ_DIR)/%.o)
CPP_OBJ_FILES := $(CPP_SRC_FILES:%=$(OBJ_DIR)/%.o)

all: scaffold compile link

.PHONY: scaffold
scaffold: # create build directory
	@echo Scaffolding folder structure...
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(addprefix $(OBJ_DIR), $(DIRECTORIES)) 2>NUL || cd .
	-@setlocal enableextensions enabledelayedexpansion && mkdir $(BUILD_DIR) 2>NUL || cd .
	@echo Done.

.PHONY: link
link: scaffold $(OBJ_FILES) $(CPP_OBJ_FILES) # link
	@echo Linking $(ASSEMBLY)...
	@clang++ $(OBJ_FILES) $(CPP_OBJ_FILES) -o $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) $(LINKER_FLAGS)

.PHONY: compile
compile: #compile .c files
	@echo Compiling...

.PHONY: clean
clean: # clean build directory
	if exist $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION) del $(BUILD_DIR)\$(ASSEMBLY)$(EXTENSION)
	rmdir /s /q $(OBJ_DIR)\$(ASSEMBLY)

$(OBJ_DIR)/%.c.o: %.c # compile .c to .c.o object
	@echo   $<...
	@clang $< $(COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

$(OBJ_DIR)/%.cpp.o: %.cpp # compile .cpp to .cpp.o object
	@echo   $<...
	@clang++ $< $(CXX_COMPILER_FLAGS) -c -o $@ $(DEFINES) $(INCLUDE_FLAGS)

-include $(OBJ_FILES:.o=.d)
-include $(CPP_OBJ_FILES:.o=.d)
