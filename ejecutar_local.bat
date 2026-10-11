@echo off
REM ============================================================
REM  Practica 1.6 - Ejecucion LOCAL (60%)
REM  5 procesos MPI en esta PC: 1 maestro (nodo 0) + 4 esclavos (nodos 1-4)
REM  Cada esclavo usa los hilos OpenMP de esta maquina.
REM ============================================================
cd /d C:\MPI\Proyecto_MPI
mpiexec -n 5 C:\MPI\Proyecto_MPI\practica1_6.exe
