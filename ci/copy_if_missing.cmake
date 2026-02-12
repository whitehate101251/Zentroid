# copy_if_missing.cmake
# Usage: cmake -DSRC=<source> -DDST=<destination> -P copy_if_missing.cmake
# Copies SRC to DST only if DST does not already exist.
if(NOT EXISTS "${DST}")
    file(COPY_FILE "${SRC}" "${DST}")
    message(STATUS "Seeded default keymap: ${DST}")
endif()
