if ./compile.sh $1; then
    echo "Executing $mode project..."
    if [ "$#" -gt 0 ]; then
        shift
    fi
    bin/projet3d "$@"
fi