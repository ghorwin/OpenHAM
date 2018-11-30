@echo off

set PATH=%PATH%;c:\Python27

python ..\..\scripts\TestSuite\run_tests.py -p ..\..\data\tests -s ..\..\bin\release_x64\OpenHAMSolver.exe -e d6p

pause
