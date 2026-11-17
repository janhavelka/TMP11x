@echo off
setlocal

if defined PLATFORMIO_CORE_DIR (
    set "PIO_EXE=%PLATFORMIO_CORE_DIR%\penv\Scripts\pio.exe"
) else (
    set "PIO_EXE=%USERPROFILE%\.platformio\penv\Scripts\pio.exe"
)

if not exist "%PIO_EXE%" (
    >&2 echo VS Code-managed PlatformIO was not found at: "%PIO_EXE%". Stop and report the missing installation; do not install another PlatformIO Core.
    exit /b 1
)

"%PIO_EXE%" %*
exit /b %ERRORLEVEL%
