@echo off
REM ============================================================
REM  Practica 1.6 - Ejecucion DISTRIBUIDA EN RED (100%)
REM  Cluster de 3 PCs con MS-MPI (5 procesos en total):
REM    192.168.137.1   -> 1 proceso  (rank 0 = MAESTRO)
REM    192.168.137.165 -> 2 procesos (esclavos)
REM    192.168.137.153 -> 2 procesos (esclavos)
REM
REM  Antes de ejecutar, en CADA PC:
REM    1. Servicio de MS-MPI activo (smpd -d o MsMpiLaunchSvc)
REM    2. practica1_6.exe en C:\MPI\Proyecto_MPI
REM    3. Firewall permitiendo mpiexec, smpd y practica1_6.exe
REM
REM  Equivalente OpenMPI/MPICH: mpirun --hostfile hosts.txt -np 5 ./practica1_6
REM ============================================================
cd /d C:\MPI\Proyecto_MPI
mpiexec -hosts 3 192.168.137.1 1 192.168.137.165 2 192.168.137.153 2 C:\MPI\Proyecto_MPI\practica1_6.exe
