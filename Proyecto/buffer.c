/*************************************************************
Autores: Juan David Rincón - Juan Felipe Morales
Fecha: 27 de abril de 2024
Materia: Sistemas Operativos
Tema: Proyecto
Archivo: Implementación de las funciones de buffer
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// Definiciones de banderas para identificar tipos de datos
//Bandera para datos de temperatura
#define FLAG_TEMP "TEMP" 

// Bandera para datos de pH
#define FLAG_PH "PH"

// Arreglos de punteros para los buffers de temperatura y pH
char **buffertemp, **bufferph;

// Variables globales para el tamaño del buffer y los semáforos y mutex asociados
int BUFFER_SIZE;            // Tamaño del buffer
sem_t empty_temp, full_temp, empty_ph, full_ph; // Semáforos para controlar el acceso al buffer
pthread_mutex_t mutex_temp, mutex_ph;           // Mutex para garantizar la exclusión mutua al acceder al buffer

// Variables para los índices de entrada y salida de los buffers
int in_temp = 0, out_temp = 0; // Índices de entrada y salida para el buffer de temperatura
int in_ph = 0, out_ph = 0;     // Índices de entrada y salida para el buffer de pH

// Variables para almacenar los nombres de los archivos de datos de temperatura y pH
char *file_temp = NULL, *file_ph = NULL;


// Función para inicializar los buffers que almacenarán datos de temperatura y pH.
void ini_buffers() {
    // Se asigna memoria para el array de punteros a char que representan los buffers.
    buffertemp = (char **)malloc(BUFFER_SIZE * sizeof(char *));
    bufferph = (char **)malloc(BUFFER_SIZE * sizeof(char *));

    // Se verifica si la asignación de memoria fue exitosa.
    if (!buffertemp || !bufferph) {
        // Si falla la asignación, se imprime un mensaje de error y se sale del programa.
        perror("Error allocating memory for buffers");
        exit(1);
    }

    // Se asigna memoria para cada uno de los buffers individuales.
    for (int i = 0; i < BUFFER_SIZE; i++) {
        // Se asigna memoria para un buffer individual de tamaño 128 chars.
        buffertemp[i] = (char *)malloc(128 * sizeof(char));
        bufferph[i] = (char *)malloc(128 * sizeof(char));

        // Se verifica si la asignación de memoria fue exitosa.
        if (!buffertemp[i] || !bufferph[i]) {
            // Si falla la asignación, se imprime un mensaje de error y se sale del programa.
            perror("Error allocating memory for buffers");
            exit(1);
        }
    }
}

// Función para liberar la memoria asignada para los buffers
void free_memory_from_buffers() {
    // Se libera la memoria asignada para cada buffer individual.
    for (int i = 0; i < BUFFER_SIZE; i++) {
        free(buffertemp[i]);
        free(bufferph[i]);
    }
    // Se libera la memoria asignada para los arrays de punteros a char que representan los buffers.
    free(buffertemp);
    free(bufferph);
}

// Función de hilo para recolectar datos de los sensores
void *recolector(void *param) {
    // Se obtiene el nombre del pipe desde el parámetro de entrada.
    char *pipe_name = (char *)param;

    // Se abre el pipe en modo lectura.
    int fd = open(pipe_name, O_RDONLY);  
    if (fd == -1) {
        // Si ocurre un error al abrir el pipe, se imprime un mensaje de error y se sale del programa.
        perror("Error opening the pipe in the collector");
        exit(1);
    }

    // Se declara un buffer para leer los datos del pipe.
    char read_buffer[1024];

    // Se inicia un bucle para leer continuamente datos del pipe.
    while (read(fd, read_buffer, sizeof(read_buffer)) > 0) {
        // Se verifica si el último caracter del buffer es un salto de línea y se reemplaza por un terminador nulo si es necesario.
        if (read_buffer[strlen(read_buffer) - 1] == '\n') {
            read_buffer[strlen(read_buffer) - 1] = '\0';
        }

        // Se divide el contenido del buffer usando el delimitador ":".
        char *token = strtok(read_buffer, ":");
        int sensor_type = atoi(token);  
        token = strtok(NULL, ":");
        char *data = token;

        // Se verifica el tipo de sensor y se procesa la lectura.
        if (sensor_type == 1 || sensor_type == 2) { 
            // Seleccionar los semáforos, mutex y buffers correspondientes según el tipo de sensor.
            sem_t *empty, *full; 
            pthread_mutex_t *mutex;  
            char **buffer;  
            int *in;  

            if (sensor_type == 1) {
                empty = &empty_temp;
                full = &full_temp;
                mutex = &mutex_temp;
                buffer = buffertemp;
                in = &in_temp;
            } else {
                empty = &empty_ph;
                full = &full_ph;
                mutex = &mutex_ph;
                buffer = bufferph;
                in = &in_ph;
            }

            // Esperar hasta que haya espacio disponible en el buffer.
            sem_wait(empty);

            // Bloquear el mutex para acceder al buffer de manera segura.
            pthread_mutex_lock(mutex);  

            // Almacenar los datos en el buffer con un formato específico.
            sprintf(buffer[*in], "%s:%s", (sensor_type == 1) ? FLAG_TEMP : FLAG_PH, data);

            // Actualizar el índice de entrada del buffer circular.
            *in = (*in + 1) % BUFFER_SIZE;  

            // Desbloquear el mutex para permitir el acceso a otros hilos.
            pthread_mutex_unlock(mutex); 

            // Aumentar el contador de elementos en el buffer.
            sem_post(full);  
        } else {
            // Si se recibe una lectura incorrecta, se imprime un mensaje de error.
          printf("Error: Incorrect measurement received.\n");
        }
    }

    // Esperar 10 segundos antes de liberar los semáforos de espacio vacío.
    sleep(10);
    sem_post(&empty_temp);
    sem_post(&empty_ph);

    // Cerrar el descriptor del pipe.
    close(fd);  

    // Terminar la ejecución del hilo.
    return NULL;
}

// Función de hilo para recolectar datos de temperatura
void *temper_thread_collec() {
    // Se abre el archivo de temperatura en modo de añadir contenido al final del archivo.
    FILE *fileData = fopen(file_temp, "a");
    if (fileData == NULL) {
        // Si ocurre un error al abrir el archivo, se imprime un mensaje de error y se sale del programa.
        perror("Error opening the temperature file");
        exit(1);
    }

    // Bucle infinito para procesar continuamente los datos de temperatura.
    while (1) {
        // Esperar a que haya datos disponibles en el buffer de temperatura.
        sem_wait(&full_temp);

        // Bloquear el mutex para acceder al buffer de temperatura de manera segura.
        pthread_mutex_lock(&mutex_temp);

        // Verificar si el dato en el buffer de temperatura es una lectura válida.
        if (strncmp(buffertemp[out_temp], FLAG_TEMP, strlen(FLAG_TEMP)) == 0) {
            // Extraer el valor de temperatura del dato en el buffer.
            char *data = buffertemp[out_temp] + strlen(FLAG_TEMP) + 1;

            // Convertir el dato de temperatura a un número flotante.
            float temp = atof(data);

            // Obtener la marca de tiempo actual.
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

            // Verificar si la temperatura está dentro del rango aceptable.
            if (temp < 20.0 || temp > 31.6) {
              printf("Alert: Temperature out of range! %.1f\n",temp);
            } else {
              // Escribir la marca de tiempo y el valor de temperatura en el archivo de temperatura.
              fprintf(fileData, "{%s} %f\n", timestamp, temp);
              fflush(fileData);
            }
        }

        // Actualizar el índice de salida del buffer circular de temperatura.
        out_temp = (out_temp + 1) % BUFFER_SIZE;

        // Desbloquear el mutex para permitir el acceso a otros hilos.
        pthread_mutex_unlock(&mutex_temp);

        // Aumentar el contador de espacios vacíos en el buffer de temperatura.
        sem_post(&empty_temp);
    }

    // Cerrar el archivo de temperatura al finalizar el hilo.
    fclose(fileData);

    // Terminar la ejecución del hilo.
    return NULL;
}

// Función de hilo para recolectar datos de pH
void *ph_thread_collec() {
    // Se abre el archivo de pH en modo de añadir contenido al final del archivo.
    FILE *fileData = fopen(file_ph, "a");
    if (fileData == NULL) {
        // Si ocurre un error al abrir el archivo, se imprime un mensaje de error y se sale del programa.
        perror("Error opening the pH file");
        exit(1);
    }

    // Bucle infinito para procesar continuamente los datos de pH.
    while (1) {
        // Esperar a que haya datos disponibles en el buffer de pH.
        sem_wait(&full_ph);

        // Bloquear el mutex para acceder al buffer de pH de manera segura.
        pthread_mutex_lock(&mutex_ph);

        // Verificar si el dato en el buffer de pH es una lectura válida.
        if (strncmp(bufferph[out_ph], FLAG_PH, strlen(FLAG_PH)) == 0) {
            // Extraer el valor de pH del dato en el buffer.
            char *data = bufferph[out_ph] + strlen(FLAG_PH) + 1; 
            float ph = atof(data);  

            // Obtener la marca de tiempo actual.
            time_t now = time(NULL);  
            struct tm *tm_info = localtime(&now);  
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);  

            // Verificar si el pH está dentro del rango aceptable.
            if (ph < 6 || ph > 8) {
              printf("Alert: pH out of range! %.1f\n", ph);
            } else {
                // Escribir la marca de tiempo y el valor de pH en el archivo de pH.
                fprintf(fileData, "{%s} %f\n", timestamp, ph);
                fflush(fileData);  
            }
        }

        // Actualizar el índice de salida del buffer circular de pH.
        out_ph = (out_ph + 1) % BUFFER_SIZE;  

        // Desbloquear el mutex para permitir el acceso a otros hilos.
        pthread_mutex_unlock(&mutex_ph);

        // Aumentar el contador de espacios vacíos en el buffer de pH.
        sem_post(&empty_ph);  
    }

    // Cerrar el archivo de pH al finalizar el hilo.
    fclose(fileData); 

    // Terminar la ejecución del hilo.
    return NULL;
}