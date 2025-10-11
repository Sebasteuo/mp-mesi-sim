#!/usr/bin/env bash
# Formatea todo el código C/C++ del repo
find include src tests -type f \( -name "*.hpp" -o -name "*.h" -o -name "*.cpp" -o -name "*.c" \) -print0 \
 | xargs -0 clang-format -i
echo "Formato aplicado."
