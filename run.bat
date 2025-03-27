@echo off 
REM 创建并切换到build目录
if not exist build mkdir build
cd build

REM 运行CMake生成MinGW Makefiles
cmake .. -G "MinGW Makefiles"
if %ERRORLEVEL% NEQ 0 (
    echo CMake failed with error code %ERRORLEVEL%
    cd ..
    exit /b %ERRORLEVEL%
)

REM 运行mingw32-make进行编译
mingw32-make
if %ERRORLEVEL% NEQ 0 (
    echo Build failed with error code %ERRORLEVEL%
    cd ..
    exit /b %ERRORLEVEL%
)

REM 编译成功
cd ..
echo Build successful!
python run/run.py run/interactor.exe run/sample_practice.in code_craft.exe -r 10000 20000 30000 40000 50000 60000 70000 80000