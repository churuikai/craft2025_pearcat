@echo off
if not exist build mkdir build
cd build


@REM cmake .. -G "MinGW Makefiles"
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_FLAGS="-DDEBUG -DINFO"
if %ERRORLEVEL% NEQ 0 (
    echo CMake failed with error code %ERRORLEVEL%
    cd ..
    exit /b %ERRORLEVEL%
)

mingw32-make
if %ERRORLEVEL% NEQ 0 (
    echo Build failed with error code %ERRORLEVEL%
    cd ..
    exit /b %ERRORLEVEL%
)

cd ..
echo Build successful!
python run/run.py run/interactor.exe run/sample_practice.in code_craft.exe
