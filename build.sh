#!/bin/bash
COMPILER="clang"        
STANDARD="c11"  
OUTPUT="sometil.exe"    
WARNINGS="-Wall -Wextra"
ERRORS="-Werror"
OPTIMIZATION="-O3"
SOURCES=(
  "src/main.c"
  "src/arena_allocator.c"
  "src/ifstream.c"
  "src/utf8_util.c"
  "src/dynamic_string.c"
)

echo "Build $OUTPUT with $COMPILER $STANDARD..."
$COMPILER "${SOURCES[@]}" -o "$OUTPUT" \
  -std="$STANDARD" \
  $WARNINGS $ERRORS $OPTIMIZATION

if [ $? -eq 0 ]; then
  echo "Succes: $OUTPUT"
  ls -lh "$OUTPUT" | awk '{print "Size:", $5}'
else
  echo "Failed to build sometil"
  exit 1
fi