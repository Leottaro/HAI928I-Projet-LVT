working_dir=$(pwd)
echo "The working dir is $working_dir"

echo "Checking argument..."
current_mode=${1:-Release}
case "$current_mode" in
    Debug)
        ;;
    Release)
        ;;
    *)
        echo "Usage: $0 {Debug|Release} [program args...]"
        exit 1
        ;;
esac

if [ "$#" -gt 0 ]; then
    shift
fi

echo "Building Cmake configurations..."
mkdir -p build || exit 1
for mode in Debug Release; do
    build_dir=$working_dir/build/$mode
    echo "building $mode into $build_dir..."
    mkdir -p $build_dir || exit 1
    cd $build_dir || exit 1
    cmake $working_dir -D CMAKE_BUILD_TYPE=$mode -D CMAKE_EXPORT_COMPILE_COMMANDS=1
    if [ $mode = $current_mode ]; then
        echo "Linking compile commands..."
        ln -sf $build_dir/compile_commands.json $working_dir/compile_commands.json || exit 1
        current_build_dir=$build_dir
    fi
done

cd $current_build_dir || exit 1
echo "Compiling $current_mode project..."
if make -j; then
    echo "Executing $current_mode project..."
    cd $working_dir
    bin/projet3d "$@"
else
    cd $working_dir
    exit 1
fi
