/*************************************************************
Autot: Juan David Rincon - Juan Felipe Morales
Fecha: 27 de abril de 2024
Materia: Sistemas Operativos
Tema: Proyecto
Fichero: Manejo de sensor.c
**************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int flags; // Almacena las flags de los argumentos de línea de comandos
  char *sensorType = NULL;   // Puntero al tipo de sensor
  char *timeInterval = NULL; // Puntero al intervalo de tiempo
  char *fileName = NULL;     // Puntero al nombre del archivo de datos
  char *pipeName = NULL;     // Puntero al nombre del pipe nominal

  // Maneja de banderas mediante argumentos de línea de comandos
  while ((flags = getopt(argc, argv, "s:t:f:p:")) != -1) {
    switch (flags) {
    case 's': // Bandera de sensor
      sensorType = argv[optind - 1];
      break;
    case 't': // Bandera de sensor del intervalo de tiempo
      timeInterval = argv[optind - 1];
      break;
    case 'f': // Bandera del nombre del archivo
      fileName = argv[optind - 1];
      break;
    case 'p': // Bandera del nombre del pipe
      pipeName = argv[optind - 1];
      break;
    default: // Mensaje de uso en caso de argumentos incorrectos
      fprintf(
          stderr,
          "Usage: %s -s sensorType -t timeInterval -f fileName -p pipeName\n",
          argv[0]);
      return 1;
    }
  }

  // Conversión de cadenas de 'sensorType' y 'timeInterval' a enteros
  int sensorTypeInt = atoi(sensorType);
  int timeIntervalInt = atoi(timeInterval);

  // Verificación de la validez del tipo de sensor
  if (sensorTypeInt != 1 && sensorTypeInt != 2) {
    fprintf(stderr,
            "Error: Invalid sensor type. Sensor type must be 1 or 2.\n");
    return 1;
  }

  // Apertura del pipe nominal en modo escritura
  int pipeNominal = open(pipeName, O_WRONLY);
  if (pipeNominal == -1) {
    perror("Error opening the pipe");
    exit(1);
  } else {
    printf("Pipe opened successfully: %s\n", pipeName);
  }

  // Apertura del archivo de datos en modo lectura
  FILE *fileData = fopen(fileName, "r");
  if (!fileData) {
    fprintf(stderr, "Error opening the file: %s\n", fileName);
    close(pipeNominal);
    exit(1);
  } else {
    printf("File opened successfully\n");
  }

  // Lectura y envío de datos al pipe nominal
  char line[1024];
  while (fgets(line, sizeof(line), fileData)) {
    float valData = atof(line);

    // Se omite si el valor es negativo
    if (valData < 0) {
      continue;
    }

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%d:%.2f\n", sensorTypeInt, valData);

    // Se imprime el tipo de sensor y el valor leído
    if (sensorTypeInt == 1) {
      printf("Sensor sends temperature: %.2f\n", valData);
    } else if (sensorTypeInt == 2) {
      printf("Sensor sends pH: %.2f\n", valData);
    }

    // Escritura en el pipe nominal
    ssize_t bytes_written = write(pipeNominal, buffer, strlen(buffer));
    if (bytes_written == -1) {
      perror("Error writing to the pipe");
      break;
    }

    // Pausa según el intervalo de tiempo especificado
    sleep(timeIntervalInt);
  }

  // Cierre del archivo y el pipe
  fclose(fileData);
  close(pipeNominal);

  return 0;
}