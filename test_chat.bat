@echo off
echo Starting Chat Room Test...
echo.

echo Starting server in background...
start "Chat Server" cmd /c "chatServer.exe 8080 TestRoom"

echo Waiting for server to start...
timeout /t 3 /nobreak >nul

echo Starting client 1 (Alice)...
start "Client 1 - Alice" cmd /c "clientApp.exe 127.0.0.1 8080 Alice"

echo Starting client 2 (Bob)...
start "Client 2 - Bob" cmd /c "clientApp.exe 127.0.0.1 8080 Bob"

echo.
echo Chat test started!
echo - Server is running on port 8080
echo - Two clients are connected (Alice and Bob)
echo - Type messages in each client window
echo - Close windows to stop the test
echo.
pause
