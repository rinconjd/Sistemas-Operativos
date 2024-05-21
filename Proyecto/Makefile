CC = gcc
CXXFLAGS = -Wall -Wextra -Iinclude -lpthread

all: sensor monitor

sensor: sensor.c
	$(CC) $(CXXFLAGS) $^ -o $@

monitor: monitor.c buffer.c
	$(CC) $(CXXFLAGS) $^ -o $@

run_t: run_sensor_t run_monitor

run_p: run_sensor_p run_monitor

run_sensor_t: sensor
	@echo "Ejecutando sensor en segundo plano..."
	@./sensor -t 3 -s 1 -f datosTemp.txt -p pipeNOM &

run_sensor_p: sensor
	@echo "Ejecutando sensor en segundo plano..."
	@./sensor -t 3 -s 2 -f datosPh.txt -p pipeNOM &

run_monitor: monitor
	@echo "Ejecutando monitor en segundo plano..."
	@./monitor -b 10 -t file-temp.txt -h file-ph.txt -p pipeNOM &

clear:
		rm -f file-temp.txt file-ph.txt