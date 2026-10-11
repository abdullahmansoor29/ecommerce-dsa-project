@echo off
set PATH=%PATH%;C:\msys64\ucrt64\bin
start "C++ Backend" cmd /k "cd /d D:\ecommerce\backend && server.exe"
timeout /t 2 >nul
start "Streamlit Frontend" cmd /k "cd /d D:\ecommerce\frontend && python -m streamlit run app.py"