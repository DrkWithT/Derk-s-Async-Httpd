argc=$#

usage_exit() {
    echo "Usage: utility.sh [help | build | unittest]\n\tutility.sh build [debug | release]\n\tutility.sh unittest\n";
    exit $1;
}

if [[ $argc -lt 1 ]]; then
    usage_exit 1;
fi

action="$1"
build_status=0

if [[ $action = "help" ]]; then
    usage_exit 0;
elif [[ $action = "build" && $argc -ge 2 ]]; then
    rm -rf ./build/;
    rm -rf ./.cache/;
    # rm -f ./build/compile_commands.json;
    # rm -f ./build/derkhttpd;
    cmake --fresh -S . -B build --preset "local-$2-build" && cmake --build build;
elif [[ $action = "unittest" && $argc -eq 1 ]]; then
    # touch ./logs/all.txt;
    # ctest --test-dir build --timeout 2 -V 1> ./logs/all.txt;

    # if [[ $? -eq 0 ]]; then
    #     echo "All tests OK";
    # else
    #     echo "Some tests failed: see logs";
    # fi
    usage_exit 1;
else
    usage_exit 1;
fi
