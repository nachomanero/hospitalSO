#include "../include/hospital.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

// ========================
// Variables Globales
// ========================
int pacientes_dados_de_alta = 0; //Contador de pacientes dados de alta
mqd_t cola_recepcion;//Cola de mensajes
sem_t sem_diagnostico, sem_farmacia; //Semaforos
pid_t pid_hospital, pid_recepcion; //PIDs de los procesos

// ========================
// Funcion:tiempo_aleatorio
// Devuelve un tiempo aleatorio (simula  un tiempo de espera)
// ========================
int tiempo_aleatorio(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// ========================
// Funcion: manejador_senal
// Funcion que maneja las señales.
// SIGUSR1-> Incrementa el contador de los pacientes dados de alta.
// SIGINT-> Libera recursos y cierra el programa.
// ========================
void manejador_senal(int signum) {
    if (signum == SIGUSR1) {
        pacientes_dados_de_alta++;
        printf("[Recepcion] Paciente dado de alta. Total: %d\n", pacientes_dados_de_alta);
    } else if (signum == SIGINT) {
        printf("\n[Hospital] Señal SIGINT recibida. Cerrando programa y liberando recursos...\n");
        mq_unlink(COLA_RECEPCION); //Elimina la cola de mensajes
        sem_destroy(&sem_diagnostico); //Elimina el semaforo diagnostico
        sem_destroy(&sem_farmacia); //Elimina el semaforo farmacia
        exit(0);
    }
}
// =======================
// Hilo: exploracion
// Recibe pacientes de la cola (de mensajes)
// Simula la exploracion inicial y notifica
// al hilo del diagnostico
// =======================
void* exploracion(void* args) {
    printf("[Exploracion] Comienzo mi ejecucion...\n");
    char paciente[128];
    while (1) {
        printf("[Exploracion] Esperando un paciente...\n");
        //Recibe el mensaje de la cola de recepcion
        if (mq_receive(cola_recepcion, paciente, sizeof(paciente), NULL) == -1) {
            perror("[Exploracion] Error al recibir mensaje");
            continue;
        }
        printf("[Exploracion] Recibido paciente: %s. Realizando exploracion...\n", paciente);
        sleep(tiempo_aleatorio(1, 3)); //tiempo para la exploracion
        printf("[Exploracion] Exploracion completa. Notificando diagnostico...\n");
        sem_post(&sem_diagnostico); //libera el semaforo de diagnostico
    }
}
// =======================
// Hilo: diagnostico
// Simula los diagnosticos y manda notificacion a la farmacia (con semaforo)
// =======================
void* diagnostico(void* args) {
    printf("[Diagnostico] Comienzo mi ejecucion...\n");
    while (1) {
        sem_wait(&sem_diagnostico); //Espera a que exploracion mande la señal
        printf("[Diagnostico] Realizando pruebas diagnosticas...\n");
        sleep(tiempo_aleatorio(5, 10)); //tiempo de diagnostico
        printf("[Diagnostico] Diagnostico completado. Notificando a farmacia...\n");
        sem_post(&sem_farmacia); //libera el semaforo
    }
}
// =======================
// Hilo: farmacia
// Simula la preparacion de la medicacion y manda la señal de alta
// al proceso de recepcion
// =======================
void* farmacia(void* args) {
    printf("[Farmacia] Comienzo mi ejecución...\n");
    while (1) {
        sem_wait(&sem_farmacia); //Espera la señal de diagnostico
        printf("[Farmacia] Preparando medicación...\n");
        sleep(tiempo_aleatorio(1, 3)); //tiempo de preparacion
        printf("[Farmacia] Medicación lista. Enviando señal de alta...\n");
        kill(getppid(), SIGUSR1); // envia señal a recepcion
    }
}
// =======================
// Funcion: main
// Crea los procesos de recepcion y hospital
// inicializa los recursos y gestiona la
// sincronizacion de procesos e hilos
// =======================

int main(int argc, char* argv[]) {
    srand(time(NULL)); //genera tiempos aleatorios

    // Configuracion de señales
    signal(SIGUSR1, manejador_senal); // para contar altas
    signal(SIGINT, manejador_senal);  // para liberar recursos y cerrar

    // ===================
    // Proceso recepcion
    // ===================
    pid_recepcion = fork();
    if (pid_recepcion == 0) {
        printf("[Recepcion] Comienzo mi ejecucion...\n");
        //configura la cola de mensajes
        struct mq_attr attr = {0, 10, 128, 0};
        cola_recepcion = mq_open(COLA_RECEPCION, O_CREAT | O_WRONLY, 0644, &attr);
        if (cola_recepcion == (mqd_t)-1) {
            perror("[Recepcion] Error al abrir la cola de mensajes");
            exit(EXIT_FAILURE);
        }
        //envia pacientes
        for (int i = 1;; i++) {
            char paciente[128];
            snprintf(paciente, sizeof(paciente), "Paciente %d", i);
            printf("[Recepcion] Registrando nuevo paciente: %s\n", paciente);
            if (mq_send(cola_recepcion, paciente, strlen(paciente) + 1, 0) == -1) {
                perror("[Recepcion] Error al enviar mensaje");
                continue;
            }
            sleep(tiempo_aleatorio(1, 10)); //tiempo espera entre envops
        }
    } else {
         // ===================
        // Proces Hospital
        // ===================
        pid_hospital = fork();
        if (pid_hospital == 0) {
            printf("[Hospital] Comienzo mi ejecucion...\n");
            cola_recepcion = mq_open(COLA_RECEPCION, O_CREAT | O_RDONLY, 0644, NULL);
            if (cola_recepcion == (mqd_t)-1) {
                perror("[Hospital] Error al abrir la cola de mensajes");
                exit(EXIT_FAILURE);
            }
           //Inicializacion de semaforos
            sem_init(&sem_diagnostico, 0, 0);
            sem_init(&sem_farmacia, 0, 0);
           //Creacion de hilos
            pthread_t hilo_exploracion, hilo_diagnostico, hilo_farmacia;
            pthread_create(&hilo_exploracion, NULL, exploracion, NULL);
            pthread_create(&hilo_diagnostico, NULL, diagnostico, NULL);
            pthread_create(&hilo_farmacia, NULL, farmacia, NULL);
           //espera a que terminen los hilos
            pthread_join(hilo_exploracion, NULL);
            pthread_join(hilo_diagnostico, NULL);
            pthread_join(hilo_farmacia, NULL);
           
            //sem_destroy(&sem_diagnostico);
            //sem_destroy(&sem_farmacia);
            mq_close(cola_recepcion);
            //mq_unlink(COLA_RECEPCION);
        } else {
            wait(NULL);
            wait(NULL);
        }
    }
    return 0;
}