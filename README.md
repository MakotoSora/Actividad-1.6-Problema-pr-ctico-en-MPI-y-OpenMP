# Práctica 1.6 — Algoritmo de Fuerza Bruta Distribuido con MPI y OpenMP

Aplicación paralela y distribuida (arquitectura **Maestro-Esclavo**) que descifra una
clave alfanumérica (A–Z, 0–9) por fuerza bruta. El espacio de búsqueda `36^N` se reparte
entre procesos **MPI**, y cada esclavo paraleliza su rango con **OpenMP**. Al hallar la
clave se dispara una **parada temprana** que detiene a todos los nodos. Cada proceso
escribe su propio **log de auditoría**.

## Equipo 11 (orden alfabético por primer apellido)
- Hermosillo Prado Carlos
- Montes de Oca del Risco Rafael Alejandro
- Ramírez Andrade Uriel Ismael Guadalupe

Materia: Programación Paralela (2026B_IH067_PP_V) · Docente: José Navarro Ríos · Grupo: Vespertino · Universidad de Guadalajara.

## Archivos
| Archivo | Para qué sirve |
|---|---|
| `practica1_6.cpp` | Código fuente completo |
| `compilar.bat` | Compilar en Windows con Visual Studio + MS-MPI |
| `ejecutar_local.bat` | Correr 5 procesos en una sola PC (60%) |
| `ejecutar_red.bat` | Correr en el clúster de 3 PCs (100%) |
| `hosts.txt` | Hostfile de MPI con las IPs del clúster |

## Cómo compilar y ejecutar (Windows — entrega real)
1. Copia todos los archivos a `C:\MPI\Proyecto_MPI` en cada PC.
2. Abre **"x64 Native Tools Command Prompt for VS"** y ejecuta `compilar.bat`.
3. **Local (60%):** `ejecutar_local.bat`
4. **Red (100%):** desde la PC maestra `ejecutar_red.bat` (ajusta las IPs de `hosts.txt` / del `.bat`).

## Menú del programa
1. Ingresar clave (A–Z, 0–9)
2. Búsqueda **secuencial** (línea base: 1 nodo, 1 hilo)
3. Búsqueda **paralela distribuida** (MPI + OpenMP)
4. Tabla de tiempos, **Speedup** y **Eficiencia**
5. Salir

## Secuencia de prueba sugerida
- **Clave de 3 caracteres** (ej. `X7B`): demo detallada con logs por nodo/hilo y parada temprana.
- **Clave de 7 caracteres**: espacio masivo (~78 mil millones) para medir el Speedup real. Opciones 2 → 3 → 4.

## Mapeo del espacio de búsqueda
Cada combinación de `N` caracteres se mapea a un entero en **base 36** (`0 … 36^N − 1`):
`36^3 = 46 656`, `36^7 ≈ 78.3 mil millones`, `36^10 ≈ 3.65 × 10^15`.

## IPs del clúster (ejemplo)
- `192.168.137.1`  → rank 0 (maestro)
- `192.168.137.165` → 2 esclavos
- `192.168.137.153` → 2 esclavos

Comando de red:
```
mpiexec -hosts 3 192.168.137.1 1 192.168.137.165 2 192.168.137.153 2 C:\MPI\Proyecto_MPI\practica1_6.exe
```
Equivalente con hostfile: `mpiexec --hostfile hosts.txt -np 5 ./practica1_6`
