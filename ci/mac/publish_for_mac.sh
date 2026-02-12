echo
echo
echo ---------------------------------------------------------------
echo check ENV
echo ---------------------------------------------------------------

# Get required parameters from environment variables
# e.g. /Users/barry/Qt5.12.5/5.12.5
echo ENV_QT_PATH $ENV_QT_PATH

# Get absolute path to ensure the script works correctly when executed from other directories
{
cd $(dirname "$0")
script_path=$(pwd)
cd -
} &> /dev/null # disable output
# Set current directory; cd affects the working directory of programs executed next
old_cd=$(pwd)
cd $(dirname "$0")

# Startup parameter declarations
publish_dir=$1
cpu_arch=$2

echo
echo
echo ---------------------------------------------------------------
echo check cpu arch[x64/arm64]
echo ---------------------------------------------------------------

if [[ $cpu_arch != "x64" && $cpu_arch != "arm64" ]]; then
    echo "error: unkonow cpu mode -- $2"
    exit 1
fi

# Display info
echo current cpu mode: $cpu_arch

if [ $cpu_arch == "x64" ]; then
    qt_clang_path=$ENV_QT_PATH/clang_64
else
    qt_clang_path=$ENV_QT_PATH/macos
fi

# Display info
echo current publish dir: $publish_dir

# Environment variable setup
keymap_path=$script_path/../../keymap
# config_path=$script_path/../../config

publish_path=$script_path/$publish_dir
release_path=$script_path/../../output/$cpu_arch/RelWithDebInfo

export PATH=$qt_clang_path/bin:$PATH

if [ -d "$publish_path" ]; then
    rm -rf $publish_path
fi

# Copy packages to publish
cp -r $release_path $publish_path
cp -r $keymap_path $publish_path/Zentroid.app/Contents/MacOS
# cp -r $config_path $publish_path/Zentroid.app/Contents/MacOS

# Add Qt dependency packages
macdeployqt $publish_path/Zentroid.app

# Remove unnecessary Qt dependency packages

# PlugIns
rm -rf $publish_path/Zentroid.app/Contents/PlugIns/iconengines
# Screenshot feature requires libqjpeg.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqgif.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqicns.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqico.dylib
# rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqjpeg.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqmacheif.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqmacjp2.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqtga.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqtiff.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqwbmp.dylib
rm -f $publish_path/Zentroid.app/Contents/PlugIns/imageformats/libqwebp.dylib
rm -rf $publish_path/Zentroid.app/Contents/PlugIns/virtualkeyboard
rm -rf $publish_path/Zentroid.app/Contents/PlugIns/printsupport
rm -rf $publish_path/Zentroid.app/Contents/PlugIns/platforminputcontexts
rm -rf $publish_path/Zentroid.app/Contents/PlugIns/iconengines
rm -rf $publish_path/Zentroid.app/Contents/PlugIns/bearer

# Frameworks
rm -rf $publish_path/Zentroid.app/Contents/Frameworks/QtVirtualKeyboard.framework
rm -rf $publish_path/Contents/Frameworks/QtSvg.framework

# qml
rm -rf $publish_path/Zentroid.app/Contents/Frameworks/QtQml.framework
rm -rf $publish_path/Zentroid.app/Contents/Frameworks/QtQuick.framework

echo
echo
echo ---------------------------------------------------------------
echo finish!!!
echo ---------------------------------------------------------------

# Restore current directory
cd $old_cd
exit 0
