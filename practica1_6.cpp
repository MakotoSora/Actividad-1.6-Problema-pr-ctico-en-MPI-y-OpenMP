// ============================================================================
//  Practica 1.6 - Algoritmo de Fuerza Bruta Distribuido con MPI y OpenMP
//  Arquitectura Maestro-Esclavo
//
//  Alfabeto: 36 caracteres -> A..Z (26) y 0..9 (10)
//  El maestro (rank 0) captura la clave, reparte rangos y coordina.
//  Cada esclavo usa OpenMP para probar combinaciones en sus nucleos.
//  Al encontrarla, se avisa al maestro y se detiene a todos (parada temprana).
//  Cada nodo escribe su propio archivo de log.
//
//  Compilar y ejecutar: ver compilar.bat / ejecutar_local.bat / ejecutar_red.bat
// ============================================================================

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <mpi.h>
#include <omp.h>

using namespace std;

// ---- Etiquetas (tags) de los mensajes MPI ----
#define TAG_RANGO     1   // maestro -> esclavo : [inicio, fin]
#define TAG_HALLAZGO  2   // esclavo -> maestro : clave encontrada
#define TAG_FIN       3   // esclavo -> maestro : termine mi rango (sin exito)
#define TAG_PARA      4   // maestro -> esclavo : detente (parada temprana)

// Alfabeto permitido. El indice (0..35) de cada caracter define el mapeo base 36.
const char *ALFABETO = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// Tamano de bloque que cada esclavo procesa antes de revisar si debe detenerse.
const unsigned long long CHUNK = 1000000ULL;

// ============================================================================
//  EQUIPO: nombres completos, iniciando por primer apellido en ORDEN ALFABETICO
//  >>> REVISA Y AJUSTA estos nombres segun tu equipo real de la 1.6 <<<
// ============================================================================
const char *NOMBRES_EQUIPO[] = {
    "Hermosillo Prado Carlos",
    "Montes de Oca del Risco Rafael Alejandro",
    "Ramirez Andrade Uriel Ismael Guadalupe"
};
const int NUM_INTEGRANTES = 3;

void imprimirEquipo()
{
    cout << "============================================================\n";
    cout << " EQUIPO 11:\n";
    for (int i = 0; i < NUM_INTEGRANTES; i++)
        cout << "   - " << NOMBRES_EQUIPO[i] << "\n";
    cout << "============================================================\n";
}

// 36^n  (n hasta 10 cabe en unsigned long long)
unsigned long long potencia36(int n)
{
    unsigned long long r = 1;
    for (int i = 0; i < n; i++) r *= 36ULL;
    return r;
}

// Convierte un indice numerico a su cadena equivalente en base 36.
// candidato debe tener espacio para (longitud + 1) caracteres.
void convertirIndiceATexto(unsigned long long idx, char *candidato, int longitud)
{
    for (int pos = longitud - 1; pos >= 0; pos--) {
        candidato[pos] = ALFABETO[idx % 36];
        idx /= 36;
    }
    candidato[longitud] = '\0';
}

// Valida que la clave solo tenga A-Z y 0-9 y mida entre 1 y 10.
bool claveValida(const char *s, int len)
{
    if (len < 1 || len > 10) return false;
    for (int i = 0; i < len; i++) {
        char c = s[i];
        bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (!ok) return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Nombre de este equipo/PC (para el log y la auditoria)
    char nombrePC[MPI_MAX_PROCESSOR_NAME];
    int lenNombre;
    MPI_Get_processor_name(nombrePC, &lenNombre);

    // ---- PRIMERA instruccion: nombres del equipo (solo el maestro) ----
    if (rank == 0) imprimirEquipo();

    // ---- Cada nodo abre su propio archivo de log ----
    char nombreLog[256];
    sprintf(nombreLog, "log_equipo_%s_nodo_%d.txt", nombrePC, rank);
    ofstream log(nombreLog);
    log << "============================================================\n";
    log << " LOG DE AUDITORIA - Practica 1.6\n";
    log << " Equipo (PC): " << nombrePC << "\n";
    log << " Rango MPI: " << rank << " de " << size << " procesos\n";
    log << " Hilos OpenMP activos: " << omp_get_max_threads() << "\n";
    log << "============================================================\n";
    log.flush();

    // Datos de la clave (buffer dinamico, como pide la practica)
    char *target = new char[12];   // hasta 10 caracteres + fin de cadena + margen
    target[0] = '\0';
    int longitud = 0;
    bool tieneClave = false;

    // Tiempos para la tabla comparativa (solo el maestro los usa)
    double tSecuencial = 0.0;
    double tParalelo   = 0.0;

    int opcion = 0;

    // ========================= BUCLE DEL MENU ================================
    // El maestro muestra el menu y transmite la opcion a todos con MPI_Bcast.
    while (true) {

        if (rank == 0) {
            cout << "\n===================== MENU =====================\n";
            cout << " 1) Ingresar clave (mayusculas A-Z, 0-9)\n";
            cout << " 2) Buscar en modo SECUENCIAL (1 nodo, 1 hilo)\n";
            cout << " 3) Buscar en modo PARALELO DISTRIBUIDO (MPI + OpenMP)\n";
            cout << " 4) Mostrar tabla de tiempos y Speedup\n";
            cout << " 5) Salir\n";
            cout << " Opcion: ";
            cin >> opcion;
        }
        MPI_Bcast(&opcion, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (opcion == 5) break;   // todos salen

        // ---------------- Opcion 1: ingresar clave ----------------
        if (opcion == 1) {
            if (rank == 0) {
                char *entrada = new char[64];
                cout << " Escribe la clave: ";
                cin >> entrada;
                // Convertir letras a mayusculas por comodidad
                for (int i = 0; entrada[i] != '\0'; i++)
                    if (entrada[i] >= 'a' && entrada[i] <= 'z')
                        entrada[i] -= 32;

                int len = (int)strlen(entrada);
                if (claveValida(entrada, len)) {
                    strcpy(target, entrada);
                    longitud = len;
                    cout << " [OK] Clave aceptada: " << target
                         << "  (" << longitud << " caracteres, "
                         << potencia36(longitud) << " combinaciones)\n";
                } else {
                    longitud = 0;
                    cout << " [ERROR] Clave invalida. Usa solo A-Z y 0-9 (1 a 10 caracteres).\n";
                }
                delete[] entrada;
            }
            // Avisar a todos la longitud; si es 0 fue invalida.
            MPI_Bcast(&longitud, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (longitud > 0) {
                MPI_Bcast(target, longitud + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
                tieneClave = true;
                if (rank == 0) {
                    log << "\n[CONFIG] Clave objetivo cargada. Longitud = "
                        << longitud << "\n";
                    log.flush();
                }
            }
            continue;
        }

        // Para las opciones de busqueda necesitamos una clave valida.
        if ((opcion == 2 || opcion == 3) && !tieneClave) {
            if (rank == 0)
                cout << " [AVISO] Primero ingresa una clave con la opcion 1.\n";
            continue;
        }

        // ---------------- Opcion 2: busqueda SECUENCIAL ----------------
        // Linea base: solo el maestro, un hilo, recorriendo todo el espacio.
        if (opcion == 2) {
            if (rank == 0) {
                cout << "\n>>> Modo SECUENCIAL (linea base)...\n";
                log << "\n[SECUENCIAL] Inicio de busqueda lineal.\n"; log.flush();

                unsigned long long T = potencia36(longitud);
                char *cand = new char[longitud + 1];
                bool hallada = false;
                unsigned long long idxHit = 0;

                double t0 = MPI_Wtime();
                for (unsigned long long idx = 0; idx < T; idx++) {
                    convertirIndiceATexto(idx, cand, longitud);
                    if (strcmp(cand, target) == 0) {
                        hallada = true;
                        idxHit = idx;
                        break;
                    }
                }
                double t1 = MPI_Wtime();
                tSecuencial = t1 - t0;

                if (hallada)
                    cout << " [EXITO] Clave '" << target << "' encontrada en indice "
                         << idxHit << ".\n";
                cout << " Tiempo secuencial: " << tSecuencial << " s\n";

                log << "[SECUENCIAL] Clave='" << target << "' Indice=" << idxHit
                    << " Tiempo=" << tSecuencial << "s\n";
                log.flush();
                delete[] cand;
            }
            continue;
        }

        // ---------------- Opcion 3: busqueda PARALELA DISTRIBUIDA ----------------
        if (opcion == 3) {

            // Con una sola clave de 3 caracteres mostramos TODO el detalle;
            // con claves grandes (7/10) desactivamos las impresiones paso a paso.
            bool modoDetallado = (longitud <= 3);

            int E = size - 1;   // numero de esclavos
            if (E < 1) {
                if (rank == 0)
                    cout << " [ERROR] Se necesita al menos 1 esclavo (ejecuta con >= 2 procesos).\n";
                continue;
            }

            // ============ MAESTRO (rank 0) ============
            if (rank == 0) {
                cout << "\n>>> Modo PARALELO DISTRIBUIDO con " << E
                     << " esclavos...\n";
                log << "\n[PARALELO] Reparto de rangos entre " << E << " esclavos.\n";

                unsigned long long T     = potencia36(longitud);
                unsigned long long base  = T / E;
                unsigned long long resto = T % E;

                double t0 = MPI_Wtime();

                // Repartir [inicio, fin] a cada esclavo (buffer dinamico)
                for (int i = 0; i < E; i++) {
                    unsigned long long *rango = new unsigned long long[2];
                    unsigned long long inicio = (unsigned long long)i * base
                                              + (i < (int)resto ? (unsigned long long)i : resto);
                    unsigned long long tam = base + (i < (int)resto ? 1ULL : 0ULL);
                    rango[0] = inicio;
                    rango[1] = inicio + tam - 1;
                    MPI_Send(rango, 2, MPI_UNSIGNED_LONG_LONG, i + 1, TAG_RANGO, MPI_COMM_WORLD);
                    log << "   Esclavo " << (i + 1) << " -> [" << rango[0]
                        << ", " << rango[1] << "]\n";
                    delete[] rango;
                }
                log.flush();

                // Escuchar resultados: cada esclavo manda 1 mensaje final.
                int terminados = 0;
                bool hallada = false;
                int nodoGanador = -1;
                char *claveHit = new char[longitud + 1];
                claveHit[0] = '\0';

                while (terminados < E) {
                    char *buf = new char[longitud + 1];
                    MPI_Status st;
                    MPI_Recv(buf, longitud + 1, MPI_CHAR, MPI_ANY_SOURCE,
                             MPI_ANY_TAG, MPI_COMM_WORLD, &st);

                    if (st.MPI_TAG == TAG_HALLAZGO) {
                        hallada = true;
                        nodoGanador = st.MPI_SOURCE;
                        strcpy(claveHit, buf);
                        // Parada temprana: avisar a TODOS los esclavos que paren.
                        char stop = 'X';
                        for (int s = 1; s < size; s++)
                            MPI_Send(&stop, 1, MPI_CHAR, s, TAG_PARA, MPI_COMM_WORLD);
                    }
                    terminados++;
                    delete[] buf;
                }

                // Seguridad: si nadie la encontro, liberar los MPI_Irecv de los esclavos.
                if (!hallada) {
                    char stop = 'X';
                    for (int s = 1; s < size; s++)
                        MPI_Send(&stop, 1, MPI_CHAR, s, TAG_PARA, MPI_COMM_WORLD);
                }

                double t1 = MPI_Wtime();
                tParalelo = t1 - t0;

                if (hallada) {
                    cout << " [EXITO] Clave '" << claveHit << "' encontrada por el nodo "
                         << nodoGanador << ".\n";
                    log << "[PARALELO][EXITO] Clave='" << claveHit
                        << "' Nodo ganador=" << nodoGanador << "\n";
                } else {
                    cout << " [SIN EXITO] La clave no se encontro en el espacio de busqueda.\n";
                }
                cout << " Tiempo paralelo: " << tParalelo << " s\n";
                cout.flush();   // mostrar el resultado de inmediato, sin esperar
                log << "[PARALELO] Tiempo=" << tParalelo << "s. Todos los procesos finalizaron.\n";
                log.flush();
                delete[] claveHit;
            }

            // ============ ESCLAVOS (rank 1..size-1) ============
            else {
                unsigned long long *rango = new unsigned long long[2];
                MPI_Recv(rango, 2, MPI_UNSIGNED_LONG_LONG, 0, TAG_RANGO,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                unsigned long long inicio = rango[0];
                unsigned long long fin    = rango[1];
                delete[] rango;

                log << "\n[PARALELO] Nodo " << rank << " inicia. Rango asignado ["
                    << inicio << ", " << fin << "]\n";
                log.flush();

                // Receptor NO bloqueante de la senal de parada del maestro.
                char stopBuf;
                MPI_Request stopReq;
                MPI_Irecv(&stopBuf, 1, MPI_CHAR, 0, TAG_PARA, MPI_COMM_WORLD, &stopReq);

                bool parar = false;
                bool encontrada = false;
                char *claveEncontrada = new char[longitud + 1];
                claveEncontrada[0] = '\0';

                // Cuantas lineas de muestra mostramos EN PANTALLA por nodo (modo
                // detallado). El LOG guarda TODAS; la consola solo unas pocas para
                // no inundarla y que el resultado final se vea claro.
                int lineasConsola = 0;
                const int MAX_CONSOLA = 40;

                // Tamano de bloque: en modo detallado (clave corta) usamos bloques
                // chicos para revisar MUY seguido la senal de parada, y asi la parada
                // temprana corta la impresion en cuanto se halla la clave. En claves
                // grandes (7/10) usamos el bloque grande para no perder eficiencia.
                unsigned long long chunkLocal = modoDetallado ? 500ULL : CHUNK;

                // Procesamos el rango por bloques. Entre bloques revisamos
                // si el maestro ya mando "parar" (parada temprana).
                for (unsigned long long cs = inicio;
                     cs <= fin && !encontrada && !parar; cs += chunkLocal) {

                    unsigned long long ce = cs + chunkLocal - 1;
                    if (ce > fin) ce = fin;
                    int n = (int)(ce - cs + 1);   // tamano del bloque (cabe en int)

                    // ---- Region paralela OpenMP: cada hilo prueba combinaciones ----
                    #pragma omp parallel shared(encontrada)
                    {
                        char *cand = new char[longitud + 1];   // buffer propio por hilo

                        #pragma omp for schedule(dynamic)
                        for (int off = 0; off < n; off++) {
                            if (encontrada) continue;  // otro hilo ya la hallo

                            unsigned long long idx = cs + (unsigned long long)off;
                            convertirIndiceATexto(idx, cand, longitud);

                            if (modoDetallado) {
                                #pragma omp critical
                                {
                                    // En el LOG siempre se guarda todo el detalle.
                                    log  << "[Equipo: " << nombrePC << "] [Nodo MPI: " << rank
                                         << "] [Hilo OpenMP: " << omp_get_thread_num()
                                         << "] [Candidato: " << cand << "] [Estado: Evaluando]\n";
                                    // En PANTALLA solo las primeras MAX_CONSOLA lineas.
                                    if (lineasConsola < MAX_CONSOLA) {
                                        cout << "[Equipo: " << nombrePC << "] [Nodo MPI: " << rank
                                             << "] [Hilo OpenMP: " << omp_get_thread_num()
                                             << "] [Candidato: " << cand << "] [Estado: Evaluando]\n";
                                        lineasConsola++;
                                        if (lineasConsola == MAX_CONSOLA)
                                            cout << "   ... (resto del detalle en el archivo de log del nodo " << rank << ")\n";
                                    }
                                }
                            }

                            if (strcmp(cand, target) == 0) {
                                #pragma omp critical
                                {
                                    encontrada = true;
                                    strcpy(claveEncontrada, cand);
                                    double tParcial = MPI_Wtime();
                                    cout << "[EXITO] [Equipo: " << nombrePC << "] [Nodo MPI: " << rank
                                         << "] [Hilo OpenMP: " << omp_get_thread_num()
                                         << "] [Clave Encontrada: \"" << cand
                                         << "\"] [Tiempo Parcial: " << tParcial << "s]\n";
                                    log  << "[EXITO] [Equipo: " << nombrePC << "] [Nodo MPI: " << rank
                                         << "] [Hilo OpenMP: " << omp_get_thread_num()
                                         << "] [Clave Encontrada: \"" << cand
                                         << "\"] [Tiempo Parcial: " << tParcial << "s]\n";
                                }
                            }
                        }
                        delete[] cand;
                    }

                    // Fuera de la region paralela: revisar si hay orden de parar.
                    int flag = 0;
                    MPI_Test(&stopReq, &flag, MPI_STATUS_IGNORE);
                    if (flag) parar = true;
                }

                // Enviar al maestro el mensaje final de este esclavo.
                if (encontrada) {
                    MPI_Send(claveEncontrada, longitud + 1, MPI_CHAR, 0,
                             TAG_HALLAZGO, MPI_COMM_WORLD);
                } else {
                    char vacio = '-';
                    MPI_Send(&vacio, 1, MPI_CHAR, 0, TAG_FIN, MPI_COMM_WORLD);
                }

                // Consumir la senal de parada que el maestro envia a todos.
                MPI_Wait(&stopReq, MPI_STATUS_IGNORE);

                log << "[PARALELO] Nodo " << rank << " finalizo correctamente.\n";
                log.flush();
                delete[] claveEncontrada;
            }

            continue;
        }

        // ---------------- Opcion 4: tabla comparativa ----------------
        if (opcion == 4) {
            if (rank == 0) {
                cout << "\n============== TABLA COMPARATIVA ==============\n";
                if (tSecuencial <= 0.0 || tParalelo <= 0.0) {
                    cout << " Ejecuta primero las opciones 2 y 3 para tener ambos tiempos.\n";
                } else {
                    int p = size;   // procesos MPI usados
                    double speedup = tSecuencial / tParalelo;
                    double eficiencia = speedup / p;
                    cout << " Tiempo Secuencial (T1): " << tSecuencial << " s\n";
                    cout << " Tiempo Paralelo   (Tp): " << tParalelo   << " s\n";
                    cout << " Procesos (p)          : " << p << "\n";
                    cout << " Speedup   Sp = T1/Tp  : " << speedup << "\n";
                    cout << " Eficiencia Ep = Sp/p  : " << eficiencia << "\n";
                }
                cout << "==============================================\n";
            }
            continue;
        }

        // ---------------- Opcion no valida ----------------
        if (rank == 0 && opcion != 1 && opcion != 2 && opcion != 3 && opcion != 4)
            cout << " [AVISO] Opcion no valida.\n";
    }
    // ======================= FIN DEL BUCLE DEL MENU =========================

    log << "\n[FIN] Proceso MPI " << rank << " finaliza limpiamente.\n";
    log.close();

    delete[] target;

    // ---- ULTIMA instruccion: nombres del equipo (solo el maestro) ----
    if (rank == 0) {
        cout << "\n Programa finalizado.\n";
        imprimirEquipo();
    }

    MPI_Finalize();
    return 0;
}
