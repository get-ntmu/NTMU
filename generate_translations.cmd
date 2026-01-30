@echo off

where npm >nul 2>nul
if (%ERRORLEVEL% equ 0) (
    echo Node.js is not installed. Please install that and rerun the script.
    pause
    goto Done
)

cd %~dp0
pushd msgmap

npm install --dry-run | findstr /C:"up to date" >nul 2>nul

if errorlevel 1 (
    echo Node packages outdated. Installing.
    call npm install
)

popd

call node .\msgmap\msgmap.js -w -o src\translations translations

:Done