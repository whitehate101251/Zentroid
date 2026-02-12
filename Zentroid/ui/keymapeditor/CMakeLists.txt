# Keymap Editor Module

set(KEYMAPEDITOR_SOURCES
    keymapeditor.cpp
    keynode.cpp
    keyassigndialog.cpp
    propertiespanel.cpp
    undocommands.cpp
    layerpanel.cpp
    keymapoverlay.cpp
)

set(KEYMAPEDITOR_HEADERS
    keymapeditor.h
    keynode.h
    editorscene.h
    keyassigndialog.h
    propertiespanel.h
    undocommands.h
    layerpanel.h
    keymapoverlay.h
)

# Add to parent target (will be included in main CMakeLists.txt)
target_sources(${PROJECT_NAME} PRIVATE
    ${KEYMAPEDITOR_SOURCES}
    ${KEYMAPEDITOR_HEADERS}
)

# Include this directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
