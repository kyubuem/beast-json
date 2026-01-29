#!/bin/bash
set -e

mkdir -p tests/libs
cd tests/libs

echo "Downloading nlohmann/json..."
curl -L -o json.hpp https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp

echo "Downloading simdjson..."
curl -L -o simdjson.h https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.h
curl -L -o simdjson.cpp https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.cpp

echo "Downloading yyjson..."
curl -L -o yyjson.h https://raw.githubusercontent.com/ibireme/yyjson/master/src/yyjson.h
curl -L -o yyjson.c https://raw.githubusercontent.com/ibireme/yyjson/master/src/yyjson.c

echo "Downloading rapidjson..."
# RapidJSON is header only but spread across files. We clone it.
if [ ! -d "rapidjson" ]; then
    git clone --depth 1 https://github.com/Tencent/rapidjson.git
fi

echo "Downloading Glaze..."
# Glaze is header only.
if [ ! -d "glaze" ]; then
    git clone --depth 1 https://github.com/stephenberry/glaze.git
fi

echo "Done."
