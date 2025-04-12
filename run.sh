#!/bin/bash

# 如果build目录不存在，创建它
if [ ! -d "build" ]; then
    mkdir build
fi
cd build

# 运行cmake
cmake .. -DCMAKE_CXX_FLAGS="-DDEBUG -DINFO"
if [ $? -ne 0 ]; then
    echo "CMake失败，错误代码 $?"
    cd ..
    exit $?
fi

# 运行make构建项目
make
if [ $? -ne 0 ]; then
    echo "构建失败，错误代码 $?"
    cd ..
    exit $?
fi

# 返回上级目录
cd ..
echo "构建成功！"

# 运行Python脚本
python run/run.py run/interactor run/sample_official.in ./code_craft