@echo off

:: setup VC environment variables
set VCVARSALL_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
call %VCVARSALL_PATH%

:: These environment variables can also be set externally
if not defined JOM_PATH (
	set JOM_PATH=c:\Qt\5.15.2\Tools\QtCreator\bin
)

if not defined CMAKE_PREFIX_PATH (
	set CMAKE_PREFIX_PATH=c:\Qt\5.15.2\msvc2019
)

:: add search path for jom.exe
set PATH=%PATH%;%JOM_PATH%

:: create and change into build subdir
mkdir bb_VC
pushd bb_VC

:: configure makefiles and build
cmake -G "NMake Makefiles JOM" .. -DCMAKE_BUILD_TYPE:String="Release"
jom
if ERRORLEVEL 1 GOTO fail

popd


:: copy executable to bin/release dir
xcopy /Y .\bb_VC\OpenHAMSolver\OpenHAMSolver.exe ..\..\bin\release

exit /b 0

:fail
exit /b 1


