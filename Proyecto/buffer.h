/*************************************************************
Autores: Juan David Rincón - Juan Felipe Morales
Fecha: 27 de abril de 2024
Materia: Sistemas Operativos
Tema: Proyecto
Fichero: Interfaz de funciones del buffer
**************************************************************/

#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// Definiciones de banderas para identificar tipos de datos
#define TEMPERATURA_FLAG "TEMP" // Bandera para datos de temperatura
#define PH_FLAG "PH"            // Bandera para datos de pH

// Declaración de variables y estructuras globales
extern int BUFFER_SIZE;          // Tamaño del buffer
extern sem_t empty_temp, full_temp, empty_ph, full_ph; // Semáforos para controlar el acceso al buffer
extern pthread_mutex_t mutex_temp, mutex_ph;           // Mutex para garantizar la exclusión mutua al acceder al buffer
extern char **buffer_temp;       // Buffer para datos de temperatura
extern char **buffer_ph;         // Buffer para datos de pH
extern int in_temp, out_temp;    // Índices de entrada y salida para el buffer de temperatura
extern int in_ph, out_ph;        // Índices de entrada y salida para el buffer de pH
extern char *file_temp, *file_ph; // Nombres de los archivos de datos de temperatura y pH

// Prototipos de funciones
void ini_buffers();            // Inicializa los buffers
void free_memory_from_buffers();           // Libera la memoria asignada para los buffers
void *recolector(void *param); // Función para recolectar datos
void *ph_thread_collec();      // Función de hilo para recolectar datos de pH
void *temper_thread_collec();  // Función de hilo para recolectar datos de temperatura

#endif // BUFFER_H