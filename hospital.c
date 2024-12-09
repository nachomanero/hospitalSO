#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

// Configuración
#define QUEUE_NAME "/hospital_queue"
#define MAX_MESSAGE_SIZE 128

// Estructura del paciente
typedef struct {
    int id;
    char name[50];
} Patient;

// Semáforos
sem_t sem_diagnostico;
sem_t sem_farmacia;

// Variables globales para procesos
pid_t pid_recepcion;
pid_t pid_hospital;

// Función para generar un tiempo aleatorio
int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Limpieza de recursos
void cleanup() {
    mq_unlink(QUEUE_NAME);
    sem_destroy(&sem_diagnostico);
    sem_destroy(&sem_farmacia);
    printf("[Sistema] Recursos liberados. Terminando programa.\n");
    exit(0);
}

// Manejador de señales para altas
void handle_sigusr1(int sig) {
    printf("[Recepción] Paciente dado de alta.\n");
}

// Proceso Recepción
void proceso_recepcion() {
    mqd_t mq;
    struct mq_attr attr;

    // Configurar la cola de mensajes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MESSAGE_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq == -1) {
        perror("[Recepción] Error al abrir la cola de mensajes");
        exit(1);
    }

    signal(SIGUSR1, handle_sigusr1);

    while (1) {
        Patient patient;
        patient.id = rand() % 1000;
        snprintf(patient.name, sizeof(patient.name), "Paciente_%d", patient.id);

        printf("[Recepción] Registrando nuevo paciente: %s...\n", patient.name);

        if (mq_send(mq, (const char *)&patient, sizeof(Patient), 0) == -1) {
            perror("[Recepción] Error al enviar el mensaje");
        } else {
            printf("[Recepción] Paciente enviado al hospital.\n");
        }

        sleep(tiempo_aleatorio(1, 5));
    }

    mq_close(mq);
}

// Hilo Exploración
void* exploracion(void* args) {
    mqd_t mq;
    Patient patient;

    mq = mq_open(QUEUE_NAME, O_RDONLY);
    if (mq == -1) {
        perror("[Exploración] Error al abrir la cola de mensajes");
        pthread_exit(NULL);
    }

    while (1) {
        printf("[Exploración] Esperando a un paciente...\n");

        if (mq_receive(mq, (char *)&patient, MAX_MESSAGE_SIZE, NULL) != -1) {
            printf("[Exploración] Recibido paciente: %s. Realizando exploración...\n", patient.name);
            sleep(tiempo_aleatorio(1, 3));
            printf("[Exploración] Exploración completa. Notificando diagnóstico...\n");
            sem_post(&sem_diagnostico);
        } else {
            perror("[Exploración] Error al recibir el mensaje");
        }
    }

    mq_close(mq);
    return NULL;
}

// Hilo Diagnóstico
void* diagnostico(void* args) {
    while (1) {
        sem_wait(&sem_diagnostico);
        printf("[Diagnóstico] Realizando pruebas diagnósticas...\n");
        sleep(tiempo_aleatorio(5, 10));
        printf("[Diagnóstico] Diagnóstico completado. Notificando farmacia...\n");
        sem_post(&sem_farmacia);
    }
    return NULL;
}

// Hilo Farmacia
void* farmacia(void* args) {
    while (1) {
        sem_wait(&sem_farmacia);
        printf("[Farmacia] Preparando medicación...\n");
        sleep(tiempo_aleatorio(1, 3));
        printf("[Farmacia] Medicación lista. Enviando señal de alta...\n");
        kill(pid_recepcion, SIGUSR1);
    }
    return NULL;
}

// Proceso Hospital
void proceso_hospital() {
    pthread_t thread_exploracion, thread_diagnostico, thread_farmacia;

    // Inicializar semáforos
    sem_init(&sem_diagnostico, 0, 0);
    sem_init(&sem_farmacia, 0, 0);

    // Crear hilos
    pthread_create(&thread_exploracion, NULL, exploracion, NULL);
    pthread_create(&thread_diagnostico, NULL, diagnostico, NULL);
    pthread_create(&thread_farmacia, NULL, farmacia, NULL);

    // Esperar a los hilos
    pthread_join(thread_exploracion, NULL);
    pthread_join(thread_diagnostico, NULL);
    pthread_join(thread_farmacia, NULL);
}

// Función principal
int main(int argc, char* argv[]) {
    srand(time(NULL));
    signal(SIGINT, cleanup);

    pid_recepcion = fork();

    if (pid_recepcion == 0) {
        proceso_recepcion();
    } else {
        pid_hospital = fork();
        if (pid_hospital == 0) {
            proceso_hospital();
        } else {
            wait(NULL);
            wait(NULL);
            cleanup();
        }
    }

    return 0;
}
