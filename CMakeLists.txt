cmake_minimum_required(VERSION 3.8)  # 不要修改
project(CodeCraft)                   # 不要修改

set(CMAKE_CXX_STANDARD      17)      # 不要修改
set(CMAKE_C_STANDARD        11)      # 不要修改

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/) # 不要修改
option(BUILD_SHARED_LIBS    ""  OFF) # 不要修改

# 收集当前目录下的所有源文件
aux_source_directory(./                     cur_src) # 不要修改

# 添加编译优化选项
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-unused-result -fsanitize=undefined")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-unused-result")

# 添加包含目录（当前目录）
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 添加所有源文件到可执行文件
add_executable(code_craft                   ${cur_src}) # 不要修改名称