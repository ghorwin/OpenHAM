@echo off

set RELDIR=OpenHAM-0.4.0-Win64

cd ..\..\build\cmake
call build_VC_x64.bat
cd ..\..\release\windows
mkdir %RELDIR%
copy ..\..\bin\release_x64\OpenHAMSolver.exe %RELDIR%\
copy VC14_x64_DLLs\* %RELDIR%\
copy ..\..\LICENSE %RELDIR%\
copy ..\..\AUTHORS %RELDIR%\
mkdir %RELDIR%\examples
xcopy /y /s /e ..\..\data\examples %RELDIR%\examples

