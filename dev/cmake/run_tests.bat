@echo off
setlocal

if --%1 == -- (set SO5_TARGET_PATH=target) else (set SO5_TARGET_PATH=%1)
if --%2 == -- (set SO5_BUILD_MODE=Release) else (set SO5_BUILD_MODE=%2)

set PATH=%PATH%;%~dp0\%SO5_TARGET_PATH%\bin
cmd /c ctest -C %SO5_BUILD_MODE% --force-new-ctest-process
if %errorlevel% neq 0 exit /b 1

endlocal
