# Declare ts files
set(QC_TS_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/zh_CN.ts 
    ${CMAKE_CURRENT_SOURCE_DIR}/en_US.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/ja_JP.ts
)
# Set qm file output directory
set_source_files_properties(${QC_TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")
# Import LinguistTools
find_package(QT NAMES Qt6 Qt5 COMPONENTS LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools REQUIRED)

# qt5_create_translation will sequentially run lupdate to update ts, then lrelease to update qm
qt5_create_translation(QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../.. ${QC_TS_FILES})
# Custom target depends on QM_FILES, otherwise qm files will not be generated
add_custom_target(QC_QM_GENERATOR DEPENDS ${QM_FILES})

# qt5_create_translation bug: cmake clean deletes the translated ts files, causing translations to be lost
# (Qt officially states qt6 does not have this issue; if only using qt6, consider qt5_create_translation)
# The CLEAN_NO_CUSTOM workaround found online only works with makefile generators, does not solve the problem
# https://cmake.org/cmake/help/latest/prop_dir/CLEAN_NO_CUSTOM.html
# set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)
# Currently the only solution is to manually restore ts files from git after each clean

#[[
Summary:
In a cmake Qt project, there are three ways to handle translations using cmake scripts:
1. Use Qt's built-in cmake LinguistTools scripts: qt5_create_translation & qt5_add_translation
Neither script fully meets requirements:
qt5_add_translation can only generate qm files from existing ts files (lrelease), cannot update ts files (lupdate)
qt5_create_translation deletes translated ts files during cmake clean, causing translation loss

2. cmake add_custom_command + cmake LinguistTools scripts (qt5_create_translation internally uses add_custom_command)
For example, use add_custom_command to run lupdate, combined with qt5_add_translation to update qm
Reference: https://github.com/maratnek/QtFirstProgrammCMake/blob/2c93b59e2ba85ff6ee0e727487e14003381687d3/CMakeLists.txt

3. Use cmake commands directly to execute lupdate and lrelease
For example, add_custom_command/add_custom_target/execute_process can all execute lupdate and lrelease commands

All 3 approaches share a common issue: translation file processing is bound to compilation, running checks on every build.
In practice, translation work should be done once after all coding is complete - run lupdate, translate, then lrelease.
It should not be bound to compilation.
Therefore, writing two shell scripts lupdate.sh and lrelease.sh is more appropriate. It's actually very simple:
1. Update ts: lupdate -no-obsolete ./Zentroid -ts ./Zentroid/res/i18n/en_US.ts ./Zentroid/res/i18n/zh_CN.ts
2. Manually translate ts files
3. Release: lrelease ./Zentroid/res/i18n/en_US.ts ./Zentroid/res/i18n/zh_CN.ts

Reference documents:
1. Qt acknowledges qt5_create_translation bug but won't fix it; confirmed qt6 is fine https://bugreports.qt.io/browse/QTBUG-96549
2. https://doc.qt.io/qt-5/qtlinguist-cmake-qt5-add-translation.html
3. https://doc.qt.io/qt-5/qtlinguist-cmake-qt5-create-translation.html
4. execute_process reference: https://blog.csdn.net/u010255072/article/details/120326833
5. add_custom_target reference: https://www.cnblogs.com/apocelipes/p/14355460.html
