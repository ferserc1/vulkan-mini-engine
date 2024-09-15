
echo off

set SHADERS_DIR=%1
set OUT_DIR=%2
set GLSLANG=%VULKAN_SDK%\Bin\glslang.exe

pushd .
cd %SHADERS_DIR%

if not exist "%OUT_DIR%" (
    mkdir %OUT_DIR%
)

echo Building shaders on directory %CD%
for /f "tokens=*" %%f in ('dir /b /a-d') do (
    REM ejecutar un echo con el nombre del archivo solo si la extensión es .glsl
    if "%%~xf"==".glsl" (
        echo %GLSLANG% -V %%f -o %OUT_DIR%\%%~nf.spv
        %GLSLANG% -V %%f -o %OUT_DIR%\%%~nf.spv
    )
)

popd
