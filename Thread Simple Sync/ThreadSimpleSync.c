#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>


/* Trabalho 3 - Sistemas Operacionais - Sincronização
            Sistema de Telemarketing

    Nome: Gabriel Gomes Pereira
    Curso: Sistemas de Informação

*/


typedef struct{
    int idThread;
}thread_args;

int waiting = 0;
int lines;
sem_t mutex;
sem_t attend;
sem_t clients_waiting;

void atende()
{
    printf("Atendente esta falando com algum cliente! Ha %d chamadas em espera.\n", waiting);
    sleep(1 +(rand()%10));
}

void tomaCafe()
{
    if(waiting <= 0)
    {
        printf("Tomando cafe...\n");
    }
}

void *clientCall(void* arg)
{

    thread_args *args = (thread_args*)  arg;

    sem_wait(&mutex); //Região crítica
    if(waiting < lines)
    {
        printf("Cliente %d esta ligando! Havia %d chamadas em espera\n", args->idThread, waiting);
        waiting++;
        sem_post(&clients_waiting); //Há clientes solicitando atendimento
        sem_post(&mutex);//libera
        sem_wait(&attend);//Se o atendente estiver atendendo outra pessoa, cliente aguarda na fila
        printf("Cliente %d esta sendo atendido! Ha %d chamadas em espera\n",args->idThread, waiting);
    }
    else{
        printf("Cliente %d nao conseguiu realizar a chamada. Todas as linhas ocupadas.\n", args->idThread);
        sem_post(&mutex);//libera
    }

}

void *attendant()
{
    while(true)
    {
        tomaCafe();
        sem_wait(&clients_waiting); //se não há clientes na fila, atendente toma um café
        waiting--; //decrementa fila porque algum cliente foi removido para ser atendido
        sem_post(&attend);//Libera o cliente para ser atendido
        atende(); // Faz o atendimento
    }
}


int main(int argc, char *argv[])
{
    pthread_t *client = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_t *callcenter = (pthread_t*) malloc(sizeof(pthread_t));

    thread_args *args;

    //Verifica entrada fornecida
    if(argc != 2 || atoi(argv[1]) <= 0)
    {
        printf("Argumentos invalidos!\n");
        exit(-1);
    }

    lines = atoi(argv[1]);

    //contador id único passado para as threads
    int id = 0;

    //Aloca semáforos
    sem_init(&mutex, 0, 1);
    sem_init(&attend, 0, 0);
    sem_init(&clients_waiting, 0, 0);
    printf("----------------------------------------------------------------------\n");
    printf("Call Center do Gabriel (numero de linhas de espera:%d)\n", lines);
    printf("----------------------------------------------------------------------\n");

    if(pthread_create(callcenter, NULL, attendant, NULL))
    {
        printf("O atendente pediu demissao!\n");
        exit(-1);
    }

    srand(time(NULL));

    while(true)
    {
        args = (thread_args *) malloc(sizeof(thread_args));
        args->idThread = id;
        sleep(1 + (rand()%10));
        if(pthread_create(client, NULL, clientCall, (void *)args))
        {
            printf("Um erro ocorreu ao criar a thread!\n");
            exit(-1);
        }
        id++;
    }

    return 0;
}