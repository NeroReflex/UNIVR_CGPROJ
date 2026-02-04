# CMake script to embed a SPIR-V binary into a C++ source file
# Expects INPUT, OUTPUT and optionally SYMBOL variables defined when invoked.
if(NOT DEFINED INPUT)
    message(FATAL_ERROR "embed_spirv.cmake: INPUT not defined")
endif()
if(NOT DEFINED OUTPUT)
    message(FATAL_ERROR "embed_spirv.cmake: OUTPUT not defined")
endif()

# Get filename for symbol naming (default) unless SYMBOL provided
if(NOT DEFINED SYMBOL)
    get_filename_component(FNAME ${INPUT} NAME)
    string(REGEX REPLACE "\.[^.]*$" "" BASENAME ${FNAME})
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" BASENAME ${BASENAME})
    set(SYMBOL "spirv_${BASENAME}_spv")
endif()
string(CONCAT SYM_LEN ${SYMBOL} "_len")

# Read binary as hex string
file(READ "${INPUT}" HEXDATA HEX)
string(LENGTH "${HEXDATA}" HEXLEN)
math(EXPR BYTELEN "${HEXLEN} / 2")
math(EXPR WORDCOUNT "${BYTELEN} / 4")

# Prepare output content
set(out "// Generated from ${INPUT}\n")
set(out "#include <cstddef>\n#include <cstdint>\n\n")
set(out "${out}extern const uint32_t ${SYMBOL}[];\nextern const size_t ${SYM_LEN};\n\n")
set(out "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
set(out "const uint32_t ${SYMBOL}[] = {\n")

# Helper: for each word, take 8 hex chars and reverse byte order
foreach(i RANGE ${WORDCOUNT})
    math(EXPR start "${i} * 8")
    string(SUBSTRING "${HEXDATA}" ${start} 8 WORDHEX)
    # reverse byte order: WORDHEX is b0b1b2b3 (each two chars)
    string(SUBSTRING "${WORDHEX}" 6 2 B3)
    string(SUBSTRING "${WORDHEX}" 4 2 B2)
    string(SUBSTRING "${WORDHEX}" 2 2 B1)
    string(SUBSTRING "${WORDHEX}" 0 2 B0)
    set(WVAL "0x${B3}${B2}${B1}${B0}")
    if(i LESS ${WORDCOUNT})
        set(out "${out}    ${WVAL},\n")
    else()
        set(out "${out}    ${WVAL}\n")
    endif()
endforeach()
set(out "};\n\n")
set(out "const size_t ${SYM_LEN} = ${WORDCOUNT};\n\n")
set(out "#ifdef __cplusplus\n}\n#endif\n")

file(WRITE "${OUTPUT}" "${out}")
message(STATUS "Embedded SPIR-V: ${INPUT} -> ${OUTPUT} (${WORDCOUNT} words)")
