#!/bin/bash
# Build script for engine
set echo on

mkdir -p ../bin

# Get a list of all the .c files.
cFilenames=$(find . -type f -name "*.c")

# echo "Files:" $cFilenames

assembly="engine"

# Detect platform
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    # Windows build
    compilerFlags="-g -shared -fdeclspec"
    includeFlags="-Isrc -I$VULKAN_SDK/include"
    linkerFlags="-luser32 -lvulkan-1 -L$VULKAN_SDK/lib"
    defines="-D_DEBUG -DKEXPORT -D_CRT_SECURE_NO_WARNINGS"
    outputFile="../bin/engine.dll"
else
    # Linux/Unix build
    compilerFlags="-g -shared -fdeclspec -fPIC"
    # -fms-extensions
    # -Wall -Werror
    includeFlags="-Isrc -I$VULKAN_SDK/include"
    linkerFlags="-lvulkan -lxcb -lX11 -lX11-xcb -lxkbcommon -L$VULKAN_SDK/lib -L/usr/X11R6/lib"
    defines="-D_DEBUG -DKEXPORT"
    outputFile="../bin/lib$assembly.so"
fi

echo "Building $assembly..."
clang $cFilenames $compilerFlags -o $outputFile $defines $includeFlags $linkerFlags