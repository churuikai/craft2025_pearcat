#!/bin/bash

# 检查是否提供了目录参数
if [ $# -eq 0 ]; then
    echo "用法: $0 <目录路径> [输出zip文件名]"
    exit 1
fi

# 获取参数
SOURCE_DIR="$1"
OUTPUT_ZIP="${2:-code_archive.zip}"

# 检查源目录是否存在
if [ ! -d "$SOURCE_DIR" ]; then
    echo "错误: 目录 '$SOURCE_DIR' 不存在"
    exit 1
fi

echo "正在收集文件..."

# 创建临时目录用于存放文件列表
TEMP_FILE=$(mktemp)

# 查找所有 .cpp, .h 和 CMakeLists.txt 文件，排除CMakeFiles目录
find "$SOURCE_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "CMakeLists.txt" \) -not -path "*/CMakeFiles/*" -not -path "*/build/*" > "$TEMP_FILE"

# 显示找到的文件数量
FILE_COUNT=$(wc -l < "$TEMP_FILE")
echo "找到 $FILE_COUNT 个文件"

# 如果没有找到文件，则退出
if [ "$FILE_COUNT" -eq 0 ]; then
    echo "没有找到匹配的文件"
    rm "$TEMP_FILE"
    exit 1
fi

# 获取OUTPUT_ZIP的绝对路径，因为我们将切换目录
CURRENT_DIR=$(pwd)
if [[ "$OUTPUT_ZIP" != /* ]]; then
    OUTPUT_ZIP="$CURRENT_DIR/$OUTPUT_ZIP"
fi

# 如果ZIP文件已经存在，先删除它
if [ -f "$OUTPUT_ZIP" ]; then
    echo "发现已存在的 $OUTPUT_ZIP，正在删除..."
    rm "$OUTPUT_ZIP"
fi

# 创建zip文件，保持目录结构
echo "正在创建 $OUTPUT_ZIP ..."
# 切换到源目录，这样zip会保持相对路径结构
cd "$SOURCE_DIR"
zip -r "$OUTPUT_ZIP" -@ < "$TEMP_FILE"

# 检查zip命令是否成功
if [ $? -eq 0 ]; then
    echo "成功创建 $OUTPUT_ZIP"
else
    echo "创建zip文件时出错"
fi

# 清理临时文件
rm "$TEMP_FILE" 