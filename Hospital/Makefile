# Configuración del compilador y banderas
CC = gcc
CFLAGS = -Wall -lpthread -lrt -Iinclude

# Directorios
SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/hospital

# Regla para compilar el programa principal
$(TARGET): $(SRC_DIR)/hospital.c include/hospital.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC_DIR)/hospital.c

# Regla para limpiar archivos generados
clean:
	rm -rf $(BUILD_DIR)/*
