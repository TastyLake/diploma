@echo off
cd /d "C:\diploma"
set PATH=C:\vcpkg\installed\x64-windows\bin;%PATH%

echo 🚀 Quick Launch - Search Engine
echo.
echo Starting components...
start "Spider" cmd /k "cd spider && SpiderApp.exe"
timeout /t 3
start "Server" cmd /k "cd http_server && HttpServerApp.exe"
timeout /t 2

echo 📍 Opening browser...
start http://localhost:8080

echo.
echo ✅ System is running!
echo 💡 Spider and Server are in separate windows
echo 🔍 Open http://localhost:8080 to search
pause