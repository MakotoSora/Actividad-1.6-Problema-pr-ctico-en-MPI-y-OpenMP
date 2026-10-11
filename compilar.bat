@echo off
REM ============================================================
REM  Practica 1.6 - Compilacion con MSVC + MS-MPI + OpenMP
REM  Ejecutar desde "x64 Native Tools Command Prompt for VS"
REM  en la carpeta C:\MPI\Proyecto_MPI
REM ============================================================
cd /d C:\MPI\Proyecto_MPI

cl /EHsc /O2 /openmp /I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" practica1_6.cpp /link /LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" msmpi.lib /out:practica1_6.exe

if errorlevel 1 (
    echo [ERROR] La compilacion fallo.
    exit /b 1
)

echo [OK] practica1_6.exe generado en C:\MPI\Proyecto_MPI
