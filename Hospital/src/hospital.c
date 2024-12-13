#include "../include/hospital.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

// Variables globales
int pacientes_dados_de_alta = 0;
mqd_t cola_recepcion;
sem_t sem_diagnostico, sem_farmacia;
pid_t pid_hospital, pid_recepcion;

// Generar tiempos aleatorios
int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Manejador de señales
void manejador_senal(int signum) {
    if (signum == SIGUSR1) {
        pacientes_dados_de_alta++;
        printf("[Recepción] Paciente dado de alta. Total: %d\n", pacientes_dados_de_alta);
    } else if (signum == SIGINT) {
        printf("\n[Hospital] Señal SIGINT recibida. Cerrando programa y liberando recursos...\n");
        mq_unlink(COLA_RECEPCION);
        sem_destroy(&sem_diagnostico);
        sem_destroy(&sem_farmacia);
        exit(0);
    }
}

// Hilo de exploración
void* exploracion(void* args) {
    printf("[Exploración] Comienzo mi ejecución...\n");
    char paciente[128];
    while (1) {
        printf("[Exploración] Esperando un paciente...\n");
        if (mq_receive(cola_recepcion, paciente, sizeof(paciente), NULL) == -1) {
            perror("[Exploración] Error al recibir mensaje");
            continue;
        }
        printf("[Exploración] Recibido paciente: %s. Realizando exploración...\n", paciente);
        sleep(tiempo_aleatorio(1, 3));
        printf("[Exploración] Exploración completa. Notificando diagnóstico...\n");
        sem_post(&sem_diagnostico);
    }
}

// Hilo de diagnóstico
void* diagnostico(void* args) {
    printf("[Diagnóstico] Comienzo mi ejecución...\n");
    while (1) {
        sem_wait(&sem_diagnostico);
        printf("[Diagnóstico] Realizando pruebas diagnósticas...\n");
        sleep(tiempo_aleatorio(5, 10));
        printf("[Diagnóstico] Diagnóstico completado. Notificando farmacia...\n");
        sem_post(&sem_farmacia);
    }
}

// Hilo de farmacia
void* farmacia(void* args) {
    printf("[Farmacia] Comienzo mi ejecución...\n");
    while (1) {
        sem_wait(&sem_farmacia);
        printf("[Farmacia] Preparando medicación...\n");
        sleep(tiempo_aleatorio(1, 3));
        printf("[Farmacia] Medicación lista. Enviando señal de alta...\n");
        kill(getppid(), SIGUSR1);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    // Configurar manejadores de señales
    signal(SIGUSR1, manejador_senal);
    signal(SIGINT, manejador_senal);

    // Crear proceso recepción
    pid_recepcion = fork();
    if (pid_recepcion == 0) {
        printf("[Recepción] Comienzo mi ejecución...\n");

        struct mq_attr attr = {0, 10, 128, 0};
        cola_recepcion = mq_open(COLA_RECEPCION, O_CREAT | O_WRONLY, 0644, &attr);
        if (cola_recepcion == (mqd_t)-1) {
            perror("[Recepción] Error al abrir la cola de mensajes");
            exit(EXIT_FAILURE);
        }

        for (int i = 1;; i++) {
            char paciente[128];
            snprintf(paciente, sizeof(paciente), "Paciente %d", i);
            printf("[Recepción] Registrando nuevo paciente: %s\n", paciente);
            if (mq_send(cola_recepcion, paciente, strlen(paciente) + 1, 0) == -1) {
                perror("[Recepción] Error al enviar mensaje");
                continue;
            }
            sleep(tiempo_aleatorio(1, 10));
        }
    } else {
        // Crear proceso hospital
        pid_hospital = fork();
        if (pid_hospital == 0) {
            printf("[Hospital] Comienzo mi ejecución...\n");

            cola_recepcion = mq_open(COLA_RECEPCION, O_CREAT | O_RDONLY, 0644, NULL);
            if (cola_recepcion == (mqd_t)-1) {
                perror("[Hospital] Error al abrir la cola de mensajes");
                exit(EXIT_FAILURE);
            }

            sem_init(&sem_diagnostico, 0, 0);
            sem_init(&sem_farmacia, 0, 0);

            pthread_t hilo_exploracion, hilo_diagnostico, hilo_farmacia;
            pthread_create(&hilo_exploracion, NULL, exploracion, NULL);
            pthread_create(&hilo_diagnostico, NULL, diagnostico, NULL);
            pthread_create(&hilo_farmacia, NULL, farmacia, NULL);

            pthread_join(hilo_exploracion, NULL);
            pthread_join(hilo_diagnostico, NULL);
            pthread_join(hilo_farmacia, NULL);

            sem_destroy(&sem_diagnostico);
            sem_destroy(&sem_farmacia);
            mq_close(cola_recepcion);
            mq_unlink(COLA_RECEPCION);
        } else {
            wait(NULL);
            wait(NULL);
        }
    }

    return 0;
}
