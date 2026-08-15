@echo off
setlocal

set "PROTOC_EXE=%~dp0bin\protoc.exe"
set "SCHEMA_DIR=%~dp0schema"
set "CPP_OUT_DIR=%~dp0generated\cpp"
set "PROTO_FILES="

if not exist "%PROTOC_EXE%" (
    echo ERROR: protoc.exe not found: "%PROTOC_EXE%"
    exit /b 1
)

mkdir "%CPP_OUT_DIR%" 2>nul

del /S /Q "%CPP_OUT_DIR%\*.pb.cc" 2>nul
del /S /Q "%CPP_OUT_DIR%\*.pb.h" 2>nul

for /R "%SCHEMA_DIR%" %%F in (*.proto) do (
    call set "PROTO_FILES=%%PROTO_FILES%% "%%F""
)

if "%PROTO_FILES%"=="" (
    echo ERROR: No .proto files found in "%SCHEMA_DIR%".
    exit /b 1
)

"%PROTOC_EXE%" -I"%SCHEMA_DIR%" --cpp_out="%CPP_OUT_DIR%" %PROTO_FILES%

if errorlevel 1 (
    echo ERROR: Protobuf compilation failed.
    exit /b 1
)

echo Protobuf compilation completed: "%CPP_OUT_DIR%"
