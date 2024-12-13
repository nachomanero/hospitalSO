#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>
#define MAX_MSG_SIZE 128
#define QUEUE_NAME "/hospital_queue"

sem_t sem_diagnostico, sem_farmacia;
mqd_t queue;
pid_t pid_recepcion, pid_hospital;

void manejador_SIGINT(int signum) {
    printf("\n[INFO] Capturando SIGINT. Cerrando recursos...\n");
    mq_close(queue);
    mq_unlink(QUEUE_NAME);
    sem_destroy(&sem_diagnostico);
    sem_destroy(&sem_farmacia);
    exit(0);
}

void* exploracion(void* args) {
    char paciente[MAX_MSG_SIZE];
    printf("[Exploración] Iniciando...\n");
    while (1) {
        mq_receive(queue, paciente, MAX_MSG_SIZE, NULL);
        printf("[Exploración] Recibido paciente: %s. Explorando...\n", paciente);
        sleep(rand() % 3 + 1);
        printf("[Exploración] Exploración completada. Notificando diagnóstico...\n");
        sem_post(&sem_diagnostico);
    }
    return NULL;
}

void* diagnostico(void* args) {
    printf("[Diagnóstico] Iniciando...\n");
    while (1) {
        sem_wait(&sem_diagnostico);
        printf("[Diagnóstico] Realizando diagnóstico...\n");
        sleep(rand() % 5 + 5);
        printf("[Diagnóstico] Diagnóstico completado. Notificando farmacia...\n");
        sem_post(&sem_farmacia);
    }
    return NULL;
}

void* farmacia(void* args) {
    printf("[Farmacia] Iniciando...\n");
    while (1) {
        sem_wait(&sem_farmacia);
        printf("[Farmacia] Preparando medicación...\n");
        sleep(rand() % 3 + 1);
        printf("[Farmacia] Medicación lista. Notificando alta...\n");
        kill(pid_recepcion, SIGUSR1);
    }
    return NULL;
}

void proceso_recepcion() {
    char paciente[MAX_MSG_SIZE];
    signal(SIGUSR1, manejador_SIGINT);
    printf("[Recepción] Iniciando...\n");
    while (1) {
        snprintf(paciente, MAX_MSG_SIZE, "Paciente_%d", rand() % 100);
        printf("[Recepción] Registrando paciente: %s...\n", paciente);
        mq_send(queue, paciente, strlen(paciente) + 1, 0);
        sleep(rand() % 5 + 1);
    }
}

void proceso_hospital() {
    pthread_t hilo_exploracion, hilo_diagnostico, hilo_farmacia;

    sem_init(&sem_diagnostico, 0, 0);
    sem_init(&sem_farmacia, 0, 0);
    pthread_create(&hilo_exploracion, NULL, exploracion, NULL);
    pthread_create(&hilo_diagnostico, NULL, diagnostico, NULL);
    pthread_create(&hilo_farmacia, NULL, farmacia, NULL);
    pthread_join(hilo_exploracion, NULL);
    pthread_join(hilo_diagnostico, NULL);
    pthread_join(hilo_farmacia, NULL);
}

int main() {
    signal(SIGINT, manejador_SIGINT);
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    queue = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (queue == -1) {
        perror("[ERROR] Error al crear la cola de mensajes");
        exit(1);
    }

    pid_recepcion = fork();

    if (pid_recepcion == 0) {
        // Proceso Recepción
        proceso_recepcion();
    } else {
        pid_hospital = fork();
        if (pid_hospital == 0) {
            // Proceso Hospital
            proceso_hospital();
        } else {
            // Proceso Padre
            wait(NULL); // Espera a que terminen los hijos
        }
    }

    return 0;
}
