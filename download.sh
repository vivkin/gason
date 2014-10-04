if [ ! -d shootout ]; then
    set -x

    mkdir shootout
    cd shootout

    [ ! -d sajson ] && git clone https://github.com/chadaustin/sajson.git
    [ ! -d rapidjson ] && git clone https://github.com/miloyip/rapidjson

    curl -fLO https://raw.githubusercontent.com/Newbilius/big_json_import_demo/master/test_data/big.json
    curl -fLO https://raw.githubusercontent.com/RichardHightower/json-parsers-benchmark/master/data/citm_catalog.json
    curl -fLo monster.json https://raw.githubusercontent.com/mrdoob/three.js/master/examples/models/animated/monster/monster.js
fi

#(mkdir -p build-android && cd build-android && cmake -DANDROID_NDK=/Users/viv/android/ndk \
#    -DANDROID_ABI=armeabi-v7a \
#    -DANDROID_NATIVE_API_LEVEL=14 \
#    -LIBRARY_OUTPUT_PATH=../ruberoid/libs/armeabi-v7a \
#    -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake \
#    ..)
