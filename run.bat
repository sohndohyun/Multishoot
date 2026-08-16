@echo off
setlocal EnableExtensions

set "configuration=%~2"
if not defined configuration set "configuration=Debug"
set "bin=%~dp0bld\x64\%configuration%"
set "server_bin=%bin%\MultishootServer"
set "client_bin=%bin%\Multishoot"
set "test_bin=%bin%\MultishootCommonTests"

if "%~1"=="" set "menu=1" & goto menu
if /i "%~1"=="server" goto server
if /i "%~1"=="client" goto client
if /i "%~1"=="test" goto test
if /i "%~1"=="db-up" goto db_up
if /i "%~1"=="db-down" goto db_down
if /i "%~1"=="db-reset" goto db_reset

echo Usage: %~nx0 ^<server^|client^|test^|db-up^|db-down^|db-reset^> [Debug^|Release] [server arguments]
exit /b 2

:menu
choice /c SCT /n /m "Select [S]erver, [C]lient, or [T]est: "
if errorlevel 3 goto test
if errorlevel 2 goto client
goto server

:server
call :require "%server_bin%\MultishootServer.exe" || goto failed
set "server_args=%~3 %~4 %~5 %~6 %~7 %~8 %~9"
"%server_bin%\MultishootServer.exe" %server_args%
set "result=%errorlevel%"
goto done

:client
call :require "%client_bin%\Multishoot.exe" || goto failed
pushd "%~dp0Multishoot"
"%client_bin%\Multishoot.exe"
set "result=%errorlevel%"
popd
goto done

:test
call :require "%test_bin%\MultishootCommonTests.exe" || goto failed
call :require "%server_bin%\MultishootServer.exe" || goto failed
call :find_docker || goto failed
set "db_status_file=%TEMP%\multishoot-compose-status-%RANDOM%.txt"
"%docker_path%" compose ps --status running --services > "%db_status_file%" 2>nul
findstr /i "mysql" "%db_status_file%" >nul
set "db_running=%errorlevel%"
del /q "%db_status_file%" >nul 2>&1
if not "%db_running%"=="0" (
    echo MySQL Compose service is not running. Run "%~nx0 db-up" first.
    goto failed
)
"%test_bin%\MultishootCommonTests.exe" || goto failed
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tests\NetworkSmoke.ps1" -ServerPath "%server_bin%\MultishootServer.exe" -DatabaseName "multishoot_test"
set "result=%errorlevel%"
goto done

:db_up
call :docker compose up -d --wait
set "result=%errorlevel%"
goto done

:db_down
call :docker compose down
set "result=%errorlevel%"
goto done

:db_reset
echo This removes the Multishoot MySQL volume and all local account data.
call :docker compose down -v || goto failed
call :docker compose up -d --wait
set "result=%errorlevel%"
goto done

:find_docker
where docker.exe >nul 2>&1
if not errorlevel 1 (
    set "docker_path=docker.exe"
    exit /b 0
)
set "docker_path=%LOCALAPPDATA%\Programs\DockerDesktop\resources\bin\docker.exe"
if exist "%docker_path%" (
    set "PATH=%LOCALAPPDATA%\Programs\DockerDesktop\resources\bin;%PATH%"
    exit /b 0
)
set "docker_path=%ProgramFiles%\Docker\Docker\resources\bin\docker.exe"
if exist "%docker_path%" (
    set "PATH=%ProgramFiles%\Docker\Docker\resources\bin;%PATH%"
    exit /b 0
)
echo Docker CLI was not found in PATH or the Docker Desktop installation path.
exit /b 1

:docker
call :find_docker || exit /b 1
"%docker_path%" %*
exit /b %errorlevel%

:failed
set "result=1"

:done
if defined menu pause
exit /b %result%

:require
if exist "%~1" exit /b 0
echo Missing executable: %~1
echo Build first: msbuild "%~dp0Multishoot.sln" /m /p:Configuration=%configuration% /p:Platform=x64
exit /b 1
