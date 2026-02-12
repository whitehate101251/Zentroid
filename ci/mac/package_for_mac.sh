# Get absolute path to ensure the script works correctly when executed from other directories
{
cd $(dirname "$0")
script_path=$(pwd)
cd -
} &> /dev/null # disable output
# Set current directory; cd affects the working directory of programs executed next
old_cd=$(pwd)
cd $(dirname "$0")

echo
echo
echo ---------------------------------------------------------------
echo pip install requirements
echo ---------------------------------------------------------------

pip install -r $script_path/package/requirements.txt
if [ $? -ne 0 ] ;then
    echo "pip install requirements failed"
    exit 1
fi

echo
echo
echo ---------------------------------------------------------------
echo create package
echo ---------------------------------------------------------------

python $script_path/package/package.py
if [ $? -ne 0 ] ;then
    echo "create package failed"
    exit 1
fi

# Restore current directory
cd $old_cd
exit 0
