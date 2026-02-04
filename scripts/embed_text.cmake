# CMake script to embed an arbitrary file (text or binary) into a C source file
# Expects INPUT, OUTPUT and SYMBOL variables defined when invoked.
if(NOT DEFINED INPUT)
    message(FATAL_ERROR "embed_text.cmake: INPUT not defined")
endif()
if(NOT DEFINED OUTPUT)
    message(FATAL_ERROR "embed_text.cmake: OUTPUT not defined")
endif()
if(NOT DEFINED SYMBOL)
    # Derive a safe symbol name from INPUT if SYMBOL not provided
    get_filename_component(_fname ${INPUT} NAME)
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" SYMBOL ${_fname})
endif()

# Read file as hex so we can reproduce exact bytes on all platforms
file(READ "${INPUT}" HEXDATA HEX)
string(LENGTH "${HEXDATA}" HEXLEN)
math(EXPR BYTELEN "${HEXLEN} / 2")
math(EXPR LAST_INDEX "${BYTELEN} - 1")

set(out "// Generated from ${INPUT}\n")
set(out "#include <cstddef>\n#include <cstdint>\n\n")
set(out "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
set(out "const unsigned char ${SYMBOL}[] = {\n")

foreach(i RANGE ${LAST_INDEX})
  math(EXPR start "${i} * 2")
  string(SUBSTRING "${HEXDATA}" ${start} 2 BYTEHEX)
  string(TOLOWER "${BYTEHEX}" BYTEHEXLOW)
  if(i LESS ${LAST_INDEX})
    set(out "${out} 0x${BYTEHEXLOW},\n")
  else()
    set(out "${out} 0x${BYTEHEXLOW}\n")
  endif()
endforeach()

set(out "};\n\nconst unsigned int ${SYMBOL}_len = ${BYTELEN};\n\n")
set(out "#ifdef __cplusplus\n}\n#endif\n")

file(WRITE "${OUTPUT}" "${out}")
message(STATUS "Embedded file: ${INPUT} -> ${OUTPUT} (${BYTELEN} bytes, symbol: ${SYMBOL})")
