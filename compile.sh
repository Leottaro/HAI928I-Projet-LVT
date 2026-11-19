working_dir=$(pwd)
echo "The working dir is $working_dir"

echo "Checking argument..."
mode=${1:-Release}
case "$mode" in
    Debug)
        ;;
    Release)
        ;;
    *)
        echo "Usage: $0 {Debug|Release} [program args...]"
        exit 1
        ;;
esac

./cmakebuild.sh || exit 1

build_dir=$working_dir/build/$mode
echo "Linking compile commands..."
ln -sf $build_dir/compile_commands.json $working_dir/compile_commands.json || exit 1

cd $build_dir || exit 1
echo "Compiling $mode project..."
if make -j; then
    cd $working_dir
    exit 0
else
    cd $working_dir
    exit 1
fi
