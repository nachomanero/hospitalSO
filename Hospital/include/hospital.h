#ifndef HOSPITAL_H
#define HOSPITAL_H

#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include <stdbool.h>
#include <signal.h>

// Definiciones y constantes
#define COLA_RECEPCION "/cola_recepcion"

// Declaración de variables globales
extern int pacientes_dados_de_alta;
extern mqd_t cola_recepcion;
extern sem_t sem_diagnostico, sem_farmacia;
extern pid_t pid_hospital, pid_recepcion;

// Funciones comunes
int tiempo_aleatorio(int min, int max);
void manejador_senal(int signum);
void* exploracion(void* args);
void* diagnostico(void* args);
void* farmacia(void* args);

#endif // HOSPITAL_H
