@echo off
echo 🧹 Preparing for GitHub...

echo Removing build directory...
rmdir /s /q build 2>nul

echo Removing executables...
del spider\*.exe 2>nul
del http_server\*.exe 2>nul

echo Removing DLL files...
del spider\*.dll 2>nul
del http_server\*.dll 2>nul

echo Removing object files...
del spider\*.obj 2>nul
del http_server\*.obj 2>nul

echo ✅ Ready for GitHub!
echo 📋 Files to commit:
dir /s /b *.cpp *.h *.md *.txt *.ini *.bat

pause