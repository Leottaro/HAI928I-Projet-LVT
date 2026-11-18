working_dir=$(pwd)
echo "Building Cmake configurations..."
mkdir -p build || exit 1
for mode in Debug Release; do
    build_dir=$working_dir/build/$mode
    echo "building $mode into $build_dir..."
    mkdir -p $build_dir || exit 1
    cd $build_dir || exit 1
    cmake $working_dir -D CMAKE_BUILD_TYPE=$mode -D CMAKE_EXPORT_COMPILE_COMMANDS=1
done