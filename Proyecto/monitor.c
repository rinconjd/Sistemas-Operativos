/*************************************************************
Autores: Juan David Rincón - Juan Felipe Morales
Fecha: 27 de abril de 2024
Materia: Sistemas Operativos
Tema: Proyecto
Archivo: Manejo de monitor.c
**************************************************************/

#include "buffer.h"
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int flags; // Almacena las flags de los argumentos de línea de comandos
  char *pipe_name = NULL;   // Puntero al nombre del pipe nominal

  while ((flags = getopt(argc, argv, "b:t:h:p:")) != -1) {
    switch (flags) {
      case 'b': // Bandera del tamaño del Buffer 
        BUFFER_SIZE = atoi(optarg);
        break;
      case 't': // Bandera del archivo de temperature
        file_temp = optarg;
        break;
      case 'h': // Bandera del archivo de pH 
        file_ph = optarg;
        break;
      case 'p': // Bandera del nombre del Pipe
        pipe_name = optarg;
        break;
      default: // Mensaje de uso en caso de argumentos incorrectos
        fprintf(stderr,
                "Usage: %s -b <buffer_size> -t <file-temp> -h <file-ph> -p "
                "<pipe-name>\n",
                argv[0]);
        return 1;
    }
  }

  // Verificar la validez de los nombres de archivo
  if (!file_temp || !file_ph) {
    fprintf(stderr, "Error opening file.\n");
    exit(1);
  }

  // Crear el pipe si no existe
  if (access(pipe_name, F_OK) == -1) {
    printf("Pipe created: '%s'\n", pipe_name);

    if (mkfifo(pipe_name, 0666) == -1) {
      perror("Error creating the pipe");
      exit(1);
    }
  } else {
    printf("Pipe already exists. Should not be created.\n");
  }

  // Inicializar semáforos para la sincronización de acceso a los buffers
  printf("Initializing semaphores...\n");
  // Inicializar semáforo de espacios vacíos en el buffer de temperatura con el tamaño del buffer
  sem_init(&empty_temp, 0, BUFFER_SIZE);
  // Inicializar semáforo de espacios llenos en el buffer de temperatura como 0 (inicialmente vacío)
  sem_init(&full_temp, 0, 0);
  // Inicializar semáforo de espacios vacíos en el buffer de pH con el tamaño del buffer
  sem_init(&empty_ph, 0, BUFFER_SIZE);
  // Inicializar semáforo de espacios llenos en el buffer de pH como 0 (inicialmente vacío)
  sem_init(&full_ph, 0, 0);
  // Inicializar mutex para el buffer de temperatura
  pthread_mutex_init(&mutex_temp, NULL);
  // Inicializar mutex para el buffer de pH
  pthread_mutex_init(&mutex_ph, NULL);

  
  // Inicializar buffers
  ini_buffers();
  printf("Buffers initialized: 2\n");
  printf("──────────────────────────────────────────\n");

  // Crear hilo para recolectar datos, hilo para recolectar datos de pH e hilo para recolectar datos de temperatura
  pthread_t recolector_thread, ph_thread, temperatura_thread;

  // Crear hilos para ejecutar las funciones correspondientes
  pthread_create(&recolector_thread, NULL, recolector, pipe_name); // Hilo para recolectar datos del pipe
  pthread_create(&ph_thread, NULL, ph_thread_collec, NULL); // Hilo para recolectar datos de pH
  pthread_create(&temperatura_thread, NULL, temper_thread_collec, NULL); // Hilo para recolectar datos de temperatura

  // Esperar a que los hilos terminen su ejecución antes de continuar
  pthread_join(recolector_thread, NULL); // Esperar a que termine el hilo de recolección
  pthread_join(ph_thread, NULL); // Esperar a que termine el hilo de recolección de datos de pH
  pthread_join(temperatura_thread, NULL); // Esperar a que termine el hilo de recolección de datos de temperatura

  // Destruir semáforos y mutex para liberar los recursos
  sem_destroy(&empty_temp); // Liberar semáforo de espacio vacío en buffer de temperatura
  sem_destroy(&full_temp); // Liberar semáforo de espacio lleno en buffer de temperatura
  sem_destroy(&empty_ph); // Liberar semáforo de espacio vacío en buffer de pH
  sem_destroy(&full_ph); // Liberar semáforo de espacio lleno en buffer de pH
  pthread_mutex_destroy(&mutex_temp); // Liberar mutex para acceso al buffer de temperatura
  pthread_mutex_destroy(&mutex_ph); // Liberar mutex para acceso al buffer de pH

  // Liberar memoria asignada para los buffers
  free_memory_from_buffers();

  return 0;
}