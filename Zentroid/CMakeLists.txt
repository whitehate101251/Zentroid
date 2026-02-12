# For VS2019 and Xcode 12+ support.
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

#
# Global config
#

# QC is "Qt CMake"
# https://www.kdab.com/wp-content/uploads/stories/QTVTC20-Using-Modern-CMake-Kevin-Funk.pdf

# QC Custom config
set(QC_PROJECT_NAME "Zentroid")
# Read version numbers from file
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/appversion QC_FILE_VERSION)
set(QC_PROJECT_VERSION ${QC_FILE_VERSION})

# Project declare
project(${QC_PROJECT_NAME} VERSION ${QC_PROJECT_VERSION} LANGUAGES CXX)
message(STATUS "[${PROJECT_NAME}] Project ${PROJECT_NAME} ${PROJECT_VERSION}")

# QC define

# check arch
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(QC_CPU_ARCH x64)
else()
    set(QC_CPU_ARCH x86)
endif()

# MacOS
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # mac default arch arm64
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES arm64)
    endif()

    if (CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        set(QC_CPU_ARCH arm64)
    endif()
endif()

message(STATUS "[${PROJECT_NAME}] CPU_ARCH:${QC_CPU_ARCH}")

# CMake set
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# default RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
message(STATUS "[${PROJECT_NAME}] BUILD_TYPE:${CMAKE_BUILD_TYPE}")

# Log configuration
option(ENABLE_DETAILED_LOGS "Enable detailed log output with file and line info" OFF)
if(ENABLE_DETAILED_LOGS)
    message(STATUS "[${PROJECT_NAME}] Detailed logs enabled")
else()
    message(STATUS "[${PROJECT_NAME}] Simple logs enabled")
endif()

# Compiler set
message(STATUS "[${PROJECT_NAME}] C++ compiler ID is: ${CMAKE_CXX_COMPILER_ID}")
if (MSVC)
    # FFmpeg cannot be compiled natively by MSVC version < 12.0 (2013)
    if(MSVC_VERSION LESS 1800)
        message(FATAL_ERROR "[${PROJECT_NAME}] ERROR: MSVC version is older than 12.0 (2013).")
    endif()

    message(STATUS "[${PROJECT_NAME}] Set Warnings as error")
    # warning level 3 and all warnings as errors
    add_compile_options(/W3 /WX /wd4566)

    # avoid warning C4819
    #add_compile_options(-source-charset:utf-8)
    # /utf-8 will set source charset and execution charset to utf-8, so we don't need to set source-charset:utf-8
    add_compile_options(/utf-8)

    # ensure we use minimal "windows.h" lib without the crazy min max macros
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    
    # disable SAFESEH - avoid "LNK2026: module unsafe"(Qt5.15&&vs2019)     
    add_link_options(/SAFESEH:NO)
endif()

if (NOT MSVC)
    message(STATUS "[${PROJECT_NAME}] Set warnings as error")
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic)

    # disable some warning
    add_compile_options(-Wno-c++17-extensions -Wno-overloaded-virtual -Wno-unused-parameter)
endif()

#
# Qt
#

# Find Qt version
if (NOT QT_DESIRED_VERSION)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
    message("   >>> Found Qt version: ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
    set(QT_DESIRED_VERSION ${QT_VERSION_MAJOR})
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(qt_required_components Widgets Network)

# Multimedia is optional - may not be available on all systems
find_package(Qt${QT_DESIRED_VERSION} QUIET COMPONENTS Multimedia)
if(Qt${QT_DESIRED_VERSION}Multimedia_FOUND)
    list(APPEND qt_required_components Multimedia)
    set(HAS_QT_MULTIMEDIA TRUE)
    message(STATUS "[${PROJECT_NAME}] Qt Multimedia found")
else()
    set(HAS_QT_MULTIMEDIA FALSE)
    message(STATUS "[${PROJECT_NAME}] Qt Multimedia NOT found - audio features disabled")
endif()

if (QT_DESIRED_VERSION MATCHES 6)
    # list(APPEND qt_required_components Core5Compat)
    list(APPEND qt_required_components OpenGL)
    list(APPEND qt_required_components OpenGLWidgets)
else()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND qt_required_components X11Extras )
    endif()
endif()

find_package(Qt${QT_DESIRED_VERSION} REQUIRED COMPONENTS ${qt_required_components})

set(LINK_LIBS
    Qt${QT_DESIRED_VERSION}::Widgets
    Qt${QT_DESIRED_VERSION}::Network
)

if(HAS_QT_MULTIMEDIA)
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::Multimedia)
endif()

if (QT_DESIRED_VERSION MATCHES 6)
    # list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::Core5Compat)
    # GuiPrivate is needed for xmousetap but may not be available
    find_package(Qt${QT_DESIRED_VERSION} QUIET COMPONENTS GuiPrivate)
    if(Qt${QT_DESIRED_VERSION}GuiPrivate_FOUND)
        list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::GuiPrivate)
        message(STATUS "[${PROJECT_NAME}] Qt GuiPrivate found")
    else()
        message(STATUS "[${PROJECT_NAME}] Qt GuiPrivate NOT found - some mouse tap features may be limited")
    endif()
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::OpenGL)
    list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::OpenGLWidgets)
else()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND LINK_LIBS Qt${QT_DESIRED_VERSION}::X11Extras)
    endif()
endif()

message(STATUS "[${PROJECT_NAME}] Qt version is: ${QT_DESIRED_VERSION}")

#
# Sources
#

# fontawesome
set(QC_FONTAWESOME_SOURCES
    fontawesome/iconhelper.h
    fontawesome/iconhelper.cpp
)
source_group(fontawesome FILES ${QC_FONTAWESOME_SOURCES})

# uibase
set(QC_UIBASE_SOURCES
    uibase/keepratiowidget.h
    uibase/keepratiowidget.cpp
    uibase/magneticwidget.h
    uibase/magneticwidget.cpp
)
source_group(uibase FILES ${QC_UIBASE_SOURCES})

# audio (conditional on Qt Multimedia)
if(HAS_QT_MULTIMEDIA)
    set(QC_AUDIO_SOURCES
        audio/audiooutput.h
        audio/audiooutput.cpp
    )
    source_group(audio FILES ${QC_AUDIO_SOURCES})
else()
    set(QC_AUDIO_SOURCES "")
endif()

# ui
set(QC_UI_SOURCES
    ui/toolform.h
    ui/toolform.cpp
    ui/toolform.ui
    ui/videoform.h
    ui/videoform.cpp
    ui/videoform.ui
    ui/dialog.cpp
    ui/dialog.h
    ui/dialog.ui
    ui/toggleswitch.h
    ui/toggleswitch.cpp
    ui/cleanmodewidget.h
    ui/cleanmodewidget.cpp
    ui/advanceddialog.h
    ui/advanceddialog.cpp
    ui/customdialog.h
    ui/customdialog.cpp
    render/qyuvopenglwidget.h
    render/qyuvopenglwidget.cpp
)
source_group(ui FILES ${QC_UI_SOURCES})

# group controller
set(QC_GROUP_CONTROLLER
    groupcontroller/groupcontroller.h
    groupcontroller/groupcontroller.cpp
)
source_group(groupcontroller FILES ${QC_GROUP_CONTROLLER})

# util
set(QC_UTIL_SOURCES
    util/config.h
    util/config.cpp
    util/mousetap/mousetap.h
    util/mousetap/mousetap.cpp
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(QC_UTIL_SOURCES ${QC_UTIL_SOURCES} 
        util/mousetap/winmousetap.h
        util/mousetap/winmousetap.cpp
        util/winutils.h
        util/winutils.cpp
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(QC_UTIL_SOURCES ${QC_UTIL_SOURCES} 
        util/mousetap/xmousetap.h
        util/mousetap/xmousetap.cpp
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(QC_UTIL_SOURCES ${QC_UTIL_SOURCES} 
        util/mousetap/cocoamousetap.h
        util/mousetap/cocoamousetap.mm
        util/path.h
        util/path.mm
    )
endif()
source_group(util FILES ${QC_UTIL_SOURCES})

# qrc
set(QC_QRC_SOURCES "res/res.qrc")

# main
set(QC_MAIN_SOURCES
    main.cpp
    ${QC_QRC_SOURCES}
)

# plantform file
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Define VERSION macros for .rc file
    add_compile_definitions(
        VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        VERSION_MINOR=${PROJECT_VERSION_MINOR}
        VERSION_PATCH=${PROJECT_VERSION_PATCH}
        VERSION_RC_STR="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
    )
    set(QC_PLANTFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.rc"
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Step 1. add icns to source file, for MACOSX_PACKAGE_LOCATION copy
    set(QC_PLANTFORM_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.icns"
    )
endif()

# Translation related (using shell scripts instead of cmake for translations)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/res/i18n)

# all sources
set(QC_PROJECT_SOURCES
    ${QC_FONTAWESOME_SOURCES}
    ${QC_UIBASE_SOURCES}
    ${QC_UI_SOURCES}
    ${QC_UTIL_SOURCES}
    ${QC_MAIN_SOURCES}
    ${QC_GROUP_CONTROLLER}
    ${QC_PLANTFORM_SOURCES}
    ${QC_AUDIO_SOURCES}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(QC_RUNTIME_TYPE MACOSX_BUNDLE)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(QC_RUNTIME_TYPE WIN32)
endif()


add_executable(${PROJECT_NAME} ${QC_RUNTIME_TYPE} ${QC_PROJECT_SOURCES})

# Log compile definitions
if(ENABLE_DETAILED_LOGS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_DETAILED_LOGS)
endif()

# Audio compile definition
if(HAS_QT_MULTIMEDIA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_QT_MULTIMEDIA)
endif()

#
# Internal include path (todo: remove this, use absolute path include)
#

target_include_directories(${PROJECT_NAME} PRIVATE fontawesome)
target_include_directories(${PROJECT_NAME} PRIVATE util)
target_include_directories(${PROJECT_NAME} PRIVATE uibase)
target_include_directories(${PROJECT_NAME} PRIVATE ui)
target_include_directories(${PROJECT_NAME} PRIVATE render)

# Keymap Editor module
add_subdirectory(ui/keymapeditor)

# output dir
# https://cmake.org/cmake/help/latest/prop_gbl/GENERATOR_IS_MULTI_CONFIG.html
get_property(QC_IS_MUTIL_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
message(STATUS "multi config:" QC_IS_MUTIL_CONFIG)

# $<0:> uses generator expressions to set RUNTIME_OUTPUT_DIRECTORY for each config, preventing multi-config from auto-appending CMAKE_BUILD_TYPE subdirectory
# 1. multi config intro: https://cmake.org/cmake/help/latest/prop_gbl/GENERATOR_IS_MULTI_CONFIG.html
# 2. multi config auto-appending subdirectory docs: https://cmake.org/cmake/help/latest/prop_tgt/RUNTIME_OUTPUT_DIRECTORY.html
# 3. using generator expressions to prevent multi config auto-appending: https://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../output/${QC_CPU_ARCH}/${CMAKE_BUILD_TYPE}/$<0:>"
)

#
# plantform deps
#

# windows
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    get_target_property(QSC_BIN_OUTPUT_PATH ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
    set(QSC_DEPLOY_PATH ${QSC_BIN_OUTPUT_PATH})

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/sndcpy/sndcpy.bat" "${QSC_BIN_OUTPUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/sndcpy/sndcpy.apk" "${QSC_BIN_OUTPUT_PATH}"
    )
endif()

# MacOS
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # qt6 need 10.15 or later
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")

    # copy bundle file
    get_target_property(MACOS_BUNDLE_PATH ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
    set(MACOS_BUNDLE_PATH ${MACOS_BUNDLE_PATH}/${PROJECT_NAME}.app/Contents)

    set(QSC_DEPLOY_PATH ${MACOS_BUNDLE_PATH})

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # config file copy to Contents/MacOS/config
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../config/config.ini" "${MACOS_BUNDLE_PATH}/MacOS/config/config.ini"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/sndcpy/sndcpy.sh" "${MACOS_BUNDLE_PATH}/MacOS"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/sndcpy/sndcpy.apk" "${MACOS_BUNDLE_PATH}/MacOS"
    )

    # Step 2. ues MACOSX_PACKAGE_LOCATION copy icns to Resources
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/res/${PROJECT_NAME}.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )

    # use MACOSX_BUNDLE_INFO_PLIST custom plist, not use MACOSX_BUNDLE_BUNDLE_NAME etc..
    set(INFO_PLIST_TEMPLATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/Info_Mac.plist.in")
    set(INFO_PLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/res/Info_Mac.plist")
    file(READ "${INFO_PLIST_TEMPLATE_FILE}" plist_contents)
    string(REPLACE "\${BUNDLE_VERSION}" "${PROJECT_VERSION}" plist_contents ${plist_contents})
    file(WRITE ${INFO_PLIST_FILE} ${plist_contents})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${INFO_PLIST_FILE}"
        # "" disable code sign
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    )

    # mac framework
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AppKit")
endif()

# Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    get_target_property(QSC_BIN_OUTPUT_PATH ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
    set(QSC_DEPLOY_PATH ${QSC_BIN_OUTPUT_PATH})

    # Collect all source keymap JSON files
    file(GLOB KEYMAP_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../keymap/*.json")

    # Build a list of copy commands that only copy if destination doesn't exist
    set(KEYMAP_COPY_COMMANDS)
    foreach(KEYMAP_FILE ${KEYMAP_SOURCE_FILES})
        get_filename_component(KEYMAP_BASENAME ${KEYMAP_FILE} NAME)
        set(KEYMAP_DEST "${QSC_BIN_OUTPUT_PATH}/keymap/${KEYMAP_BASENAME}")
        # cmake -E true is a no-op; we use a small cmake -P script inline
        # to test existence and copy only when missing.
        list(APPEND KEYMAP_COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND}
                -DSRC=${KEYMAP_FILE}
                -DDST=${KEYMAP_DEST}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/../ci/copy_if_missing.cmake
        )
    endforeach()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/sndcpy/sndcpy.sh" "${QSC_BIN_OUTPUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/sndcpy/sndcpy.apk" "${QSC_BIN_OUTPUT_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${QSC_BIN_OUTPUT_PATH}/keymap"
        # Seed default keymaps only if they don't already exist (preserve user edits)
        ${KEYMAP_COPY_COMMANDS}
    )

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        # xcb https://doc.qt.io/qt-5/linux-requirements.html
        xcb
        # pthread
        Threads::Threads
    )

    # linux set app icon: https://blog.csdn.net/MrNoboday/article/details/82870853
endif()

#
# common deps
#

add_subdirectory(ZentroidCore)

# Qt
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LINK_LIBS}
    ZentroidCore
)
